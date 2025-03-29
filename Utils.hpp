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

		static int toInt(std::string str)
		{
			std::stringstream ss(str);
			int number;
			ss >> number;
			return number;
		}

		static int ft_memcmp(const void *s1, const void *s2, size_t n)
		{
			unsigned char	*p1;
			unsigned char	*p2;

			p1 = (unsigned char *)s1;
			p2 = (unsigned char *)s2;
			while (n > 0)
			{
				if (*p1 != *p2)
					return (-1);
				p1++;
				p2++;
				n--;
			}
			return (0);
		}

		static void	*ft_memset(void *s, int c, size_t len)
		{
			unsigned char	*temp;
			int				i;
		
			i = 0;
			temp = (unsigned char *)s; 
			while (len > 0)
			{
				temp[i] = c;
				i++;
				len--;
			}
			return (temp);
		}

		static char toLowerChar(char c)
		{
			return std::tolower(static_cast<unsigned char>(c));
		}
	private:
		Utils();		
};

#endif