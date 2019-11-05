cmake_minimum_required(VERSION 3.15)
project(renum_adj_matrix)

set(CMAKE_CXX_STANDARD 17)

find_package (Threads)
add_executable(renum_adj_matrix renum_adj_matrix.cpp)

target_link_libraries (renum_adj_matrix ${CMAKE_THREAD_LIBS_INIT})
