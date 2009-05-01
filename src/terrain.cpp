/* $Id$ */
/*
   Copyright (C) 2003 - 2009 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2
   or at your option any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "global.hpp"

#include "foreach.hpp"
#include "gettext.hpp"
#include "terrain.hpp"



terrain_type::terrain_type() :
		minimap_image_("void"),
		minimap_image_overlay_("void"),
		editor_image_("void"),
		id_(),
		name_(),
		description_(),
		number_(t_translation::VOID_TERRAIN),
		mvt_type_(1, t_translation::VOID_TERRAIN),
		def_type_(1, t_translation::VOID_TERRAIN),
		union_type_(1, t_translation::VOID_TERRAIN),
        height_adjust_(0),
		submerge_(0.0),
		light_modification_(0),
        heals_(0),
		income_description_(),
		income_description_ally_(),
		income_description_enemy_(),
		income_description_own_(),
		editor_group_(),
		village_(false),
		castle_(false),
		keep_(false),
		overlay_(false),
		combined_(false),
		editor_default_base_(t_translation::VOID_TERRAIN),
		hide_in_editor_(false)
{}

terrain_type::terrain_type(const config& cfg) :
		minimap_image_(cfg["symbol_image"]),
		minimap_image_overlay_("void"),
		editor_image_(cfg["editor_image"]),
		id_(cfg["id"]),
		name_(cfg["name"]),
		description_(cfg["description"]),
		number_(t_translation::read_terrain_code(cfg["string"])),
		mvt_type_(),
		def_type_(),
		union_type_(),
		height_adjust_(atoi(cfg["unit_height_adjust"].c_str())),
		submerge_(atof(cfg["submerge"].c_str())),
		light_modification_(atoi(cfg["light"].c_str())),
		heals_(lexical_cast_default<int>(cfg["heals"], 0)),
		income_description_(),
		income_description_ally_(),
		income_description_enemy_(),
		income_description_own_(),
		editor_group_(cfg["editor_group"]),
		village_(utils::string_bool(cfg["gives_income"])),
		castle_(utils::string_bool(cfg["recruit_onto"])),
		keep_(utils::string_bool(cfg["recruit_from"])),
		overlay_(number_.base == t_translation::NO_LAYER),
		combined_(false),
		editor_default_base_(t_translation::read_terrain_code(cfg["default_base"])),
		hide_in_editor_(utils::string_bool(cfg["hidden"], false))
{
/**
 *  @todo reenable these validations. The problem is that all MP
 *  scenarios/campaigns share the same namespace and one rogue scenario
 *  can avoid the player to create a MP game. So every scenario/campaign
 *  should get it's own namespace to be safe.
 */
#if 0
	VALIDATE(number_ != t_translation::NONE_TERRAIN,
		missing_mandatory_wml_key("terrain_type", "string"));
	VALIDATE(!minimap_image_.empty(),
		missing_mandatory_wml_key("terrain_type", "symbol_image", "string",
		t_translation::write_terrain_code(number_)));
	VALIDATE(!name_.empty(),
		missing_mandatory_wml_key("terrain_type", "name", "string",
		t_translation::write_terrain_code(number_)));
#endif

	if(editor_image_.empty()) {
		editor_image_ = minimap_image_;
	}

	mvt_type_.push_back(number_);
	def_type_.push_back(number_);
	const t_translation::t_list& alias = t_translation::read_list(cfg["aliasof"]);
	if(!alias.empty()) {
		mvt_type_ = alias;
		def_type_ = alias;
	}

	const t_translation::t_list& mvt_alias = t_translation::read_list(cfg["mvt_alias"]);
	if(!mvt_alias.empty()) {
		mvt_type_ = mvt_alias;
	}

	const t_translation::t_list& def_alias = t_translation::read_list(cfg["def_alias"]);
	if(!def_alias.empty()) {
		def_type_ = def_alias;
	}
	union_type_ = mvt_type_;
	union_type_.insert( union_type_.end(), def_type_.begin(), def_type_.end() );

	// remove + and -
	union_type_.erase(std::remove(union_type_.begin(), union_type_.end(),
				t_translation::MINUS), union_type_.end());

	union_type_.erase(std::remove(union_type_.begin(), union_type_.end(),
				t_translation::PLUS), union_type_.end());

	// remove doubles
	std::sort(union_type_.begin(),union_type_.end());
	union_type_.erase(std::unique(union_type_.begin(), union_type_.end()), union_type_.end());

#ifdef USE_TINY_GUI
	height_adjust_ /= 2;
