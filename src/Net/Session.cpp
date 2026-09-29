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
#include "Session.h"

#include <algorithm>
#include <cstring>

#include "../Configuration.h"

namespace ms
{
	Session::Session()
	{
		connected = false;
		length = 0;
		pos = 0;
	}

	Session::~Session()
	{
		if (connected)
			socket.close();
	}

	bool Session::init(const char* host, const char* port)
	{
		// A new stream cannot continue a packet from the previous connection.
		length = pos = header_pos = 0;
		++connection_id;
		connected = socket.open(host, port);

		if (connected)
		{
			// Read keys necessary for communicating with the server
			cryptography = { socket.get_buffer() };
		}

		return connected;
	}

	Error Session::init()
	{
		std::string HOST = Setting<ServerIP>::get().load();
		std::string PORT = Setting<ServerPort>::get().load();

		if (!init(HOST.c_str(), PORT.c_str()))
			return Error::CONNECTION;

		return Error::NONE;
	}

	void Session::reconnect(const char* address, const char* port)
	{
		// Close the current connection and open a new one
		bool success = socket.close();

		if (success)
			init(address, port);
		else
			connected = false;
	}

	void Session::process(const int8_t* bytes, size_t available)
	{
		const auto stream = connection_id;
		while (available > 0 && connected && stream == connection_id)
		{
			if (length == 0)
			{
				const auto count = std::min(HEADER_LENGTH - header_pos, available);
				std::memcpy(header + header_pos, bytes, count);
				header_pos += count;
				bytes += count;
				available -= count;
				if (header_pos < HEADER_LENGTH)
					return;

				length = cryptography.check_length(header);
				header_pos = 0;
				// Every body must contain an opcode and fit the receive buffer.
				if (length < MIN_PACKET_LENGTH - HEADER_LENGTH || length > sizeof(buffer))
				{
					socket.close();
					connected = false;
					length = pos = 0;
					return;
				}
			}

			const auto count = std::min(length - pos, available);
			std::memcpy(buffer + pos, bytes, count);
			pos += count;
			bytes += count;
			available -= count;
			if (pos < length)
				return;

			const auto packet_length = length;
			cryptography.decrypt(buffer, packet_length);
			length = pos = 0;
			try
			{
				packetswitch.forward(buffer, packet_length);
			}
			catch (const PacketError&)
			{
			}
			catch (const std::exception&)
			{
				// Keep existing handler error isolation.
			}
			// A handler can reconnect (e.g. channel change). Never feed the old
			// stream's remaining bytes into that connection's fresh crypto state.
		}
	}

	void Session::write(int8_t* packet_bytes, size_t packet_length)
	{
		if (!connected)
			return;

		int8_t header[HEADER_LENGTH];
		cryptography.create_header(header, packet_length);
		cryptography.encrypt(packet_bytes, packet_length);

		socket.dispatch(header, HEADER_LENGTH);
		socket.dispatch(packet_bytes, packet_length);
	}

	void Session::read()
	{
		// TCP reads may split either the header or the body at any byte.
		size_t result = socket.receive(&connected);

		if (result > 0)
		{
			// Retrieve buffer from the socket and process it
			const int8_t* bytes = socket.get_buffer();
			process(bytes, result);
		}
	}

	void Session::reconnect()
	{
		std::string HOST = Setting<ServerIP>::get().load();
		std::string PORT = Setting<ServerPort>::get().load();

		reconnect(HOST.c_str(), PORT.c_str());
	}

	bool Session::is_connected() const
	{
		return connected;
	}
}