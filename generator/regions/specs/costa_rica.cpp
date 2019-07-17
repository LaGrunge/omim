#include "generator/regions/specs/costa_rica.hpp"

#include "generator/regions/country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
namespace specs
{
REGISTER_COUNTRY_SPECIFIER(CostaRicaSpecifier);

PlaceLevel CostaRicaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Four: return PlaceLevel::Region;      // Provincia
  case AdminLevel::Six: return PlaceLevel::Subregion;    // Cantón
  case AdminLevel::Ten: return PlaceLevel::Sublocality;  // 	Barrio
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
