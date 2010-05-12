cmake_minimum_required (VERSION 2.8)
project (dfterm2)

subdirs (trankesbel)

add_executable(dfterm2 main.cc client.cc nanoclock.cc logger.cc slot.cc slot_dfglue.cc cp437_to_unicode.cc pointerpath.cc)

target_link_libraries(dfterm2 trankesbel)
target_link_libraries(dfterm2 psapi)
include_directories(./trankesbel)
include_directories(./trankesbel/auxiliary)
include_directories(${ICU_INCLUDE_PATH})
include_directories(${BOOST_ROOT})
include_directories(${CURSES_INCLUDE_DIR})
include_directories(${PCRE_INCLUDE_PATH})


