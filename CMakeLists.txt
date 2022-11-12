cmake_minimum_required(VERSION 3.16)

project(Breakthrough VERSION 1.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)

include_directories(include)

add_compile_definitions(_USE_MATH_DEFINES)

add_executable(breakthrough src/app.cpp)
target_compile_options(breakthrough PUBLIC -O3)
target_link_options(breakthrough PUBLIC
  --embed-file ${CMAKE_SOURCE_DIR}/rsrc@rsrc
  --shell-file ${CMAKE_SOURCE_DIR}/public/index.template.html)
set_target_properties(breakthrough PROPERTIES OUTPUT_NAME index)
set_target_properties(breakthrough PROPERTIES SUFFIX .html)
