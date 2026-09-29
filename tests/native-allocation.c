#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    void *memory = aligned_alloc(64, 128);
    assert(memory && (uintptr_t)memory % 64 == 0);
    free(memory);
    assert(!aligned_alloc(0, 128) && errno == EINVAL);
    assert(!aligned_alloc(24, 128) && errno == EINVAL);
    assert(!aligned_alloc(64, 127) && errno == EINVAL);
    char *copy = strndup("OpenStory", 4);
    assert(copy && strcmp(copy, "Open") == 0);
    free(copy);
    copy = strndup("OpenStory", 0);
    assert(copy && *copy == 0);
    free(copy);
    assert(asprintf(&copy, "%s %d", "OpenStory", 83) == 12);
    assert(strcmp(copy, "OpenStory 83") == 0);
    free(copy);
    puts("Native allocation adapters passed");
}
