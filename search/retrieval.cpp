#include "retrieval.hpp"

#include "search/cancel_exception.hpp"
#include "search/feature_offset_match.hpp"
#include "search/interval_set.hpp"
#include "search/mwm_context.hpp"
#include "search/search_index_values.hpp"
#include "search/search_trie.hpp"
#include "search/token_slice.hpp"

#include "indexer/classificator.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_source.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/trie_reader.hpp"

#include "platform/mwm_version.hpp"

#include "coding/compressed_bit_vector.hpp"
#include "coding/reader_wrapper.hpp"

#include "base/checked_cast.hpp"
#include "base/control_flow.hpp"
#include "base/macros.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

using namespace std;
using namespace strings;
using osm::EditableMapObject;

namespace search
{
namespace
{
class FeaturesCollector
{
public:
  FeaturesCollector(base::Cancellable const & cancellable, vector<uint64_t> & features,
                    vector<uint64_t> & exactlyMatchedFeatures)
    : m_cancellable(cancellable)
    , m_features(features)
    , m_exactlyMatchedFeatures(exactlyMatchedFeatures)
    , m_counter(0)
  {
  }

  template <typename Value>
  void operator()(Value const & value, bool exactMatch)
  {
    if ((++m_counter & 0xFF) == 0)
      BailIfCancelled(m_cancellable);
    m_features.emplace_back(value.m_featureId);
    if (exactMatch)
      m_exactlyMatchedFeatures.emplace_back(value.m_featureId);
  }

  void operator()(uint32_t feature) { m_features.emplace_back(feature); }

