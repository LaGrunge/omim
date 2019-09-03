project(pysearch)

set(
  SRC
  bindings.cpp
)

include_directories(${CMAKE_BINARY_DIR})

omim_add_library(${PROJECT_NAME} MODULE ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  search_quality
  search_tests_support
  search
  storage
  editor
  indexer
  geometry
  platform
  mwm_diff
  coding
  base
  bsdiff
  icu
  jansson
  oauthcpp
  opening_hours
  protobuf
  pugixml
  succinct
  ${PYTHON_LIBRARIES}
  ${Boost_LIBRARIES}
  ${LIBZ}
)


set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
