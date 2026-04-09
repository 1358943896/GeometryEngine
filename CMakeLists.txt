cmake_minimum_required(VERSION 4.2)
project(GeometryEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(OUTPUT_BASE ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BASE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BASE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_BASE})
include(CMakeScripts.cmake)
link_directories(${OUTPUT_BASE})
include_directories(PUBLIC include)

add_subdirectory(src)
