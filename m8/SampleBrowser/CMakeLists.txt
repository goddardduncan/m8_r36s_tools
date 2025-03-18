cmake_minimum_required(VERSION 3.10)
project(FileBrowser C)

set(CMAKE_C_STANDARD 99)

# Find SDL2 and required libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
pkg_check_modules(CURL REQUIRED libcurl)  # ✅ Added libcurl

# Include directories
include_directories(
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_TTF_INCLUDE_DIRS} 
    ${SDL2_MIXER_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}  # ✅ Include curl headers
)

# Add executable
add_executable(file_browser file_browser.c)

# Link libraries
target_link_libraries(file_browser 
    ${SDL2_LIBRARIES} 
    ${SDL2_TTF_LIBRARIES} 
    ${SDL2_MIXER_LIBRARIES}
    ${CURL_LIBRARIES}  # ✅ Link against libcurl
)

# Ensure we use the right flags for curl
target_compile_options(file_browser PRIVATE ${CURL_CFLAGS_OTHER})
target_link_options(file_browser PRIVATE ${CURL_LDFLAGS_OTHER})
