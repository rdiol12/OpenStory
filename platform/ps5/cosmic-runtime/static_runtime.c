/* Native lookup for the Java libraries compiled into this application.
 * No external module loading is implemented; unknown libraries fail explicitly.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "static_symbols.inc"

extern char __cosmic_image_start[], __cosmic_image_end[];
static int process_handle;
static _Thread_local const char *last_error;

void *dlopen(const char *path, int mode) {
    const int binding = mode & (RTLD_LAZY | RTLD_NOW);
    const int allowed = RTLD_LAZY | RTLD_NOW | RTLD_GLOBAL | RTLD_LOCAL | RTLD_NODELETE | RTLD_NOLOAD;
    if ((binding != RTLD_LAZY && binding != RTLD_NOW) || (mode & ~allowed)) {
        last_error = "Unsupported static runtime dlopen flags";
        return NULL;
    }
    if (path == NULL) return &process_handle;
    const char *name = strrchr(path, '/');
    name = name ? name + 1 : path;
    for (size_t i = 0; i < sizeof(libraries) / sizeof(libraries[0]); ++i)
        if (strcmp(name, libraries[i]) == 0) return &process_handle;
    last_error = "Native library is not compiled into this application";
    return NULL;
}

static int compare_symbol(const void *name, const void *entry) {
    return strcmp(name, ((const struct native_symbol *)entry)->name);
}

void *dlsym(void *handle, const char *name) {
    if (handle != &process_handle && handle != RTLD_DEFAULT
#ifdef RTLD_SELF
        && handle != RTLD_SELF
#endif
    ) {
        last_error = "Invalid static runtime library handle";
        return NULL;
    }
    const struct native_symbol *found = bsearch(name, symbols,
        sizeof(symbols) / sizeof(symbols[0]), sizeof(symbols[0]), compare_symbol);
    if (found) return (void *)found->address;
    last_error = "Native symbol is not compiled into this application";
    return NULL;
}

int dlclose(void *handle) {
    if (handle == &process_handle) return 0; /* Builtins live as long as the process. */
    last_error = "Invalid static runtime library handle";
    return -1;
}

char *dlerror(void) {
    const char *error = last_error;
    last_error = NULL;
    return (char *)error;
}

int dladdr(const void *address, Dl_info *info) {
    const uintptr_t value = (uintptr_t)address;
    if (value < (uintptr_t)__cosmic_image_start || value >= (uintptr_t)__cosmic_image_end)
        return 0;
    *info = (Dl_info){.dli_fname = "/app0/eboot.bin", .dli_fbase = __cosmic_image_start};
    /* ponytail: report this image only; add symbolization/system modules for richer crash reports. */
    return 1;
}
