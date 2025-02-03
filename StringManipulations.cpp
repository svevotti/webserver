#include "StringManipulations.hpp"

void	*ft_memset(void *s, int c, size_t len)
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
