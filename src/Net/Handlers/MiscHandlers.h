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

#include "../PacketHandler.h"

namespace ms
{
	class NpcActionHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class YellowTipHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class BroadcastMsgHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class AdminResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class SetNpcScriptableHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class AutoHpPotHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class AutoMpPotHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class QuickSlotInitHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class ClaimStatusChangedHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class SueCharacterResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class SpouseChatHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class BlockedMapHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class BlockedServerHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class SetExtraPendantSlotHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class SkillLearnItemResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class MakerResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class CatchMonsterHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class CatchMonsterWithItemHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class NewYearCardHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};
}
