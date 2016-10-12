/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCubeSave.hpp: Nintendo GameCube save file reader.                   *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#include "GameCubeSave.hpp"
#include "NintendoPublishers.hpp"
#include "gcn_card.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"
#include "img/rp_image.hpp"
#include "img/ImageDecoder.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

class GameCubeSavePrivate
{
	public:
		GameCubeSavePrivate();

	private:
		GameCubeSavePrivate(const GameCubeSavePrivate &other);
		GameCubeSavePrivate &operator=(const GameCubeSavePrivate &other);

	public:
		// RomFields data.

		// Date/Time. (RFT_DATETIME)
		static const RomFields::DateTimeDesc last_modified_dt;

		// Monospace string formatting.
		static const RomFields::StringDesc gcn_save_string_monospace;

		// RomFields data.
		static const struct RomFields::Desc gcn_save_fields[];

		// Directory entry from the GCI header.
		card_direntry direntry;

		/**
		 * Byteswap a card_direntry struct.
		 * @param direntry card_direntry struct.
		 * @param maxdrive_sav If true, adjust for MaxDrive.
		 */
		static void byteswap_direntry(card_direntry *direntry, bool maxdrive_sav);

		// Save file type.
		enum SaveType {
			SAVE_TYPE_UNKNOWN = -1,	// Unknown save type.

			SAVE_TYPE_GCI = 0,	// USB Memory Adapter
			SAVE_TYPE_GCS = 1,	// GameShark
			SAVE_TYPE_SAV = 2,	// MaxDrive
		};
		int saveType;

		// Data offset. This is the actual starting address
		// of the game data, past the file-specific headers
		// and the CARD directory entry.
		int dataOffset;

		/**
		 * Is the specified buffer a valid CARD directory entry?
		 * @param buffer CARD directory entry. (Must be 64 bytes.)
		 * @param data_size Data area size. (no headers)
		 * @param maxdrive_sav If true, adjust for MaxDrive changes.
		 * @return True if this appears to be a valid CARD directory entry; false if not.
		 */
		static bool isCardDirEntry(const uint8_t *buffer, uint32_t data_size, bool maxdrive_sav);

		// TODO: load image function.
};

/** GameCubeSavePrivate **/

GameCubeSavePrivate::GameCubeSavePrivate()
	: saveType(SAVE_TYPE_UNKNOWN)
	, dataOffset(-1)
{
	// Clear the directory entry.
	memset(&direntry, 0, sizeof(direntry));
}

// Last Modified timestamp.
const RomFields::DateTimeDesc GameCubeSavePrivate::last_modified_dt = {
	RomFields::RFT_DATETIME_HAS_DATE |
	RomFields::RFT_DATETIME_HAS_TIME |
	RomFields::RFT_DATETIME_IS_UTC	// GameCube doesn't support timezones.
};

// Monospace string formatting.
const RomFields::StringDesc GameCubeSavePrivate::gcn_save_string_monospace = {
	RomFields::StringDesc::STRF_MONOSPACE
};

