// Ordinary native-title startup; uses the existing PS5Library runtime.
#include <cerrno>
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <exception>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int openstory_main(int, char**);
extern "C" int sceNetInit();
extern "C" int sceNetPoolCreate(const char*, int, int);
extern "C" int sceNetPoolDestroy(int);
extern "C" int __real_fcntl(int, int, ...);
#ifdef OPENSTORY_LAN_LOG
extern "C" void openstory_diagnostics_stdio();
extern "C" void openstory_diagnostics_network();
extern "C" void openstory_diagnostics_phase(const char*, std::uintptr_t);
extern "C" void openstory_diagnostics_checkpoint(const char*, unsigned);

struct pipe_screen;
extern "C" int __real_ps5_screen_prepare_present(pipe_screen*);
extern "C" int __wrap_ps5_screen_prepare_present(pipe_screen* screen)
{
    const int result = __real_ps5_screen_prepare_present(screen);
    if (result) openstory_diagnostics_phase("display-prepare-result", static_cast<uint32_t>(result));
    return result;
}
extern "C" int __real_ps5_agc_gate2_present(unsigned);
extern "C" int __wrap_ps5_agc_gate2_present(unsigned buffer)
{
    const int result = __real_ps5_agc_gate2_present(buffer);
    if (result) openstory_diagnostics_phase("display-present-result", static_cast<uint32_t>(result));
    return result;
}
extern "C" int __real_sceVideoOutSubmitFlip(int, int, uint32_t, int64_t);
extern "C" int __wrap_sceVideoOutSubmitFlip(int handle, int buffer, uint32_t mode, int64_t argument)
{
    const int result = __real_sceVideoOutSubmitFlip(handle, buffer, mode, argument);
    if (result) openstory_diagnostics_phase("video-submit-flip-result", static_cast<uint32_t>(result));
    return result;
}
extern "C" int __real_sceVideoOutWaitVblank(int);
extern "C" int __wrap_sceVideoOutWaitVblank(int handle)
{
    const int result = __real_sceVideoOutWaitVblank(handle);
    if (result) openstory_diagnostics_phase("video-wait-vblank-result", static_cast<uint32_t>(result));
    return result;
}
extern "C" int __real_sceVideoOutIsFlipPending(int);
extern "C" int __wrap_sceVideoOutIsFlipPending(int handle)
{
    const int result = __real_sceVideoOutIsFlipPending(handle);
    if (result < 0) openstory_diagnostics_phase("video-flip-pending-result", static_cast<uint32_t>(result));
    return result;
}
#endif

// The native socket layer does not implement Unix close-on-exec flags.
extern "C" int __wrap_fcntl(int fd, int command, ...)
{
    if (command == F_SETFD || command == F_SETFL || command == F_DUPFD) {
        va_list args;
        va_start(args, command);
        int flags = va_arg(args, int);
        va_end(args);
        if (command == F_SETFD && flags == FD_CLOEXEC) return 0;
        return __real_fcntl(fd, command, flags);
    }
    return __real_fcntl(fd, command);
}

// PS5Library records these optional USB calls faulting on its native 4.51 app.
extern "C" int __wrap_sceKeyboardInit() { errno = ENOSYS; return -1; }
extern "C" int __wrap_sceKeyboardOpen(int, int, int, void*) { errno = ENOSYS; return -1; }

int main(int argc, char** argv)
{
#ifdef OPENSTORY_LAN_LOG
    openstory_diagnostics_phase("main-entry", 0);
    openstory_diagnostics_stdio();
#endif
    mkdir("/download0/openstory", 0700);
#ifndef OPENSTORY_LAN_LOG
    const int log = open("/download0/openstory/startup.log", O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (log >= 0) { dup2(log, STDOUT_FILENO); dup2(log, STDERR_FILENO); close(log); }
#endif
    setvbuf(stdout, nullptr, _IONBF, 0);
    std::fprintf(stderr, "[OpenStory] Entered main; static constructors complete\n");
    int pool = -1;
    int result = 1;
    try {
        std::fprintf(stderr, "[OpenStory] Initializing network\n");
        int network = sceNetInit();
        std::fprintf(stderr, "[OpenStory] sceNetInit: %#x\n", network);
        if (network < 0) std::fprintf(stderr, "Network initialization: %#x\n", network);
        else pool = sceNetPoolCreate("OpenStory", 5 * 1024 * 1024, 0);
        std::fprintf(stderr, "[OpenStory] sceNetPoolCreate: %#x\n", pool);
        if (pool < 0) std::fprintf(stderr, "Network pool unavailable: %#x\n", pool);
        else {
#ifdef OPENSTORY_LAN_LOG
            openstory_diagnostics_network();
#endif
            result = openstory_main(argc, argv);
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr, "OpenStory: %s\n", error.what());
    }
    std::fprintf(stderr, "[OpenStory] Returning from main: %d\n", result);
#ifdef OPENSTORY_LAN_LOG
    openstory_diagnostics_phase("native-main-return", result);
    openstory_diagnostics_checkpoint("native-main-return", 0);
#endif
    // Session's socket is a process-lifetime singleton; the OS reclaims the pool.
    return result;
}
