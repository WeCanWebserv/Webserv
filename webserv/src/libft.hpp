#ifndef LIBFT_HPP
#define LIBFT_HPP

#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> ft_split(const std::string& str, char delim = ' ');
void ft_trim(std::string &str, const std::string &ws = "\t\n\v\f\r ");
void ft_replace(std::stringstream &strStream, const std::string &s1, const std::string &s2);

#endif // LIBFT_HPP
