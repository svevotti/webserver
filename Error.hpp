#ifndef ERROR_H
#define ERROR_H

#include <iostream>
#include <exception>
#include <string>

class Error : std::exception
{
	public:
	Error(std::string message)
	{
		this->_message = message;
	}
	virtual const char* what()  const noexcept override
	{
		return this->_message.c_str();
	}
	private:
		std::string _message;
};





#endif