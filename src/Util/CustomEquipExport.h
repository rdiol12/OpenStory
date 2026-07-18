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

namespace ms
{
	// Template exporter for the AI-generated equip pipeline.
	//
	// If Custom/export.txt (next to the exe) exists, each line holds an equip item
	// id. For every id, all part bitmaps of its .img are dumped to
	// Custom/Export/<id>/<stance>.<frame>.<part>.png together with a frames.txt
	// listing dimensions, origins and anchor maps. Those PNGs are the img2img
	// templates: generate art over them (same canvas size!) and drop the results
	// into Custom/Equip/<id>/ under the SAME file names — the client then renders
	// them instead of the NX bitmaps, with vanilla-perfect alignment.
	namespace CustomEquipExport
	{
		// Run once at startup, after the NX files are loaded.
		void run();
	}
}
