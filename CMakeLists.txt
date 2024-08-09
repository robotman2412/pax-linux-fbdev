
# SPDX-License-Identidier: MIT

cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(pax-fbdev)
set(target pax-fbdev)
add_compile_options(-ggdb -O2)

add_subdirectory(pax-graphics)

add_executable(${target}
    src/main.c
)

target_include_directories(${target} PRIVATE src)
target_link_libraries(${target} pax_graphics)
