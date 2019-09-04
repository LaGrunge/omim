project(generator_tool)

set(SRC generator_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})
omim_link_libraries(
  ${PROJECT_NAME}
  generator
  gflags
  ${CMAKE_DL_LIBS}
)

link_qt5_core(${PROJECT_NAME})
link_qt5_network(${PROJECT_NAME})