// Save file fields.
const struct RomFields::Desc GameCubeSavePrivate::gcn_save_fields[] = {
	// TODO: Banner?
	{_RP("Game ID"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Publisher"), RomFields::RFT_STRING, {nullptr}},
	{_RP("File Name"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Description"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Last Modified"), RomFields::RFT_DATETIME, {&last_modified_dt}},
	{_RP("Mode"), RomFields::RFT_STRING, {&gcn_save_string_monospace}},
	{_RP("Copy Count"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Blocks"), RomFields::RFT_STRING, {nullptr}},
};

/**
 * Byteswap a card_direntry struct.
 * @param direntry card_direntry struct.
 * @param maxdrive_sav If true, adjust for MaxDrive.
 */
void GameCubeSavePrivate::byteswap_direntry(card_direntry *direntry, bool maxdrive_sav)
{
	if (maxdrive_sav) {
		// Swap 16-bit values at 0x2E through 0x40.
		// Also 0x06 (pad_00 / bannerfmt).
		// Reference: https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/HW/GCMemcard.cpp
		uint16_t *u16ptr = reinterpret_cast<uint16_t*>(direntry);
		u16ptr[0x06>>2] = __swab16(u16ptr[0x06>>2]);
		for (int i = (0x2C>>1); i < (0x40>>1); i++) {
			u16ptr[i] = __swab16(u16ptr[i]);
		}
	}

	// FIXME: Dolphin says GCS length field might not be accurate.

	// 16-bit fields.
	direntry->iconfmt	= be16_to_cpu(direntry->iconfmt);
	direntry->iconspeed	= be16_to_cpu(direntry->iconspeed);
	direntry->block		= be16_to_cpu(direntry->block);
	direntry->length	= be16_to_cpu(direntry->length);
	direntry->pad_01	= be16_to_cpu(direntry->pad_01);

	// 32-bit fields.
	direntry->lastmodified	= be32_to_cpu(direntry->lastmodified);
	direntry->iconaddr	= be32_to_cpu(direntry->iconaddr);
	direntry->commentaddr	= be32_to_cpu(direntry->commentaddr);
}

/**
 * Is the specified buffer a valid CARD directory entry?
 * @param buffer CARD directory entry. (Must be 64 bytes.)
 * @param data_size Data area size. (no headers)
 * @param maxdrive_sav If true, adjust for MaxDrive changes.
 * @return True if this appears to be a valid CARD directory entry; false if not.
 */
bool GameCubeSavePrivate::isCardDirEntry(const uint8_t *buffer, uint32_t data_size, bool maxdrive_sav)
{
	// MaxDrive SAV files use 16-bit byteswapping for non-text fields.
	// This means PDP-endian for 32-bit fields!
	const card_direntry *const direntry = reinterpret_cast<const card_direntry*>(buffer);

	// Game ID should be alphanumeric.
	// TODO: NDDEMO has a NULL in the game ID, but I don't think
	// it has save files.
	for (int i = 6-1; i >= 0; i--) {
		if (!isalnum(direntry->id6[i])) {
			// Non-alphanumeric character.
			return false;
		}
	}

	// Padding should be 0xFF.
	if (maxdrive_sav) {
		// MaxDrive SAV. pad_00 and bannerfmt are swappde.
		if (direntry->bannerfmt != 0xFF) {
			// Incorrect padding.
			return false;
		}
	} else {
		// Other formats.
		if (direntry->pad_00 != 0xFF) {
			// Incorrect padding.
			return false;
		}
	}

	if (be16_to_cpu(direntry->pad_01) != 0xFFFF) {
		// Incorrect padding.
		return false;
	}

	// Verify the block count.
	unsigned int length;
	if (maxdrive_sav) {
		length = le16_to_cpu(direntry->length);
	} else {
		length = be16_to_cpu(direntry->length);
	}
	if (length * 8192 != data_size) {
		// Incorrect block count.
		return false;
	}

	// Comment and icon addresses should both be less than the file size,
	// minus 64 bytes for the GCI header.
#define PDP_SWAP(dest, src) \
	do { \
		union { uint16_t w[2]; uint32_t d; } tmp; \
		tmp.d = be32_to_cpu(src); \
		tmp.w[0] = __swab16(tmp.w[0]); \
		tmp.w[1] = __swab16(tmp.w[1]); \
		(dest) = tmp.d; \
	} while (0)
	uint32_t iconaddr, commentaddr;
	if (!maxdrive_sav) {
		iconaddr = be32_to_cpu(direntry->iconaddr);
		commentaddr = be32_to_cpu(direntry->commentaddr);
	} else {
		PDP_SWAP(iconaddr, direntry->iconaddr);
		PDP_SWAP(commentaddr, direntry->commentaddr);
	}
	if (iconaddr >= data_size || commentaddr >= data_size) {
		// Comment and/or icon are out of bounds.
		return false;
	}

	// This appears to be a valid CARD directory entry.
	return true;
}

/** GameCubeSave **/

/**
 * Read a Nintendo GameCube save file.
 *
 * A save file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open disc image.
 */
GameCubeSave::GameCubeSave(IRpFile *file)
	: RomData(file, GameCubeSavePrivate::gcn_save_fields, ARRAY_SIZE(GameCubeSavePrivate::gcn_save_fields))
	, d(new GameCubeSavePrivate())
{
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the save file header.
	uint8_t header[1024];
	m_file->rewind();
	size_t size = m_file->read(&header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this disc image is supported.
	DetectInfo info;
	info.pHeader = header;
	info.szHeader = sizeof(header);
	info.ext = nullptr;	// TODO: GCI, GCS, SAV, etc?
	info.szFile = m_file->fileSize();
	d->saveType = isRomSupported(&info);

	// Save the directory entry for later.
	uint32_t gciOffset;	// offset to GCI header
	switch (d->saveType) {
		case GameCubeSavePrivate::SAVE_TYPE_GCI:
			gciOffset = 0;
			break;
		case GameCubeSavePrivate::SAVE_TYPE_GCS:
			gciOffset = 0x110;
			break;
		case GameCubeSavePrivate::SAVE_TYPE_SAV:
			gciOffset = 0x80;
			break;
		default:
			// Unknown disc type.
			d->saveType = GameCubeSavePrivate::SAVE_TYPE_UNKNOWN;
			return;
	}

	m_isValid = true;

	// Save the directory entry for later.
	memcpy(&d->direntry, &header[gciOffset], sizeof(d->direntry));
	d->byteswap_direntry(&d->direntry, (d->saveType == GameCubeSavePrivate::SAVE_TYPE_SAV));
	// Data area offset.
	d->dataOffset = gciOffset + 0x40;
}

GameCubeSave::~GameCubeSave()
{
	delete d;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameCubeSave::isRomSupported_static(const DetectInfo *info)
{
	if (!info || info->szHeader < 1024) {
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	if (info->szFile > 8192*2043) {
		// File is larger than 2043 blocks.
		// This isn't possible on an actual memory card.
		return -1;
	}

	// Check for GCS. (GameShark)
	static const uint8_t gcs_magic[] = {'G','C','S','A','V','E',0x01,0x00};
	if (!memcmp(info->pHeader, gcs_magic, sizeof(gcs_magic))) {
		// Is the size correct?
		// GCS files are a multiple of 8 KB, plus 336 bytes:
		// - 272 bytes: GCS-specific header.
		// - 64 bytes: CARD directory entry.
		// TODO: GCS has a user-specified description field and other stuff.
		const uint32_t data_size = (uint32_t)(info->szFile - 336);
		if (data_size % 8192 == 0) {
			// Check the CARD directory entry.
			if (GameCubeSavePrivate::isCardDirEntry(&info->pHeader[0x110], data_size, false)) {
				// This is a GCS file.
				return GameCubeSavePrivate::SAVE_TYPE_GCS;
			}
		}
	}

	// Check for SAV. (MaxDrive)
	static const uint8_t sav_magic[] = {'D','A','T','E','L','G','C','_','S','A','V','E',0,0,0,0};
	if (!memcmp(info->pHeader, sav_magic, sizeof(sav_magic))) {
		// Is the size correct?
		// SAVE files are a multiple of 8 KB, plus 192 bytes:
		// - 128 bytes: SAV-specific header.
		// - 64 bytes: CARD directory entry.
		// TODO: SAV has a copy of the description, plus other fields?
		const uint32_t data_size = (uint32_t)(info->szFile - 192);
		if (data_size % 8192 == 0) {
			// Check the CARD directory entry.
			if (GameCubeSavePrivate::isCardDirEntry(&info->pHeader[0x80], data_size, true)) {
				// This is a GCS file.
				return GameCubeSavePrivate::SAVE_TYPE_SAV;
			}
		}
	}

	// Check for GCI.
	// GCI files are a multiple of 8 KB, plus 64 bytes:
	// - 64 bytes: CARD directory entry.
	const uint32_t data_size = (uint32_t)(info->szFile - 64);
	if (data_size % 8192 == 0) {
		// Check the CARD directory entry.
		if (GameCubeSavePrivate::isCardDirEntry(info->pHeader, data_size, false)) {
			// This is a GCI file.
			return GameCubeSavePrivate::SAVE_TYPE_GCI;
		}
	}

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameCubeSave::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *GameCubeSave::systemName(uint32_t type) const
{
	if (!m_isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	// Bits 2-3: DISC_SYSTEM_MASK (GCN, Wii, Triforce)
	static const rp_char *const sysNames[4] = {
		// FIXME: "NGC" in Japan?
		_RP("Nintendo GameCube"), _RP("GameCube"), _RP("GCN"), nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> GameCubeSave::supportedFileExtensions_static(void)
{
	vector<const rp_char*> ret;
	ret.reserve(3);
	ret.push_back(_RP(".gci"));	// USB Memory Adapter
	ret.push_back(_RP(".gcs"));	// GameShark
	ret.push_back(_RP(".sav"));	// MaxDrive (TODO: Too generic?)
	return ret;
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> GameCubeSave::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t GameCubeSave::supportedImageTypes_static(void)
{
	// TODO: Banner.
	return IMGBF_INT_ICON; //| IMGBF_INT_BANNER;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t GameCubeSave::supportedImageTypes(void) const
{
	return supportedImageTypes_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GameCubeSave::loadFieldData(void)
{
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!m_file || !m_file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid || d->saveType < 0 || d->dataOffset < 0) {
		// Unknown save file type.
		return -EIO;
	}

	// Save file header is read and byteswapped in the constructor.

	// Game ID.
	// Replace any non-printable characters with underscores.
	// (NDDEMO has ID6 "00\0E01".)
	char id6[7];
	for (int i = 0; i < 6; i++) {
		id6[i] = (isprint(d->direntry.id6[i])
			? d->direntry.id6[i]
			: '_');
	}
	id6[6] = 0;
	m_fields->addData_string(latin1_to_rp_string(id6, 6));

	// Look up the publisher.
	const rp_char *publisher = NintendoPublishers::lookup(d->direntry.company);
	m_fields->addData_string(publisher ? publisher : _RP("Unknown"));

	// Filename.
	// TODO: Remove trailing nulls and/or spaces.
	// (Implicit length version of cp1252_sjis_to_rp_string()?)
	m_fields->addData_string(cp1252_sjis_to_rp_string(
		d->direntry.filename, sizeof(d->direntry.filename)));

	// Description.
	char desc_buf[64];
	m_file->seek(d->dataOffset + d->direntry.commentaddr);
	size_t size = m_file->read(desc_buf, sizeof(desc_buf));
	if (size != sizeof(desc_buf)) {
		// Error reading the description.
		m_fields->addData_invalid();
	} else {
		// Add the description.
		rp_string desc = cp1252_sjis_to_rp_string(desc_buf, 32);
		desc += _RP_CHR('\n');
		desc += cp1252_sjis_to_rp_string(&desc_buf[32], 32);
		m_fields->addData_string(desc);
	}

	// Last Modified timestamp.
	m_fields->addData_dateTime((int64_t)d->direntry.lastmodified + GC_UNIX_TIME_DIFF);

	// File mode.
	rp_char file_mode[5];
	file_mode[0] = ((d->direntry.permission & CARD_ATTRIB_GLOBAL) ? _RP_CHR('G') : _RP_CHR('-'));
	file_mode[1] = ((d->direntry.permission & CARD_ATTRIB_NOMOVE) ? _RP_CHR('M') : _RP_CHR('-'));
	file_mode[2] = ((d->direntry.permission & CARD_ATTRIB_NOCOPY) ? _RP_CHR('C') : _RP_CHR('-'));
	file_mode[3] = ((d->direntry.permission & CARD_ATTRIB_PUBLIC) ? _RP_CHR('P') : _RP_CHR('-'));
	file_mode[4] = 0;
	m_fields->addData_string(file_mode);

	// Copy count.
	m_fields->addData_string_numeric(d->direntry.copytimes);
	// Blocks.
	m_fields->addData_string_numeric(d->direntry.length);

	// Finished reading the field data.
	return (int)m_fields->count();
}

/**
 * Blit a tile to a linear image buffer.
 * @tparam pixel	[in] Pixel type.
 * @tparam tileW	[in] Tile width.
 * @tparam tileH	[in] Tile height.
 * @param img		[out] rp_image.
 * @param tileBuf	[in] Tile buffer.
 * @param tileX		[in] Horizontal tile number.
 * @param tileY		[in] Vertical tile number.
 */
template<typename pixel, int tileW, int tileH>
static inline void BlitTile(rp_image *img, const pixel *tileBuf, int tileX, int tileY)
{
	switch (sizeof(pixel)) {
		case 4:
			assert(img->format() == rp_image::FORMAT_ARGB32);
			break;
		case 1:
			assert(img->format() == rp_image::FORMAT_CI8);
			break;
		default:
			assert(!"Unsupported sizeof(pixel).");
			return;
	}

	// Go to the first pixel for this tile.
	const int stride_px = img->stride() / sizeof(pixel);
	pixel *imgBuf = reinterpret_cast<pixel*>(img->scanLine(tileY * tileH));
	imgBuf += (tileX * tileW);

	for (int y = tileH; y > 0; y--) {
		memcpy(imgBuf, tileBuf, (tileW * sizeof(pixel)));
		imgBuf += stride_px;
		tileBuf += tileW;
	}
}

/**
 * Load an internal image.
 * Called by RomData::image() if the image data hasn't been loaded yet.
 * @param imageType Image type to load.
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCubeSave::loadInternalImage(ImageType imageType)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}
	if (m_images[imageType]) {
		// Icon *has* been loaded...
		return 0;
	} else if (!m_file) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Check for supported image types.
	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported by DS.
		return -ENOENT;
	}

	// Use nearest-neighbor scaling when resizing.
	m_imgpf[imageType] = IMGPF_RESCALE_NEAREST;

	// CARD directory entry.
	// Constructor has already byteswapped everything.
	const card_direntry *direntry = &d->direntry;

	// Calculate the icon start address.
	// The icon is located directly after the banner.
	uint32_t iconaddr = direntry->iconaddr;
	switch (direntry->bannerfmt & CARD_BANNER_MASK) {
		case CARD_BANNER_CI:
			// CI8 banner.
			iconaddr += (CARD_BANNER_W * CARD_BANNER_H * 1);
			iconaddr += (256 * 2);	// RGB5A3 palette
			break;
		case CARD_BANNER_RGB:
			// RGB5A3 banner.
			iconaddr += (CARD_BANNER_W * CARD_BANNER_H * 2);
			break;
		default:
			// No banner.
			break;
	}

	// Determine the format of the first icon.
	uint32_t iconsize, palsize, paladdr;
	switch (direntry->iconfmt & CARD_ICON_MASK) {
		case CARD_ICON_RGB:
			// RGB5A3.
			iconsize = CARD_ICON_W * CARD_ICON_H * 2;
			palsize = 0;
			paladdr = 0;
			break;

		case CARD_ICON_CI_UNIQUE:
			// CI8 with a unique palette.
			// Palette is located immediately after the icon.
			iconsize = CARD_ICON_W * CARD_ICON_H * 1;
			palsize = 256 * 2;
			paladdr = iconaddr + iconsize;
			break;

		case CARD_ICON_CI_SHARED: {
			// CI8 with a shared palette.
			// Palette is located after *all* of the icons.
			iconsize = CARD_ICON_W * CARD_ICON_H * 1;
			palsize = 256 * 2;
			paladdr = iconaddr;
			uint16_t iconfmt = direntry->iconfmt;
			uint16_t iconspeed = direntry->iconspeed;
			for (int i = 0; i < CARD_MAXICONS; i++, iconfmt >>= 2, iconspeed >>= 2) {
				if ((iconspeed & CARD_SPEED_MASK) == CARD_SPEED_END) {
					// End of the icons.
					break;
				}

				switch (iconfmt & CARD_ICON_MASK) {
					case CARD_ICON_RGB:
						paladdr += (CARD_ICON_W * CARD_ICON_H * 2);
						break;
					case CARD_ICON_CI_UNIQUE:
						paladdr += (CARD_ICON_W * CARD_ICON_H * 1);
						paladdr += (256 * 2);
						break;
					case CARD_ICON_CI_SHARED:
						paladdr += (CARD_ICON_W * CARD_ICON_H * 1);
						break;
					default:
						break;
				}
			}

			// paladdr is now after all of the icons.
			break;
		}

		default:
			// No icon.
			return -ENOENT;
	}

	// Read the icon data.
	// NOTE: Only the first frame.
	static const int MAX_ICON_SIZE = CARD_ICON_W * CARD_ICON_H * 2;
	uint8_t iconbuf[MAX_ICON_SIZE];
	m_file->seek(d->dataOffset + iconaddr);
	size_t size = m_file->read(iconbuf, iconsize);
	if (size != iconsize) {
		// Error reading the icon data.
		return -EIO;
	}

	// Convert the icon from GCN format to rp_image.
	rp_image *icon = nullptr;
	if ((direntry->iconfmt & CARD_ICON_MASK) == CARD_ICON_RGB) {
		// GCN RGB5A3 -> ARGB32
		icon = ImageDecoder::fromGcnRGB5A3(CARD_ICON_W, CARD_ICON_H,
			reinterpret_cast<const uint16_t*>(iconbuf), iconsize);
	} else {
		// Read the palette data.
		uint16_t palbuf[256];
		assert(palsize == sizeof(palbuf));
		if (palsize != sizeof(palbuf)) {
			// Size is incorrect.
			return -EIO;
		}
		m_file->seek(d->dataOffset + paladdr);
		size = m_file->read(palbuf, sizeof(palbuf));
		if (size != sizeof(palbuf)) {
			// Error reading the palette data.
			return -EIO;
		}

		// GCN CI8 -> CI8
		icon = ImageDecoder::fromGcnCI8(CARD_ICON_W, CARD_ICON_H,
			iconbuf, iconsize, palbuf, sizeof(palbuf));
	}

	if (!icon) {
		// Error converting the icon.
		return -EIO;
	}

	// Finished decoding the icon.
	m_images[imageType] = icon;
	return 0;
}

}
