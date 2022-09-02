#ifndef __FT_HEADER_H__
#define __FT_HEADER_H__

#include "field_value.hpp"
#include <map>
#include <string>
#include <vector>

#if DEBUG
#include <iostream>
#endif

struct Header
{
	/**
	 * struct FieldValue
	 * {
	 * 		std::string value;
	 * 		std::map<std::string, std::string> descriptions;
	 * };
	*/
private:
	typedef std::map<std::string, std::string> DescriptionMap;
	typedef std::vector<FieldValue> FieldValueList;
	typedef std::map<std::string, FieldValueList> HeaderMap;
	HeaderMap headerMap;

public:
	/**
	 * getHeaderMap
	* insertField
	* hasField
	* findField
	* findFieldValueList
	* hasFieldValue
	* findFieldValue
	* findValueDescription
	*/
	Header()
	{
		std::vector<std::string> tokenvec;

		/**
		 * Representation
		 */
		tokenvec.push_back("Content-Type");
		tokenvec.push_back("Content-Encoding");
		tokenvec.push_back("Content-Language");
		tokenvec.push_back("Content-Length");

		/**
		 * Negotiation
		 */
		tokenvec.push_back("Accept");
		tokenvec.push_back("Accept-Charset");
		tokenvec.push_back("Accept-Encoding");
		tokenvec.push_back("Accept-Language");

		/**
		 * Transfer
		 */
		tokenvec.push_back("Transfer-Encoding");
		tokenvec.push_back("Content-Range");
		tokenvec.push_back("Range");
		tokenvec.push_back("If-Range");

		/**
		 * General information
		 */
		tokenvec.push_back("From");
		tokenvec.push_back("Referer");
		tokenvec.push_back("User-Agent");
		tokenvec.push_back("Date");

		/**
		 * Special information
		 */
		tokenvec.push_back("Host");
		tokenvec.push_back("Authorization");
		tokenvec.push_back("Origin");
		tokenvec.push_back("Cookie");
		tokenvec.push_back("Connection"); //keep-alive

		/**
		 * Cache
		 */
		tokenvec.push_back("Cache-Control");
		tokenvec.push_back("If-Modified-Since");
		tokenvec.push_back("If-Unmodified-Since");
		tokenvec.push_back("If-None-Match");
		tokenvec.push_back("If-Match");

		std::vector<std::string>::iterator end = tokenvec.end();
		for (size_t i = 0; i < tokenvec.size(); i++)
			headerMap[tolowerStr(tokenvec[i])];
#if DEBUG > 1
		size_t i = 0;
		std::cout << "[ Predefined Header Fields ]\n";
		for (std::map<std::string, std::vector<FieldValue> >::iterator it = headerMap.begin();
				 it != headerMap.end(); it++)
		{
			std::cout << "[" << i << "] " << it->first << std::endl;
			i++;
		}
		std::cout << "\n";
#endif
	}

	const HeaderMap &getHeaderMap(void) const
	{
		return this->headerMap;
	}

	void insertField(const std::pair<std::string, FieldValueList> &field)
	{
		this->headerMap[tolowerStr(field.first)] = field.second;
	}

	bool hasField(const std::string &fieldname) const
	{
		return (this->findFieldValueList(tolowerStr(fieldname)).size() > 0);
	}

	HeaderMap::const_iterator findField(const std::string &fieldname) const
	{
		return headerMap.find(tolowerStr(fieldname));
	}

	const FieldValueList &findFieldValueList(const std::string &fieldname) const
	{
		return headerMap.find(tolowerStr(fieldname))->second;
	}

	bool hasFieldValue(const std::string &fieldname, const std::string &value) const
	{
		FieldValueList values = this->findFieldValueList(fieldname);
		for (size_t i = 0; i < values.size(); i++)
		{
			if (values[i].value == value)
				return true;
		}
		return false;
	}

	std::pair<FieldValue, bool>
	findFieldValue(const std::string &fieldname, const std::string &value) const
	{
		FieldValueList values = this->findFieldValueList(fieldname);
		for (size_t i = 0; i < values.size(); i++)
		{
			if (values[i].value == value)
				return std::make_pair(values[i], true);
		}
		return std::make_pair(FieldValue(), false);
	}

	const DescriptionMap::const_iterator
	findValueDescription(const FieldValue &fieldvalue, const std::string &description) const
	{
		return fieldvalue.descriptions.find(tolowerStr(description));
	}

#if DEBUG
	void print(void)
	{
		for (std::map<std::string, std::vector<FieldValue> >::iterator it = headerMap.begin();
				 it != headerMap.end(); it++)
		{
			if (!it->second.size())
				continue;
			std::cout << "[" << it->first << "] "
								<< "\n";
			for (size_t i = 0; i < it->second.size(); i++)
			{
				std::cout << "\t"
									<< "value[" << i << "]: " << (it->second)[i].value << std::endl;
				size_t j = 0;
				for (std::map<std::string, std::string>::iterator descit =
								 (it->second)[i].descriptions.begin();
						 descit != (it->second)[i].descriptions.end(); descit++)
				{
					std::cout << "\t- description[" << j++ << "]: " << descit->first << " = "
										<< descit->second << std::endl;
				}
			}
		}
	}
#endif
private:
	std::string tolowerStr(const std::string &str) const
	{
		std::string copystr = str;

		std::transform(copystr.begin(), copystr.end(), copystr.begin(), ::tolower);
		return copystr;
	}
};

#endif