#ifndef __FT_HEADER_H__
#define __FT_HEADER_H__

#include "field_value.hpp"
#include <map>
#include <string>
#include <vector>

#include "../Logger.hpp"

#if DEBUG
#include <iostream>
#endif

#define TRANSFER_ENCODING "Transfer-Encoding"
#define CONTENT_LENGTH "Content-Length"

template<typename T>
struct True : std::binary_function<T, T, T>
{
	bool operator()(const T &a, const T &b) const
	{
		return true;
	}
};

struct Header
{
	/**
	 * struct FieldValue
	 * {
	 * 		std::string value;
	 * 		std::map<std::string, std::string> descriptions;
	 * };
	*/
	typedef std::map<std::string, std::string> DescriptionMap;
	typedef std::map<std::string, std::vector<FieldValue>, True<std::string> > HeaderMap;

private:
	HeaderMap headerMap;

public:
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

		// std::vector<std::string>::iterator end = tokenvec.end();
		// for (size_t i = 0; i < tokenvec.size(); i++)
		// 	headerMap[tolowerStr(tokenvec[i])];
		// #if DEBUG > 1
		// 		size_t i = 0;
		// 		std::cout << "[ Predefined Header Fields ]\n";
		// 		for (std::map<std::string, std::vector<FieldValue> >::iterator it = headerMap.begin();
		// 				 it != headerMap.end(); it++)
		// 		{
		// 			std::cout << "[" << i << "] " << it->first << std::endl;
		// 			i++;
		// 		}
		// 		std::cout << "\n";
		// #endif
	}

	const HeaderMap &getFields(void) const
	{
		return this->headerMap;
	}

	void insertField(const std::pair<std::string, std::vector<FieldValue> > &field)
	{
		this->headerMap[tolowerStr(field.first)] = field.second;
	}

	bool hasField(const std::string &fieldname) const
	{
		HeaderMap::const_iterator it;

		for (it = headerMap.begin(); it != headerMap.end(); it++)
		{
			if (it->first == tolowerStr(fieldname))
				return true;
		}
		return false;
		// return (this->getFieldValueList(tolowerStr(fieldname)).size() > 0);
	}

	HeaderMap::const_iterator getFieldPair(const std::string &fieldname) const
	{
		HeaderMap::const_iterator it;

		for (it = headerMap.begin(); it != headerMap.end(); it++)
		{
			if (it->first == tolowerStr(fieldname))
				return it;
		}
		return it;
	}

	const std::vector<FieldValue> getFieldValueList(const std::string &fieldname) const
	{
		HeaderMap::const_iterator it;

		for (it = headerMap.begin(); it != headerMap.end(); it++)
		{
			if (it->first == tolowerStr(fieldname))
				return it->second;
		}
		return std::vector<FieldValue>();
	}

	bool hasFieldValue(const std::string &fieldname, const std::string &value) const
	{
		std::vector<FieldValue> values = this->getFieldValueList(tolowerStr(fieldname));

		for (size_t i = 0; i < values.size(); i++)
		{
			if (values[i].value == value)
				return true;
		}
		return false;
	}

	FieldValue getFieldValue(const std::string &fieldname, const std::string &value) const
	{
		std::vector<FieldValue> values = this->getFieldValueList(tolowerStr(fieldname));

		for (size_t i = 0; i < values.size(); i++)
		{
			if (values[i].value == value)
				return values[i];
		}
		return FieldValue();
	}

	void print(void)
	{
		std::string str;

		str.append("\n################### [ DEBUG: Header print ] ##################\n\n");

		for (HeaderMap::iterator it = headerMap.begin(); it != headerMap.end(); it++)
		{
			str.append("[");
			str.append(it->first + "]\n");
			for (size_t i = 0; i < it->second.size(); i++)
			{
				char tmp[1] = {i + '0'};
				str.append("\tvalue[");
				str.append(tmp, 1);
				str.append("]: ");
				str.append((it->second)[i].value + "\n");
				size_t j = 0;
				for (std::map<std::string, std::string>::iterator descit =
								 (it->second)[i].descriptions.begin();
						 descit != (it->second)[i].descriptions.end(); descit++)
				{
					char tmp2[1] = {j++ + '0'};
					str.append("\t- description[");
					str.append(tmp2, 1);
					str.append("]: ");
					str.append(descit->first + " = " + descit->second + "\n");
				}
			}
		}
		str.append("\n##########################################################\n");
		Logger::log(Logger::LOGLEVEL_INFO) << str << "\n";
	}

private:
	std::string tolowerStr(const std::string &str) const
	{
		std::string copystr = str;

		std::transform(copystr.begin(), copystr.end(), copystr.begin(), ::tolower);
		return copystr;
	}
};

#endif