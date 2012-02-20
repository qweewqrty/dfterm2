cmake_minimum_required (VERSION 2.8)
project (dfterm2)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
FIND_PACKAGE(Lua REQUIRED)
INCLUDE_DIRECTORIES(${LUA_INCLUDE_PATH})

FIND_PACKAGE(Boost 1.41.0 REQUIRED COMPONENTS thread)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(.)
link_directories(${Boost_LIBRARY_DIRS})

FIND_PACKAGE(ICU REQUIRED)
FIND_PACKAGE(PCRE REQUIRED)

add_definitions(-DNO_DFHACK)

IF(UNIX)
  add_definitions(-DLINUX_BUILD)
ENDIF(UNIX)

FIND_PACKAGE (Threads REQUIRED)

SET(NO_CURSES 1)

SET(COMMON_SOURCE main.cc client.cc logger.cc slot.cc cp437_to_unicode.cc configuration_interface.cc configuration_db.cc sqlite3.c state.cc usergroup_serialize.cc id.cc hash.cc rng.cc minimal_http_server.cc server_to_server_configuration_pair.cc server_to_server_session.cc lua_configuration.cc sockets.cc telnet.cc nanoclock.cc cpp_regexes.cc types.cc socketevents.cc socketaddressrange.cc termemu.cc utf8.cc interface_ncurses.cc keypress.cc pty.cc)

# Some parts of dfterm2 work very differently on different platforms and use different source files.
# Maybe we should add directories for platform-dependent files at some point.
IF (WIN32)
    add_executable(dfterm2 ${COMMON_SOURCE} slot_dfglue.cc pointerpath.cc)
    target_link_libraries(dfterm2 psapi)
ELSE (WIN32)
    add_executable(dfterm2 ${COMMON_SOURCE} slot_terminal.cc slot_linux_common.cc bsd_pty.cc)
    target_link_libraries(dfterm2 dl)
ENDIF (WIN32)

SET(COMMON_LIBS ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} ${ICU_LIBRARIES} ${PCRE_LIBRARIES} ${LUA_LIBRARIES})

TARGET_LINK_LIBRARIES (dfterm2 ${COMMON_LIBS})

add_executable(dfterm2_configure dfterm2_configure.cc configuration_db.cc sqlite3.c usergroup_serialize.cc logger.cc id.cc hash.cc rng.cc lua_configuration.cc utf8.cc types.cc socketaddressrange.cc sockets.cc cpp_regexes.cc)
IF(NOT WIN32)
    target_link_libraries(dfterm2_configure dl ${COMMON_LIBS})
ELSE(NOT WIN32)
    target_link_libraries(dfterm2_configure ${COMMON_LIBS})
ENDIF(NOT WIN32)

add_executable(dfterm2_sha512 dfterm2_sha512.cc hash.cc)
IF(NOT WIN32)
    target_link_libraries(dfterm2_sha512 dl ${COMMON_LIBS})
ELSE(NOT WIN32)
    target_link_libraries(dfterm2_sha512 ${COMMON_LIBS})
ENDIF(NOT WIN32)

FIND_PACKAGE(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(dfterm2 ${OPENSSL_LIBRARIES})
target_link_libraries(dfterm2_configure ${OPENSSL_LIBRARIES})
target_link_libraries(dfterm2_sha512 ${OPENSSL_LIBRARIES})

include_directories(.)
include_directories(${ICU_INCLUDE_PATH})
include_directories(${BOOST_ROOT})
include_directories(${CURSES_INCLUDE_DIR})
include_directories(${PCRE_INCLUDE_PATH})

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
ADD_TEST(lockedresource_test ${EXECUTABLE_OUTPUT_PATH}/lockedresource_test)

ADD_EXECUTABLE(server_to_server_configuration_pair_test tests/server_to_server_configuration_pair_test.cc server_to_server_configuration_pair.cc)
TARGET_LINK_LIBRARIES(server_to_server_configuration_pair_test)
ADD_TEST(server_to_server_configuration_pair_test ${EXECUTABLE_OUTPUT_PATH}/server_to_server_configuration_pair_test)

# Test borken at the moment
#ADD_EXECUTABLE(server_to_server_session_test tests/server_to_server_session_test.cc server_to_server_configuration_pair.cc server_to_server_session.cc logger.cc)
#TARGET_LINK_LIBRARIES(server_to_server_session_test)
#ADD_TEST(server_to_server_session_test ${EXECUTABLE_OUTPUT_PATH}/server_to_server_session_test)

SET_TARGET_PROPERTIES(dfterm2_sha512 dfterm2 dfterm2_configure lockedresource_test server_to_server_configuration_pair_test PROPERTIES COMPILE_DEFINITIONS _UNICODE)


