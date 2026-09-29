//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
//	Copyright (C) 2015-2019  Daniel Allendorf, Ryan Payton						//
//																				//
//	This program is free software: you can redistribute it and/or modify		//
//	it under the terms of the GNU Affero General Public License as published by	//
//	the Free Software Foundation, either version 3 of the License, or			//
//	(at your option) any later version.											//
//																				//
//	This program is distributed in the hope that it will be useful,				//
//	but WITHOUT ANY WARRANTY; without even the implied warranty of				//
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the				//
//	GNU Affero General Public License for more details.							//
//																				//
//	You should have received a copy of the GNU Affero General Public License	//
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.		//
//////////////////////////////////////////////////////////////////////////////////
#include "SocketAsio.h"

#include <charconv>
#include <chrono>
#include <cstring>

#ifdef OPENSTORY_LAN_LOG
extern "C" void openstory_diagnostics_phase(const char*, std::uintptr_t);
static void network_phase(const char* phase, int error = 0) {
	openstory_diagnostics_phase(phase, static_cast<std::uintptr_t>(error));
}
#else
static void network_phase(const char*, int = 0) {}
#endif

#ifdef USE_ASIO
namespace ms
{
	SocketAsio::SocketAsio() : resolver(iocontext), socket(iocontext) {
		network_phase("socket-created");
	}

	SocketAsio::~SocketAsio()
	{
		if (socket.is_open())
		{
			error_code error;
			socket.close(error);
		}
	}

	bool SocketAsio::open(const char* address, const char* port)
	{
		error_code error;
		if (socket.is_open()) socket.close(error);
		iocontext.restart();
		unsigned port_number = 0;
		const auto parsed = std::from_chars(port, port + std::strlen(port), port_number);
		if (parsed.ec != std::errc{} || *parsed.ptr || !port_number || port_number > 65535)
			return false;
		const auto numeric_address = asio::ip::make_address(address, error);
#ifdef PLATFORM_PS5
		// ServerIP is numeric on the console; avoid host/service database lookups.
		if (error) {
			network_phase("socket-address-invalid", error.value());
			return false;
		}
#endif
		bool ready = false;
		asio::steady_timer deadline(iocontext, std::chrono::seconds(8));
		deadline.async_wait([&](const error_code& expired) {
			if (expired) return;
			network_phase("socket-timeout");
			resolver.cancel();
			error_code ignored;
			socket.close(ignored);
		});
		auto connected = [&](const error_code& failure) {
			if (failure) {
				network_phase("socket-connect-failed", failure.value());
				deadline.cancel();
				return;
			}
			network_phase("socket-handshake");
			// TCP may split a handshake or coalesce it with the next packet.
			asio::async_read(socket, asio::buffer(buffer, HANDSHAKE_LEN),
				[&](const error_code& failure, size_t length) {
					ready = !failure && length == HANDSHAKE_LEN;
					network_phase(ready ? "socket-ready" : "socket-handshake-failed", failure.value());
					deadline.cancel();
				});
		};
		if (!error) {
			network_phase("socket-connect");
			socket.async_connect(tcp::endpoint(numeric_address, static_cast<unsigned short>(port_number)), connected);
		} else {
			network_phase("socket-resolve");
			resolver.async_resolve(address, port, [&](const error_code& failure, tcp::resolver::results_type endpoints) {
				if (failure) { connected(failure); return; }
				network_phase("socket-connect");
				asio::async_connect(socket, endpoints,
					[&](const error_code& failure, const tcp::endpoint&) { connected(failure); });
			});
		}
		iocontext.run();
		if (!ready && socket.is_open()) socket.close(error);
		return ready && socket.is_open();
	}

	bool SocketAsio::close()
	{
		error_code error;
		socket.shutdown(tcp::socket::shutdown_both, error);
		socket.close(error);

		return !error;
	}

	size_t SocketAsio::receive(bool* recvok)
	{
		if (socket.available() > 0)
		{
			error_code error;
			size_t result = socket.read_some(asio::buffer(buffer), error);
			*recvok = !error;

			return result;
		}

		return 0;
	}

	const int8_t* SocketAsio::get_buffer() const
	{
		return buffer;
	}

	bool SocketAsio::dispatch(const int8_t* bytes, size_t length)
	{
		error_code error;
		size_t result = asio::write(socket, asio::buffer(bytes, length), error);

		return !error && (result == length);
	}
}
#endif
