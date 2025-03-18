cmake_minimum_required(VERSION 3.10)
project(USBTest C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-Wall -Wextra -Wformat-truncation -O2")

# Find SDL2 and SDL2_ttf
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# Include directories
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})

# Add executable
add_executable(usb_test usb_test.c)

# Link libraries
target_link_libraries(usb_test ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
