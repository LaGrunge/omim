project(routing_quality_tool)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(
  SRC
  parse_input_params.hpp
  routing_quality_tool.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  routing_quality
  generator
  routing
  traffic
  routing_common
  transit
  storage
  indexer
  platform
  mwm_diff
  bsdiff
  geometry
  coding
  base
  icu
  jansson
  oauthcpp
  protobuf
  gflags
  ${LIBZ}
)

