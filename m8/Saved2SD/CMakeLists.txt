cmake_minimum_required(VERSION 3.10)
project(AccompanimentApp C)

set(CMAKE_C_STANDARD 99)

# Find SDL2 and required libraries
find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# Include necessary directories
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})

# Add executable
add_executable(accompaniment_app sendsaved.c)

# Link libraries
target_link_libraries(accompaniment_app ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
