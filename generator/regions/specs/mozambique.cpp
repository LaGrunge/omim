#include "generator/regions/specs/mozambique.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel MozambiqueSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // States (Províncias)
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
