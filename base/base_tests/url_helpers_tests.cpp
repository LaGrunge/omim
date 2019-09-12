#include "testing/testing.hpp"

#include "base/url_helpers.hpp"

UNIT_TEST(Url_Join)
{
  TEST_EQUAL("", base::url::Join("", ""), ());
  TEST_EQUAL("geocore/", base::url::Join("", "geocore/"), ());
  TEST_EQUAL("geocore/", base::url::Join("geocore/", ""), ());
  TEST_EQUAL("geocore/strings", base::url::Join("geocore", "strings"), ());
  TEST_EQUAL("geocore/strings", base::url::Join("geocore/", "strings"), ());
  TEST_EQUAL("../../geocore/strings", base::url::Join("..", "..", "geocore", "strings"), ());
  TEST_EQUAL("../../geocore/strings", base::url::Join("../", "..", "geocore/", "strings"), ());
  TEST_EQUAL("geocore/strings", base::url::Join("geocore/", "/strings"), ());
  TEST_EQUAL("../../geocore/strings", base::url::Join("../", "/../", "/geocore/", "/strings"), ());
  TEST_EQUAL("../geocore/strings", base::url::Join("../", "", "/geocore/", "/strings"), ());
}