  void operator()(uint64_t feature, bool exactMatch)
  {
    m_features.emplace_back(feature);
    if (exactMatch)
      m_exactlyMatchedFeatures.emplace_back(feature);
  }

private:
  base::Cancellable const & m_cancellable;
  vector<uint64_t> & m_features;
  vector<uint64_t> & m_exactlyMatchedFeatures;
  uint32_t m_counter;
};


Retrieval::ExtendedFeatures SortFeaturesAndBuildResult(vector<uint64_t> && features,
                                                       vector<uint64_t> && exactlyMatchedFeatures)
{
  using Builder = coding::CompressedBitVectorBuilder;
  base::SortUnique(features);
  base::SortUnique(exactlyMatchedFeatures);
  auto featuresCBV = CBV(Builder::FromBitPositions(move(features)));
  auto exactlyMatchedFeaturesCBV = CBV(Builder::FromBitPositions(move(exactlyMatchedFeatures)));
  return Retrieval::ExtendedFeatures(move(featuresCBV), move(exactlyMatchedFeaturesCBV));
}

Retrieval::ExtendedFeatures SortFeaturesAndBuildResult(vector<uint64_t> && features)
{
  using Builder = coding::CompressedBitVectorBuilder;
  base::SortUnique(features);
  auto const featuresCBV = CBV(Builder::FromBitPositions(move(features)));
  return Retrieval::ExtendedFeatures(featuresCBV);
}

template <typename DFA>
pair<bool, bool> MatchesByName(vector<UniString> const & tokens, vector<DFA> const & dfas)
{
  for (auto const & dfa : dfas)
  {
    for (auto const & token : tokens)
    {
      auto it = dfa.Begin();
      DFAMove(it, token);
      if (it.Accepts())
        return {true, it.ErrorsMade() == 0};
    }
  }

  return {false, false};
}

template <typename DFA>
pair<bool, bool> MatchesByType(feature::TypesHolder const & types, vector<DFA> const & dfas)
{
  if (dfas.empty())
    return {false, false};

  auto const & c = classif();

  for (auto const & type : types)
  {
    UniString const s = FeatureTypeToString(c.GetIndexForType(type));

    for (auto const & dfa : dfas)
    {
      auto it = dfa.Begin();
      DFAMove(it, s);
      if (it.Accepts())
        return {true, it.ErrorsMade() == 0};
    }
  }

  return {false, false};
}

template <typename DFA>
pair<bool, bool> MatchFeatureByNameAndType(EditableMapObject const & emo,
                                           SearchTrieRequest<DFA> const & request)
{
  auto const & th = emo.GetTypes();

  pair<bool, bool> matchedByType = MatchesByType(th, request.m_categories);

  // Exactly matched by type.
  if (matchedByType.second)
    return {true, true};

  pair<bool, bool> matchedByName = {false, false};
  emo.GetNameMultilang().ForEach([&](int8_t lang, string const & name) {
    if (name.empty() || !request.HasLang(lang))
      return base::ControlFlow::Continue;

    vector<UniString> tokens;
    NormalizeAndTokenizeString(name, tokens, Delimiters());
    auto const matched = MatchesByName(tokens, request.m_names);
    matchedByName = {matchedByName.first || matched.first, matchedByName.second || matched.second};
    if (!matchedByName.second)
      return base::ControlFlow::Continue;

    return base::ControlFlow::Break;
  });

  return {matchedByType.first || matchedByName.first, matchedByType.second || matchedByName.second};
}

bool MatchFeatureByPostcode(EditableMapObject const & emo, TokenSlice const & slice)
{
  string const postcode = emo.GetMetadata().Get(feature::Metadata::FMD_POSTCODE);
  vector<UniString> tokens;
  NormalizeAndTokenizeString(postcode, tokens, Delimiters());
  if (slice.Size() > tokens.size())
    return false;
  for (size_t i = 0; i < slice.Size(); ++i)
  {
    if (slice.IsPrefix(i))
    {
      if (!StartsWith(tokens[i], slice.Get(i).GetOriginal()))
        return false;
    }
    else if (tokens[i] != slice.Get(i).GetOriginal())
    {
      return false;
    }
  }
  return true;
}

template <typename Value, typename DFA>
Retrieval::ExtendedFeatures RetrieveAddressFeaturesImpl(Retrieval::TrieRoot<Value> const & root,
                                                        MwmContext const & context,
                                                        base::Cancellable const & cancellable,
                                                        SearchTrieRequest<DFA> const & request)
{
  vector<uint64_t> features;
  vector<uint64_t> exactlyMatchedFeatures;
  FeaturesCollector collector(cancellable, features, exactlyMatchedFeatures);

  MatchFeaturesInTrie(request, root, [](Value const &) { return true; } /* filter */,
      collector);

  return SortFeaturesAndBuildResult(move(features), move(exactlyMatchedFeatures));
}

template <typename Value>
Retrieval::ExtendedFeatures RetrievePostcodeFeaturesImpl(Retrieval::TrieRoot<Value> const & root,
                                                         MwmContext const & context,
                                                         base::Cancellable const & cancellable,
                                                         TokenSlice const & slice)
{
  vector<uint64_t> features;
  vector<uint64_t> exactlyMatchedFeatures;
  FeaturesCollector collector(cancellable, features, exactlyMatchedFeatures);

  MatchPostcodesInTrie(slice, root, [](Value const&) { return true; } /* filter */, collector);

  return SortFeaturesAndBuildResult(move(features));
}

Retrieval::ExtendedFeatures RetrieveGeometryFeaturesImpl(MwmContext const & context,
                                                         base::Cancellable const & cancellable,
                                                         m2::RectD const & rect, int scale)
{
  covering::Intervals coverage;
  CoverRect(rect, scale, coverage);

  vector<uint64_t> features;
  vector<uint64_t> exactlyMatchedFeatures;

  FeaturesCollector collector(cancellable, features, exactlyMatchedFeatures);

  context.ForEachIndex(coverage, scale, collector);

  return SortFeaturesAndBuildResult(move(features), move(exactlyMatchedFeatures));
}

template <typename T>
struct RetrieveAddressFeaturesAdaptor
{
  template <typename... Args>
  Retrieval::ExtendedFeatures operator()(Args &&... args)
  {
    return RetrieveAddressFeaturesImpl<T>(forward<Args>(args)...);
  }
};

template <typename T>
struct RetrievePostcodeFeaturesAdaptor
{
  template <typename... Args>
  Retrieval::ExtendedFeatures operator()(Args &&... args)
  {
    return RetrievePostcodeFeaturesImpl<T>(forward<Args>(args)...);
  }
};

template <typename Value>
unique_ptr<Retrieval::TrieRoot<Value>> ReadTrie(MwmValue & value, ModelReaderPtr & reader)
{
  serial::GeometryCodingParams params(
      trie::GetGeometryCodingParams(value.GetHeader().GetDefGeometryCodingParams()));
  return trie::ReadTrie<SubReaderWrapper<Reader>, ValueList<Value>>(
      SubReaderWrapper<Reader>(reader.GetPtr()), SingleValueSerializer<Value>(params));
}
}  // namespace

Retrieval::Retrieval(MwmContext const & context, base::Cancellable const & cancellable)
  : m_context(context)
  , m_cancellable(cancellable)
  , m_reader(context.m_value.m_cont.GetReader(SEARCH_INDEX_FILE_TAG))
{
  auto & value = context.m_value;

  version::MwmTraits mwmTraits(value.GetMwmVersion());
  m_format = mwmTraits.GetSearchIndexFormat();

  switch (m_format)
  {
  case version::MwmTraits::SearchIndexFormat::FeaturesWithRankAndCenter:
    m_root0 = ReadTrie<FeatureWithRankAndCenter>(value, m_reader);
    break;
  case version::MwmTraits::SearchIndexFormat::CompressedBitVector:
    m_root1 = ReadTrie<FeatureIndexValue>(value, m_reader);
    break;
  }
}

Retrieval::ExtendedFeatures Retrieval::RetrieveAddressFeatures(
    SearchTrieRequest<UniStringDFA> const & request) const
{
  return Retrieve<RetrieveAddressFeaturesAdaptor>(request);
}

Retrieval::ExtendedFeatures Retrieval::RetrieveAddressFeatures(
    SearchTrieRequest<PrefixDFAModifier<UniStringDFA>> const & request) const
{
  return Retrieve<RetrieveAddressFeaturesAdaptor>(request);
}

Retrieval::ExtendedFeatures Retrieval::RetrieveAddressFeatures(
    SearchTrieRequest<LevenshteinDFA> const & request) const
{
  return Retrieve<RetrieveAddressFeaturesAdaptor>(request);
}

Retrieval::ExtendedFeatures Retrieval::RetrieveAddressFeatures(
    SearchTrieRequest<PrefixDFAModifier<LevenshteinDFA>> const & request) const
{
  return Retrieve<RetrieveAddressFeaturesAdaptor>(request);
}

Retrieval::Features Retrieval::RetrievePostcodeFeatures(TokenSlice const & slice) const
{
  return Retrieve<RetrievePostcodeFeaturesAdaptor>(slice).m_features;
}

Retrieval::Features Retrieval::RetrieveGeometryFeatures(m2::RectD const & rect, int scale) const
{
  return RetrieveGeometryFeaturesImpl(m_context, m_cancellable, rect, scale).m_features;
}

template <template <typename> class R, typename... Args>
Retrieval::ExtendedFeatures Retrieval::Retrieve(Args &&... args) const
{
  switch (m_format)
  {
  case version::MwmTraits::SearchIndexFormat::FeaturesWithRankAndCenter:
  {
    R<FeatureWithRankAndCenter> r;
    ASSERT(m_root0, ());
    return r(*m_root0, m_context, m_cancellable, forward<Args>(args)...);
  }
  case version::MwmTraits::SearchIndexFormat::CompressedBitVector:
  {
    R<FeatureIndexValue> r;
    ASSERT(m_root1, ());
    return r(*m_root1, m_context, m_cancellable, forward<Args>(args)...);
  }
  }
  UNREACHABLE();
}
}  // namespace search
