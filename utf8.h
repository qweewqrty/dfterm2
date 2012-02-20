/** \file utf8.h
 * Functions to handle utf8 encoding.
 */

#ifndef utf8_h
#define utf8_h

#include <string>
 
/// Returns a UTF8 string of given unicode symbol.
std::string getUTF8Str(unsigned int unicode_symbol);
/// Same as above but puts it into the referenced C-string.
/**
 * The string should at least have allocated space for 4 characters.
 * Trailing NULL will NOT be appended. Returns the number of characters
 * used.
 * @return 0 if can't be turned into UTF-8.
 */
int getUTF8Str(unsigned int unicode_symbol, char* str);

/// Fetches and returns an unicode symbol in a UTF8 string.
/** The cursor is moved as many bytes as the unicode character took space in the string.
 * If NULL terminating character is reached, returns 0 and cursor is not moved.
 */
unsigned int fetchUTF8Unicode(const char* str, int &cursor);

/// Returns the true length of a UTF8-string.
unsigned int getTrueLengthUTF8(const char* str);

/// Returns the UTF8 string until char_num-th character
std::string getPrefixUTF8(const char* str, int char_num);

#ifndef WIN32
/// Checks if the system is using a UTF-8 locale and spits a warning to
/// logger if it's not. Not applicable on WIN32
void checkForUTF8Locale();
#endif

#endif

