/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/05 17:41:24 by donghank          #+#    #+#             */
/*   Updated: 2026/04/02 00:00:00 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/malloc.h"

/* ================================================================ */
/*  Output helpers (no printf – ft_putstr_fd / ft_print_unsigned_fd) */
/* ================================================================ */

static void	put_pass(void)
{
	ft_putstr_fd("[PASS]\n", 1);
}

static void	put_fail(void)
{
	ft_putstr_fd("[FAIL]\n", 1);
}

/*
** Print "  <desc>: [PASS|FAIL]\n"
*/
static void	check(int cond, const char *desc)
{
	ft_putstr_fd("  ", 1);
	ft_putstr_fd(desc, 1);
	ft_putstr_fd(": ", 1);
	if (cond)
		put_pass();
	else
		put_fail();
}

/*
** Print "  <desc> @ 0x<addr>: [PASS|FAIL]\n"
** PASS only when ptr is non-NULL AND 16-byte aligned.
*/
static void	check_aligned(void *ptr, const char *desc)
{
	ft_putstr_fd("  ", 1);
	ft_putstr_fd(desc, 1);
	ft_putstr_fd(" @ ", 1);
	print_memory_address_portable(ptr);
	ft_putstr_fd(": ", 1);
	if (ptr && ((uintptr_t)ptr % 16 == 0))
		put_pass();
	else
		put_fail();
}

static void	section(const char *title)
{
	ft_putstr_fd("\n--- ", 1);
	ft_putstr_fd(title, 1);
	ft_putstr_fd(" ---\n", 1);
}

/* ================================================================ */
/*  Data pattern helpers                                            */
/* ================================================================ */

/*
** Write 'pattern' to every byte in [ptr, ptr+size).
*/
static void	fill_pattern(void *ptr, unsigned char pattern, size_t size)
{
	unsigned char	*bytes;
	size_t			i;

	bytes = (unsigned char *)ptr;
	i = 0;
	while (i < size)
		bytes[i++] = pattern;
}

/*
** Return 1 if every byte in [ptr, ptr+size) equals 'pattern', else 0.
*/
static int	verify_pattern(void *ptr, unsigned char pattern, size_t size)
{
	unsigned char	*bytes;
	size_t			i;

	bytes = (unsigned char *)ptr;
	i = 0;
	while (i < size)
	{
		if (bytes[i] != pattern)
			return (0);
		i++;
	}
	return (1);
}

/* ================================================================ */
/*  1. malloc – basic TINY / SMALL / LARGE                          */
/* ================================================================ */

static void	test_malloc_basic(void)
{
	void	*p_tiny;
	void	*p_small;
	void	*p_large;

	section("1. malloc – basic TINY / SMALL / LARGE");

	p_tiny  = malloc(100);
	p_small = malloc(500);
	p_large = malloc(3000);

	check(p_tiny  != NULL, "malloc(100)  TINY  non-NULL");
	check(p_small != NULL, "malloc(500)  SMALL non-NULL");
	check(p_large != NULL, "malloc(3000) LARGE non-NULL");

	/* 16-byte alignment */
	check_aligned(p_tiny,  "malloc(100)  aligned");
	check_aligned(p_small, "malloc(500)  aligned");
	check_aligned(p_large, "malloc(3000) aligned");

	/* malloc must initialise memory with 0xaa */
	if (p_tiny)
		check(verify_pattern(p_tiny,  0xaa, 100),  "malloc(100)  init 0xaa");
	if (p_small)
		check(verify_pattern(p_small, 0xaa, 500),  "malloc(500)  init 0xaa");
	if (p_large)
		check(verify_pattern(p_large, 0xaa, 3000), "malloc(3000) init 0xaa");

	ft_putstr_fd("\n  Heap after basic allocs:\n", 1);
	show_alloc_mem();

	free(p_tiny);
	free(p_small);
	free(p_large);
}

/* ================================================================ */
/*  2. malloc – size 0 and boundary values                          */
/* ================================================================ */

