cmake_minimum_required(VERSION 3.24)
project(Voidlock LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VOIDLOCK_FETCH_SDL "Download SDL3 when it is not installed" ON)
find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND AND VOIDLOCK_FETCH_SDL)
  include(FetchContent)
  FetchContent_Declare(SDL3 URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.20/SDL3-3.2.20.tar.gz)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(SDL3)
endif()
if(NOT TARGET SDL3::SDL3)
  message(FATAL_ERROR "SDL3 was not found. Install SDL3 or configure with VOIDLOCK_FETCH_SDL=ON")
endif()

find_package(OpenGL REQUIRED)

add_executable(voidlock
  src/main.cpp
  src/core/Game.cpp
  src/input/Input.cpp
  src/world/Level.cpp
  src/entities/Player.cpp
  src/entities/Enemy.cpp
  src/weapons/Shotgun.cpp
  src/renderer/Renderer.cpp
  src/audio/Audio.cpp
)
target_include_directories(voidlock PRIVATE src)
target_link_libraries(voidlock PRIVATE SDL3::SDL3 OpenGL::GL)
if(MSVC)
  target_compile_options(voidlock PRIVATE /W4 /permissive-)
else()
  target_compile_options(voidlock PRIVATE -Wall -Wextra -Wpedantic)
endif()

