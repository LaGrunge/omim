#include "generator/geo_objects/geo_objects.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/geo_objects/geo_object_info_getter.hpp"
#include "generator/geo_objects/geo_objects_filter.hpp"
#include "generator/key_value_storage.hpp"
#include "generator/locality_sorter.hpp"
#include "generator/regions/region_base.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/locality_index.hpp"
#include "indexer/locality_index_builder.hpp"

#include "coding/mmap_reader.hpp"

#include "coding/internal/file_data.hpp"

#include "geometry/mercator.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <fstream>
#include <functional>
#include <future>
#include <mutex>

#include <boost/optional.hpp>

#include "3party/jansson/myjansson.hpp"

using namespace feature;

namespace generator
{
namespace geo_objects
{
namespace
{
void UpdateCoordinates(m2::PointD const & point, base::JSONPtr & json)
{
  auto geometry = json_object_get(json.get(), "geometry");
  auto coordinates = json_object_get(geometry, "coordinates");
  if (json_array_size(coordinates) == 2)
  {
    auto const latLon = MercatorBounds::ToLatLon(point);
    json_array_set_new(coordinates, 0, ToJSON(latLon.m_lon).release());
    json_array_set_new(coordinates, 1, ToJSON(latLon.m_lat).release());
  }
}

base::JSONPtr AddAddress(FeatureBuilder const & fb, KeyValue const & regionKeyValue)
{
  auto result = regionKeyValue.second->MakeDeepCopyJson();

  UpdateCoordinates(fb.GetKeyPoint(), result);

  auto properties = base::GetJSONObligatoryField(result.get(), "properties");
  auto address = base::GetJSONObligatoryFieldByPath(properties, "locales", "default", "address");
  auto const street = fb.GetParams().GetStreet();
  if (!street.empty())
    ToJSONObject(*address, "street", street);

  // By writing home null in the field we can understand that the house has no address.
  auto const house = fb.GetParams().house.Get();
  if (!house.empty())
    ToJSONObject(*address, "building", house);
  else
    ToJSONObject(*address, "building", base::NewJSONNull());

  Localizator localizator(*properties);
  localizator.SetLocale("name", Localizator::EasyObjectWithTranslation(fb.GetMultilangName()));

  int const kHouseOrPoiRank = 30;
  ToJSONObject(*properties, "rank", kHouseOrPoiRank);

  ToJSONObject(*properties, "dref", KeyValueStorage::SerializeDref(regionKeyValue.first));
  // auto locales = json_object_get(result.get(), "locales");
  // auto en = json_object_get(result.get(), "en");
  // todo(maksimandrianov): Add en locales.
  return result;
}

void AddBuildingsAndThingsWithHousesThenEnrichAllWithRegionAddresses(
    KeyValueStorage & geoObjectsKv,
    GeoObjectsGenerator::RegionInfoGetterProxy const & regionInfoGetter,
    std::string const & pathInGeoObjectsTmpMwm, bool verbose, size_t threadsCount)
{
  std::mutex updateMutex;
  auto const concurrentTransformer = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
    if (!GeoObjectsFilter::IsBuilding(fb) && !GeoObjectsFilter::HasHouse(fb))
      return;

    auto regionKeyValue = regionInfoGetter.FindDeepest(fb.GetKeyPoint());
    if (!regionKeyValue)
      return;

    auto const id = fb.GetMostGenericOsmId().GetEncodedId();
    auto jsonValue = AddAddress(fb, *regionKeyValue);

    std::lock_guard<std::mutex> lock(updateMutex);
    geoObjectsKv.Insert(id, JsonValue{std::move(jsonValue)});
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, concurrentTransformer);
  LOG(LINFO, ("Added", geoObjectsKv.Size(), "geo objects with addresses."));
}

struct NullBuildingsInfo
{
  std::unordered_map<base::GeoObjectId, base::GeoObjectId> m_addressPoints2Buildings;
  // Quite possible to have many points for one building. We want to use
  // their addresses for POIs according to buildings and have no idea how to distinguish between
  // them, so take one random
  std::unordered_map<base::GeoObjectId, base::GeoObjectId> m_Buildings2AddressPoint;
};

NullBuildingsInfo GetHelpfulNullBuildings(GeoObjectInfoGetter const & geoObjectInfoGetter,
                                          std::string const & pathInGeoObjectsTmpMwm,
                                          size_t threadsCount)
{
  NullBuildingsInfo result;
  static int64_t counter = 0;
  std::mutex updateMutex;
  auto const saveIdFold = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
    if (!GeoObjectsFilter::HasHouse(fb) || !fb.IsPoint())
      return;

    auto const buildingId = geoObjectInfoGetter.Search(
        fb.GetKeyPoint(), [](JsonValue const & json) { return !JsonHasBuilding(json); });
    if (!buildingId)
      return;

    auto const id = fb.GetMostGenericOsmId();

    std::lock_guard<std::mutex> lock(updateMutex);
    result.m_addressPoints2Buildings[id] = *buildingId;
    counter++;
    if (counter % 100000)
      LOG(LINFO, (counter, "Helpful building added"));
    result.m_Buildings2AddressPoint[*buildingId] = id;
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, saveIdFold);
  return result;
}

using BuildingsGeometries =
    std::unordered_map<base::GeoObjectId, feature::FeatureBuilder::Geometry>;

BuildingsGeometries GetBuildingsGeometry(std::string const & pathInGeoObjectsTmpMwm,
                                         NullBuildingsInfo const & buildingsInfo,
                                         size_t threadsCount)
{
  BuildingsGeometries result;
  std::mutex updateMutex;
  static int64_t counter = 0;

  auto const saveIdFold = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
    auto const id = fb.GetMostGenericOsmId();
    if (buildingsInfo.m_Buildings2AddressPoint.find(id) ==
            buildingsInfo.m_Buildings2AddressPoint.end() ||
        fb.GetParams().GetGeomType() != feature::GeomType::Area)
      return;

    std::lock_guard<std::mutex> lock(updateMutex);

    if (result.find(id) != result.end())
      LOG(LINFO, ("More than one geometry for", id));
    else
      result[id] = fb.GetGeometry();

    counter++;
    if (counter % 100000)
      LOG(LINFO, (counter, "Building geometries added"));
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, saveIdFold);
  LOG(LINFO, (sizeof(result), " is size of geometries"));
  return result;
}

