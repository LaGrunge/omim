#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/country_specifier.hpp"
#include "generator/regions/country_specifier_builder.hpp"
#include "generator/regions/region.hpp"
#include "generator/regions/region_base.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include <string>
#include <vector>

#include <boost/geometry.hpp>

namespace generator
{
namespace regions
{
namespace specs
{
class UkraineSpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames() { return {"Ukraine"}; }

  void RectifyBoundary(std::vector<Region> & outers, std::vector<Region> const & planet) override
  {
    FixAdministrativeRegion1(outers, planet);
    FixAdministrativeRegion2(outers, planet);
  }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override
  {
    AdminLevel adminLevel = region.GetAdminLevel();
    switch (adminLevel)
    {
    case AdminLevel::Four: return PlaceLevel::Region;    // Oblasts
    case AdminLevel::Six: return PlaceLevel::Subregion;  // районы в областях
    case AdminLevel::Seven: return PlaceLevel::Sublocality;  // Административные районы в городах
    default: break;
    }

    return PlaceLevel::Unknown;
  }

  void FixAdministrativeRegion1(std::vector<Region> & outers, std::vector<Region> const & planet)
  {
    auto const * region = FindAdministrativeRegion1(planet);
    if (!region)
    {
      LOG(LWARNING, ("Failed to fix region1 for Ukraine"));
      return;
    }

    ExcludeRegionArea(outers, *region);
  }

  void FixAdministrativeRegion2(std::vector<Region> & outers, std::vector<Region> const & planet)
  {
    auto const * region = FindAdministrativeRegion2(planet);
    if (!region)
    {
      LOG(LWARNING, ("Failed to fix region2 for Ukraine"));
      return;
    }

    ExcludeRegionArea(outers, *region);
  }

  Region const * FindAdministrativeRegion1(std::vector<Region> const & planet)
  {
    auto const center = MercatorBounds::FromLatLon({45.1890034, 34.7401104});
    for (auto const & region : planet)
    {
      if (region.GetAdminLevel() == AdminLevel::Unknown)
        continue;

      auto && isoCode = region.GetIsoCode();
      if (!isoCode || *isoCode != "RU")
        continue;

      auto const & name = region.GetName();
      if ((name == "Республика Крым" || name == "Крым") &&
          region.Contains({center.x, center.y}))
      {
        return &region;
      }
    }

    return nullptr;
  }

  Region const * FindAdministrativeRegion2(std::vector<Region> const & planet)
  {
    auto const center = MercatorBounds::FromLatLon({44.5547288, 33.4720239});
    for (auto const & region : planet)
    {
      if (region.GetAdminLevel() == AdminLevel::Unknown)
        continue;

      auto && isoCode = region.GetIsoCode();
      if (!isoCode || *isoCode != "RU")
        continue;

      auto const & name = region.GetName();
      if (name == "Севастополь" && region.Contains({center.x, center.y}))
        return &region;
    }

    return nullptr;
  }
};

REGISTER_COUNTRY_SPECIFIER(UkraineSpecifier);
}  // namespace specs
}  // namespace regions
}  // namespace generator
