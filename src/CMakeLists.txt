set(SRC_LIST main.cpp timer.cpp timer.h)
add_executable(hello ${SRC_LIST})
target_link_libraries(hello pthread)
