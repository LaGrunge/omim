project(map)


set(SRC
  feature_vec_model.cpp
  feature_vec_model.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
omim_link_libraries(${PROJECT_NAME}
  base
  coding
  editor
  geometry
  indexer
)

