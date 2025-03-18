cmake_minimum_required(VERSION 3.10)
project(M8WavDownloader C)

set(CMAKE_C_STANDARD 99)

# Find SDL2 and required libraries
find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

find_package(CURL REQUIRED)
pkg_check_modules(JSON-C REQUIRED json-c)

# Include necessary directories
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${JSON-C_INCLUDE_DIRS})

# Add executable
add_executable(m8_wavs_download sdl_file_select.c)

# Link libraries
target_link_libraries(m8_wavs_download ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${CURL_LIBRARIES} ${JSON-C_LIBRARIES})

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
