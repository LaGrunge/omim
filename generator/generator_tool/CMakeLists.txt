project(generator_tool)

set(SRC generator_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})
omim_link_libraries(
  ${PROJECT_NAME}
  generator
  geocoder
  gflags
  ${CMAKE_DL_LIBS}  
)