#endif


	//mouse over message are only shown on villages
	if(village_) {
		income_description_ = cfg["income_description"];
		if(income_description_ == "") {
			income_description_ = _("Village");
		}

		income_description_ally_ = cfg["income_description_ally"];
		if(income_description_ally_ == "") {
			income_description_ally_ = _("Allied village");
		}

		income_description_enemy_ = cfg["income_description_enemy"];
		if(income_description_enemy_ == "") {
			income_description_enemy_ = _("Enemy village");
		}

		income_description_own_ = cfg["income_description_own"];
		if(income_description_own_ == "") {
			income_description_own_ = _("Owned village");
		}
	}
}

terrain_type::terrain_type(const terrain_type& base, const terrain_type& overlay) :
	minimap_image_(base.minimap_image_),
	minimap_image_overlay_(overlay.minimap_image_),
	editor_image_(overlay.editor_image_),
	id_(base.id_+"^"+overlay.id_),
	name_(overlay.name_),
	description_(overlay.description_),
	number_(t_translation::t_terrain(base.number_.base, overlay.number_.overlay)),
	mvt_type_(overlay.mvt_type_),
	def_type_(overlay.def_type_),
	union_type_(),
	height_adjust_(overlay.height_adjust_),
	submerge_(overlay.submerge_),
	light_modification_(base.light_modification_ + overlay.light_modification_),
	heals_(std::max<int>(base.heals_, overlay.heals_)),
	income_description_(),
	income_description_ally_(),
	income_description_enemy_(),
	income_description_own_(),
	editor_group_(),
	village_(base.village_ || overlay.village_),
	castle_(base.castle_ || overlay.castle_),
	keep_(base.keep_ || overlay.keep_),
	overlay_(false),
	combined_(true),
	editor_default_base_(),
	hide_in_editor_(base.hide_in_editor_ || overlay.hide_in_editor_)
{

	merge_alias_lists(mvt_type_, base.mvt_type_);
	merge_alias_lists(def_type_, base.def_type_);

	union_type_ = mvt_type_;
	union_type_.insert( union_type_.end(), def_type_.begin(), def_type_.end() );

	// remove + and -
	union_type_.erase(std::remove(union_type_.begin(), union_type_.end(),
				t_translation::MINUS), union_type_.end());

	union_type_.erase(std::remove(union_type_.begin(), union_type_.end(),
				t_translation::PLUS), union_type_.end());

	// remove doubles
	std::sort(union_type_.begin(),union_type_.end());
	union_type_.erase(std::unique(union_type_.begin(), union_type_.end()), union_type_.end());



	//mouse over message are only shown on villages
	if(base.village_) {
		income_description_ = base.income_description_;
		income_description_ally_ = base.income_description_ally_;
		income_description_enemy_ = base.income_description_enemy_;
		income_description_own_ = base.income_description_own_;
	}
	else if (overlay.village_) {
		income_description_ = overlay.income_description_;
		income_description_ally_ = overlay.income_description_ally_;
		income_description_enemy_ = overlay.income_description_enemy_;
		income_description_own_ = overlay.income_description_own_;
	}

}

t_translation::t_terrain terrain_type::terrain_with_default_base() const {
	if(overlay_ && editor_default_base_ != t_translation::NONE_TERRAIN) {
		return t_translation::t_terrain(editor_default_base_.base, number_.overlay);
	}
	return number_;
}

void create_terrain_maps(const config::const_child_itors &cfgs,
                         t_translation::t_list& terrain_list,
                         std::map<t_translation::t_terrain, terrain_type>& letter_to_terrain)
{
	foreach (const config &t, cfgs)
	{
		terrain_type terrain(t);
		terrain_list.push_back(terrain.number());
		letter_to_terrain.insert(std::make_pair(terrain.number(), terrain));
	}
}

void merge_alias_lists(t_translation::t_list& first, const t_translation::t_list& second)
{
	// Insert second vector into first when the terrain _ref^base is encountered

	bool revert = (first.front() == t_translation::MINUS ? true : false);
	t_translation::t_list::iterator i;

	for(i = first.begin(); i != first.end(); i++) {
		if(*i == t_translation::PLUS) {
			revert = false;
			continue;
		} else if(*i == t_translation::MINUS) {
			revert = true;
			continue;
		}

		if(*i == t_translation::BASE) {
			t_translation::t_list::iterator insert_it = first.erase(i);
			//if we are in reverse mode, insert PLUS before and MINUS after the base list
            //so calculation of base aliases will work normal
			if(revert) {
//				insert_it = first.insert(insert_it, t_translation::PLUS);
//				insert_it++;
				insert_it = first.insert(insert_it, t_translation::MINUS);
			}
			else {
                //else insert PLUS after the base aliases to restore previous "reverse state"
				insert_it =  first.insert(insert_it, t_translation::PLUS);
			}

			first.insert(insert_it, second.begin(), second.end());

			break;
		}
	}

}
