project(extract_addr)

include_directories(
  ${OMIM_ROOT}/3party/jansson/src
)

set(
  SRC
  extract_addr.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator
  search
  routing
  traffic
  routing_common
  indexer
  editor
  geometry
  platform
  coding
  base
  minizip
  jansson
  pugixml
  opening_hours
  succinct
  oauthcpp
  expat
  protobuf
  icu
  ${LIBZ}
)

