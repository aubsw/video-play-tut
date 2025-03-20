cmake_minimum_required(VERSION 3.10)
project(AubreyVision C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
find_package(SDL2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale)

add_executable(AubreyVision main.c)

target_include_directories(AubreyVision PUBLIC
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
        ${FFMPEG_INCLUDE_DIRS})

target_link_directories(AubreyVision PRIVATE
        ${SDL2_LIBRARY_DIRS}
        ${SDL2_TTF_LIBRARY_DIRS}
        ${FFMPEG_LIBRARY_DIRS})

target_link_libraries(AubreyVision PRIVATE
        ${SDL2_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        ${FFMPEG_LIBRARIES})

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -g -O0 -DDEBUG")
