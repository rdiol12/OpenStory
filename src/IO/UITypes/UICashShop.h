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

#include "../UIElement.h"

#include "../Components/Charset.h"
#include "../Components/Gauge.h"
#include "../Components/Slider.h"

#include "../../Character/Look/CharLook.h"
#include "../../Data/ItemData.h"
#include "../../Graphics/Text.h"

namespace ms
{
	class UICashShop : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::CASHSHOP;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = false;

		UICashShop();

			void draw(float inter) const;
		void update() override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		Button::State button_pressed(uint16_t buttonid);

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;

		UIElement::Type get_type() const override;

		void exit_cashshop();

	private:
		void update_items();

		// 5 columns x 3 rows of the classic card cells (119x184) in the
		// center panel, clear of the baked right-column panels
		static constexpr uint8_t GRID_COLS = 5u;
		static constexpr uint8_t GRID_ROWS = 3u;
		static constexpr uint8_t MAX_ITEMS = GRID_COLS * GRID_ROWS;
		static constexpr int16_t GRID_X = 140;
		static constexpr int16_t GRID_Y = 50;
		static constexpr int16_t STRIDE_X = 128;
		static constexpr int16_t STRIDE_Y = 210;
		static constexpr int16_t CARD_W = 119;
		static constexpr int16_t CARD_H = 184;

		class Item
		{
		public:
			enum Label : uint8_t
			{
				ACTION,
				BOMB_SALE,
				BONUS,
				EVENT = 4,
				HOT,
				LIMITED,
				LIMITED_BRONZE,
				LIMITED_GOLD,
				LIMITED_SILVER,
				LUNA_CRYSTAL,
				MASTER = 12,
				MUST,
				NEW,
				SALE = 17,
				SPEICAL,
				SPECIAL_PRICE,
				TIME,
				TODAY,
				WEEKLY,
				WONDER_BERRY,
				WORLD_SALE,
				NONE
			};

			// price is the NX cash price from Commodity.img, not the
			// item's meso shop price
			Item(int32_t itemid, int32_t sn, Label label, int32_t price, uint16_t count) : sn(sn), label(label), price(price), discount_price(0), count(count), data(ItemData::get(itemid)) {}

			int32_t sn;
			Label label;
			int32_t price;
			int32_t discount_price;
			uint16_t count;

			void draw(const DrawArgument& args) const
			{
				data.get_icon(false).draw(args);
			}

			const std::string get_name() const
			{
				return data.get_name();
			}

			const std::string& get_desc() const
			{
				return data.get_desc();
			}

			int32_t get_itemid() const
			{
				return data.get_id();
			}

			const int32_t get_price() const
			{
				return price;
			}

		private:
			const ItemData& data;
		};

		enum Buttons : uint16_t
		{
			BtPreview1,
			BtPreview2,
			BtPreview3,
			BtExit,
			BtChargeNX,
			BtChargeRefresh,
			BtMileage,
			BtHelp,
			BtCoupon,
			BtNext,
			BtPrev,
			BtDetailPackage,
			BtNonGrade,
			BtBuyAvatar,
			BtDefaultAvatar,
			BtSaveAvatar,
			BtTakeoffAvatar,
			BtBuy
		};

		Point<int16_t> BestNew_dim;

		Sprite preview_sprites[3];
		uint8_t preview_index;

		Sprite menu_tabs[9];
		uint8_t menu_index;

		Text job_label;
		Text name_label;
		mutable Text cash_balance_text[3];

		int16_t selected_item;
		CharLook preview_look;

		// Controllable stage character
		float char_x = 820.0f;
		float char_yoff = 0.0f;
		float char_vy = 0.0f;
		bool char_jumping = false;
		bool key_left = false;
		bool key_right = false;
		bool facing_right = true;
		uint8_t cur_stance = 0;

		Texture preview_scene[3];
		mutable Text preview_name;
		mutable Text preview_desc;
		mutable Text preview_price;

		std::vector<Sprite> promotion_sprites;
		Point<int16_t> promotion_pos;
		int8_t promotion_index;

		Sprite mvp_sprites[7];
		Point<int16_t> mvp_pos;
		uint8_t mvp_grade;
		Gauge mvp_gauge;
		float_t mvp_exp;

		Charset charge_charset;

		Sprite item_base;
		Sprite item_line;
		Sprite item_none;
		std::vector<Sprite> item_labels;
		std::vector<Item> items;
		Text item_name[MAX_ITEMS];
		Text item_price[MAX_ITEMS];
		Text item_discount[MAX_ITEMS];
		Text item_percent[MAX_ITEMS];

		Slider list_slider;
		int16_t list_offset;

		// Sub-panel backgrounds
		Texture wishlist_bg;
		Texture gift_bg;
		Texture coupon_bg;
		Texture search_bg;
		Texture cs_inventory_bg;
		Texture purchase_bg;

		// Active sub-panel tracking
		enum SubPanel : uint8_t
		{
			SUBPANEL_NONE,
			SUBPANEL_WISHLIST,
			SUBPANEL_GIFT,
			SUBPANEL_COUPON,
			SUBPANEL_SEARCH,
			SUBPANEL_INVENTORY,
			SUBPANEL_PURCHASE
		};
		SubPanel active_subpanel;
	};
}