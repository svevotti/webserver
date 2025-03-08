#include "Config.hpp"

// bool	test(InfoServer server)
// {

// }

int	main(int argc, char** argv){
	if (argc != 2)
	{
		std::cout << "Error! Please include only a path to a config file" << std::endl;
		return 1;
	}
	Config	configuration(argv[1]);
	InfoServer server = *configuration._servlist.begin();

	// if (test(server))
	// 	std::cout << "All worked well!" << std::endl;
	// else
	// 	std::cout << "Error!" << std::endl;
	return 0;
}
