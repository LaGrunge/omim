project(generator_tool)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(SRC generator_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  generator
  indexer
  platform
  geometry
  coding
  base
  opening_hours
  expat
  icu
  jansson
  protobuf
  minizip
  succinct
  tess2
  gflags
  oauthcpp
  ${CMAKE_DL_LIBS}
  ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
