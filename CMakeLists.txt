cmake_minimum_required (VERSION 2.8)
project (dfterm2)

# For DFHack headers to work correctly
IF(UNIX)
  add_definitions(-DLINUX_BUILD)
ENDIF(UNIX)

link_directories(${Boost_LIBRARY_DIRS})

FIND_PACKAGE (Threads REQUIRED)

SET(BUILD_DFHACK_DOCUMENTATION NO)
SET(BUILD_DFHACK_EXAMPLES NO)
SET(BUILD_DFHACK_PLAYGROUND NO)
subdirs (trankesbel)

SET(NO_CURSES 1)

OPTION(BUILD_DFHACK "Build DFHack and DFHack slot type. (buggy)" OFF)
IF (BUILD_DFHACK)
    IF (WIN32)
        SET(DFHACK_SLOT_SOURCE slot_dfhack.cc)
    ELSE (WIN32)
        SET(DFHACK_SLOT_SOURCE slot_dfhack_linux.cc)
    ENDIF (WIN32)
ELSE (BUILD_DFHACK)
    ADD_DEFINITIONS(-DNO_DFHACK)
ENDIF (BUILD_DFHACK)

# Some parts of dfterm2 work very differently on different platforms and use different source files.
# Maybe we should add directories for platform-dependent files at some point.
IF (WIN32)
    add_executable(dfterm2 main.cc client.cc logger.cc slot.cc slot_dfglue.cc cp437_to_unicode.cc pointerpath.cc configuration_interface.cc configuration_db.cc sqlite3.c state.cc usergroup_serialize.cc id.cc hash.cc rng.cc minimal_http_server.cc server_to_server_configuration_pair.cc server_to_server_session.cc ${DFHACK_SLOT_SOURCE})
    target_link_libraries(dfterm2 psapi)
ELSE (WIN32)
    add_executable(dfterm2 main.cc client.cc logger.cc slot.cc slot_terminal.cc cp437_to_unicode.cc pty.cc bsd_pty.cc configuration_interface.cc configuration_db.cc sqlite3.c state.cc usergroup_serialize.cc id.cc hash.cc rng.cc minimal_http_server.cc server_to_server_configuration_pair.cc server_to_server_session.cc ${DFHACK_SLOT_SOURCE})
    target_link_libraries(dfterm2 dl)
ENDIF (WIN32)

target_link_libraries(dfterm2 trankesbel)

add_executable(dfterm2_configure dfterm2_configure.cc configuration_db.cc sqlite3.c usergroup_serialize.cc logger.cc id.cc hash.cc rng.cc)
IF(NOT WIN32)
target_link_libraries(dfterm2_configure trankesbel dl)
ELSE(NOT WIN32)
target_link_libraries(dfterm2_configure trankesbel)
ENDIF(NOT WIN32)

add_executable(dfterm2_sha512 dfterm2_sha512.cc hash.cc)
IF(NOT WIN32)
target_link_libraries(dfterm2_sha512 trankesbel dl)
ELSE(NOT WIN32)
target_link_libraries(dfterm2_sha512 trankesbel)
ENDIF(NOT WIN32)

FIND_PACKAGE(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(dfterm2 ${OPENSSL_LIBRARIES})
target_link_libraries(dfterm2_configure ${OPENSSL_LIBRARIES})
target_link_libraries(dfterm2_sha512 ${OPENSSL_LIBRARIES})

include_directories(.)
include_directories(./trankesbel)
include_directories(./trankesbel/auxiliary)
include_directories(${ICU_INCLUDE_PATH})
include_directories(${BOOST_ROOT})
include_directories(${CURSES_INCLUDE_DIR})
include_directories(${PCRE_INCLUDE_PATH})
include_directories(./dfhack/library/include)

# The injection glue is only for Win32
IF (WIN32)
    add_library(dfterm_injection_glue SHARED dfterm_injection_glue.cc)
    target_link_libraries(dfterm_injection_glue psapi)
ENDIF (WIN32)

IF (NO_CURSES)
    IF (MSVC)
        ADD_DEFINITIONS("/DNO_CURSES")
    ENDIF (MSVC)
    IF (CMAKE_COMPILER_IS_GNUCXX)
        ADD_DEFINITIONS("-DNO_CURSES")
    ENDIF ()
ENDIF (NO_CURSES)

IF (PROFILE)
    IF (CMAKE_COMPILER_IS_GNUCXX)
        ADD_DEFINITIONS("-pg")
    ENDIF (CMAKE_COMPILER_IS_GNUCXX)

    SET_TARGET_PROPERTIES(dfterm2_sha512 dfterm2 dfterm2_configure PROPERTIES LINK_FLAGS -pg)
ENDIF (PROFILE)

# Testing stuff
ENABLE_TESTING()

ADD_EXECUTABLE(lockedresource_test tests/lockedresource_test.cc)
target_link_libraries(lockedresource_test trankesbel)
ADD_TEST(lockedresource_test ${EXECUTABLE_OUTPUT_PATH}/lockedresource_test)

ADD_EXECUTABLE(server_to_server_configuration_pair_test tests/server_to_server_configuration_pair_test.cc server_to_server_configuration_pair.cc)
ADD_TEST(server_to_server_configuration_pair_test ${EXECUTABLE_OUTPUT_PATH}/server_to_server_configuration_pair_test)

ADD_EXECUTABLE(server_to_server_session_test tests/server_to_server_session_test.cc server_to_server_configuration_pair.cc server_to_server_session.cc logger.cc)
TARGET_LINK_LIBRARIES(server_to_server_session_test trankesbel)
ADD_TEST(server_to_server_session_test ${EXECUTABLE_OUTPUT_PATH}/server_to_server_session_test)

SET_TARGET_PROPERTIES(dfterm2_sha512 dfterm2 dfterm2_configure lockedresource_test server_to_server_configuration_pair_test PROPERTIES COMPILE_DEFINITIONS _UNICODE)



# Finally include DFHack if that was set
IF (BUILD_DFHACK)
    subdirs(dfhack)
    target_link_libraries(dfterm2 dfhack)
ENDIF (BUILD_DFHACK)

