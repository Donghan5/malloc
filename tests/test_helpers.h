#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "malloc.h"

int starts_with(const char *text, const char *prefix);
char *find_text(char *text, const char *needle);
int parse_number(const char **cursor, unsigned int base, uintptr_t *value);
int pattern(const unsigned char *p, size_t n, unsigned char value);
t_heap *owner(void *p);

#endif
