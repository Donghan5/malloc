#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stddef.h>

/*
** Function mandatory prototypes
*/
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);

/*
** src/block
*/
int		get_order_from_size(size_t size);
void	find_available_buddy_block(size_t size, t_block **res_block);
void	init_block(t_block *block, size_t size);
void	add_to_free_list(t_block *block, int order);
void	free_and_merge_buddies(t_block *block_to_free);
void	remove_block_if_last(t_heap *heap, t_block *block);
t_block	*remove_block_from_free_list(t_block *block);


/*
** src/heap
*/
size_t	get_heap_group_from_block_size(const size_t size);
t_heap	*get_heap_of_block_size(size_t size);
t_heap	*get_last_heap(t_heap *heap);
void	*append_empty_block(t_heap *heap, size_t size);
t_heap	*create_new_heap(const t_heap_group group, const size_t size);
void	remove_heap(t_heap *heap);
t_block	*get_last_block(t_heap *heap);

/*
** src/tools
*/
void	print_heap_header(char *name, t_heap *heap);
size_t	print_block_list(t_block *block);
void	print_alloc_mem(void);
void	show_alloc_mem(void);
size_t	ft_strlen(const char *s);
void	ft_putstr_fd(const char *str, int fd);
void	*ft_memset(void *dest, int value, size_t count);
void	*ft_memmove(void *dest, const void *src, size_t len);
void	print_memory_address_portable(void *addr);
void	ft_print_unsigned_fd(unsigned long long n, int fd);

#endif
