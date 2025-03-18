cmake_minimum_required(VERSION 3.10)
project(ListSelectDownload C)

# Find required libraries
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)

# Find JSON-C library
pkg_check_modules(JSON-C REQUIRED json-c)

# Define sources
add_executable(download list_select_download.c)

# Include necessary directories
include_directories(${CURL_INCLUDE_DIRS} ${JSON-C_INCLUDE_DIRS})

# Link libraries
target_link_libraries(download ${CURL_LIBRARIES} ${JSON-C_LIBRARIES} ncurses)
