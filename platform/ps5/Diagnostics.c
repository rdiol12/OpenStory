// Ordinary app-level diagnostics, initialized before the native C runtime.
#define _GNU_SOURCE
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <ucontext.h>
#include <unistd.h>
#ifdef OPENSTORY_CONTENT_VERSION_HEADER
#include OPENSTORY_CONTENT_VERSION_HEADER
#endif

#ifndef OPENSTORY_LOG_PORT
#define OPENSTORY_LOG_PORT 9978
#endif
#ifndef OPENSTORY_LOG_FILE
#define OPENSTORY_LOG_FILE "/download0/openstory-startup.log"
#endif
#ifdef __FreeBSD__
extern ssize_t _write(int, const void *, size_t);
#define diagnostic_write _write
#else
#define diagnostic_write write
#endif

static int log_socket = -1, log_file = -1;
static unsigned char signal_stack[65536];
_Static_assert(ATOMIC_POINTER_LOCK_FREE == 2 && ATOMIC_INT_LOCK_FREE == 2,
               "Fault checkpoints require lock-free atomics");
static _Atomic(const char *) main_phase = "startup";
static atomic_uint main_frame, altstack_ready;
void __real__init_env(void *);
void __wrap__init_env(void *);

// Main thread only; phase must be a string literal. No per-frame I/O or allocation.
void openstory_diagnostics_checkpoint(const char *phase, unsigned frame)
{
    if (frame) atomic_store_explicit(&main_frame, frame, memory_order_relaxed);
    atomic_store_explicit(&main_phase, phase, memory_order_relaxed);
}

// Stack buffers and async-signal-safe I/O; no allocator or stdio in the handler.
static char *text(char *out, const char *value)
{
    while (*value) *out++ = *value++;
    return out;
}

static char *hex(char *out, uint64_t value)
{
    out = text(out, "0x");
    for (int shift = 60; shift >= 0; shift -= 4)
        *out++ = "0123456789abcdef"[(value >> shift) & 15];
    return out;
}

static void emit(const char *data, size_t length)
{
    if (log_file >= 0) (void)diagnostic_write(log_file, data, length);
    // ponytail: best-effort UDP; the local file preserves critical records if packets drop.
    if (log_socket >= 0) (void)send(log_socket, data, length, MSG_DONTWAIT);
}

void openstory_diagnostics_phase(const char *phase, uintptr_t address)
{
    char line[256], *out = text(line, "[OpenStory phase] ");
    for (unsigned i = 0; phase[i] && i < 160; ++i) *out++ = phase[i];
    out = text(out, " address=");
    out = hex(out, address);
    *out++ = '\n';
    emit(line, (size_t)(out - line));
}

static void fault(int number, siginfo_t *info, void *raw_context)
{
    char line[256], *out = text(line, "[OpenStory fault] signal=");
    out = hex(out, (unsigned)number);
    out = text(out, " code=");
    out = hex(out, info ? (unsigned)info->si_code : 0);
    *out++ = '\n';
    emit(line, (size_t)(out - line));

    // This is the last main-loop checkpoint, even if another thread faulted.
    out = text(line, "[OpenStory fault] main-phase=");
    const char *phase = atomic_load_explicit(&main_phase, memory_order_relaxed);
    for (unsigned i = 0; phase[i] && i < 80; ++i) *out++ = phase[i];
    out = text(out, " frame=");
    out = hex(out, atomic_load_explicit(&main_frame, memory_order_relaxed));
    out = text(out, " altstack-ready=");
    out = hex(out, atomic_load_explicit(&altstack_ready, memory_order_relaxed));
    out = text(out, " on-main-altstack=");
    out = hex(out, (uintptr_t)line - (uintptr_t)signal_stack < sizeof(signal_stack));
    *out++ = '\n';
    emit(line, (size_t)(out - line));

    // Emit the signal/checkpoint first so an unexpected context ABI cannot hide them.
    const ucontext_t *context = raw_context;
    uint64_t pc = 0, sp = 0;
    if (context) {
#ifdef __FreeBSD__
        pc = context->uc_mcontext.mc_rip;
        sp = context->uc_mcontext.mc_rsp;
#elif defined(__linux__) && defined(__x86_64__)
        pc = context->uc_mcontext.gregs[REG_RIP];
        sp = context->uc_mcontext.gregs[REG_RSP];
#endif
    }
    out = text(line, "[OpenStory fault] pc=");
    out = hex(out, pc);
    out = text(out, " sp=");
    out = hex(out, sp);
    out = text(out, " address=");
    out = hex(out, info ? (uintptr_t)info->si_addr : 0);
    out = text(out, " anchor=");
    out = hex(out, (uintptr_t)&__wrap__init_env);
    *out++ = '\n';
    emit(line, (size_t)(out - line));
    if (log_file >= 0) (void)fsync(log_file);
    // SA_RESETHAND restores the normal OS crash handling; do not swallow the fault.
    if (kill(getpid(), number) != 0) _exit(128 + number);
}

