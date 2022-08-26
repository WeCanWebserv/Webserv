#ifndef __FT_HEADER_H__
#define __FT_HEADER_H__

#include "field_value.hpp"
#include <map>
#include <string>
#include <vector>

struct Header
{
	std::map<std::string, std::vector<FieldValue> > headerMap;

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
		tokenvec.push_back("Connection");

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
		{
			std::transform(tokenvec[i].begin(), tokenvec[i].end(), tokenvec[i].begin(), ::tolower);
			headerMap[tokenvec[i]];
		}
	}
};

#endif