size_t AddBuildingGeometriesToAddressPoints(std::string const & pathInGeoObjectsTmpMwm,
                                            NullBuildingsInfo const & buildingsInfo,
                                            BuildingsGeometries const & geometries,
                                            size_t threadsCount)
{
  auto const path = GetPlatform().TmpPathForFile();
  FeaturesCollector collector(path);
  std::atomic_size_t pointsEnriched{0};
  std::mutex collectorMutex;
  auto concurrentCollector = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
    auto const id = fb.GetMostGenericOsmId();
    auto point2BuildingIt = buildingsInfo.m_addressPoints2Buildings.find(id);
    if (point2BuildingIt != buildingsInfo.m_addressPoints2Buildings.end())
    {
      auto geometryIt = geometries.find(point2BuildingIt->second);
      if (geometryIt != geometries.end())
      {
        auto const & geometry = geometryIt->second;

        // ResetGeometry does not reset center but SetCenter changes geometry type to Point and
        // adds center to bounding rect
        fb.SetCenter({});
        // ResetGeometry clears bounding rect
        fb.ResetGeometry();
        fb.GetParams().SetGeomType(feature::GeomType::Area);

        for (std::vector<m2::PointD> poly : geometry)
          fb.AddPolygon(poly);

        fb.PreSerialize();
        ++pointsEnriched;
        if (pointsEnriched % 100000)
          LOG(LINFO, (pointsEnriched, "Points enriched with geometry"));
      }
      else
      {
        LOG(LINFO, (point2BuildingIt->second, "is a null building with strange geometry"));
      }
    }
    std::lock_guard<std::mutex> lock(collectorMutex);
    collector.Collect(fb);
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, concurrentCollector);

  CHECK(base::RenameFileX(path, pathInGeoObjectsTmpMwm), ());
  return pointsEnriched;
}