// Native apps may start with 0/1/2 closed; stdio redirection must not replace our log.
static int reserve_descriptor(int descriptor)
{
    if (descriptor >= 0 && descriptor < 3) {
        const int saved = fcntl(descriptor, F_DUPFD, 3);
        close(descriptor);
        return saved;
    }
    return descriptor;
}

// Called from main, once the runtime and C++ constructors have completed.
void openstory_diagnostics_stdio(void)
{
    openstory_diagnostics_phase("stdio-redirect", 0);
    if (log_file >= 0) {
        // Native dup2 returns -1. Let libc reopen its own streams instead.
        openstory_diagnostics_phase("stdout-reopen-ok", freopen(OPENSTORY_LOG_FILE, "a", stdout) != NULL);
        openstory_diagnostics_phase("stderr-reopen-ok", freopen(OPENSTORY_LOG_FILE, "a", stderr) != NULL);
    } else if (log_socket >= 0) {
        (void)dup2(log_socket, STDOUT_FILENO);
        (void)dup2(log_socket, STDERR_FILENO);
    }
    openstory_diagnostics_phase("stdout-unbuffer", 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    openstory_diagnostics_phase("stderr-unbuffer", 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    openstory_diagnostics_phase("stdio-ready", 0);
}

void __wrap__init_env(void *parameters)
{
    // Reuse the native-title tracer's open/_write path: no FILE*, parsing or allocation.
    // Direct diagnostics and libc streams have separate descriptors; all append.
    log_file = reserve_descriptor(open(OPENSTORY_LOG_FILE, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0644));
    const char opened[] = "[OpenStory diagnostics] entry-file; before runtime\n";
    emit(opened, sizeof(opened) - 1);

    stack_t stack = {0};
    stack.ss_sp = signal_stack;
    stack.ss_size = sizeof(signal_stack);
    struct sigaction action = {0};
    action.sa_sigaction = fault;
    action.sa_flags = SA_SIGINFO | SA_RESETHAND;
    if (sigaltstack(&stack, NULL) == 0) {
        atomic_store_explicit(&altstack_ready, 1, memory_order_relaxed);
        action.sa_flags |= SA_ONSTACK;
    }
    sigfillset(&action.sa_mask);
    const int signals[] = {SIGSEGV, SIGABRT, SIGBUS, SIGILL, SIGFPE, SIGSYS};
    for (unsigned i = 0; i < sizeof(signals) / sizeof(signals[0]); ++i)
        if (sigaction(signals[i], &action, NULL) < 0) {
            const char error[] = "[OpenStory diagnostics] signal handler unavailable\n";
            emit(error, sizeof(error) - 1);
        }

#ifndef OPENSTORY_CONTENT_VERSION
#define OPENSTORY_CONTENT_VERSION "development"
#endif
    const char entry[] = "[OpenStory diagnostics] content-version=" OPENSTORY_CONTENT_VERSION
                         "; entry; calling _init_env\n";
    emit(entry, sizeof(entry) - 1);
    __real__init_env(parameters);
    const char ready[] = "[OpenStory diagnostics] runtime-ready; before static constructors\n";
    emit(ready, sizeof(ready) - 1);
}

// Called only after the application's native network pool is ready.
void openstory_diagnostics_network(void)
{
    openstory_diagnostics_phase("network-log-enable", 0);
    struct sockaddr_in destination = {0};
#ifdef __FreeBSD__
    destination.sin_len = sizeof(destination);
#endif
    destination.sin_family = AF_INET;
    destination.sin_port = htons(OPENSTORY_LOG_PORT);
    destination.sin_addr.s_addr = htonl(OPENSTORY_LOG_ADDRESS);
    log_socket = reserve_descriptor(socket(AF_INET, SOCK_DGRAM, 0));
    if (log_socket >= 0 &&
        (fcntl(log_socket, F_SETFL, O_NONBLOCK) < 0 ||
         connect(log_socket, (struct sockaddr *)&destination, sizeof(destination)) < 0)) {
        close(log_socket);
        log_socket = -1;
    }
    openstory_diagnostics_phase("network-log-ready", 0);
}
