cmake_minimum_required(VERSION 3.10)
project(Reactor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g")

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)


# ------------------------------------------------------------------------------
# server main
# ------------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES ${SRC_DIR}/*.cpp)

# 过滤不需要的 cpp
list(REMOVE_ITEM SOURCES ${SRC_DIR}/main.cpp)
# foreach(file ${SOURCES})
#     if (NOT file MATCHES ".*/base/Buffer/.*\\.cpp$")
#         list(APPEND FILTERED_SOURCES ${file})
#     endif()
# endforeach()
# set(SOURCES ${FILTERED_SOURCES})


# add_executable(Reactor ${SOURCES})

# add_custom_command(TARGET Reactor POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy
#     $<TARGET_FILE:Reactor>
#     ${TARGET_DIR}/$<TARGET_FILE_NAME:Reactor>
#     COMMENT "Copy Reactor executable to ${TARGET_DIR}"
# )



# ------------------------------------------------------------------------------
# test
# ------------------------------------------------------------------------------
add_executable(test1 ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/test1.cpp)

add_executable(test_send_client
    ${CMAKE_CURRENT_SOURCE_DIR}/test/clients/test_send_client.cpp
)

add_executable(test_max_concurrent_client
    ${CMAKE_CURRENT_SOURCE_DIR}/test/clients/test_max_concurrent_client.cpp
)

add_executable(test_buffer
    ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test/test_buffer.cpp
)



set(TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/debug)

add_custom_command(TARGET test1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:test1>
    ${TARGET_DIR}/$<TARGET_FILE_NAME:test1>
    COMMENT "Copy test1 executable to ${TARGET_DIR}"
)

add_custom_command(TARGET test_send_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:test_send_client>
    ${TARGET_DIR}/$<TARGET_FILE_NAME:test_send_client>
    COMMENT "Copy test_send_client executable to ${TARGET_DIR}"
)

add_custom_command(TARGET test_max_concurrent_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:test_max_concurrent_client>
    ${TARGET_DIR}/$<TARGET_FILE_NAME:test_max_concurrent_client>
    COMMENT "Copy test_max_concurrent_client executable to ${TARGET_DIR}"
)