NullBuildingsInfo EnrichPointsWithOuterBuildingGeometry(
    GeoObjectInfoGetter const & geoObjectInfoGetter, std::string const & pathInGeoObjectsTmpMwm,
    size_t threadsCount)
{
  auto const buildingInfo =
      GetHelpfulNullBuildings(geoObjectInfoGetter, pathInGeoObjectsTmpMwm, threadsCount);

  LOG(LINFO, ("Found", buildingInfo.m_addressPoints2Buildings.size(),
              "address points with outer building geometry"));
  LOG(LINFO,
      ("Found", buildingInfo.m_Buildings2AddressPoint.size(), "helpful addressless buildings"));
  auto const buildingGeometries =
      GetBuildingsGeometry(pathInGeoObjectsTmpMwm, buildingInfo, threadsCount);
  LOG(LINFO, ("Saved", buildingGeometries.size(), "buildings geometries"));

  size_t const pointsCount = AddBuildingGeometriesToAddressPoints(
      pathInGeoObjectsTmpMwm, buildingInfo, buildingGeometries, threadsCount);

  LOG(LINFO, (pointsCount, "address points were enriched with outer building geomery"));
  return buildingInfo;
}

template <class Activist>
auto Measure(std::string activity, Activist && activist)
{
  LOG(LINFO, ("Start", activity));
  auto timer = base::Timer();
  SCOPE_GUARD(_, [&]() { LOG(LINFO, ("Finish", activity, timer.ElapsedSeconds(), "seconds.")); });

  return activist();
}
}  // namespace

bool JsonHasBuilding(JsonValue const & json)
{
  auto && address =
      base::GetJSONObligatoryFieldByPath(json, "properties", "locales", "default", "address");

  auto && building = base::GetJSONOptionalField(address, "building");
  return building && !base::JSONIsNull(building);
}

boost::optional<indexer::GeoObjectsIndex<IndexReader>> MakeTempGeoObjectsIndex(
    std::string const & pathToGeoObjectsTmpMwm)
{
  auto const dataFile = GetPlatform().TmpPathForFile();
  SCOPE_GUARD(removeDataFile, std::bind(Platform::RemoveFileIfExists, std::cref(dataFile)));
  if (!GenerateGeoObjectsData(pathToGeoObjectsTmpMwm, "" /* nodesFile */, dataFile))
  {
    LOG(LCRITICAL, ("Error generating geo objects data."));
    return {};
  }

  auto const indexFile = GetPlatform().TmpPathForFile();
  SCOPE_GUARD(removeIndexFile, std::bind(Platform::RemoveFileIfExists, std::cref(indexFile)));
  if (!indexer::BuildGeoObjectsIndexFromDataFile(dataFile, indexFile))
  {
    LOG(LCRITICAL, ("Error generating geo objects index."));
    return {};
  }

  return indexer::ReadIndex<indexer::GeoObjectsIndexBox<IndexReader>, MmapReader>(indexFile);
}

std::shared_ptr<JsonValue> FindHousePoi(FeatureBuilder const & fb,
                                        GeoObjectInfoGetter const & geoObjectInfoGetter,
                                        NullBuildingsInfo const & buildingsInfo)
{
  std::shared_ptr<JsonValue> house = geoObjectInfoGetter.Find(fb.GetKeyPoint(), JsonHasBuilding);
  if (house)
    return house;

  std::vector<base::GeoObjectId> potentialIds =
      geoObjectInfoGetter.SearchObjectsInIndex(fb.GetKeyPoint());

  for (base::GeoObjectId id : potentialIds)
  {
    auto const it = buildingsInfo.m_Buildings2AddressPoint.find(id);
    if (it != buildingsInfo.m_Buildings2AddressPoint.end())
      return geoObjectInfoGetter.GetKeyValueStorage().Find(it->second.GetEncodedId());
  }

  return {};
}

