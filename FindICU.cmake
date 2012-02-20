# A quick and dirty module finder to find ICU


# If this is set, prefer static libraries to dynamic.
IF (ICU_USE_STATIC_LIBRARIES)
    SET(ICU_SEARCH_LIBRARIES libicuuc.a libicuuc44.a libicuuc42.a icuuc.lib icuuc44.lib icuuc42.lib libicuuc.so libicuuc44.so libicuuc42.so)
ELSE (ICU_USE_STATIC_LIBRARIES)
    SET(ICU_SEARCH_LIBRARIES libicuuc.so libicuuc44.so libicuuc42.so icuuc.lib icuuc44.lib icuuc42.lib libicuuc.a libicuuc44.a libicuuc42.a)
ENDIF (ICU_USE_STATIC_LIBRARIES)

FIND_PATH(ICU_INCLUDE_PATH unistr.h
          /usr/local/include/unicode
          /usr/include/unicode
          /usr/include
          /usr/local/include
          /opt/include
          /opt/local/include
          DOC "The directory where unistr.h resides")

FIND_LIBRARY(ICU_LIBRARIES NAMES ${ICU_SEARCH_LIBRARIES}
              HINTS
              /usr/lib64
              /usr/lib
              /usr/local/lib64
              /usr/local/lib
              /opt/lib64
              /opt/lib
              DOC "The directory where icuuc resides")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ICU DEFAULT_MSG ICU_LIBRARIES ICU_INCLUDE_PATH)

