find_package(OpenCV REQUIRED)

add_executable(urg_handler src/urg_handler.cpp src/lidar.cpp)
target_include_directories(urg_handler PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../config/include)
target_link_libraries(urg_handler PRIVATE scip2awd ssm m)

add_executable(urg_viewer src/urg_viewer.cpp)
target_include_directories(urg_viewer PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(urg_viewer PRIVATE ssm m ${OpenCV_LIBS})
