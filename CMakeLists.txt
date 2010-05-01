cmake_minimum_required (VERSION 2.8)
project (dfterm2)

subdirs (trankesbel)

add_executable(dfterm2 main.cc)

target_link_libraries(dfterm2 trankesbel)
include_directories(dfterm2 ./trankesbel)


