cmake_minimum_required(VERSION 3.13)
#set(CMAKE_VERBOSE_MAKEFILE ON) -DCMAKE_VERBOSE_MAKEFILE=ON --debug-output --trace
project(microtech C CXX)

include(FetchContent)
#if(NOT DEFINED HAL AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
#  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/msp430.cmake")
#  include(${CMAKE_TOOLCHAIN_FILE})
#elseif(DEFINED CMAKE_TOOLCHAIN_FILE)
#  include(${CMAKE_TOOLCHAIN_FILE})
#endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT MSP_COMPILER)
  add_subdirectory(mock)
endif()

add_subdirectory(common)
if (NOT MSP_COMPILER)
  add_subdirectory(example)
endif()

add_subdirectory(exercise1)
add_subdirectory(exercise2)
add_subdirectory(exercise3)
add_subdirectory(exercise4)
add_subdirectory(exercise5)
add_subdirectory(exercise6)
add_subdirectory(exercise7)
add_subdirectory(exercise8)

