cmake_minimum_required(VERSION 3.10)
project(R36s_Channel_Selector C)

set(CMAKE_C_STANDARD 11)

# Find SDL2 using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# Include necessary directories
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})

# Link directories
link_directories(${SDL2_LIBRARY_DIRS} ${SDL2_TTF_LIBRARY_DIRS})

# Add executable
add_executable(channel_selector ff.c)

# Link libraries manually
target_link_libraries(channel_selector ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")