// Keep library-returned buffers in the same allocation family as native free().
#define _GNU_SOURCE
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *aligned_alloc(size_t alignment, size_t size)
{
    if (!alignment || (alignment & (alignment - 1)) || size % alignment) {
        errno = EINVAL;
        return NULL;
    }
    if (alignment < sizeof(void *)) alignment = sizeof(void *);
    void *address = NULL;
    int result = posix_memalign(&address, alignment, size);
    if (result) errno = result;
    return result ? NULL : address;
}

char *strndup(const char *source, size_t maximum)
{
    size_t length = strnlen(source, maximum);
    if (length == SIZE_MAX) { errno = ENOMEM; return NULL; }
    char *copy = malloc(length + 1);
    if (copy) { memcpy(copy, source, length); copy[length] = 0; }
    return copy;
}

int asprintf(char **output, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vasprintf(output, format, args);
    va_end(args);
    return result;
}
