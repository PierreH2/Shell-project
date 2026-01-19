cmake_minimum_required(VERSION 3.16)
project(ShellProject C)
enable_testing()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-Wall -Wextra -g")

add_subdirectory(src)
add_subdirectory(debug)
add_subdirectory(tests)
