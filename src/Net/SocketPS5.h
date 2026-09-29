#pragma once

#include "NetConstants.h"

namespace ms
{
	// sceNet handles are distinct from kernel file descriptors.
	class SocketPS5
	{
	public:
		SocketPS5() = default;
		~SocketPS5();
		SocketPS5(const SocketPS5&) = delete;
		SocketPS5& operator=(const SocketPS5&) = delete;
		bool open(const char* address, const char* port);
		bool close();
		size_t receive(bool* connected);
		const int8_t* get_buffer() const { return buffer; }
		bool dispatch(const int8_t* bytes, size_t length);

	private:
		int socket = -1;
		int8_t buffer[MAX_PACKET_LENGTH];
	};
}
