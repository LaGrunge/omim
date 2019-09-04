#include "generator/check_model.hpp"
#include "generator/data_version.hpp"
#include "generator/dumper.hpp"
#include "generator/feature_generator.hpp"
#include "generator/generate_info.hpp"
#include "generator/geo_objects/geo_objects_generator.hpp"
#include "generator/locality_sorter.hpp"
#include "generator/osm_source.hpp"
#include "generator/processor_factory.hpp"
#include "generator/raw_generator.hpp"
#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/regions.hpp"
#include "generator/statistics.hpp"
#include "generator/streets/streets.hpp"
#include "generator/translator_collection.hpp"
#include "generator/translator_factory.hpp"
#include "generator/unpack_mwm.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/locality_index_builder.hpp"
#include "indexer/map_style_reader.hpp"

#include "platform/platform.hpp"

#include "coding/endianness.hpp"

#include "base/file_name_utils.hpp"
#include "base/timer.hpp"

#include <csignal>
#include <cstdlib>
#include <fstream>
#include <string>

#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#include <boost/stacktrace.hpp>

#include "build_version.hpp"
#include "defines.hpp"

#include "3party/gflags/src/gflags/gflags.h"

using namespace std;

namespace
{
char const * GetDataPathHelp()
{
  static string const kHelp =
      "Directory where the generated mwms are put into. Also used as the path for helper "
      "functions, such as those that calculate statistics and regenerate sections. "
      "Default: " +
      Platform::GetCurrentWorkingDirectory() + "/../../data'.";
  return kHelp.c_str();
}
}  // namespace

// Coastlines.
DEFINE_bool(make_coasts, false, "Create intermediate file with coasts data.");
DEFINE_bool(fail_on_coasts, false,
            "Stop and exit with '255' code if some coastlines are not merged.");
DEFINE_bool(emit_coasts, false,
            "Push coasts features from intermediate file to out files/countries.");

// Generator settings and paths.
DEFINE_string(osm_file_name, "", "Input osm area file.");
DEFINE_string(osm_file_type, "xml", "Input osm area file type [xml, o5m].");
DEFINE_string(data_path, "", GetDataPathHelp());
DEFINE_string(user_resource_path, "", "User defined resource path for classificator.txt and etc.");
DEFINE_string(intermediate_data_path, "", "Path to stored nodes, ways, relations.");
DEFINE_string(output, "", "File name for process (without 'mwm' ext).");
DEFINE_bool(preload_cache, false, "Preload all ways and relations cache.");
DEFINE_string(node_storage, "map",
              "Type of storage for intermediate points representation. Available: raw, map, mem.");
DEFINE_uint64(planet_version, base::SecondsSinceEpoch(),
              "Version as seconds since epoch, by default - now.");

// Preprocessing and feature generator.
DEFINE_bool(preprocess, false, "1st pass - create nodes/ways/relations data.");
DEFINE_bool(generate_features, false, "2nd pass - generate intermediate features.");
DEFINE_bool(no_ads, false, "generation without ads.");
DEFINE_bool(generate_region_features, false,
            "Generate intermediate features for regions to use in regions index and borders generation.");
DEFINE_bool(generate_streets_features, false,
            "Generate intermediate features for streets to use in server-side forward geocoder.");
DEFINE_bool(generate_geo_objects_features, false,
            "Generate intermediate features for geo objects to use in geo objects index.");
DEFINE_bool(generate_geometry, false,
            "3rd pass - split and simplify geometry and triangles for features.");
DEFINE_bool(generate_index, false, "4rd pass - generate index.");
DEFINE_bool(generate_search_index, false, "5th pass - generate search index.");
DEFINE_bool(generate_geo_objects_index, false,
            "Generate objects and index for server-side reverse geocoder.");
DEFINE_bool(generate_regions, false,
            "Generate regions index and borders for server-side reverse geocoder.");
