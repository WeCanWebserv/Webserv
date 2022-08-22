int main(int ac, char **av)
{
	if (ac > 2)
	{
		usage(av[1]);
		exit(1);
	}

	ServerManager manager(av[1]);
	try
	{
		manager.initialize();
		manager.loop();
	}
	catch (...)
	{
		//
	}
}