base::JSONPtr MakeJsonValueWithNameFromFeature(FeatureBuilder const & fb, JsonValue const & json)
{
  auto jsonWithAddress = json.MakeDeepCopyJson();

  auto properties = json_object_get(jsonWithAddress.get(), "properties");
  Localizator localizator(*properties);
  localizator.SetLocale("name", Localizator::EasyObjectWithTranslation(fb.GetMultilangName()));

  UpdateCoordinates(fb.GetKeyPoint(), jsonWithAddress);
  return jsonWithAddress;
}

void AddPoisEnrichedWithHouseAddresses(KeyValueStorage & geoObjectsKv,
                                       GeoObjectInfoGetter const & geoObjectInfoGetter,
                                       NullBuildingsInfo const & buildingsInfo,
                                       std::string const & pathInGeoObjectsTmpMwm,
                                       std::ostream & streamPoiIdsToAddToLocalityIndex,
                                       bool verbose, size_t threadsCount)
{
  auto const addressObjectsCount = geoObjectsKv.Size();
  size_t counter = 0;
  std::mutex updateMutex;
  auto const concurrentTransformer = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
    if (!GeoObjectsFilter::IsPoi(fb))
      return;
    if (GeoObjectsFilter::IsBuilding(fb) || GeoObjectsFilter::HasHouse(fb))
      return;

    auto const house = FindHousePoi(fb, geoObjectInfoGetter, buildingsInfo);
    if (!house)
      return;

    auto const id = fb.GetMostGenericOsmId().GetEncodedId();
    auto jsonValue = MakeJsonValueWithNameFromFeature(fb, *house);

    std::lock_guard<std::mutex> lock(updateMutex);
    counter++;
    if (counter % 100000)
      LOG(LINFO, (counter, "pois added added"));
    geoObjectsKv.Insert(id, JsonValue{std::move(jsonValue)});
    streamPoiIdsToAddToLocalityIndex << id << "\n";
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, concurrentTransformer);
  LOG(LINFO,
      ("Added ", geoObjectsKv.Size() - addressObjectsCount, "geo objects without addresses."));
}

void FilterAddresslessThanGaveTheirGeometryToInnerPoints(std::string const & pathInGeoObjectsTmpMwm,
                                                         NullBuildingsInfo const & buildingsInfo,
                                                         size_t threadsCount)
{
  auto const path = GetPlatform().TmpPathForFile();
  FeaturesCollector collector(path);
  std::mutex collectorMutex;
  auto concurrentCollect = [&](FeatureBuilder const & fb, uint64_t /* currPos */) {
    auto const id = fb.GetMostGenericOsmId();
    if (buildingsInfo.m_Buildings2AddressPoint.find(id) !=
        buildingsInfo.m_Buildings2AddressPoint.end())
      return;

    std::lock_guard<std::mutex> lock(collectorMutex);
    collector.Collect(fb);
  };

  ForEachParallelFromDatRawFormat(threadsCount, pathInGeoObjectsTmpMwm, concurrentCollect);
  CHECK(base::RenameFileX(path, pathInGeoObjectsTmpMwm), ());
}

