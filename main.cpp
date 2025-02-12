#include "Config.hpp"

int	main(int argc, char** argv){
	if (argc != 2)
	{
		std::cout << "Error! Please include only a path to a config file" << std::endl;
		return 1;
	}
	Config	configuration(argv[1]);
	Server server = *configuration._servlist.begin();
	std::cout << "here" << server._port << std::endl;
	return 0;
}
