cmake_minimum_required(VERSION 3.24)

project(atom_forge VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(atom_forge_build_app "Build the SDL3/OpenGL application" ON)
option(atom_forge_build_tests "Build backend tests" ON)
option(atom_forge_fetch_sdl3 "Fetch SDL3 when no installed package is available" OFF)
set(atom_forge_glad_source_dir "" CACHE PATH "Existing GLAD 2 source checkout")

include(CTest)

if(atom_forge_build_app)
  if(atom_forge_fetch_sdl3)
    find_package(SDL3 3.2 CONFIG QUIET COMPONENTS SDL3)
    if(NOT SDL3_FOUND)
      include(FetchContent)
      set(SDL_SHARED ON CACHE BOOL "" FORCE)
      set(SDL_STATIC OFF CACHE BOOL "" FORCE)
      set(SDL_TESTS OFF CACHE BOOL "" FORCE)
      set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
      FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.18
        GIT_SHALLOW TRUE
      )
      FetchContent_MakeAvailable(SDL3)
    endif()
  else()
    find_package(SDL3 3.2 CONFIG REQUIRED COMPONENTS SDL3)
  endif()

  if(atom_forge_glad_source_dir)
    set(atom_forge_glad_root "${atom_forge_glad_source_dir}")
    add_subdirectory(
      "${atom_forge_glad_root}/cmake"
      atom_forge_glad_cmake
    )
  else()
    include(FetchContent)
    FetchContent_Declare(
      glad
      GIT_REPOSITORY https://github.com/Dav1dde/glad.git
      GIT_TAG v2.0.8
      GIT_SHALLOW TRUE
      SOURCE_SUBDIR cmake
    )
    FetchContent_MakeAvailable(glad)
    set(atom_forge_glad_root "${glad_SOURCE_DIR}")
  endif()

  # GladConfig.cmake runs `python -m glad` from this source directory.
  set(GLAD_SOURCES_DIR "${atom_forge_glad_root}")
  glad_add_library(atom_forge_glad STATIC REPRODUCIBLE LOADER API gl:core=3.3)

  find_package(OpenGL REQUIRED)

  add_executable(atom_forge
    src/main.c
    src/editor_input.c
    src/editor_ui.c
    src/vec2.c
    src/world.c
    src/chemistry.c
    src/physics.c
    src/renderer.c
    src/renderer_draw.c
  )
  target_include_directories(atom_forge PRIVATE include src)
  target_link_libraries(atom_forge PRIVATE SDL3::SDL3 atom_forge_glad OpenGL::GL)
  if(NOT WIN32)
    target_link_libraries(atom_forge PRIVATE m)
  endif()

  if(MSVC)
    target_compile_options(atom_forge PRIVATE /W4)
  else()
    target_compile_options(atom_forge PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  endif()
endif()

if(BUILD_TESTING AND atom_forge_build_tests)
  add_executable(atom_forge_tests
    tests/test_backend.c
    src/vec2.c
    src/world.c
    src/chemistry.c
    src/physics.c
  )
  target_include_directories(atom_forge_tests PRIVATE include)
  if(NOT WIN32)
    target_link_libraries(atom_forge_tests PRIVATE m)
  endif()
  if(MSVC)
    target_compile_options(atom_forge_tests PRIVATE /W4)
  else()
    target_compile_options(atom_forge_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  endif()
  add_test(NAME atom_forge_backend COMMAND atom_forge_tests)
endif()
