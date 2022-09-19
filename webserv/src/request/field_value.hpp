#ifndef __FT_FIELD_VALUE_H__
#define __FT_FIELD_VALUE_H__

#include <map>
#include <string>
#include <vector>

struct FieldValue
{
	std::string value;
	std::map<std::string, std::string> descriptions;

	FieldValue() {}
	FieldValue(const std::string &value, const std::map<std::string, std::string> &descriptions)
			: value(value), descriptions(descriptions)
	{}

	FieldValue(const FieldValue &other)
	{
		*this = other;
	}
	FieldValue &operator=(const FieldValue &other)
	{
		this->value = other.value;
		this->descriptions = other.descriptions;
		return (*this);
	}
};

#endif