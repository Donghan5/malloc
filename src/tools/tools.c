/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tools.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/20 20:41:57 by donghank          #+#    #+#             */
/*   Updated: 2025/11/05 17:41:02 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/malloc.h"

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


/*
** Description: Prints a memory address in a portable hexadecimal format
** Parameters:
**   - addr: The memory address to print
** sizeof(void *) * 2: Each byte is represented by two hex digits --> 32-bit = 8 digits, 64-bit = 16 digits
*/
void    print_memory_address_portable(void *addr)
{
    uintptr_t   address;
    size_t      num_digits; 
    int         i;
    char        buffer[sizeof(void *) * 2 + 1]; 

    address = (uintptr_t)addr;
    
    num_digits = sizeof(void *) * 2;

    buffer[num_digits] = '\0'; 
    i = num_digits - 1;      
    if (address == 0)
    {
        while (i >= 0)
            buffer[i--] = '0';
    }
    else
    {
        while (address > 0 && i >= 0)
        {
            buffer[i] = "0123456789abcdef"[address % 16]; 
            address /= 16;
            i--;
        }
        while (i >= 0)
        {
            buffer[i] = '0';
            i--;
        }
    }
    
    ft_putstr_fd("0x", 1);
    ft_putstr_fd(buffer, 1);
}

void	print_hex_byte(unsigned char byte)
{
	char	hex[2];

	hex[0] = "0123456789abcdef"[byte / 16];
	hex[1] = "0123456789abcdef"[byte % 16];
	write(1, hex, 2);
}

void	ft_print_hex_dump(void *addr, size_t size)
{
	size_t	i;

	i = 0;
	while (i < size)
	{
		print_hex_byte(((unsigned char *)addr)[i]);
		ft_putstr_fd(" ", 1);
		// line change each 16 bytes
		if ((i + 1) % 16 == 0)
			ft_putstr_fd("\n", 1);
		i++;
	}
	if (size % 16 != 0)
		ft_putstr_fd("\n", 1);
}

void	ft_print_unsigned_fd(unsigned long long n, int fd)
{
	char			c;
	unsigned long long	num;

	num = n;
	if (num >= 10)
		ft_print_unsigned_fd(num / 10, fd);
	c = (num % 10) + '0';
	write(fd, &c, 1);
}

void	init_debug_flags(void)
{
	char	*val;

	if (!g_data.initialized)
	{
		g_data.debug = (getenv("MALLOC_DEBUG") != NULL);
		g_data.initialized = 1;
	}
	val = getenv("MALLOC_SCRIBBLE");
	g_data.scribble = ! (val && val[0] == '0');
}