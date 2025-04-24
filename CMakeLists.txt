cmake_minimum_required(VERSION 3.10)
project(Reactor LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)

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
# 5) 单元测试
# ------------------------------------------------------------------------------
enable_testing()
find_package(GTest REQUIRED)

file(GLOB TEST_SOURCES tests/*_test.cpp)
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_include_directories(${test_name} PRIVATE ${INCLUDE_DIR})
    target_link_libraries(${test_name}
        PRIVATE
            base
            GTest::GTest
            GTest::Main
            pthread
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