static void	test_malloc_edge(void)
{
	void	*p0;
	void	*p1;
	void	*p128;
	void	*p129;
	void	*p1024;
	void	*p1025;

	section("2. malloc – size 0 and boundary values");

	p0    = malloc(0);
	p1    = malloc(1);
	p128  = malloc(128);
	p129  = malloc(129);
	p1024 = malloc(1024);
	p1025 = malloc(1025);

	check(p0    == NULL, "malloc(0)    -> NULL");
	check(p1    != NULL, "malloc(1)    TINY min  non-NULL");
	check(p128  != NULL, "malloc(128)  TINY max  non-NULL");
	check(p129  != NULL, "malloc(129)  SMALL min non-NULL");
	check(p1024 != NULL, "malloc(1024) SMALL max non-NULL");
	check(p1025 != NULL, "malloc(1025) LARGE min non-NULL");

	if (p1)    check_aligned(p1,    "malloc(1)    aligned");
	if (p128)  check_aligned(p128,  "malloc(128)  aligned");
	if (p129)  check_aligned(p129,  "malloc(129)  aligned");
	if (p1024) check_aligned(p1024, "malloc(1024) aligned");
	if (p1025) check_aligned(p1025, "malloc(1025) aligned");

	ft_putstr_fd("\n  Heap after boundary allocs:\n", 1);
	show_alloc_mem();

	free(p1);
	free(p128);
	free(p129);
	free(p1024);
	free(p1025);
}

/* ================================================================ */
/*  3. free – NULL, single, and out-of-order frees                  */
/* ================================================================ */

static void	test_free(void)
{
	void	*a;
	void	*b;
	void	*c;
	void	*d;

	section("3. free – NULL and out-of-order frees");

	/* free(NULL) must not crash */
	ft_putstr_fd("  free(NULL): ", 1);
	free(NULL);
	put_pass();

	/* allocate four blocks, free in non-insertion order */
	a = malloc(1);
	b = malloc(128);
	c = malloc(1024);
	d = malloc(2048);

	free(d);
	free(b);
	free(a);
	free(c);
	ft_putstr_fd("  free out-of-order (1, 128, 1024, 2048): ", 1);
	put_pass();

	ft_putstr_fd("\n  Heap after free tests:\n", 1);
	show_alloc_mem();
}

/* ================================================================ */
/*  4. realloc – edge cases (NULL pointer, zero size)               */
/* ================================================================ */

static void	test_realloc_edge(void)
{
	void	*p;
	void	*q;

	section("4. realloc – edge cases");

	/* realloc(NULL, n) must behave like malloc(n) */
	p = realloc(NULL, 200);
	check(p != NULL, "realloc(NULL, 200) -> non-NULL (like malloc)");
	if (p)
		check_aligned(p, "realloc(NULL, 200) aligned");
	free(p);

	/* realloc(ptr, 0) must behave like free and return NULL */
	p = malloc(100);
	check(p != NULL, "malloc(100) before realloc(ptr, 0)");
	q = realloc(p, 0);
	check(q == NULL, "realloc(ptr, 0)    -> NULL (like free)");

	ft_putstr_fd("\n  Heap after realloc edge tests:\n", 1);
	show_alloc_mem();
}

/* ================================================================ */
/*  5. realloc – shrink, grow, and data integrity                   */
/* ================================================================ */

static void	test_realloc_data(void)
{
	void	*p;
	void	*q;

	section("5. realloc – shrink, grow, data integrity");

	/* --- shrink: data in the kept region must be preserved --- */
	p = malloc(256);
	check(p != NULL, "malloc(256) for shrink test");
	if (p)
	{
		fill_pattern(p, 0xAB, 128);
		q = realloc(p, 128);
		check(q != NULL,                    "realloc(256->128) non-NULL");
		if (q)
		{
			check(verify_pattern(q, 0xAB, 128), "realloc(256->128) data 0xAB preserved");
			free(q);
		}
	}

	/* --- grow TINY -> SMALL: first bytes must survive --- */
	p = malloc(64);
	check(p != NULL, "malloc(64) for TINY->SMALL grow test");
	if (p)
	{
		fill_pattern(p, 0xCD, 64);
		q = realloc(p, 500);
		check(q != NULL,                   "realloc(64->500)  non-NULL");
		if (q)
		{
			check(verify_pattern(q, 0xCD, 64), "realloc(64->500)  first 64 bytes 0xCD preserved");
			check_aligned(q,                   "realloc(64->500)  aligned");
			free(q);
		}
	}

	/* --- grow SMALL -> LARGE: first bytes must survive --- */
	p = malloc(512);
	check(p != NULL, "malloc(512) for SMALL->LARGE grow test");
	if (p)
	{
		fill_pattern(p, 0xEF, 512);
		q = realloc(p, 2048);
		check(q != NULL,                    "realloc(512->2048) non-NULL");
		if (q)
		{
			check(verify_pattern(q, 0xEF, 512), "realloc(512->2048) first 512 bytes 0xEF preserved");
			check_aligned(q,                    "realloc(512->2048) aligned");
			free(q);
		}
	}

	ft_putstr_fd("\n  Heap after realloc data tests:\n", 1);
	show_alloc_mem();
}

