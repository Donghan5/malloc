#include "../inc/malloc.h"

static void	debug_calloc(size_t count, size_t size, void *result)
{
	if (g_data.debug)
	{
	    ft_putstr_fd("[MALLOC] calloc(", 1);
		ft_print_unsigned_fd(count, 1);
		ft_putstr_fd(", ", 1);
		ft_print_unsigned_fd(size, 1);
		ft_putstr_fd(") = ", 1);
		print_memory_address_portable(result);
		ft_putstr_fd("\n", 1);
	}
}

void    *calloc(size_t count, size_t size)
{
    void	*ptr;
    if (size != 0 && count > SIZE_MAX / size)
        return (NULL);

    ptr = malloc(count * size);
    if (!ptr)
        return (NULL);

    pthread_mutex_lock(&g_malloc_mutex);
    debug_calloc(count, size, ptr);
    pthread_mutex_unlock(&g_malloc_mutex);
    ft_memset(ptr, 0, count * size);
    return (ptr);
}
