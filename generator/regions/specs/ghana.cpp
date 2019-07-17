#include "generator/regions/specs/ghana.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel GhanaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    // Region
  case AdminLevel::Six: return PlaceLevel::Subregion;  // District
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
