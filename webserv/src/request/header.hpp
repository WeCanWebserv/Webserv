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
		{
			std::transform(tokenvec[i].begin(), tokenvec[i].end(), tokenvec[i].begin(), ::tolower);
			headerMap[tokenvec[i]];
		}
#if DEBUG
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

#if DEBUG
	void print(void)
	{
		for (std::map<std::string, std::vector<FieldValue> >::iterator it = headerMap.begin();
				 it != headerMap.end(); it++)
		{
			std::cout << "[" << it->first << "] " << "\n";
			for (size_t i = 0; i < it->second.size(); i++)
			{
				std::cout << "\t" << "value[" << i << "]: " << (it->second)[i].value << std::endl;
				size_t j = 0;
				for (std::map<std::string, std::string>::iterator descit = (it->second)[i].descriptions.begin();
				 descit != (it->second)[i].descriptions.end(); descit++)
				 {
					std::cout << "\t- description[" << j++ << "]: " << descit->first << " = " << descit->second << std::endl;
				 }
			}
		}
	}
#endif
};

#endif