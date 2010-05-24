cmake_minimum_required (VERSION 2.8)
project (dfterm2)

subdirs (trankesbel)

# Some parts of dfterm2 work very differently on different platforms and use different source files.
# Maybe we should add directories for platform-dependent files at some point.
IF (WIN32)
    add_executable(dfterm2 main.cc client.cc nanoclock.cc logger.cc slot.cc slot_dfglue.cc cp437_to_unicode.cc pointerpath.cc)
    target_link_libraries(dfterm2 psapi)
ELSE (WIN32)
    add_executable(dfterm2 main.cc client.cc nanoclock.cc logger.cc slot.cc slot_terminal.cc cp437_to_unicode.cc pty.cc bsd_pty.c)
ENDIF (WIN32)


target_link_libraries(dfterm2 trankesbel)

include_directories(./trankesbel)
include_directories(./trankesbel/auxiliary)
include_directories(${ICU_INCLUDE_PATH})
include_directories(${BOOST_ROOT})
include_directories(${CURSES_INCLUDE_DIR})
include_directories(${PCRE_INCLUDE_PATH})

# The injection glue is only for Win32
IF (WIN32)
    add_library(dfterm_injection_glue SHARED dfterm_injection_glue.cc)
ENDIF (WIN32)

