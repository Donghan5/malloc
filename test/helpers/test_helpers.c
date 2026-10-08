#include "malloc.h"
#include "test_helpers.h"

/* Keep comparison and parsing independent of the allocator under test. */
int starts_with(const char *text, const char *prefix)
{
    while (*prefix && *text == *prefix)
    {
        ++text;
        ++prefix;
    }
    return *prefix == '\0';
}

char *find_text(char *text, const char *needle)
{
    while (*text)
    {
        if (starts_with(text, needle))
            return text;
        ++text;
    }
    return NULL;
}

int parse_number(const char **cursor, unsigned int base, uintptr_t *value)
{
    const char *p = *cursor;
    unsigned int digit;
    size_t count = 0;

    *value = 0;
    while (*p)
    {
        if (*p >= '0' && *p <= '9')
            digit = (unsigned int)(*p - '0');
        else if (*p >= 'a' && *p <= 'f')
            digit = (unsigned int)(*p - 'a' + 10);
        else
            break;
        if (digit >= base)
            break;
        if (*value > (UINTPTR_MAX - digit) / base)
            return 0;
        *value = *value * base + digit;
        ++p;
        ++count;
    }
    *cursor = p;
    return count != 0;
}

int pattern(const unsigned char *p, size_t n, unsigned char value)
{
    size_t i;
    for (i = 0; i < n; ++i)
        if (p[i] != value)
            return 0;
    return 1;
}

t_heap *owner(void *p)
{
    t_heap *heap = NULL;
    t_block *block = NULL;
    search_pointer(&heap, &block, g_data.heap_anchor, p);
    return heap;
}

