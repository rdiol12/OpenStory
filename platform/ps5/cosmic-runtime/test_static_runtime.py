"""Exercise the real generated lookup table and C runtime on the build host."""
from pathlib import Path
import subprocess
import tempfile
from static_symbols import generate

HERE = Path(__file__).resolve().parent

with tempfile.TemporaryDirectory() as directory:
    work = Path(directory)
    (work / 'symbols.c').write_text('int Java_probe(void) { return 42; }\n')
    subprocess.run(['clang-18', '-c', work / 'symbols.c', '-o', work / 'symbols.o'], check=True)
    subprocess.run(['llvm-ar-18', 'rcs', work / 'libjava.a', work / 'symbols.o'], check=True)
    generate([work / 'libjava.a'], work / 'static_symbols.inc')
    (work / 'check.c').write_text(r'''
#define _GNU_SOURCE
#include <assert.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
extern int Java_probe(void);
static void *other_thread(void *unused) {
    assert(dlerror() == NULL);
    assert(dlopen("missing.so", RTLD_NOW) == NULL);
    assert(dlerror() != NULL);
    return NULL;
}
int main(void) {
    void *process = dlopen(NULL, RTLD_NOW);
    assert(process != NULL);
    assert(dlopen("/app0/java/lib/libjava.so", RTLD_LAZY) == process);
    int (*probe)(void) = (int (*)(void))dlsym(process, "Java_probe");
    assert(probe && probe() == 42);
    assert(dlsym(RTLD_DEFAULT, "Java_probe") == (void *)probe);
    assert(dlsym((void *)(uintptr_t)1234, "Java_probe") == NULL);
    assert(dlerror() != NULL && dlerror() == NULL);
    assert(dlsym(process, "unknown") == NULL);
    pthread_t thread;
    assert(pthread_create(&thread, NULL, other_thread, NULL) == 0);
    assert(pthread_join(thread, NULL) == 0);
    assert(dlerror() != NULL && dlerror() == NULL);
    assert(dlopen("libmissing.so", RTLD_NOW) == NULL);
    assert(dlopen("libjava.so", 0) == NULL);
    assert(dlopen("libjava.so", RTLD_LAZY | RTLD_NOW) == NULL);
    assert(dlclose((void *)(uintptr_t)1234) != 0);
    assert(dlclose(process) == 0);
    /* Builtins remain available for the process lifetime after dlclose. */
    assert(((int (*)(void))dlsym(process, "Java_probe"))() == 42);
    Dl_info info;
    assert(dladdr((void *)Java_probe, &info) == 1);
    assert(strcmp(info.dli_fname, "/app0/eboot.bin") == 0);
    assert(info.dli_fbase != NULL);
    assert(dladdr((void *)(uintptr_t)1, &info) == 0);
    return 0;
}
''')
    subprocess.run(['clang-18', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
                    '-I' + str(work), HERE / 'static_runtime.c', work / 'check.c',
                    work / 'libjava.a', '-pthread',
                    '-Wl,--defsym=__cosmic_image_start=__executable_start',
                    '-Wl,--defsym=__cosmic_image_end=_end', '-o', work / 'check'], check=True)
    subprocess.run([work / 'check'], check=True, timeout=10)
print('PASS: generated native lookup, callable function, errors, thread isolation and image bounds')
