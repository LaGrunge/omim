project(geocoder_tests)

set(
  SRC
  geocoder_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  platform_tests_support
  geocoder
  search
  indexer
  platform
  coding
  base
  jansson
  ${LIBZ}
)

