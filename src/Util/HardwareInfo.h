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
#pragma once

#include "../Configuration.h"

#ifdef _WIN32
#include <Windows.h>
#include <IPHlpApi.h>
#else
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <cstdint>
#endif

namespace ms
{
	class HardwareInfo
	{
	public:
#ifdef _WIN32
		HardwareInfo()
		{
			size_t size = 18;

			// Hard Drive VolumeSerialNumber
			char* volumeSerialNumber = (char*)malloc(size);

			TCHAR szVolume[MAX_PATH + 1];
			TCHAR szFileSystem[MAX_PATH + 1];

			DWORD dwSerialNumber, dwMaxLen, dwSystemFlags;

			TCHAR szDrives[MAX_PATH + 1];
			DWORD dwLen = GetLogicalDriveStrings(MAX_PATH, szDrives);
			TCHAR* pLetter = szDrives;

			BOOL bSuccess;

			bSuccess = GetVolumeInformation(pLetter, szVolume, MAX_PATH, &dwSerialNumber, &dwMaxLen, &dwSystemFlags, szFileSystem, MAX_PATH);

			if (bSuccess)
			{
				sprintf_s(volumeSerialNumber, size, "%X%X", HIWORD(dwSerialNumber), LOWORD(dwSerialNumber));
			}
			else
			{
				printf("Cannot retrieve Volume information for %s\n", pLetter);
				free(volumeSerialNumber);
				return;
			}

			// HWID/MACS
			PIP_ADAPTER_INFO AdapterInfo;
			DWORD dwBufLen = sizeof(IP_ADAPTER_INFO);
			char* hwid = (char*)malloc(size);
			char* macs = (char*)malloc(size);

			AdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));

			if (AdapterInfo == NULL)
			{
				printf("Error allocating memory needed to call GetAdaptersinfo\n");
				free(volumeSerialNumber);
				free(hwid);
				free(macs);
				return;
			}

			// Make an initial call to GetAdaptersInfo to get the necessary size into the dwBufLen variable
			if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW)
			{
				free(AdapterInfo);
				AdapterInfo = (IP_ADAPTER_INFO*)malloc(dwBufLen);

				if (AdapterInfo == NULL)
				{
					printf("Error allocating memory needed to call GetAdaptersinfo\n");
					free(volumeSerialNumber);
					free(hwid);
					free(macs);
					return;
				}
			}

			if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR)
			{
				// Contains pointer to current adapter info
				PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;

				// Technically should look at pAdapterInfo->AddressLength and not assume it is 6
				sprintf_s(hwid, size, "%02X%02X%02X%02X%02X%02X",
					pAdapterInfo->Address[0], pAdapterInfo->Address[1],
					pAdapterInfo->Address[2], pAdapterInfo->Address[3],
					pAdapterInfo->Address[4], pAdapterInfo->Address[5]);

				Configuration::get().set_hwid(hwid, volumeSerialNumber);

				pAdapterInfo = pAdapterInfo->Next;

				// Technically should look at pAdapterInfo->AddressLength and not assume it is 6
				sprintf_s(macs, size, "%02X-%02X-%02X-%02X-%02X-%02X",
					pAdapterInfo->Address[0], pAdapterInfo->Address[1],
					pAdapterInfo->Address[2], pAdapterInfo->Address[3],
					pAdapterInfo->Address[4], pAdapterInfo->Address[5]);

				Configuration::get().set_macs(macs);
			}

			free(AdapterInfo);
			free(volumeSerialNumber);
			free(hwid);
			free(macs);
		}
