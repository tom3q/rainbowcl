# required cmake version
cmake_minimum_required(VERSION 2.8.12)

# project name
set(PROJECT_NAME rainbowcl)
project(${PROJECT_NAME})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(GENERATOR_NAME TablesGenerator)
set(CRACKER_NAME PasswordCracker)
set(DUMPER_NAME TableDumper)
set(REDUCEGEN_NAME ReduceGenerator)
set(SHAREDLIB_NAME util)
set(SFMT_NAME SFMT)

# options passed to the compiler
add_definitions(-Wall)

# output settings
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_HOME_DIRECTORY}/bin)

# source and includes path
include_directories(Lib)
include_directories(SFMT)

add_subdirectory(${GENERATOR_NAME})
add_subdirectory(${CRACKER_NAME})
add_subdirectory(${DUMPER_NAME})
add_subdirectory(${REDUCEGEN_NAME})
add_subdirectory(Lib)
add_subdirectory(SFMT)
