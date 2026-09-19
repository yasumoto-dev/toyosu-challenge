add_executable(odometry src/odom_converter.cpp)
target_include_directories(odometry PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../config/include
)
target_link_libraries(odometry PRIVATE utility ssm m)

# 簡易 odom_gl 用ロガー
add_executable(odom_logger src/odom_logger.cpp)
target_include_directories(odom_logger PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(odom_logger PRIVATE utility ssm m)
