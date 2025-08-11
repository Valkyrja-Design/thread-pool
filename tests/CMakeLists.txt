target_sources(ThreadPool
    PRIVATE
    thread_pool.cpp
)

target_include_directories(ThreadPool
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
