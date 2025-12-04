cmake_minimum_required(VERSION 4.0)
project(test)
find_package(fmt CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
set(CMAKE_CXX_STANDARD 20)

add_executable(test main.cpp)

target_link_libraries(test PRIVATE
        fmt::fmt
        absl::btree
)