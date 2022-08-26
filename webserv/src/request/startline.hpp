#ifndef __FT_STARTLINE_H__
#define __FT_STARTLINE_H__

#include <string>

struct Startline
{
    std::string method;
    std::string uri;
    std::string httpVersion;
};

#endif