DEFINE_bool(generate_regions_kv, false,
            "Generate regions key-value for server-side reverse geocoder.");

DEFINE_bool(dump_cities_boundaries, false, "Dump cities boundaries to a file");
DEFINE_bool(generate_cities_boundaries, false, "Generate the cities boundaries section");
DEFINE_string(cities_boundaries_data, "", "File with cities boundaries");

DEFINE_bool(generate_cities_ids, false, "Generate the cities ids section");

DEFINE_bool(generate_world, false, "Generate separate world file.");
DEFINE_bool(have_borders_for_whole_world, false, "If it is set to true, the optimization of checking that the "
                                                 "fb belongs to the country border will be applied.");

DEFINE_string(
    nodes_list_path, "",
    "Path to file containing list of node ids we need to add to locality index. May be empty.");

// Routing.
DEFINE_bool(make_routing_index, false, "Make sections with the routing information.");
DEFINE_bool(make_cross_mwm, false,
            "Make section for cross mwm routing (for dynamic indexed routing).");
DEFINE_bool(make_transit_cross_mwm, false, "Make section for cross mwm transit routing.");
DEFINE_bool(disable_cross_mwm_progress, false,
            "Disable log of cross mwm section building progress.");
DEFINE_string(srtm_path, "",
              "Path to srtm directory. If set, generates a section with altitude information "
              "about roads.");
DEFINE_string(transit_path, "", "Path to directory with transit graphs in json.");
DEFINE_bool(generate_cameras, false, "Generate section with speed cameras info.");
DEFINE_bool(
    make_city_roads, false,
    "Calculates which roads lie inside cities and makes a section with ids of these roads.");
DEFINE_bool(generate_maxspeed, false, "Generate section with maxspeed of road features.");

// Sponsored-related.
DEFINE_string(booking_data, "", "Path to booking data in tsv format.");
DEFINE_string(opentable_data, "", "Path to opentable data in tsv format.");
DEFINE_string(promo_catalog_cities, "",
              "Path to list geo object ids of cities which contain promo catalog in json format.");

DEFINE_string(wikipedia_pages, "", "Input dir with wikipedia pages.");
DEFINE_string(idToWikidata, "", "Path to file with id to wikidata mapping.");
DEFINE_string(dump_wikipedia_urls, "", "Output file with wikipedia urls.");

DEFINE_bool(generate_popular_places, false, "Generate popular places section.");
DEFINE_string(popular_places_data, "",
              "Input Popular Places source file name. Needed both for World intermediate features "
              "generation (2nd pass for World) and popular places section generation (5th pass for "
              "countries).");
DEFINE_string(brands_data, "", "Path to json with OSM objects to brand ID map.");
DEFINE_string(brands_translations_data, "", "Path to json with brands translations and synonyms.");

// Printing stuff.
DEFINE_bool(calc_statistics, false, "Calculate feature statistics for specified mwm bucket files.");
DEFINE_bool(type_statistics, false, "Calculate statistics by type for specified mwm bucket files.");
DEFINE_bool(dump_types, false, "Prints all types combinations and their total count.");
DEFINE_bool(dump_prefixes, false, "Prints statistics on feature's' name prefixes.");
DEFINE_bool(dump_search_tokens, false, "Print statistics on search tokens.");
DEFINE_string(dump_feature_names, "", "Print all feature names by 2-letter locale.");

// Service functions.
DEFINE_bool(generate_classif, false, "Generate classificator.");
DEFINE_bool(generate_packed_borders, false, "Generate packed file with country polygons.");
DEFINE_string(unpack_borders, "",
              "Convert packed_polygons to a directory of polygon files (specify folder).");
DEFINE_bool(unpack_mwm, false,
            "Unpack each section of mwm into a separate file with name filePath.sectionName.");
