#include "SocketPS5.h"

#ifdef PLATFORM_PS5
#include "../MapleStory.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <charconv>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstring>

namespace
{
	// Native networking ABI also used by ProsperoLight's ps5_sockets.c.
	struct Address {
		uint8_t length, family;
		uint16_t port;
		uint32_t address;
		uint8_t padding[8];
	};
	struct Event { uint32_t events, padding; uint64_t ident, data; };
	static_assert(sizeof(Address) == 16 && sizeof(Event) == 24);
	constexpr int SOL_SOCKET_NATIVE = 0xffff, SO_NBIO_NATIVE = 0x1200, SO_ERROR_NATIVE = 0x1007;
	constexpr int WOULD_BLOCK = 35, IN_PROGRESS = 36, ALREADY = 37, INTERRUPTED = 4;
	constexpr uint32_t READABLE = 1, WRITABLE = 2;
	using Clock = std::chrono::steady_clock;

	extern "C" {
		int sceNetSocket(const char*, int, int, int);
		int sceNetSocketClose(int);
		int sceNetConnect(int, const void*, uint32_t);
		int sceNetRecv(int, void*, size_t, int);
		int sceNetSend(int, const void*, size_t, int);
		int sceNetSetsockopt(int, int, int, const void*, uint32_t);
		int sceNetGetsockopt(int, int, int, void*, uint32_t*);
		int* sceNetErrnoLoc();
		int sceNetEpollCreate(const char*, int);
		int sceNetEpollControl(int, int, int, Event*);
		int sceNetEpollWait(int, Event*, int, int);
		int sceNetEpollDestroy(int);
#ifdef OPENSTORY_LAN_LOG
		void openstory_diagnostics_phase(const char*, std::uintptr_t);
#endif
	}

	int network_error() {
		const int* error = sceNetErrnoLoc();
		return error ? *error : 5;
	}
	void phase(const char* name, int error = 0) {
#ifdef OPENSTORY_LAN_LOG
		openstory_diagnostics_phase(name, static_cast<uint32_t>(error));
#else
		(void)name; (void)error;
#endif
	}

	bool wait_socket(int socket, uint32_t events, Clock::time_point deadline) {
		const int poll = sceNetEpollCreate("OpenStory", 0);
		if (poll < 0) { phase("socket-poll-create-failed", network_error()); return false; }
		Event event{events, 0, 0, 0};
		if (sceNetEpollControl(poll, 1, socket, &event) < 0) {
			phase("socket-poll-register-failed", network_error());
			sceNetEpollDestroy(poll);
			return false;
		}
		int result = 0;
		for (;;) {
			const auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline - Clock::now()).count();
			if (remaining <= 0) break;
			result = sceNetEpollWait(poll, &event, 1, static_cast<int>(remaining > INT_MAX ? INT_MAX : remaining));
			if (result >= 0 || network_error() != INTERRUPTED) break;
		}
		if (result < 0) phase("socket-poll-failed", network_error());
		else if (!result) phase("socket-timeout");
		sceNetEpollDestroy(poll);
		return result > 0;
	}
}

namespace ms
{
	SocketPS5::~SocketPS5() { close(); }

	bool SocketPS5::close() {
		const int previous = socket;
		socket = -1;
		return previous < 0 || sceNetSocketClose(previous) >= 0;
	}

	bool SocketPS5::open(const char* host, const char* service) {
		close();
		if (!host || !service) return false;
		unsigned port = 0;
		const auto parsed = std::from_chars(service, service + std::strlen(service), port);
		Address address{};
		if (parsed.ec != std::errc{} || *parsed.ptr || !port || port > 65535
			|| inet_pton(AF_INET, host, &address.address) != 1) return false;
		address.length = sizeof(address);
		address.family = 2;
		address.port = htons(static_cast<uint16_t>(port));
		std::fprintf(stderr, "[Network] sceNet connecting to %s:%u\n", host, port);
		auto fail = [&](const char* name, int error) {
			phase(name, error);
			close();
			return false;
		};
		socket = sceNetSocket("OpenStory", 2, 1, 6);
		if (socket < 0) return fail("socket-open-failed", network_error());
		const int nonblocking = 1;
		if (sceNetSetsockopt(socket, SOL_SOCKET_NATIVE, SO_NBIO_NATIVE, &nonblocking, sizeof(nonblocking)) < 0)
			return fail("socket-nonblocking-failed", network_error());
		const auto deadline = Clock::now() + std::chrono::seconds(8);
		phase("socket-connect");
		if (sceNetConnect(socket, &address, sizeof(address)) < 0) {
			const int error = network_error();
			if (error != IN_PROGRESS && error != WOULD_BLOCK && error != ALREADY)
				return fail("socket-connect-failed", error);
			if (!wait_socket(socket, WRITABLE, deadline)) return fail("socket-connect-wait-failed", 0);
			int pending = 0;
			uint32_t size = sizeof(pending);
			if (sceNetGetsockopt(socket, SOL_SOCKET_NATIVE, SO_ERROR_NATIVE, &pending, &size) < 0)
				return fail("socket-status-failed", network_error());
			if (pending) return fail("socket-connect-failed", pending);
		}
#ifdef USE_CRYPTO
		constexpr size_t handshake_length = 16;
#else
		constexpr size_t handshake_length = 2;
#endif
		phase("socket-handshake");
		for (size_t received = 0; received < handshake_length;) {
			const int count = sceNetRecv(socket, buffer + received, handshake_length - received, 0);
			if (count > 0) received += static_cast<size_t>(count);
			else if (!count) return fail("socket-handshake-eof", 0);
			else {
				const int error = network_error();
				if ((error != WOULD_BLOCK && error != INTERRUPTED) || !wait_socket(socket, READABLE, deadline))
					return fail("socket-handshake-failed", error);
			}
		}
		phase("socket-ready");
		return true;
	}

	size_t SocketPS5::receive(bool* connected) {
		*connected = socket >= 0;
		if (!*connected) return 0;
		const int count = sceNetRecv(socket, buffer, sizeof(buffer), 0);
		if (count > 0) return static_cast<size_t>(count);
		if (count < 0) {
			const int error = network_error();
			if (error == WOULD_BLOCK || error == INTERRUPTED) return 0;
			phase("socket-receive-failed", error);
		}
		*connected = false;
		close();
		return 0;
	}

	bool SocketPS5::dispatch(const int8_t* bytes, size_t length) {
		if (socket < 0) return false;
		const auto deadline = Clock::now() + std::chrono::seconds(8);
		while (length) {
			const int count = sceNetSend(socket, bytes, length, 0);
			if (count > 0) { bytes += count; length -= static_cast<size_t>(count); }
			else if (count < 0) {
				const int error = network_error();
				if ((error == WOULD_BLOCK || error == INTERRUPTED) && wait_socket(socket, WRITABLE, deadline)) continue;
				phase("socket-send-failed", error);
				close();
				return false;
			} else { close(); return false; }
		}
		return true;
	}
}
#endif
