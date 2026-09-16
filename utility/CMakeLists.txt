# utilityライブラリを作成
add_library(utility STATIC src/utility.cpp)

target_include_directories(utility PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


