#!/usr/bin/env python3
"""Exercise LAN startup capture and a real signal in a disposable Linux process."""
from pathlib import Path
import resource
import signal
import socket
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'platform/ps5'))
from startup import instrument

with tempfile.TemporaryDirectory() as directory:
    work = Path(directory)
    (work / 'version.h').write_text('#define OPENSTORY_CONTENT_VERSION "01.000.017"\n')
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reserve:
        reserve.bind(('127.0.0.1', 0))
        port = reserve.getsockname()[1]
    harness = work / 'harness.c'
    harness.write_text('''#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <errno.h>
void __wrap__init_env(void *);
void openstory_diagnostics_stdio(void);
void openstory_diagnostics_network(void);
void openstory_diagnostics_checkpoint(const char *, unsigned);
void openstory_diagnostics_phase(const char *, uintptr_t);
void __real__init_env(void *p) { if (p) raise(SIGABRT); }
/* Match the native console: dup2 cannot redirect the standard streams. */
int __wrap_dup2(int oldfd, int newfd) {
    (void)oldfd; (void)newfd; errno = ENOSYS; return -1;
}
void *worker(void *p) { (void)p; raise(SIGABRT); return 0; }
int main(int argc, char **argv) {
    if (argc > 1 && argv[1][0] == 'z') { close(0); close(1); close(2); }
    __wrap__init_env(argc > 1 && argv[1][0] == 'e' ? (void *)1 : 0);
    openstory_diagnostics_stdio();
    openstory_diagnostics_network();
    if (argc > 1 && argv[1][0] == 'd') {
        close(1); close(2);
        openstory_diagnostics_phase("exit-without-stdio", 7);
        return 0;
    }
    puts("startup-before-crash");
    fflush(stdout);
    openstory_diagnostics_checkpoint("frame-start", 42);
    openstory_diagnostics_checkpoint("buffer-swap", 0);
    if (argc > 1 && argv[1][0] == 'w') {
        pthread_t thread;
        if (pthread_create(&thread, 0, worker, 0)) return 1;
        pthread_join(thread, 0);
    }
    if (argc > 1) raise(SIGABRT);
    return 0;
}
''')
    subprocess.run(['cc', '-Wall', '-Wextra', '-Werror', '-g', '-pthread',
                    '-DOPENSTORY_LOG_ADDRESS=0x7f000001', '-Wl,--wrap=dup2',
                    f'-DOPENSTORY_CONTENT_VERSION_HEADER="{work}/version.h"',
                    f'-DOPENSTORY_LOG_PORT={port}',
                    f'-DOPENSTORY_LOG_FILE="{work}/startup.log"',
                    str(root / 'platform/ps5/Diagnostics.c'), str(harness),
                    '-o', str(work / 'sender')], check=True)
    receiver = subprocess.Popen([sys.executable, '-u', str(root / 'platform/ps5/receive-log.py'),
                                 '--bind', '127.0.0.1', '--ps5', '127.0.0.1',
                                 '--port', str(port), '--seconds', '3',
                                 '--output', str(work / 'lan.log')],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        assert receiver.stdout.readline().startswith('Listening'), 'receiver failed to start'
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as stranger:
            stranger.bind(('127.0.0.2', 0))
            stranger.sendto(b'wrong-console\n', ('127.0.0.1', port))
        subprocess.run([str(work / 'sender')], check=True, timeout=2)
        assert 'content-version=01.000.017' in (work / 'startup.log').read_text()
        assert 'startup-before-crash' in (work / 'startup.log').read_text(), 'stdout was lost from the local fallback'
        subprocess.run([str(work / 'sender'), 'direct'], check=True, timeout=2)
        assert 'exit-without-stdio address=0x0000000000000007' in (work / 'startup.log').read_text()
        def no_core():
            resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
        crash = subprocess.run([str(work / 'sender'), 'crash'], timeout=2, preexec_fn=no_core)
        assert crash.returncode == -signal.SIGABRT, crash.returncode
        log = (work / 'startup.log').read_text()
        assert 'main-phase=buffer-swap frame=0x000000000000002a' in log, log
        assert 'altstack-ready=0x0000000000000001 on-main-altstack=0x0000000000000001' in log, log
        worker = subprocess.run([str(work / 'sender'), 'worker'], timeout=2, preexec_fn=no_core)
        assert worker.returncode == -signal.SIGABRT, worker.returncode
        log = (work / 'startup.log').read_text()
        assert 'main-phase=buffer-swap frame=0x000000000000002a' in log, log
        assert 'altstack-ready=0x0000000000000001 on-main-altstack=0x0000000000000000' in log, log
        early = subprocess.run([str(work / 'sender'), 'early'], timeout=2, preexec_fn=no_core)
        assert early.returncode == -signal.SIGABRT, early.returncode
        assert 'signal=0x0000000000000006' in (work / 'startup.log').read_text()
        closed = subprocess.run([str(work / 'sender'), 'zero-stdio'], timeout=2, preexec_fn=no_core)
        assert closed.returncode == -signal.SIGABRT, closed.returncode
        receiver.communicate(timeout=5)
        assert receiver.returncode == 0
        log = (work / 'lan.log').read_text()
        assert 'signal=0x0000000000000006' in log, log
        assert 'pc=0x' in log and 'anchor=0x' in log, log
        assert log.count('[OpenStory fault] signal=') == 3, 'a post-network fault was not captured: ' + log
        assert 'wrong-console' not in log
        assert 'signal=0x0000000000000006' in (work / 'startup.log').read_text()
        assert 'startup-before-crash' in (work / 'startup.log').read_text()
    finally:
        if receiver.poll() is None:
            receiver.kill()
            receiver.wait()
    # Execute the instrumented, reused CRT loop with one good and one failing constructor.
    crt = root.parent / 'PS5Library/upstream/ProsperoTV/tooling/native/app_crt.cpp'
    traced = instrument(crt.read_text())
    try:
        instrument(traced)
        raise AssertionError('already instrumented startup should be rejected')
    except ValueError:
        pass
    subprocess.run(['cc', '-Wall', '-Wextra', '-Werror', '-g',
                    '-DOPENSTORY_LOG_ADDRESS=0x7f000001',
                    f'-DOPENSTORY_LOG_FILE="{work}/startup.log"',
                    '-c', str(root / 'platform/ps5/Diagnostics.c'),
                    '-o', str(work / 'diagnostics.o')], check=True)
    constructors = work / 'constructors.cpp'
    constructors.write_text(traced + '''
#include <csignal>
extern "C" void __wrap__init_env(void *);
extern "C" void __real__init_env(void *) {}
extern "C" void _init_env(void *p) { __wrap__init_env(p); }
void good() {}
void bad() { raise(SIGABRT); }
int main() {
    __wrap__init_env(nullptr);
    Initializer callbacks[] = {good, bad};
    run_forward(callbacks, callbacks + 2);
}
''')
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-D_start=test_start', '-D_init=test_init', '-D_fini=test_fini',
                    str(constructors), str(work / 'diagnostics.o'),
                    '-o', str(work / 'constructors')], check=True)
    crash = subprocess.run([str(work / 'constructors')], timeout=2, preexec_fn=no_core)
    assert crash.returncode == -signal.SIGABRT, crash.returncode
    log = (work / 'startup.log').read_text()
    assert log.count('constructor-begin') == 2 and log.count('constructor-end') == 1, log
    assert log.index('constructor-end') < log.rindex('constructor-begin') < log.index('[OpenStory fault]'), log
    # The production display probes must preserve API results and arguments.
    probes = work / 'display.cpp'
    probes.write_text(r'''
#include <cassert>
#include <cstdint>
#include <cstring>
struct pipe_screen;
static int result;
static const char *label;
static std::uintptr_t value;
extern "C" void openstory_diagnostics_phase(const char *p, std::uintptr_t v) { label=p; value=v; }
extern "C" int __real_ps5_screen_prepare_present(pipe_screen *p) { assert(!p); return result; }
extern "C" int __real_ps5_agc_gate2_present(unsigned b) { assert(b==1); return result; }
extern "C" int __real_sceVideoOutSubmitFlip(int h,int b,uint32_t m,int64_t a) { assert(h==4 && b==1 && m==1 && a==42); return result; }
extern "C" int __real_sceVideoOutWaitVblank(int h) { assert(h==4); return result; }
extern "C" int __real_sceVideoOutIsFlipPending(int h) { assert(h==4); return result; }
extern "C" int __wrap_ps5_screen_prepare_present(pipe_screen*);
extern "C" int __wrap_ps5_agc_gate2_present(unsigned);
extern "C" int __wrap_sceVideoOutSubmitFlip(int,int,uint32_t,int64_t);
extern "C" int __wrap_sceVideoOutWaitVblank(int);
extern "C" int __wrap_sceVideoOutIsFlipPending(int);
int main() {
    result=-7;
    assert(__wrap_ps5_screen_prepare_present(nullptr)==-7 && !strcmp(label,"display-prepare-result"));
    assert(__wrap_ps5_agc_gate2_present(1)==-7 && !strcmp(label,"display-present-result"));
    assert(__wrap_sceVideoOutSubmitFlip(4,1,1,42)==-7 && !strcmp(label,"video-submit-flip-result"));
    assert(__wrap_sceVideoOutWaitVblank(4)==-7 && !strcmp(label,"video-wait-vblank-result"));
    assert(__wrap_sceVideoOutIsFlipPending(4)==-7 && !strcmp(label,"video-flip-pending-result"));
    assert(value==static_cast<uint32_t>(-7));
    result=0; label=nullptr;
    assert(__wrap_ps5_screen_prepare_present(nullptr)==0 && !label);
    assert(__wrap_ps5_agc_gate2_present(1)==0 && !label);
    assert(__wrap_sceVideoOutSubmitFlip(4,1,1,42)==0 && !label);
    assert(__wrap_sceVideoOutWaitVblank(4)==0 && !label);
    result=1;
    assert(__wrap_sceVideoOutIsFlipPending(4)==1 && !label);
    assert(__wrap_sceVideoOutWaitVblank(4)==1 && !strcmp(label,"video-wait-vblank-result"));
}
''')
    subprocess.run(['c++', '-std=c++17', '-ffunction-sections', '-DOPENSTORY_LAN_LOG',
                    '-Dmain=unused_native_main', '-c', str(root / 'platform/ps5/Native.cpp'),
                    '-o', str(work / 'native.o')], check=True)
    subprocess.run(['c++', '-std=c++17', '-Wl,--gc-sections', str(probes),
                    str(work / 'native.o'), '-o', str(work / 'display')], check=True)
    subprocess.run([str(work / 'display')], check=True, timeout=2)
print('PASS: display probes preserve API arguments/results and log only unexpected results')
print('PASS: local stdout, early/constructor/main/worker faults, checkpoints, LAN source filter, original signal preserved')
