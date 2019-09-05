project(geocoder_cli)

set(
  SRC
  geocoder_cli.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  geocoder
  gflags
)