#else
		// macOS / POSIX. Produces exactly the same string formats the Windows
		// path puts on the wire, because the v83 login packet carries them:
		//   hwid   "%02X%02X%02X%02X%02X%02X"   (6 MAC bytes, no separator)
		//   macs   "%02X-%02X-%02X-%02X-%02X-%02X"
		//   serial 8 uppercase hex digits (Configuration::set_hwid slices it
		//          into four 2-char parts, so it must be at least 8 long)
		HardwareInfo()
		{
			uint8_t mac[6] = { 0, 0, 0, 0, 0, 0 };

			if (!primary_mac(mac))
			{
				// Never hand the server an empty MAC — it is used for ban
				// tracking and an empty value may be rejected. Fall back to a
				// stable synthetic address in the locally-administered range
				// (0x02 prefix) derived from the host identifier.
				uint32_t h = host_hash();

				mac[0] = 0x02;
				mac[1] = 0x00;
				mac[2] = static_cast<uint8_t>((h >> 24) & 0xFF);
				mac[3] = static_cast<uint8_t>((h >> 16) & 0xFF);
				mac[4] = static_cast<uint8_t>((h >> 8) & 0xFF);
				mac[5] = static_cast<uint8_t>(h & 0xFF);
			}

			char hwid[18];
			snprintf(hwid, sizeof(hwid), "%02X%02X%02X%02X%02X%02X",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

			char macs[18];
			snprintf(macs, sizeof(macs), "%02X-%02X-%02X-%02X-%02X-%02X",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

			// Stand-in for the Windows volume serial: a stable 32-bit machine
			// identifier rendered as 8 hex digits.
			uint32_t serial = machine_serial(mac);

			char volumeSerialNumber[9];
			snprintf(volumeSerialNumber, sizeof(volumeSerialNumber), "%08X", serial);

			Configuration::get().set_hwid(hwid, volumeSerialNumber);
			Configuration::get().set_macs(macs);
		}

	private:
		// FNV-1a, 32 bit.
		static uint32_t fnv1a(const void* data, size_t len, uint32_t hash = 2166136261u)
		{
			const uint8_t* bytes = static_cast<const uint8_t*>(data);

			for (size_t i = 0; i < len; i++)
			{
				hash ^= bytes[i];
				hash *= 16777619u;
			}

			return hash;
		}

		// Host UUID is assigned by the OS and survives reboots. It can fail
		// (EPERM under a sandbox), in which case the caller falls back to the
		// MAC, which is also stable.
		static uint32_t host_hash()
		{
			unsigned char uuid[16] = { 0 };
			struct timespec wait = { 0, 0 };

			if (gethostuuid(uuid, &wait) == 0)
				return fnv1a(uuid, sizeof(uuid));

			return 0;
		}

		static uint32_t machine_serial(const uint8_t (&mac)[6])
		{
			uint32_t h = host_hash();

			if (h != 0)
				return h;

			return fnv1a(mac, 6);
		}

		// Picks the MAC of the primary interface: prefers en0 (the built-in
		// Ethernet/Wi-Fi port on every Mac), otherwise the first non-loopback
		// link-layer address with a 6-byte, non-zero hardware address.
		static bool primary_mac(uint8_t (&out)[6])
		{
			struct ifaddrs* ifaddr = nullptr;

			if (getifaddrs(&ifaddr) != 0 || ifaddr == nullptr)
				return false;

			bool found = false;

			for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
			{
				if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_LINK)
					continue;

				if (ifa->ifa_flags & IFF_LOOPBACK)
					continue;

				const struct sockaddr_dl* sdl =
					reinterpret_cast<const struct sockaddr_dl*>(ifa->ifa_addr);

				if (sdl->sdl_alen != 6)
					continue;

				const uint8_t* addr =
					reinterpret_cast<const uint8_t*>(LLADDR(const_cast<struct sockaddr_dl*>(sdl)));

				if (addr[0] == 0 && addr[1] == 0 && addr[2] == 0
					&& addr[3] == 0 && addr[4] == 0 && addr[5] == 0)
					continue;

				bool is_en0 = ifa->ifa_name != nullptr && strcmp(ifa->ifa_name, "en0") == 0;

				if (!found || is_en0)
				{
					memcpy(out, addr, 6);
					found = true;
				}

				if (is_en0)
					break;
			}

			freeifaddrs(ifaddr);

			return found;
		}
#endif
	};
}