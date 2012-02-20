#include <string>
#include <unicode/ustring.h>
#include <unicode/unistr.h>

std::string TO_UTF8(const UnicodeString &us)
{
    std::string result;
    us.toUTF8String(result);
    return result;
}

std::string TO_UTF8(const wchar_t* wc, size_t wc_len)
{
    UChar* destination;
    UErrorCode uerror = U_ZERO_ERROR;
    int32_t destination_len = 0;

    u_strFromWCS((UChar*) 0, 0, &destination_len, wc, wc_len, &uerror);
    if (uerror != U_BUFFER_OVERFLOW_ERROR && destination_len == 0)
        return std::string();

    uerror = U_ZERO_ERROR;

    destination = new UChar[destination_len+1];
    destination[destination_len] = 0;
    u_strFromWCS(destination, destination_len, (int32_t*) 0, wc, wc_len, &uerror);
    if (U_FAILURE(uerror))
    {
        delete[] destination;
        return std::string();
    }

    UnicodeString us(destination, destination_len);
    delete[] destination;

    return TO_UTF8(us);
}

UnicodeString TO_UNICODESTRING(const std::string &us)
{
    return UnicodeString::fromUTF8(us);
}

std::string TO_UTF8(const wchar_t* wc)
{
    size_t len;
    for (len = 0; wc[len]; ++len)
    { };
    return TO_UTF8(wc, len);
}

void TO_WCHAR_STRING(const UnicodeString &us, wchar_t* wc, size_t* wc_len)
{
    int32_t len = us.length();
    int32_t wc_len_int = (*wc_len);
    const UChar* char_data = us.getBuffer();

    UErrorCode uerror = U_ZERO_ERROR;
    int32_t len_needed = 0;

    u_strToWCS(wc, wc_len_int, &len_needed, char_data, len, &uerror);
    if (U_FAILURE(uerror) && uerror != U_BUFFER_OVERFLOW_ERROR)
    {
        (*wc_len) = 0;
        return;
    }
    (*wc_len) = (size_t) len_needed;
};

void TO_WCHAR_STRING(const std::string &us, wchar_t* wc, size_t* wc_len)
{
    TO_WCHAR_STRING(TO_UNICODESTRING(us), wc, wc_len);
}

