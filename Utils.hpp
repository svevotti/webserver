#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <sstream>

class Utils {
	public:
		static std::string toString(int number)
		{
			std::ostringstream oss;
   			oss << number;
			return oss.str();
		}

	private:
		Utils();		
};

#endif