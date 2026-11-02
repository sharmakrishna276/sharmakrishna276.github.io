cmake_minimum_required(VERSION 3.20)
project(ssg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(ssg
  src/main.cpp
  src/util.cpp
  src/frontmatter.cpp
  src/markdown.cpp
  src/template.cpp
)
target_compile_options(ssg PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
