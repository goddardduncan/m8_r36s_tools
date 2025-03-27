cmake_minimum_required(VERSION 3.10)
project(backup C)

set(CMAKE_C_STANDARD 99)

# Find SDL2 and SDL2_ttf via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# Include directories for SDL2 and TTF
include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)

# Source file(s)
add_executable(backup backup.c)

# Link SDL2 and SDL2_ttf libraries
target_link_libraries(backup
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
)

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
