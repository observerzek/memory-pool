cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


project(MemoryPool)


# set(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

file(GLOB SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB MAIN ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(mainTest ${SOURCE} ${MAIN})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(mainTest PROPERTIES COMPILE_OPTIONS "-g")