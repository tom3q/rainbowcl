# required cmake version
cmake_minimum_required(VERSION 2.8.12)

# project name
set(PROJECT_NAME rainbowcl)
project(${PROJECT_NAME})

set(GENERATOR_NAME TablesGenerator)
set(CRACKER_NAME PasswordCracker)
set(SHAREDLIB_NAME util)

# options passed to the compiler
add_definitions(-Wall)

# output settings
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_HOME_DIRECTORY}/bin)

# source and includes path
include_directories(Lib)

add_subdirectory(${GENERATOR_NAME})
add_subdirectory(${CRACKER_NAME})
add_subdirectory(Lib)