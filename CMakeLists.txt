cmake_minimum_required(VERSION 2.6)
project(engine)



if( CMAKE_CXX_COMPILER_ID  STREQUAL "GNU" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall" )
endif(  CMAKE_CXX_COMPILER_ID  STREQUAL "GNU" )



include_directories( include/ )

add_subdirectory(include)
add_subdirectory(src)

add_executable(engine main.cpp)
target_link_libraries( engine core )

add_subdirectory(tests)