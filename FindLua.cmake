# A quick and dirty module finder to find Lua
# There's a CMake module for lua too, but we also want to find
# luajit if it's available


# If this is set, prefer static libraries to dynamic.
IF (LUA_USE_STATIC_LIBRARIES)
    SET(LUA_SEARCH_LIBRARIES libluajit-5.1.a libluajit.a liblua.a liblua5.1.a
                             libluajit-5.1.so libluajit.so liblua.so liblua5.1.so
                             luajit-5.1.lib)
ELSE (LUA_USE_STATIC_LIBRARIES)
    SET(LUA_SEARCH_LIBRARIES libluajit-5.1.so libluajit.so liblua.so liblua5.1.so
                             libluajit-5.1.a libluajit.a liblua.a liblua5.1.a
                             luajit-5.1.lib)
ENDIF (LUA_USE_STATIC_LIBRARIES)

FIND_PATH(LUA_INCLUDE_PATH lua.h
          /usr/local/include/luajit-2.0
          /usr/local/include/lua5.1
          /usr/local/include/lua
          /usr/local/include
          /usr/include/luajit-2.0
          /usr/include/lua5.1
          /usr/include/lua
          /opt/include/luajit-2.0
          /opt/include/lua5.1
          /opt/include/lua
          DOC "The directory where lua.h resides")

FIND_LIBRARY(LUA_LIBRARIES NAMES ${LUA_SEARCH_LIBRARIES}
              HINTS
              /usr/local/lib64
              /usr/local/lib
              /usr/lib64
              /usr/lib
              /opt/lib64
              /opt/lib
              DOC "The directory where lua library resides")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LUA DEFAULT_MSG LUA_LIBRARIES LUA_INCLUDE_PATH)

