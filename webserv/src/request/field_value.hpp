#ifndef __FT_FIELD_VALUE_H__
#define __FT_FIELD_VALUE_H__

#include <map>
#include <string>

struct FieldValue
{
	std::string value;
	std::map<std::string, std::string> description;
};

#endif