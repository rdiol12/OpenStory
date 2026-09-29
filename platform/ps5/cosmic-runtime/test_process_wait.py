"""Exercise the actual patched native wait method on the host, with PS5 selection."""
from pathlib import Path
import resource
import subprocess
import sys
from tempfile import TemporaryDirectory

work = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.home() / 'build/cosmic-ps5'
source = (work / 'openjdk21-bsd/src/java.base/unix/native/libjava/ProcessHandleImpl_unix.c').read_text()
start = source.index('JNIEXPORT jint JNICALL\nJava_java_lang_ProcessHandleImpl_waitForProcessExit0')
method = source[start:source.index('\n/*', start)]
prefix = r'''
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
typedef void JNIEnv;
typedef void *jclass;
typedef int jint;
typedef long long jlong;
typedef unsigned char jboolean;
#define JNIEXPORT
#define JNICALL
#define JNI_FALSE 0
#define java_lang_ProcessHandleImpl_NOT_A_CHILD -2
#define WTERMSIG_RETURN(status) (WTERMSIG(status) + 0x80)
'''
check = r'''
int main(void) {
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) _exit(37);
    assert(Java_java_lang_ProcessHandleImpl_waitForProcessExit0(NULL, NULL, child, 0) == -2);
    int status;
    assert(waitpid(child, &status, 0) == child); /* Non-reaping path must leave the status intact. */
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 37);
    child = fork();
    assert(child >= 0);
    if (child == 0) _exit(42);
    assert(Java_java_lang_ProcessHandleImpl_waitForProcessExit0(NULL, NULL, child, 1) == 42);
    assert(Java_java_lang_ProcessHandleImpl_waitForProcessExit0(NULL, NULL, child, 1) == -2);
    puts("PASS: polling fallback preserves exit status; reaping and absent child work (host only)");
}
'''
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
with TemporaryDirectory() as directory:
    native = Path(directory) / 'wait.c'
    native.write_text(prefix + method + check)
    executable = Path(directory) / 'wait-check'
    subprocess.run(['clang-18', '-D__PROSPERO__', native, '-o', executable], check=True)
    subprocess.run([executable], check=True)
