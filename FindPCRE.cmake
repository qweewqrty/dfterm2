# A quick and dirty module finder to find PCRE

IF (USE_STATIC_PCRE_LIBRARIES)
    SET(PCRE_SEARCH_LIBRARIES libpcre.a pcre.lib libpcre.so)
ELSE (USE_STATIC_PCRE_LIBRARIES)
    SET(PCRE_SEARCH_LIBRARIES libpcre.so pcre.lib libpcre.a)
ENDIF (USE_STATIC_PCRE_LIBRARIES)

FIND_PATH(PCRE_INCLUDE_PATH pcre.h
          /usr/local/include/unicode
          /usr/include/unicode
          /usr/include
          /usr/local/include
          /opt/include
          /opt/local/include
          DOC "The directory where pcre.h resides")
FIND_LIBRARY(PCRE_LIBRARIES NAMES ${PCRE_SEARCH_LIBRARIES}
          HINTS
          /usr/lib64
          /usr/lib
          /usr/local/lib64
          /usr/local/lib
          /opt/lib64
          /opt/lib
          DOC "The directory where libpcre.a resides")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCRE DEFAULT_MSG PCRE_LIBRARIES PCRE_INCLUDE_PATH)

