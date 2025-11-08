/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/05 17:41:24 by donghank          #+#    #+#             */
/*   Updated: 2025/11/05 17:41:25 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../inc/malloc.h"

int main(void)
{
	ft_putstr_fd("===== Malloc Test Start =====\n\n", 1);

	ft_putstr_fd("--- 1. Basic Allocations (TINY, SMALL, LARGE) ---\n", 1);
	// TINY (<= 128 bytes)
	ft_putstr_fd("Allocating ptr1 (TINY, 100 bytes):   ", 1);
	void *ptr1 = malloc(100);
	print_memory_address_portable(ptr1);
	ft_putstr_fd("\n", 1);

	// SMALL (<= 1024 bytes)
	ft_putstr_fd("Allocating ptr2 (SMALL, 500 bytes):  ", 1);
	void *ptr2 = malloc(500);
	print_memory_address_portable(ptr2);
	ft_putstr_fd("\n", 1);

	// LARGE (> 1024 bytes)
	ft_putstr_fd("Allocating ptr3 (LARGE, 3000 bytes): ", 1);
	void *ptr3 = malloc(3000);
	print_memory_address_portable(ptr3);
	ft_putstr_fd("\n", 1);

	ft_putstr_fd("\n--- Current State (1) ---\n", 1);
	show_alloc_mem();

	ft_putstr_fd("\n--- 2. Edge Case Allocations ---\n", 1);
	// TINY 경계값
	ft_putstr_fd("Allocating ptr_t_min (TINY, 1 byte):     ", 1);
	void *ptr_t_min = malloc(1);
	print_memory_address_portable(ptr_t_min);
	ft_putstr_fd("\n", 1);

	ft_putstr_fd("Allocating ptr_t_max (TINY, 128 bytes):  ", 1);
	void *ptr_t_max = malloc(128);
	print_memory_address_portable(ptr_t_max);
	ft_putstr_fd("\n", 1);

	// SMALL 경계값
	ft_putstr_fd("Allocating ptr_s_min (SMALL, 129 bytes): ", 1);
	void *ptr_s_min = malloc(129);
	print_memory_address_portable(ptr_s_min);
	ft_putstr_fd("\n", 1);

	ft_putstr_fd("Allocating ptr_s_max (SMALL, 1024 bytes):", 1);
	void *ptr_s_max = malloc(1024);
	print_memory_address_portable(ptr_s_max);
	ft_putstr_fd("\n", 1);

	// LARGE 경계값
	ft_putstr_fd("Allocating ptr_l_min (LARGE, 1025 bytes):", 1);
	void *ptr_l_min = malloc(1025);
	print_memory_address_portable(ptr_l_min);
	ft_putstr_fd("\n", 1);

	ft_putstr_fd("\n--- Current State (2) ---\n", 1);
	show_alloc_mem();

	ft_putstr_fd("\n--- 3. Freeing Operations (Out of Order) ---\n", 1);
	ft_putstr_fd("Freeing ptr2 (SMALL, 500)\n", 1);
	free(ptr2);
	ft_putstr_fd("Freeing ptr_l_min (LARGE, 1025)\n", 1);
	free(ptr_l_min);
	ft_putstr_fd("Freeing ptr_t_max (TINY, 128)\n", 1);
	free(ptr_t_max);

	ft_putstr_fd("\n--- Current State (3) ---\n", 1);
	show_alloc_mem();

	ft_putstr_fd("\n--- 4. Freeing All Remaining Pointers ---\n", 1);
	ft_putstr_fd("Freeing ptr1\n", 1);
	free(ptr1);
	ft_putstr_fd("Freeing ptr3\n", 1);
	free(ptr3);
	ft_putstr_fd("Freeing ptr_t_min\n", 1);
	free(ptr_t_min);
	ft_putstr_fd("Freeing ptr_s_min\n", 1);
	free(ptr_s_min);
	ft_putstr_fd("Freeing ptr_s_max\n", 1);
	free(ptr_s_max);

	ft_putstr_fd("Freeing NULL...\n", 1);
	free(NULL);

	// malloc(0) 테스트 (NULL을 반환해야 함)
	ft_putstr_fd("Allocating malloc(0)...\n", 1);
	void *ptr_zero = malloc(0);
	ft_putstr_fd("ptr_zero (malloc(0)): ", 1);
	print_memory_address_portable(ptr_zero); // 0x0이 출력되어야 함
	ft_putstr_fd("\n", 1);

	ft_putstr_fd("\n--- Final State (Should be empty or only show heap headers) ---\n", 1);
	show_alloc_mem();

	ft_putstr_fd("\n===== Malloc Test End =====\n", 1);
	return (0);
}