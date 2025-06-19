cmake_minimum_required(VERSION 3.10)
project(Reactor LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 23)

# 定义头文件与源文件的根目录
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ------------------------------------------------------------------------------
# 1) base 模块
# ------------------------------------------------------------------------------
file(GLOB BASE_SOURCES ${SRC_DIR}/base/*.cpp)
add_library(base STATIC
    ${BASE_SOURCES}
)
target_include_directories(base PUBLIC
    ${INCLUDE_DIR}/base
)

# ------------------------------------------------------------------------------
# 2) net 模块
# ------------------------------------------------------------------------------
file(GLOB NET_SOURCES
    ${SRC_DIR}/net/Connection/*.cpp
    ${SRC_DIR}/net/Event/*.cpp
)
add_library(net STATIC ${NET_SOURCES})
target_include_directories(net PUBLIC
    ${INCLUDE_DIR}/net/Connection
    ${INCLUDE_DIR}/net/Event
)
target_link_libraries(net PUBLIC base)

# ------------------------------------------------------------------------------
# 3) server 模块
# ------------------------------------------------------------------------------
file(GLOB SERVER_SOURCES ${SRC_DIR}/server/*.cpp)
add_library(server STATIC
    ${SERVER_SOURCES}
)
target_include_directories(server PUBLIC
    ${INCLUDE_DIR}/server
)
target_link_libraries(server PUBLIC net)

# ------------------------------------------------------------------------------
# 4) 主程序
# ------------------------------------------------------------------------------
add_executable(Reactor
    ${SRC_DIR}/main.cpp
)
target_include_directories(Reactor PRIVATE
    ${INCLUDE_DIR}
)
target_link_libraries(Reactor PRIVATE server)

# ------------------------------------------------------------------------------
# 编译完成后
# ------------------------------------------------------------------------------
set(TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/debug)

add_custom_command(TARGET Reactor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:Reactor>
        ${TARGET_DIR}/$<TARGET_FILE_NAME:Reactor>
    COMMENT "Copy Reactor executable to ${TARGET_DIR}"
)