/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * TImageTypesConfig.cpp: Image Types editor template.                     *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_CONFIG_TIMAGETYPESCONFIG_CPP__
#define __ROMPROPERTIES_LIBROMDATA_CONFIG_TIMAGETYPESCONFIG_CPP__

#include "TImageTypesConfig.hpp"

#include "../RpWin32.hpp"
#include "../RomData.hpp"
#include "Config.hpp"
using LibRomData::Config;
using LibRomData::RomData;

namespace LibRomData {

// Image type names.
template<typename ComboBox>
const rp_char *const TImageTypesConfig<ComboBox>::imageTypeNames[IMG_TYPE_COUNT] = {
	_RP("Internal\nIcon"),
	_RP("Internal\nBanner"),
	_RP("Internal\nMedia"),
	_RP("External\nMedia"),
	_RP("External\nCover"),
	_RP("External\n3D Cover"),
	_RP("External\nFull Cover"),
	_RP("External\nBox"),
};

// System data.
template<typename ComboBox>
const SysData_t TImageTypesConfig<ComboBox>::sysData[SYS_COUNT] = {
	SysDataEntry(Amiibo,		_RP("amiibo")),
	SysDataEntry(DreamcastSave,	_RP("Dreamcast Saves")),
	SysDataEntry(GameCube,		_RP("GameCube / Wii")),
	SysDataEntry(GameCubeSave,	_RP("GameCube Saves")),
	SysDataEntry(NintendoDS,	_RP("Nintendo DS(i)")),
	SysDataEntry(Nintendo3DS,	_RP("Nintendo 3DS")),
	SysDataEntry(PlayStationSave,	_RP("PlayStation Saves")),
	SysDataEntry(WiiU,		_RP("Wii U")),
};

template<typename ComboBox>
TImageTypesConfig<ComboBox>::TImageTypesConfig()
	: changed(false)
{
	static_assert(std::is_pointer<ComboBox>::value, "TImageTypesConfig template parameter must be a pointer.");

	// Clear the arrays.
	memset(cboImageType, 0, sizeof(cboImageType));
	memset(imageTypes, 0xFF, sizeof(imageTypes));
	memset(validImageTypes, 0, sizeof(validImageTypes));
	memset(sysIsDefault, 0, sizeof(sysIsDefault));
}

template<typename ComboBox>
TImageTypesConfig<ComboBox>::~TImageTypesConfig()
{ }

/**
 * (Re-)Load the configuration into the grid.
 */
template<typename ComboBox>
void TImageTypesConfig<ComboBox>::reset(void)
{
	// Reset all ComboBox objects first.
	for (int sys = SYS_COUNT-1; sys >= 0; sys--) {
		unsigned int cbid = sysAndImageTypeToCbid(sys, IMG_TYPE_COUNT-1);
		for (int imageType = IMG_TYPE_COUNT-1; imageType >= 0; imageType--, cbid--) {
			if (cboImageType[sys][imageType]) {
				// NOTE: Using the actual priority value, not the ComboBox index.
				cboImageType_setPriorityValue(cbid, 0xFF);
			}
		}
	}

	const Config *const config = Config::instance();
	for (int sys = SYS_COUNT-1; sys >= 0; sys--) {
		ComboBox *p_cboImageType = &cboImageType[sys][0];

		// Get the image priority.
		Config::ImgTypePrio_t imgTypePrio;
		Config::ImgTypeResult res = config->getImgTypePrio(sysData[sys].classNameA, &imgTypePrio);
		bool no_thumbs = false;
		switch (res) {
			case Config::IMGTR_SUCCESS:
				// Image type priority received successfully.
				sysIsDefault[sys] = false;
				break;
			case Config::IMGTR_SUCCESS_DEFAULTS:
				// Image type priority received successfully.
				// IMGTR_SUCCESS_DEFAULTS indicates the returned
				// data is the default priority, since a custom
				// configuration was not found for this class.
				sysIsDefault[sys] = true;
				break;
			case Config::IMGTR_DISABLED:
				// Thumbnails are disabled for this class.
				no_thumbs = true;
				break;
			default:
				// Should not happen...
				assert(!"Invalid return value from Config::getImgTypePrio().");
				no_thumbs = true;
				break;
		}

		if (no_thumbs)
			continue;

		int nextPrio = 0;	// Next priority value to use.
		bool imageTypeSet[RomData::IMG_EXT_MAX+1];	// Element set to true once an image type priority is read.
		memset(imageTypeSet, 0, sizeof(imageTypeSet));

		p_cboImageType = &cboImageType[sys][0];
		for (unsigned int i = 0; i < imgTypePrio.length && nextPrio <= validImageTypes[sys]; i++)
		{
			uint8_t imageType = imgTypePrio.imgTypes[i];
			assert(imageType == 0xFF || imageType <= RomData::IMG_EXT_MAX);
			if (imageType > RomData::IMG_EXT_MAX && imageType != 0xFF) {
				// Invalid image type.
				continue;
			}
			if (p_cboImageType[imageType] && !imageTypeSet[imageType]) {
				// Set the image type.
				imageTypeSet[imageType] = true;
				if (imageType <= RomData::IMG_EXT_MAX) {
					imageTypes[sys][imageType] = nextPrio;
					// NOTE: Using the actual priority value, not the ComboBox index.
					cboImageType_setPriorityValue(sysAndImageTypeToCbid(sys, imageType), nextPrio);
					nextPrio++;
				}
			}
		}
	}

	// No longer changed.
	changed = false;
}

/**
 * A ComboBox index was changed by the user.
 * @param cbid ComboBox ID.
 * @param prio New priority value. (0xFF == no)
 */
template<typename ComboBox>
void TImageTypesConfig<ComboBox>::cboImageType_priorityValueChanged(unsigned int cbid, unsigned int prio)
{
	const unsigned int sys = sysFromCbid(cbid);
	const unsigned int imageType = imageTypeFromCbid(cbid);
	assert(sys < SYS_COUNT);
	assert(imageType < IMG_TYPE_COUNT);
	if (sys >= SYS_COUNT || imageType >= IMG_TYPE_COUNT)
		return;

	if (prio >= 0 && prio != 0xFF) {
		// Check for any image types that have the new priority.
		const uint8_t prev_prio = imageTypes[sys][imageType];
		for (int i = RomData::IMG_EXT_MAX; i >= 0; i--) {
			if (i == imageType)
				continue;
			if (cboImageType[sys][i] != nullptr && imageTypes[sys][i] == (uint8_t)prio) {
				// Found a match! Swap the priority.
				imageTypes[sys][i] = prev_prio;
				cboImageType_setPriorityValue(sysAndImageTypeToCbid(sys, i), prev_prio);
				break;
			}
		}
	}

	// Save the image priority value.
	imageTypes[sys][imageType] = prio;
	// Mark this configuration as no longer being default.
	sysIsDefault[sys] = false;
	// Configuration has been changed.
	changed = true;
}

}

#endif /* __ROMPROPERTIES_LIBROMDATA_CONFIG_TIMAGETYPESCONFIG_CPP__ */
