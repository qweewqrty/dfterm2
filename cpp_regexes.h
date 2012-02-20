#ifndef cpp_regexes_h
#define cpp_regexes_h

/*
 Wrapper around (a subset) PCRE.
 For C++ apps, provides easier interface and self-cleanup.
 Stores regex strings in a table for caching.
 
 For now, matches are stored in the regex class itself.
*/

extern "C"
{
#define PCRE_STATIC
#include "pcre.h"
}
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <map>

class _regex_t_refcounter
{
    private:
        _regex_t_refcounter() { abort(); };
        _regex_t_refcounter(const _regex_t_refcounter &rtr) { abort(); };
        _regex_t_refcounter& operator=(const _regex_t_refcounter &rtr) { abort(); };
        
    public:
        pcre* re;
        unsigned int refcounter;
        
        _regex_t_refcounter(pcre* new_re)
        {
            re = new_re;
            refcounter = 1;
        };
        ~_regex_t_refcounter()
        {
            assert(refcounter == 0);
            
            pcre_free(re);
        };
};

class Regex
{
    private:
        bool Invalid;
        
        _regex_t_refcounter* TheRegex;
        int* OutputMatches;
        int NumMatches;
        
        int LastNumMatches;
        
        std::string MatchString;
        
        /// Last string used when executing regex.
        std::string LastMatchedString;
        
        /// Cached strings
        static std::map<std::string, _regex_t_refcounter*> CachedRegexes;
        
        /// Default constructor is private
        Regex() { };
        
        void _buildConstruct(const char* match_str);
        void _releaseRegex();
        
    public:
        Regex(const char* match_str);
        Regex(std::string match_str);
        
        ~Regex();
        
        Regex& operator=(const Regex& re);
        Regex(const Regex& re);

        // Returns true if regex is invalid and can't be used
        // (e.g. from malformed regex string you tried to compile)
        bool isInvalid() const { return Invalid; };
        
        // Returns the regex string
        std::string getMatchString() const;
        
        /// Sets maximum number of matches. Defaults to 15. Invalidates current matches.
        void setNumberOfMatches(int matches);
        /// Gets maximum number of matches.
        int getMaxNumberOfMatches() const;
        /// Gets number of matches in the last match
        int getNumberOfMatches() const;
        
        /// Executes regular expression on a string.
        bool execute(const char* str);
        bool execute(std::string str);
        
        /// Gets a match
        std::string getMatch(int match_num) const;
        /// Gets a match start index
        int getMatchStartIndex(int match_num) const;
        /// Gets a match end index
        int getMatchEndIndex(int match_num) const;
};

#endif

