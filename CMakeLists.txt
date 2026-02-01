



# cmake_minimum_required(VERSION 3.16)
# project(Calypso)

# add_subdirectory(server)

# add_custom_target(Client ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/client/CMakeLists.txt WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/client)
# add_custom_command(TARGET Client POST_BUILD COMMAND ${CMAKE_COMMAND} -B ${CMAKE_CURRENT_SOURCE_DIR}/client/build -S ${CMAKE_CURRENT_SOURCE_DIR}/client)
# add_custom_command(TARGET Client POST_BUILD COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_SOURCE_DIR}/client/build)

cmake -B client/build -S client
cmake --build client/build