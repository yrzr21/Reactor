cmake_minimum_required(VERSION 3.10)

project(Reactor)

# client 源文件
file(GLOB CLIENT_SRC_FILES
    "${PROJECT_SOURCE_DIR}/src/client/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/client/*.h")
add_executable(client ${CLIENT_SRC_FILES})

# server 源文件
file(GLOB SERVER_SRC_FILES
    "${PROJECT_SOURCE_DIR}/src/server/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/*.h"
    "${PROJECT_SOURCE_DIR}/src/server/tools/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/server/tools/*.h"
    )
add_executable(server ${SERVER_SRC_FILES})


# 构建完成后复制可执行文件到 ${PROJECT_SOURCE_DIR}/debug
add_custom_command(TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:client> ${PROJECT_SOURCE_DIR}/debug/)

add_custom_command(TARGET server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:server> ${PROJECT_SOURCE_DIR}/debug/)

# add_executable(test test.cpp)

# add_custom_command(TARGET server POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:test> ${PROJECT_SOURCE_DIR}/debug/)






##################################################################
# 教程

# 获取文件，然后传递给add...
# file(GLOB SRC_FILES
#     "${PROJECT_SOURCE_DIR}/src/*.cpp"
#     "${PROJECT_SOURCE_DIR}/src/*.h")

# 不会解析通配符
# add_executable(reactor ${SRC_FILES})