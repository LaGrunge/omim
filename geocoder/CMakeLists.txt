project(geocoder)

set(
  SRC
  geocoder.cpp
  geocoder.hpp
  hierarchy.cpp
  hierarchy.hpp
  hierarchy_reader.cpp
  hierarchy_reader.hpp
  house_numbers_matcher.cpp
  house_numbers_matcher.hpp
  index.cpp
  index.hpp
  name_dictionary.cpp
  name_dictionary.hpp
  result.cpp
  result.hpp
  types.cpp
  types.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
omim_link_libraries(${PROJECT_NAME} base indexer jansson)

add_subdirectory(geocoder_cli)
omim_add_test_subdirectory(geocoder_tests)