DEFINE_bool(check_mwm, false, "Check map file to be correct.");
DEFINE_string(delete_section, "", "Delete specified section (defines.hpp) from container.");
DEFINE_bool(generate_addresses_file, false,
            "Generate .addr file (for '--output' option) with full addresses list.");
DEFINE_bool(generate_traffic_keys, false,
            "Generate keys for the traffic map (road segment -> speed group).");

// Generating geo objects key-value.
DEFINE_string(regions_index, "", "Input regions index file.");
DEFINE_string(regions_key_value, "", "Input regions key-value file.");
DEFINE_string(streets_features, "", "Input tmp.mwm file with streets.");
DEFINE_string(streets_key_value, "", "Output streets key-value file.");
DEFINE_string(geo_objects_features, "", "Input tmp.mwm file with geo objects.");
DEFINE_string(ids_without_addresses, "", "Output file with objects ids without addresses.");
DEFINE_string(geo_objects_key_value, "", "Output geo objects key-value file.");
DEFINE_string(allow_addressless_for_countries, "*",
              "Allow addressless buildings for only specified countries separated by commas.");

DEFINE_string(regions_features, "", "Input tmp.mwm file with regions.");

DEFINE_string(popularity_csv, "", "Output csv for popularity.");

DEFINE_bool(dump_mwm_tmp, false, "Prints feature builder objects from .mwm.tmp");

// Common.
DEFINE_bool(verbose, false, "Provide more detailed output.");

using namespace generator;

