cmake_minimum_required(VERSION 3.15)
project(app VERSION 0.0.1)

if (MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -pedantic -Wno-unused-parameter)
endif()

# export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        192196711a7d0d7c2d60454d42654cf090498a74
  PATCH_COMMAND git restore . && git apply --ignore-space-change --ignore-whitespace ${CMAKE_CURRENT_SOURCE_DIR}/patches/imgui/draw_idx32.patch)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG        33c5a965f55f80057f197257d1d1cdb06523e963)
FetchContent_MakeAvailable(implot)

include(FindPkgConfig)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
pkg_search_module(jack REQUIRED IMPORTED_TARGET jack)
pkg_search_module(luajit REQUIRED IMPORTED_TARGET luajit)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp)
target_include_directories(imgui
  PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/misc/cpp
    ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui
  PUBLIC
    glfw)

add_library(implot STATIC
  ${implot_SOURCE_DIR}/implot.cpp
  ${implot_SOURCE_DIR}/implot_items.cpp
  ${implot_SOURCE_DIR}/implot_demo.cpp)
target_include_directories(implot
  PUBLIC
    ${implot_SOURCE_DIR})
target_link_libraries(implot
  PUBLIC
    imgui)

add_executable(app
  main.cc
  gui.cc)
target_link_libraries(app
  PRIVATE
    glfw
    imgui
    implot
    OpenGL::GL
    PkgConfig::jack
    PkgConfig::luajit)
