#include "generator/place_node.hpp"

#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/country_specifier.hpp"
#include "generator/regions/region.hpp"
#include "generator/regions/country_specifier_builder.hpp"

#include <vector>
#include <string>

namespace generator
{
namespace regions
{
namespace specs
{

class RomaniaSpecifier final : public CountrySpecifier
{
public:
  static std::vector<std::string> GetCountryNames() { return {"Romania"}; }

private:
  // CountrySpecifier overrides:
  PlaceLevel GetSpecificCountryLevel(Region const & region) const override;
};

REGISTER_COUNTRY_SPECIFIER(RomaniaSpecifier);

PlaceLevel RomaniaSpecifier::GetSpecificCountryLevel(Region const & region) const
{
  AdminLevel adminLevel = region.GetAdminLevel();
  switch (adminLevel)
  {
  case AdminLevel::Three:
    return PlaceLevel::Region;  // Historical provinces (Transylvania, Moldavia ...)
  case AdminLevel::Four:
    return PlaceLevel::Subregion;  // Counties (Judeţe) and the Municipality of Bucharest
  default: break;
  }

  return PlaceLevel::Unknown;
}
}  // namespace specs
}  // namespace regions
}  // namespace generator
