#include <cstdlib>

#include <exception>
#include <iostream>

#include "ServerManager.hpp"

void usage(char *name)
{
	std::cerr << "Wrong number of arguments\n";
	std::cerr << "Usage: " << name << " [configuration file path]\n";
	exit(1);
}

int main(int ac, char **av)
{
	if (ac > 2)
	{
		usage(av[1]);
		exit(1);
	}
	try
	{
		ServerManager manager(av[1]);
		manager.loop();
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}