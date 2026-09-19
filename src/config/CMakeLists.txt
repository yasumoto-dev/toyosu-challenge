find_package(yaml-cpp REQUIRED)

add_executable(config src/config.cpp src/param.cpp)
target_include_directories(config PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(config PRIVATE utility yaml-cpp::yaml-cpp ssm m pthread)
