link_libraries("cppev")

find_package(GTest)
link_libraries("gtest")

add_executable("test_event_loop" test_event_loop.cc)
add_executable("test_runnable" test_runnable.cc)
add_executable("demo" demo.cc)

add_executable(test_async_logger test_async_logger.cc )
add_executable(test_thread_pool test_thread_pool.cc)
add_executable(test_buffer test_buffer.cc)

enable_testing()
add_test(NAME test_thread_pool COMMAND test_thread_pool)
add_test(NAME test_async_logger COMMAND test_async_logger)
add_test(NAME test_buffer COMMAND test_buffer)
