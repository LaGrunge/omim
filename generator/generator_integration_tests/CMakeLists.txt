project(generator_integration_tests)

set(
  SRC
  features_tests.cpp
  helpers.cpp
  helpers.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator_tests_support
  platform_tests_support
  generator
  routing
  search
  storage
  traffic
  routing_common
  descriptions
  transit
  editor
  indexer
  platform
  geometry
  coding
  base
  expat
  icu
  jansson
  protobuf
  minizip
  succinct
  pugixml
  tess2
  gflags
  oauthcpp
  opening_hours
  # TODO(syershov): Use FindPackage.
  sqlite3
  ${LIBZ}
)

if (PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    dl
  )
endif()