GeoObjectsGenerator::GeoObjectsGenerator(std::string pathInRegionsIndex,
                                         std::string pathInRegionsKv,
                                         std::string pathInGeoObjectsTmpMwm,
                                         std::string pathOutIdsWithoutAddress,
                                         std::string pathOutGeoObjectsKv, bool verbose,
                                         size_t threadsCount)
  : m_pathInGeoObjectsTmpMwm(std::move(pathInGeoObjectsTmpMwm))
  , m_pathOutPoiIdsToAddToLocalityIndex(std::move(pathOutIdsWithoutAddress))
  , m_pathOutGeoObjectsKv(std::move(pathOutGeoObjectsKv))
  , m_verbose(verbose)
  , m_threadsCount(threadsCount)
  , m_geoObjectsKv(InitGeoObjectsKv(m_pathOutGeoObjectsKv))
  , m_regionInfoGetter(pathInRegionsIndex, pathInRegionsKv)

{
}

GeoObjectsGenerator::GeoObjectsGenerator(RegionInfoGetter && regionInfoGetter,
                                         std::string pathInGeoObjectsTmpMwm,
                                         std::string pathOutIdsWithoutAddress,
                                         std::string pathOutGeoObjectsKv, bool verbose,
                                         size_t threadsCount)
  : m_pathInGeoObjectsTmpMwm(std::move(pathInGeoObjectsTmpMwm))
  , m_pathOutPoiIdsToAddToLocalityIndex(std::move(pathOutIdsWithoutAddress))
  , m_pathOutGeoObjectsKv(std::move(pathOutGeoObjectsKv))
  , m_verbose(verbose)
  , m_threadsCount(threadsCount)
  , m_geoObjectsKv(InitGeoObjectsKv(m_pathOutGeoObjectsKv))
  , m_regionInfoGetter(std::move(regionInfoGetter))
{
}

bool GeoObjectsGenerator::GenerateGeoObjects()
{
  return Measure("generating geo objects", [&]() { return GenerateGeoObjectsPrivate(); });
}

bool GeoObjectsGenerator::GenerateGeoObjectsPrivate()
{
  auto geoObjectIndexFuture =
      std::async(std::launch::async, MakeTempGeoObjectsIndex, m_pathInGeoObjectsTmpMwm);

  AddBuildingsAndThingsWithHousesThenEnrichAllWithRegionAddresses(
      m_geoObjectsKv, m_regionInfoGetter, m_pathInGeoObjectsTmpMwm, m_verbose, m_threadsCount);

  LOG(LINFO, ("Geo objects with addresses were built."));

  auto geoObjectIndex = geoObjectIndexFuture.get();

  LOG(LINFO, ("Index was built."));

  if (!geoObjectIndex)
    return false;

  GeoObjectInfoGetter const geoObjectInfoGetter{std::move(*geoObjectIndex), m_geoObjectsKv};

  LOG(LINFO, ("Enrich address points with outer null building geometry."));

  NullBuildingsInfo const & buildingInfo = EnrichPointsWithOuterBuildingGeometry(
      geoObjectInfoGetter, m_pathInGeoObjectsTmpMwm, m_threadsCount);

  std::ofstream streamPoiIdsToAddToLocalityIndex(m_pathOutPoiIdsToAddToLocalityIndex);

  AddPoisEnrichedWithHouseAddresses(m_geoObjectsKv, geoObjectInfoGetter, buildingInfo,
                                    m_pathInGeoObjectsTmpMwm, streamPoiIdsToAddToLocalityIndex,
                                    m_verbose, m_threadsCount);

  FilterAddresslessThanGaveTheirGeometryToInnerPoints(m_pathInGeoObjectsTmpMwm, buildingInfo,
                                                      m_threadsCount);

  LOG(LINFO, ("Addressless buildings with geometry we used for inner points were filtered"));

  LOG(LINFO, ("Geo objects without addresses were built."));
  LOG(LINFO, ("Geo objects key-value storage saved to", m_pathOutGeoObjectsKv));
  LOG(LINFO, ("Ids of POIs without addresses saved to", m_pathOutPoiIdsToAddToLocalityIndex));
  return true;
}
}  // namespace geo_objects
}  // namespace generator
