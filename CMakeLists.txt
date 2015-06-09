cmake_minimum_required(VERSION 2.6)
project(engine)

add_executable(engine main.cpp)

install(TARGETS engine RUNTIME DESTINATION bin)