/* ================================================================ */
/*  6. Stress – 32 LARGE allocs, mixed frees                        */
/* ================================================================ */

static void	test_stress_large(void)
{
	void		*ptrs[32];
	int			ok_alloc;
	int			ok_align;
	int			i;

	section("6. Stress – 32 LARGE allocs (2048 bytes each)");

	i = 0;
	while (i < 32)
	{
		ptrs[i] = malloc(2048);
		i++;
	}

	ok_alloc = 0;
	ok_align = 0;
	i = 0;
	while (i < 32)
	{
		if (ptrs[i] != NULL)
			ok_alloc++;
		if (ptrs[i] && (uintptr_t)ptrs[i] % 16 == 0)
			ok_align++;
		i++;
	}

	ft_putstr_fd("  non-NULL (", 1);
	ft_print_unsigned_fd(ok_alloc, 1);
	ft_putstr_fd("/32): ", 1);
	if (ok_alloc == 32)
		put_pass();
	else
		put_fail();

	ft_putstr_fd("  16-byte aligned (", 1);
	ft_print_unsigned_fd(ok_align, 1);
	ft_putstr_fd("/32): ", 1);
	if (ok_align == 32)
		put_pass();
	else
		put_fail();

	ft_putstr_fd("\n  Heap during stress:\n", 1);
	show_alloc_mem();

	/* free even indices first, then odd */
	i = 0;
	while (i < 32)
	{
		if (i % 2 == 0)
			free(ptrs[i]);
		i++;
	}
	i = 1;
	while (i < 32)
	{
		if (i % 2 != 0)
			free(ptrs[i]);
		i++;
	}

	ft_putstr_fd("\n  Heap after stress free:\n", 1);
	show_alloc_mem();
}

/* ================================================================ */
/*  7. Multi-thread – concurrent malloc/free from N threads         */
/* ================================================================ */

#define THREAD_COUNT	4
#define ALLOCS_PER_THR	50

static void	*thread_routine(void *arg)
{
	void	*ptrs[ALLOCS_PER_THR];
	int		i;

	(void)arg;
	i = 0;
	while (i < ALLOCS_PER_THR)
	{
		ptrs[i] = malloc(64 + i * 8);
		if (ptrs[i])
			fill_pattern(ptrs[i], 0xBB, 64);
		i++;
	}
	i = 0;
	while (i < ALLOCS_PER_THR)
	{
		if (ptrs[i])
		{
			if (!verify_pattern(ptrs[i], 0xBB, 64))
				ft_putstr_fd("  [THREAD] data corruption detected\n", 1);
			free(ptrs[i]);
		}
		i++;
	}
	return (NULL);
}

static void	test_multithread(void)
{
	pthread_t	threads[THREAD_COUNT];
	int			i;
	int			ok;

	section("7. Multi-thread – concurrent malloc/free");

	i = 0;
	ok = 1;
	while (i < THREAD_COUNT)
	{
		if (pthread_create(&threads[i], NULL, thread_routine, NULL) != 0)
		{
			ft_putstr_fd("  pthread_create failed\n", 1);
			ok = 0;
		}
		i++;
	}
	i = 0;
	while (i < THREAD_COUNT)
	{
		pthread_join(threads[i], NULL);
		i++;
	}
	check(ok, "all threads completed without crash");

	ft_putstr_fd("\n  Heap after multi-thread test:\n", 1);
	show_alloc_mem();
}

/* ================================================================ */
/*  main                                                            */
/* ================================================================ */

int	main(void)
{
	ft_putstr_fd("===== ft_malloc Test Suite =====\n", 1);

	test_malloc_basic();
	test_malloc_edge();
	test_free();
	test_realloc_edge();
	test_realloc_data();
	test_stress_large();
	test_multithread();

	ft_putstr_fd("\n===== Test Suite End =====\n", 1);
	return (0);
}
