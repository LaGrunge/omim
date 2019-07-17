#include "generator/regions/specs/mali.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(MaliSpecifier);

PlaceLevel MaliSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;    //	Régions
  case AdminLevel::Six: return PlaceLevel::Subregion;  // Cercles
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
