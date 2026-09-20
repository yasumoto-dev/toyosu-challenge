add_executable(urg_handler src/urg_handler.cpp)
target_include_directories(urg_handler PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(urg_handler PRIVATE ssm m scip2awd)
