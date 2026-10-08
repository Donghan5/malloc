/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_ui.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: donghank <donghank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 21:35:37 by donghank          #+#    #+#             */
/*   Updated: 2026/10/08 21:35:37 by donghank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <stddef.h>
#include "test_ui.h"

static void put(const char *text)
{
    size_t size = 0;
    ssize_t written;
    while (text[size]) ++size;
    while (size)
    {
        written = write(1, text, size);
        if (written <= 0) return;
        text += written;
        size -= (size_t)written;
    }
}

static void number(unsigned int value)
{
    char digit;
    if (value >= 10) number(value / 10);
    digit = (char)('0' + value % 10);
    write(1, &digit, 1);
}

void test_ui_start(const char *name)
{
    put("  RUN   "); put(name); put("\n");
}

void test_ui_result(const char *name, int success)
{
    put(success ? "  \033[32mPASS\033[0m  " : "  \033[31mFAIL\033[0m  ");
    put(name); put("\n");
}

void test_ui_heading(const char *suite)
{
    put("\n========== "); put(suite); put(" ==========\n");
}

void test_ui_summary(const char *suite, unsigned int passed, unsigned int total)
{
    put("\n"); put(passed == total ? "\033[32m" : "\033[31m");
    put(suite); put(": "); number(passed); put("/"); number(total);
    put(" passed; "); number(total - passed); put(" failed\033[0m\n");
}
