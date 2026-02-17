cmake_minimum_required(VERSION 3.16)
project(CalypsoServer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(${PROJECT_NAME}
    WIN32 MACOSX_BUNDLE
)

add_subdirectory(src)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
)