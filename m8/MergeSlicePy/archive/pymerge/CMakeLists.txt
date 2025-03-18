cmake_minimum_required(VERSION 3.10)
project(pymerge C)

set(CMAKE_C_STANDARD 99)

# Find SDL2 using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(JSON-C REQUIRED json-c)

# Include necessary directories
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${JSON-C_INCLUDE_DIRS})

# Link directories
link_directories(${SDL2_LIBRARY_DIRS} ${SDL2_TTF_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS} ${JSON-C_LIBRARY_DIRS})

# Add executable
add_executable(pymerge pymerge.c)

# Link libraries manually
target_link_libraries(pymerge ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${CURL_LIBRARIES} ${JSON-C_LIBRARIES})

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