int GeneratorToolMain(int argc, char ** argv)
{
  CHECK(IsLittleEndian(), ("Only little-endian architectures are supported."));

  google::SetUsageMessage(
        "Takes OSM XML data from stdin and creates data and index files in several passes.");
  google::SetVersionString(std::to_string(omim::build_version::git::kTimestamp) + " " +
                           omim::build_version::git::kHash);
  google::ParseCommandLineFlags(&argc, &argv, true);

  Platform & pl = GetPlatform();
  auto threadsCount = pl.CpuCores();

  if (!FLAGS_user_resource_path.empty())
  {
    pl.SetResourceDir(FLAGS_user_resource_path);
    pl.SetSettingsDir(FLAGS_user_resource_path);
  }

  string const path =
      FLAGS_data_path.empty() ? pl.WritableDir() : base::AddSlashIfNeeded(FLAGS_data_path);

  // So that stray GetWritablePathForFile calls do not crash the generator.
  pl.SetWritableDirForTests(path);

  feature::GenerateInfo genInfo;
  genInfo.m_verbose = FLAGS_verbose;
  genInfo.m_intermediateDir = FLAGS_intermediate_data_path.empty()
                              ? path
                              : base::AddSlashIfNeeded(FLAGS_intermediate_data_path);
  genInfo.m_targetDir = genInfo.m_tmpDir = path;

  /// @todo Probably, it's better to add separate option for .mwm.tmp files.
  if (!FLAGS_intermediate_data_path.empty())
  {
    string const tmpPath = base::JoinPath(genInfo.m_intermediateDir, "tmp");
    if (Platform::MkDir(tmpPath) != Platform::ERR_UNKNOWN)
      genInfo.m_tmpDir = tmpPath;
  }
  if (!FLAGS_node_storage.empty())
    genInfo.SetNodeStorageType(FLAGS_node_storage);
  if (!FLAGS_osm_file_type.empty())
    genInfo.SetOsmFileType(FLAGS_osm_file_type);

  genInfo.m_osmFileName = FLAGS_osm_file_name;
  genInfo.m_failOnCoasts = FLAGS_fail_on_coasts;
  genInfo.m_preloadCache = FLAGS_preload_cache;
  genInfo.m_bookingDataFilename = FLAGS_booking_data;
  genInfo.m_opentableDataFilename = FLAGS_opentable_data;
  genInfo.m_promoCatalogCitiesFilename = FLAGS_promo_catalog_cities;
  genInfo.m_popularPlacesFilename = FLAGS_popular_places_data;
  genInfo.m_brandsFilename = FLAGS_brands_data;
  genInfo.m_brandsTranslationsFilename = FLAGS_brands_translations_data;
  genInfo.m_citiesBoundariesFilename = FLAGS_cities_boundaries_data;
  genInfo.m_versionDate = static_cast<uint32_t>(FLAGS_planet_version);
  genInfo.m_haveBordersForWholeWorld = FLAGS_have_borders_for_whole_world;
  genInfo.m_createWorld = FLAGS_generate_world;
  genInfo.m_makeCoasts = FLAGS_make_coasts;
  genInfo.m_emitCoasts = FLAGS_emit_coasts;
  genInfo.m_fileName = FLAGS_output;
  genInfo.m_genAddresses = FLAGS_generate_addresses_file;
  genInfo.m_idToWikidataFilename = FLAGS_idToWikidata;

  // Use merged style.
  GetStyleReader().SetCurrentStyle(MapStyleMerged);

  classificator::Load();

  // Generate intermediate files.
  if (FLAGS_preprocess)
  {
    LOG(LINFO, ("Generating intermediate data ...."));
    if (!GenerateIntermediateData(genInfo))
      return EXIT_FAILURE;
  }

  DataVersion{FLAGS_osm_file_name}.DumpToPath(genInfo.m_intermediateDir);

  // Generate .mwm.tmp files.
  if (FLAGS_generate_features ||
      FLAGS_generate_world ||
      FLAGS_make_coasts ||
      FLAGS_generate_region_features ||
      FLAGS_generate_streets_features ||
      FLAGS_generate_geo_objects_features)
  {
    RawGenerator rawGenerator(genInfo, threadsCount);
    if (FLAGS_generate_region_features)
      rawGenerator.GenerateRegionFeatures(FLAGS_output);
    if (FLAGS_generate_streets_features)
      rawGenerator.GenerateStreetsFeatures(FLAGS_output);
    if (FLAGS_generate_geo_objects_features)
      rawGenerator.GenerateGeoObjectsFeatures(FLAGS_output);

    if (!rawGenerator.Execute())
      return EXIT_FAILURE;

    genInfo.m_bucketNames = rawGenerator.GetNames();
  }

  if (genInfo.m_bucketNames.empty() && !FLAGS_output.empty())
    genInfo.m_bucketNames.push_back(FLAGS_output);

  if (FLAGS_dump_mwm_tmp)
  {
    for (auto const & fb : feature::ReadAllDatRawFormat(genInfo.GetTmpFileName(FLAGS_output)))
      std::cout << DebugPrint(fb) << std::endl;
  }

  if (!FLAGS_streets_key_value.empty())
  {
    streets::GenerateStreets(FLAGS_regions_index, FLAGS_regions_key_value, FLAGS_streets_features,
                             FLAGS_geo_objects_features, FLAGS_streets_key_value, FLAGS_verbose,
                             threadsCount);
  }

  if (!FLAGS_geo_objects_key_value.empty())
  {
    if (!geo_objects::GenerateGeoObjects(FLAGS_regions_index, FLAGS_regions_key_value,
                                         FLAGS_geo_objects_features, FLAGS_ids_without_addresses,
                                         FLAGS_geo_objects_key_value, FLAGS_verbose, threadsCount))
      return EXIT_FAILURE;
  }

  if (FLAGS_generate_geo_objects_index || FLAGS_generate_regions)
  {
    if (FLAGS_output.empty())
    {
      LOG(LCRITICAL, ("Bad output or intermediate_data_path. Output:", FLAGS_output));
      return EXIT_FAILURE;
    }

    auto const locDataFile = base::JoinPath(path, FLAGS_output + LOC_DATA_FILE_EXTENSION);
    auto const outFile = base::JoinPath(path, FLAGS_output + LOC_IDX_FILE_EXTENSION);
    if (FLAGS_generate_geo_objects_index)
    {
      if (!feature::GenerateGeoObjectsData(FLAGS_geo_objects_features, FLAGS_nodes_list_path,
                                           locDataFile))
      {
        LOG(LCRITICAL, ("Error generating geo objects data."));
        return EXIT_FAILURE;
      }

      LOG(LINFO, ("Saving geo objects index to", outFile));
      if (!indexer::BuildGeoObjectsIndexFromDataFile(
            locDataFile, outFile, DataVersion::LoadFromPath(path).GetVersionJson(),
            DataVersion::kFileTag))
      {
        LOG(LCRITICAL, ("Error generating geo objects index."));
        return EXIT_FAILURE;
      }
    }

    if (FLAGS_generate_regions)
    {
      if (!feature::GenerateRegionsData(FLAGS_regions_features, locDataFile))
      {
        LOG(LCRITICAL, ("Error generating regions data."));
        return EXIT_FAILURE;
      }

      LOG(LINFO, ("Saving regions index to", outFile));

      if (!indexer::BuildRegionsIndexFromDataFile(locDataFile, outFile,
                                                  DataVersion::LoadFromPath(path).GetVersionJson(),
                                                  DataVersion::kFileTag))
      {
        LOG(LCRITICAL, ("Error generating regions index."));
        return EXIT_FAILURE;
      }
      if (!feature::GenerateBorders(FLAGS_regions_features, outFile))
      {
        LOG(LCRITICAL, ("Error generating regions borders."));
        return EXIT_FAILURE;
      }
    }
  }

  if (FLAGS_generate_regions_kv)
  {
    auto const pathInRegionsCollector =
        genInfo.GetTmpFileName(genInfo.m_fileName, regions::CollectorRegionInfo::kDefaultExt);
    auto const pathInRegionsTmpMwm = genInfo.GetTmpFileName(genInfo.m_fileName);
    auto const pathOutRepackedRegionsTmpMwm =
        genInfo.GetTmpFileName(genInfo.m_fileName + "_repacked");
    auto const pathOutRegionsKv = genInfo.GetIntermediateFileName(genInfo.m_fileName, ".jsonl");
    regions::GenerateRegions(pathInRegionsTmpMwm, pathInRegionsCollector, pathOutRegionsKv,
                             pathOutRepackedRegionsTmpMwm, FLAGS_verbose, threadsCount);
  }

  string const datFile = base::JoinPath(path, FLAGS_output + DATA_FILE_EXTENSION);

  if (FLAGS_dump_types)
    feature::DumpTypes(datFile);

  if (FLAGS_dump_prefixes)
    feature::DumpPrefixes(datFile);

  if (FLAGS_dump_feature_names != "")
    feature::DumpFeatureNames(datFile, FLAGS_dump_feature_names);

  if (FLAGS_unpack_mwm)
    UnpackMwm(datFile);

  if (FLAGS_check_mwm)
    check_model::ReadFeatures(datFile);

  return 0;
}

void ErrorHandler(int signum)
{
  // Avoid recursive calls.
  signal(signum, SIG_DFL);

  // If there was an exception, then we will print the message.
  try
  {
    if (auto const eptr = current_exception())
      rethrow_exception(eptr);
  }
  catch (RootException const & e)
  {
    cerr << "Core exception: " << e.Msg() << "\n";
  }
  catch (exception const & e)
  {
    cerr << "Std exception: " << e.what() << "\n";
  }
  catch (...)
  {
    cerr << "Unknown exception.\n";
  }

  // Print stack stack.
  cerr << boost::stacktrace::stacktrace();
  // We raise the signal SIGABRT, so that there would be an opportunity to make a core dump.
  raise(SIGABRT);
}

int main(int argc, char ** argv)
{
  signal(SIGABRT, ErrorHandler);
  signal(SIGSEGV, ErrorHandler);

  return GeneratorToolMain(argc, argv);
}
