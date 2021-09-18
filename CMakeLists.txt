cmake_minimum_required(VERSION 3.18)

project(Expandpass LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(MSVC)
  add_compile_options(/FC)
endif()

set(SOURCE_FILES 
  src/util.cpp
  src/expansion.cpp
  src/expand.cpp
  src/parse.cpp
  src/expandpass.cpp
)
add_executable(expandpass ${SOURCE_FILES})

