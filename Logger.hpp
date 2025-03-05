#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>

#define RESET   "\033[0m"
#define BLUE   "\033[34m"  // Blue
#define GREEN    "\033[32m"  // Green
#define YELLOW "\033[33m"  // Yellow
#define RED   "\033[31m"  // Red

enum LogPriority {DEBUG, INFO, WARN, ERROR};

class Logger
{
	private:
		Logger();
		static LogPriority priority;
	public:
		void setLevelLog(LogPriority new_priority) {priority = new_priority;};

		static void debug(std::string message)
		{
			if (priority <= DEBUG)
			{
				std::cout << BLUE << "[DEBUG]: " << message << RESET << std::endl;
			}
		}

		static void info(std::string message)
		{
			if (priority <= INFO)
			{
				std::cout << GREEN << "[INFO]: " << message << RESET << std::endl;
			}
		}

		static void warn(std::string message)
		{
			if (priority <= WARN)
			{
				std::cout << YELLOW << "[WARN]: " << message << RESET << std::endl;
			}
		}

		static void error(std::string message)
		{
			if (priority <= ERROR)
			{
				std::cout << RED << "[ERROR]: " << message << RESET << std::endl;
			}
		}
};

#endif