// Host-only adapter for ordinary sceNet networking. Raw kernel sockets stay denied.
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <map>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

struct NetEvent {
    uint32_t events, pad;
    uint64_t ident, data;
};
static_assert(sizeof(NetEvent) == 24);

static thread_local int net_errno;
static std::map<std::pair<int, int>, NetEvent> registrations;

static int native_error(int error) {
    switch (error) {
        case EAGAIN: return 35;
        case EINPROGRESS: return 36;
        case EALREADY: return 37;
        case ECONNRESET: return 54;
        case ENOTCONN: return 57;
        case ETIMEDOUT: return 60;
        case ECONNREFUSED: return 61;
        default: return error;
    }
}

static int result(int value) {
    if (value < 0) net_errno = native_error(errno);
    return value;
}

extern "C" {
int __real_socket(int, int, int);
int __wrap_socket(int, int, int) {
    errno = EACCES;
    return -1;
}

void openstory_diagnostics_phase(const char* phase, uintptr_t error) {
    std::fprintf(stderr, "%s: %zu\n", phase, static_cast<size_t>(error));
}

int* sceNetErrnoLoc() { return &net_errno; }
int sceNetSocket(const char*, int domain, int type, int protocol) {
    assert(domain == 2 && type == 1 && protocol == 6);
    return result(__real_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
}
int sceNetSocketClose(int descriptor) { return result(close(descriptor)); }
int sceNetShutdown(int descriptor, int how) { return result(shutdown(descriptor, how)); }

int sceNetConnect(int descriptor, const void* address, uint32_t length) {
    const auto* bytes = static_cast<const uint8_t*>(address);
    assert(length == 16 && bytes[0] == 16 && bytes[1] == 2);
    const uint8_t loopback[] = {127, 0, 0, 1};
    assert(std::memcmp(bytes + 4, loopback, 4) == 0); // Never contact external hosts.
    sockaddr_in host{};
    host.sin_family = AF_INET;
    std::memcpy(&host.sin_port, bytes + 2, 2);
    std::memcpy(&host.sin_addr, bytes + 4, 4);
    return result(connect(descriptor, reinterpret_cast<sockaddr*>(&host), sizeof(host)));
}

int sceNetSetsockopt(int descriptor, int level, int option, const void* value, uint32_t length) {
    assert(level == 0xffff && option == 0x1200 && length == sizeof(int));
    int flags = fcntl(descriptor, F_GETFL);
    if (flags < 0) return result(flags);
    flags = *static_cast<const int*>(value) ? flags | O_NONBLOCK : flags & ~O_NONBLOCK;
    return result(fcntl(descriptor, F_SETFL, flags));
}

int sceNetGetsockopt(int descriptor, int level, int option, void* value, uint32_t* length) {
    assert(level == 0xffff && option == 0x1007 && *length == sizeof(int));
    socklen_t host_length = *length;
    int status = getsockopt(descriptor, SOL_SOCKET, SO_ERROR, value, &host_length);
    if (status == 0) *static_cast<int*>(value) = native_error(*static_cast<int*>(value));
    *length = host_length;
    return result(status);
}

int sceNetRecv(int descriptor, void* buffer, size_t length, int flags) {
    return result(static_cast<int>(recv(descriptor, buffer, length, flags)));
}
int sceNetSend(int descriptor, const void* buffer, size_t length, int flags) {
    // Force short successful writes so a single-send dispatch cannot pass.
    return result(static_cast<int>(send(descriptor, buffer, std::min(length, size_t{4093}),
                                        flags | MSG_NOSIGNAL)));
}

int sceNetEpollCreate(const char*, int flags) {
    assert(flags == 0);
    return result(epoll_create1(0));
}
int sceNetEpollControl(int epoll, int operation, int descriptor, NetEvent* event) {
    assert(operation == 1); // This backend only registers one socket per wait.
    epoll_event host{};
    if (event->events & 1) host.events |= EPOLLIN;
    if (event->events & 2) host.events |= EPOLLOUT;
    host.data.fd = descriptor;
    int status = epoll_ctl(epoll, EPOLL_CTL_ADD, descriptor, &host);
    if (status == 0) registrations[{epoll, descriptor}] = *event;
    return result(status);
}
int sceNetEpollWait(int epoll, NetEvent* events, int maximum, int microseconds) {
    assert(maximum > 0);
    epoll_event host{};
    const int milliseconds = microseconds < 0 ? -1 :
        static_cast<int>((static_cast<int64_t>(microseconds) + 999) / 1000);
    int ready = epoll_wait(epoll, &host, 1, milliseconds);
    if (ready > 0) {
        const int descriptor = host.data.fd;
        *events = registrations.at({epoll, descriptor});
        events->ident = descriptor;
        events->events = 0;
        if (host.events & EPOLLIN) events->events |= 1;
        if (host.events & EPOLLOUT) events->events |= 2;
        if (host.events & EPOLLERR) events->events |= 8;
        if (host.events & EPOLLHUP) events->events |= 16;
    }
    return result(ready);
}
int sceNetEpollDestroy(int epoll) {
    for (auto entry = registrations.begin(); entry != registrations.end();) {
        if (entry->first.first == epoll) entry = registrations.erase(entry);
        else ++entry;
    }
    return result(close(epoll));
}
}
