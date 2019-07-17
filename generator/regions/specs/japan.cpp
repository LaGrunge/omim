#include "generator/regions/specs/japan.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel JapanSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // Prefecture border
  case AdminLevel::Five:
    return PlaceLevel::Subregion;  // Sub Prefecture border (振興局・支庁 in Hokkaido)
  case AdminLevel::Seven:
    return PlaceLevel::Locality;  // Municipal border (Cities, Towns, Villages, Special wards of
                                  // Tokyo)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
