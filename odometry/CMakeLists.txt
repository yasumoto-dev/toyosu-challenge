add_executable(odometry src/odom_converter.cpp)
target_include_directories(odometry PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../config/include
)
target_link_libraries(odometry PRIVATE utility ssm m)
