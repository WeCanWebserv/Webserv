#ifndef LIBFT_HPP
#define LIBFT_HPP

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> ft_split(const std::string &str, char delim = ' ');
void ft_trim(std::string &str, const std::string &ws = "\t\n\v\f\r ");
void ft_replace(std::stringstream &strStream, const std::string &s1, const std::string &s2);

namespace ft
{
char *strdup(const std::string &str);
int toUnderscore(int c);

template<class T>
std::string toString(T value)
{
	std::stringstream ss;

	ss << value;
	return (ss.str());
}

template<class UnaryOP>
std::string transform(const std::string &str, UnaryOP op)
{
	std::string up = str;

	std::transform(up.begin(), up.end(), up.begin(), op);
	return (up);
}
} // namespace ft

#endif // LIBFT_HPP
