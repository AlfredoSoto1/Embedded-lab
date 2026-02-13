cmake_minimum_required(VERSION 3.16)
project(lcd_gpiod C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(lcd lcd2.c)

# Prefer pkg-config if available (most Raspberry Pi OS setups)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)

target_include_directories(lcd PRIVATE ${GPIOD_INCLUDE_DIRS})
target_link_directories(lcd PRIVATE ${GPIOD_LIBRARY_DIRS})
target_link_libraries(lcd PRIVATE ${GPIOD_LIBRARIES})

# Helpful warnings
target_compile_options(lcd PRIVATE -Wall -Wextra -Wpedantic)
