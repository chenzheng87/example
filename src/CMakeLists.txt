set(SRC_LIST main.cpp timer.cpp timer.h threadpool.cpp threadpool.h taskmaker.h taskprocessor.h epoll_client.cpp epoll_client.h shell_loop.cpp)
add_executable(example ${SRC_LIST})
target_link_libraries(example pthread)
