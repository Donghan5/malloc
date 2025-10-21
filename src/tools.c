/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tools.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/20 20:41:57 by donghank          #+#    #+#             */
/*   Updated: 2025/10/20 20:44:04 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/malloc.h"

size_t	ft_strlen(const char *s)
{
	size_t	i;

	i = 0;
	while (s[i] != '\0')
		i++;
	return (i);
}

void	ft_putstr_fd(const char *str, int fd)
{
	write(fd, str, ft_strlen(str));
}

void	*ft_memset(void *dest, int value, size_t count)
{
	size_t			i;
	unsigned char	*temp;

	temp = (unsigned char *)dest;
	i = 0;
	while (i < count)
	{
		temp[i] = (unsigned char)value;
		i++;
	}
	return (dest);
}

void	*ft_memmove(void *dest, const void *src, size_t len)
{
	unsigned char		*d;
	const unsigned char	*s;

	d = (unsigned char *)dest;
	s = (const unsigned char *)src;
	if (dest == src)
		return (dest);
	if ((size_t)(d - s) > len)
	{
		while (len--)
			*d++ = *s++;
	}
	else
	{
		while (len--)
			*(d + len) = *(s + len);
	}
	return (dest);
}

static size_t	num_len(int n)
{
	size_t	i;

	i = 0;
	if (n < 0)
		n *= -1;
	if (n == 0)
		return (1);
	while (n)
	{
		n /= 10;
		i++;
	}
	return (i);
}

void    print_memory_address(void *addr)
{
    uintptr_t    address;
    char        buffer[19];
    int         i;

    address = (uintptr_t)addr;
    buffer[18] = '\0';
    i = 17;
    while (i >= 0)
    {
        buffer[i] = "0123456789ABCDEF"[address % 16];
        address /= 16;
        i--;
    }
    ft_putstr_fd("0x", 1);
    ft_putstr_fd(buffer, 1);
}