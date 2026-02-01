cmake_minimum_required(VERSION 3.16)
project(CalypsoServer)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE src/main.cpp)