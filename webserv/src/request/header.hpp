#ifndef __FT_HEADER_H__
#define __FT_HEADER_H__

#include "field_value.hpp"
#include <map>
#include <string>

struct Header
{
	std::map<std::string, FieldValue> headerMap;
};

#endif