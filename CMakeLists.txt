cmake_minimum_required(VERSION 3.16)
project(DescompunereCelulara CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# raylib: folosim versiunea instalata (ex. `brew install raylib`), altfel o descarcam automat.
find_package(raylib 5.0 QUIET)
if(NOT raylib_FOUND)
  include(FetchContent)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  # descarcam arhiva .tar.gz (nu e nevoie de git)
  FetchContent_Declare(raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_MakeAvailable(raylib)
endif()

add_executable(descompunere
  src/main.cpp
  src/ui.cpp
  src/geometry.cpp
  src/scene.cpp
  src/omogen.cpp
  src/neomogen.cpp)
target_link_libraries(descompunere PRIVATE raylib)
if(APPLE)
  target_link_libraries(descompunere PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()
if(MSVC)
  target_compile_options(descompunere PRIVATE /utf-8)
endif()
