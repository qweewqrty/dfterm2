#include <string.h>
#include "cpp_regexes.h"

std::map<std::string, _regex_t_refcounter*> Regex::CachedRegexes;

Regex::Regex(const char* match_str)
{
    _buildConstruct(match_str);
}

Regex::Regex(std::string match_str)
{
    _buildConstruct(match_str.c_str());
}

int Regex::getMaxNumberOfMatches() const { return NumMatches; };
int Regex::getNumberOfMatches() const { return LastNumMatches; };

std::string Regex::getMatchString() const { return MatchString; };

bool Regex::execute(std::string str)
{
    return execute(str.c_str());
}

void Regex::_buildConstruct(const char* match_str)
{
    NumMatches = 0;
    LastNumMatches = 0;
    OutputMatches = 0;
    Invalid = false;
    TheRegex = (_regex_t_refcounter*) 0;
    
    setNumberOfMatches(15);
    
    MatchString = match_str;
    
    std::map<std::string, _regex_t_refcounter*>::iterator find_cache = CachedRegexes.find(std::string(match_str));
    if (find_cache != CachedRegexes.end())
    {
        find_cache->second->refcounter++;
        TheRegex = find_cache->second;
        return;
    };
    
    pcre* re;
    const char* error = (char*) 0;
    int erroffset = 0;
    re = pcre_compile(match_str, 0, &error, &erroffset, NULL);
    if (!re)
    {
        Invalid = true;
        return;
    };
    
    CachedRegexes[MatchString] = new _regex_t_refcounter(re);
    TheRegex = CachedRegexes[MatchString];
};

void Regex::_releaseRegex()
{
    if (TheRegex)
    {
        TheRegex->refcounter--;
        if (TheRegex->refcounter == 0)
        {
            delete TheRegex;
            CachedRegexes.erase(MatchString);
        };
        
        TheRegex = NULL;
    };
};

Regex::~Regex()
{
    _releaseRegex();
    
    if (OutputMatches) delete[] OutputMatches; OutputMatches = NULL;
};

void Regex::setNumberOfMatches(int matches)
{
    if (OutputMatches) delete[] OutputMatches; OutputMatches = NULL;
    
    NumMatches = matches;
    OutputMatches = new int[NumMatches*2*3];
    memset(OutputMatches, 0, sizeof(int) * NumMatches * 2 * 3);
};

bool Regex::execute(const char* str)
{
    if (Invalid) return false;
    
    LastMatchedString = str;
    
    memset(OutputMatches, 0, NumMatches * sizeof(int) * 2 * 3);
    int results = pcre_exec(TheRegex->re, NULL, str, strlen(str), 0, 0, OutputMatches, NumMatches * 3);
    LastNumMatches = 0;
    
    if (results < 0)
        return false;
    
    LastNumMatches = results;
    
    if (results == 0)
        LastNumMatches = NumMatches;
    
    return true;
};

std::string Regex::getMatch(int match_num) const
{
    if (Invalid) return std::string("");
    
    if (OutputMatches[match_num*2] == -1) return std::string("");
    return LastMatchedString.substr(OutputMatches[match_num*2], OutputMatches[match_num*2+1] - OutputMatches[match_num*2]);
};

int Regex::getMatchStartIndex(int match_num) const
{
    if (Invalid) return 0;
    
    return OutputMatches[match_num*2];
};

int Regex::getMatchEndIndex(int match_num) const
{
    if (Invalid) return 0;
    
    return OutputMatches[match_num*2+1];
};

Regex& Regex::operator=(const Regex& re)
{
    if (this == &re)
        return (*this);
    
    if (OutputMatches) delete[] OutputMatches; OutputMatches = NULL;
    OutputMatches = new int[re.NumMatches*2*3];
    NumMatches = re.NumMatches;
    LastNumMatches = re.LastNumMatches;
    
    if (re.Invalid)
    {
        Invalid = true;
        return (*this);
    }
    
    memcpy(OutputMatches, re.OutputMatches, sizeof(int) * NumMatches * 2 * 3);
    
    MatchString = re.MatchString;
    
    _releaseRegex();
    LastMatchedString = re.LastMatchedString;
    
    std::map<std::string, _regex_t_refcounter*>::iterator find_cache = CachedRegexes.find(std::string(MatchString));
    if (find_cache != CachedRegexes.end())
    {
        find_cache->second->refcounter++;
        TheRegex = find_cache->second;
        return (*this);
    };
    
    // Should be unreachable
    assert(false);
    return (*this);
};

Regex::Regex(const Regex& re)
{
    _buildConstruct(re.MatchString.c_str());
    
    LastNumMatches = re.LastNumMatches;
    
    setNumberOfMatches(re.NumMatches);
    memcpy(OutputMatches, re.OutputMatches, sizeof(int) * NumMatches * 2 * 3);
    
    LastMatchedString = re.LastMatchedString;
};


