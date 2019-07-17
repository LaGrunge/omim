#include "generator/regions/specs/antigua_and_barbuda.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
PlaceLevel AntiguaAndBarbudaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;  // Parishes and dependencies borders
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
