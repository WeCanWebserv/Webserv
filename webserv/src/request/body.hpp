#ifndef __FT_BODY_H__
#define __FT_BODY_H__

#include <map>
#include <string>
#include <vector>
#include "field_value.hpp"

struct Body
{
	std::vector<char> payload;
	std::vector<std::map<std::string, FieldValue> > multipartFormData;
};

#endif