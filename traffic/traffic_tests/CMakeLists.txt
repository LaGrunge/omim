project(traffic_tests)

include_directories(${OMIM_ROOT}/3party/jansson/src)

set(
  SRC
  traffic_info_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  traffic
  routing_common
  indexer
  editor
  platform_tests_support
  platform
  coding
  geometry
  base
  icu
  jansson
  oauthcpp
  opening_hours
  protobuf
  pugixml
  ${LIBZ}
)

