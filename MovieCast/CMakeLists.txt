cmake_minimum_required(VERSION 3.10)
project(R36sHLSPlayer C)

set(CMAKE_C_STANDARD 99)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2TTF REQUIRED SDL2_ttf)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(JSONC REQUIRED json-c)

include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${SDL2TTF_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${JSONC_INCLUDE_DIRS}
)

link_directories(
    ${SDL2_LIBRARY_DIRS}
    ${SDL2TTF_LIBRARY_DIRS}
    ${CURL_LIBRARY_DIRS}
    ${JSONC_LIBRARY_DIRS}
)

add_executable(r36s_hls_player r36s_hls_player.c)

target_link_libraries(r36s_hls_player
    ${SDL2_LIBRARIES}
    ${SDL2TTF_LIBRARIES}
    ${CURL_LIBRARIES}
    ${JSONC_LIBRARIES}
)
