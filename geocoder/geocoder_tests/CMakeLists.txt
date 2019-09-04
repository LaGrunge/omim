project(geocoder_tests)

set(
  SRC
  geocoder_tests.cpp
  house_numbers_matcher_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  platform_tests_support
  geocoder
  indexer
  platform
  coding
  base
  jansson
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
