cmake_minimum_required(VERSION 3.10)
project(Reactor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2")

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(spdlog REQUIRED)



# ------------------------------------------------------------------------------
# server main
# ------------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES ${SRC_DIR}/*.cpp)

# 过滤不需要的 cpp
# list(REMOVE_ITEM SOURCES ${SRC_DIR}/main.cpp)
# foreach(file ${SOURCES})
#     if (NOT file MATCHES ".*/base/Buffer/.*\\.cpp$")
#         list(APPEND FILTERED_SOURCES ${file})
#     endif()
# endforeach()
# set(SOURCES ${FILTERED_SOURCES})


add_executable(Reactor ${SOURCES})
target_link_libraries(Reactor PRIVATE spdlog::spdlog)



add_executable(basic_client
    ${CMAKE_CURRENT_SOURCE_DIR}/test/clients/basic_client.cpp
)

# ------------------------------------------------------------------------------
# test
# ------------------------------------------------------------------------------
# add_executable(test1 ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/test1.cpp)

# add_executable(test_send_client
#     ${CMAKE_CURRENT_SOURCE_DIR}/test/clients/test_send_client.cpp ${SOURCES}
# )

# add_executable(test_max_concurrent_client
#     ${CMAKE_CURRENT_SOURCE_DIR}/test/clients/test_max_concurrent_client.cpp ${SOURCES}
# )

# add_executable(test_auto_release_pool
#     ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test/Buffer/test_auto_release_pool.cpp ${SOURCES}
# )

# add_executable(test_buffer_pool
#     ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test/Buffer/test_buffer_pool.cpp ${SOURCES}
# )

add_executable(test_msg_view
    ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test/Buffer/test_msg_view.cpp
)

# add_executable(test_recv_send_buffer
#     ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test/Buffer/test_recv_send_buffer.cpp ${SOURCES}
# )

add_executable(test_send_buffer
    ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test/Buffer/test_send_buffer.cpp
)

add_executable(test_smart_mono_pool
    ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test/Buffer/test_smart_mono_pool.cpp
)

# add_executable(test_mono_recycle_pool
#     ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test/Buffer/test_mono_recycle_pool.cpp
# )

add_executable(test_recv_buffer
    ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test/Buffer/test_recv_buffer.cpp
)

add_executable(test_sync_pool
    ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test/Buffer/test_sync_pool.cpp
)

add_executable(performance_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test/PerformanceTest/performance_test.cpp
)

add_executable(test_lock_free_queue
    ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test/LockFreeQueue/test_lock_free_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base/LockFreeQueue/NonOverlapObjectPool.cpp
)

add_executable(test_virtual_region_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test/LockFreeQueue/test_virtual_region_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base/LockFreeQueue/NonOverlapObjectPool.cpp
)

# add_executable(test_local_mono_recycle_pool
#     ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test/ServiceProvider/test_local_mono_recycle_pool.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/src/base/ThreadPool.cpp
# )
