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

	std::string toString() const
	{
		std::string str;

		str.append(this->value);
		for (std::map<std::string, std::string>::const_iterator descit = this->descriptions.begin();
				 descit != this->descriptions.end(); descit++)
		{
			str.append(";");
			str.append(descit->first);

			if (descit->second.size())
			{
				str.append("=");
				str.append(descit->second);
			}
		}
		return (str);
	}
};

#endif
