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

int	ft_memcmp(const void *s1, const void *s2, size_t n)
{
	unsigned char	*p1;
	unsigned char	*p2;
	// int				subs;

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

void	*ft_memcpy(void *dest, const void *src, size_t n)
{
	const char	*t_src;
	char		*t_dest;
	int			i;

	i = 0;
	t_src = (const char*)src;
	t_dest = (char*)dest;
	if (t_src == NULL && t_dest == NULL)
		return (dest);
	while (n > 0)
	{
		t_dest[i] = t_src[i];
		i++;
		n--;
	}
	return (dest);
}