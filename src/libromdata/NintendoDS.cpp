/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoDS.hpp: Nintendo DS(i) ROM reader.                              *
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

#include "NintendoDS.hpp"
#include "NintendoPublishers.hpp"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"
#include "img/rp_image.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>
#include <cstddef>

// C++ includes.
#include <vector>
using std::vector;

namespace LibRomData {

class NintendoDSPrivate
{
	public:
		NintendoDSPrivate() { }

	private:
		NintendoDSPrivate(const NintendoDSPrivate &other);
		NintendoDSPrivate &operator=(const NintendoDSPrivate &other);

	public:
		/** RomFields **/

		// Hardware type. (RFT_BITFIELD)
		enum NDS_HWType {
			DS_HW_DS	= (1 << 0),
			DS_HW_DSi	= (1 << 1),
		};
		static const rp_char *nds_hw_bitfield_names[];
		static const RomFields::BitfieldDesc nds_hw_bitfield;

		// DS region. (RFT_BITFIELD)
		enum NDS_Region {
			NDS_REGION_FREE		= (1 << 0),
			NDS_REGION_SKOREA	= (1 << 1),
			NDS_REGION_CHINA	= (1 << 2),
		};
		static const rp_char *const nds_region_bitfield_names[];
		static const RomFields::BitfieldDesc nds_region_bitfield;

		// DSi region. (RFT_BITFIELD)
		enum DSi_Region {
			DSi_REGION_JAPAN	= (1 << 0),
			DSi_REGION_USA		= (1 << 1),
			DSi_REGION_EUROPE	= (1 << 2),
			DSi_REGION_AUSTRALIA	= (1 << 3),
			DSi_REGION_CHINA	= (1 << 4),
			DSi_REGION_SKOREA	= (1 << 5),
		};
		static const rp_char *const dsi_region_bitfield_names[];
		static const RomFields::BitfieldDesc dsi_region_bitfield;

		// ROM fields.
		static const struct RomFields::Desc nds_fields[];

		/** Internal ROM data. **/

		/**
		 * Nintendo DS ROM header.
		 * This matches the ROM header format exactly.
		 * Reference: http://problemkaputt.de/gbatek.htm#dscartridgeheader
		 * 
		 * All fields are little-endian.
		 * NOTE: Strings are NOT null-terminated!
		 */
		#pragma pack(1)
		#define NDS_RomHeader_SIZE 4096
		struct PACKED NDS_RomHeader {
			char title[12];
			union {
				char id6[6];	// Game code. (ID6)
				struct {
					char id4[4];		// Game code. (ID4)
					char company[2];	// Company code.
				};
			};

			// 0x12
			uint8_t unitcode;	// 00h == NDS, 02h == NDS+DSi, 03h == DSi only
			uint8_t enc_seed_select;
			uint8_t device_capacity;
			uint8_t reserved1[7];
			uint8_t reserved2_dsi;
			uint8_t nds_region;	// 0x00 == normal, 0x80 == China, 0x40 == Korea
			uint8_t rom_version;
			uint8_t autostart;

			// 0x20
			struct {
				uint32_t rom_offset;
				uint32_t entry_address;
				uint32_t ram_address;
				uint32_t size;
			} arm9;
			struct {
				uint32_t rom_offset;
				uint32_t entry_address;
				uint32_t ram_address;
				uint32_t size;
			} arm7;

			// 0x40
			uint32_t fnt_offset;	// File Name Table offset
			uint32_t fnt_size;	// File Name Table size
			uint32_t fat_offset;
			uint32_t fat_size;

			// 0x50
			uint32_t arm9_overlay_offset;
			uint32_t arm9_overlay_size;
			uint32_t arm7_overlay_offset;
			uint32_t arm7_overlay_size;

			// 0x60
			uint32_t cardControl13;	// Port 0x40001A4 setting for normal commands (usually 0x00586000)
			uint32_t cardControlBF;	// Port 0x40001A4 setting for KEY1 commands (usually 0x001808F8)

			// 0x68
			uint32_t icon_offset;
			uint16_t secure_area_checksum;		// CRC32 of 0x0020...0x7FFF
			uint16_t secure_area_delay;		// Delay, in 131 kHz units (0x051E=10ms, 0x0D7E=26ms)

			uint32_t arm9_auto_load_list_ram_address;
			uint32_t arm7_auto_load_list_ram_address;

			uint64_t secure_area_disable;

			// 0x80
			uint32_t total_used_rom_size;		// Excluding DSi area
			uint32_t rom_header_size;		// Usually 0x4000
			uint8_t reserved3[0x38];
			uint8_t nintendo_logo[0x9C];		// GBA-style Nintendo logo
			uint16_t nintendo_logo_checksum;	// CRC16 of nintendo_logo[] (always 0xCF56)
			uint16_t header_checksum;		// CRC16 of 0x0000...0x015D

			// 0x160
			struct {
				uint32_t rom_offset;
				uint32_t size;
				uint32_t ram_address;
			} debug;

			// 0x16C
			uint8_t reserved4[4];
			uint8_t reserved5[0x10];

			// 0x180 [DSi header]
			uint32_t dsi_region;	// DSi region flags.

			// TODO: More DSi header entries.
			// Reference: http://problemkaputt.de/gbatek.htm#dsicartridgeheader
			uint8_t reserved_more_dsi[3708];
		};
		#pragma pack()

		/**
		 * Nintendo DS icon and title struct.
		 * Reference: http://problemkaputt.de/gbatek.htm#dscartridgeicontitle
		 *
		 * All fields are little-endian.
		 */
		#define NDS_IconTitleData_SIZE 9152
		#pragma pack(1)
		struct PACKED NDS_IconTitleData {
			uint16_t version;		// known values: 0x0001, 0x0002, 0x0003, 0x0103
			uint16_t crc16[4];		// CRC16s for the four known versions.
			uint8_t reserved1[0x16];

			uint8_t icon_data[0x200];	// Icon data. (32x32, 4x4 tiles, 4-bit color)
			uint16_t icon_pal[0x10];	// Icon palette. (16-bit color; color 0 is transparent)

			// [0x240] Titles. (128 characters each; UTF-16)
			// Order: JP, EN, FR, DE, IT, ES, ZH (v0002), KR (v0003)
			char16_t title[8][128];

			// [0xA40] Reserved space, possibly for other titles.
			char reserved2[0x800];

			// [0x1240] DSi animated icons (v0103h)
			// Icons use the same format as DS icons.
			uint8_t dsi_icon_data[8][0x200];	// Icon data. (Up to 8 frames)
			uint16_t dsi_icon_pal[8][0x10];		// Icon palettes.
			uint16_t dsi_icon_seq[0x40];		// Icon animation sequence.
		};
		#pragma pack()

	public:
		// ROM header.
		NDS_RomHeader romHeader;
};

/** NintendoDSPrivate **/

// Hardware bitfield.
const rp_char *NintendoDSPrivate::nds_hw_bitfield_names[] = {
	_RP("Nintendo DS"), _RP("Nintendo DSi")
};

const RomFields::BitfieldDesc NintendoDSPrivate::nds_hw_bitfield = {
	ARRAY_SIZE(nds_hw_bitfield_names), 2, nds_hw_bitfield_names
};

// DS region bitfield.
const rp_char *const NintendoDSPrivate::nds_region_bitfield_names[] = {
	_RP("Region-Free"), _RP("South Korea"), _RP("China")
};

const RomFields::BitfieldDesc NintendoDSPrivate::nds_region_bitfield = {
	ARRAY_SIZE(nds_region_bitfield_names), 3, nds_region_bitfield_names
};

// DSi region bitfield.
const rp_char *const NintendoDSPrivate::dsi_region_bitfield_names[] = {
	_RP("Japan"), _RP("USA"), _RP("Europe"),
	_RP("Australia"), _RP("China"), _RP("South Korea")
};

const RomFields::BitfieldDesc NintendoDSPrivate::dsi_region_bitfield = {
	ARRAY_SIZE(dsi_region_bitfield_names), 3, dsi_region_bitfield_names
};

// ROM fields.
const struct RomFields::Desc NintendoDSPrivate::nds_fields[] = {
	// TODO: Banner?
	{_RP("Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Game ID"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Publisher"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Revision"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Hardware"), RomFields::RFT_BITFIELD, {&nds_hw_bitfield}},
	{_RP("DS Region"), RomFields::RFT_BITFIELD, {&nds_region_bitfield}},
	{_RP("DSi Region"), RomFields::RFT_BITFIELD, {&dsi_region_bitfield}},

	// TODO: Icon, full game title.
};

/**
 * Read a Nintendo DS ROM image.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
NintendoDS::NintendoDS(IRpFile *file)
	: RomData(file, NintendoDSPrivate::nds_fields, ARRAY_SIZE(NintendoDSPrivate::nds_fields))
	, d(new NintendoDSPrivate())
{
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the ROM header.
	static_assert(sizeof(NintendoDSPrivate::NDS_RomHeader) == NDS_RomHeader_SIZE,
		"NDS_RomHeader is not 4,096 bytes.");
	NintendoDSPrivate::NDS_RomHeader header;
	m_file->rewind();
	size_t size = m_file->read(&header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM image is supported.
	DetectInfo info;
	info.pHeader = reinterpret_cast<const uint8_t*>(&header);
	info.szHeader = sizeof(header);
	info.ext = nullptr;	// Not needed for NDS.
	info.szFile = 0;	// Not needed for NDS.
	m_isValid = (isRomSupported(&info) >= 0);

	if (m_isValid) {
		// Save the header for later.
		memcpy(&d->romHeader, &header, sizeof(d->romHeader));
	}
}

NintendoDS::~NintendoDS()
{
	delete d;
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int NintendoDS::isRomSupported_static(const DetectInfo *info)
{
	if (!info || info->szHeader < sizeof(NintendoDSPrivate::NDS_RomHeader)) {
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// TODO: Detect DS vs. DSi and return the system ID.

	// Check the first 16 bytes of the Nintendo logo.
	static const uint8_t nintendo_gba_logo[16] = {
		0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21,
		0x3D, 0x84, 0x82, 0x0A, 0x84, 0xE4, 0x09, 0xAD
	};

	const NintendoDSPrivate::NDS_RomHeader *romHeader =
		reinterpret_cast<const NintendoDSPrivate::NDS_RomHeader*>(info->pHeader);
	if (!memcmp(romHeader->nintendo_logo, nintendo_gba_logo, sizeof(nintendo_gba_logo))) {
		// Nintendo logo is present at the correct location.
		// TODO: DS vs. DSi?
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int NintendoDS::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *NintendoDS::systemName(uint32_t type) const
{
	if (!m_isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// NDS/DSi are mostly the same worldwide, except for China.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"NintendoDS::systemName() array index optimization needs to be updated.");
	static_assert(SYSNAME_REGION_MASK == (1 << 2),
		"NintendoDS::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (short, long, abbreviation)
	// Bit 2: 0 for NDS, 1 for DSi-exclusive.
	// Bit 3: 0 for worldwide, 1 for China. (iQue DS)
	static const rp_char *const sysNames[16] = {
		// Nintendo (worldwide)
		_RP("Nintendo DS"), _RP("Nintendo DS"), _RP("NDS"), nullptr,
		_RP("Nintendo DSi"), _RP("Nintendo DSi"), _RP("DSi"), nullptr,

		// iQue (China)
		_RP("iQue DS"), _RP("iQue DS"), _RP("NDS"), nullptr,
		_RP("iQue DSi"), _RP("iQue DSi"), _RP("DSi"), nullptr
	};

	uint32_t idx = (type & SYSNAME_TYPE_MASK);
	if ((d->romHeader.unitcode & 0x03) == 0x03) {
		// DSi-exclusive game.
		idx |= (1 << 2);
		if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_ROM_LOCAL) {
			if ((d->romHeader.dsi_region & NintendoDSPrivate::DSi_REGION_CHINA) ||
			    (d->romHeader.nds_region & 0x80))
			{
				// iQue DSi.
				idx |= (1 << 3);
			}
		}
	} else {
		// NDS-only and/or DSi-enhanced game.
		if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_ROM_LOCAL) {
			if (d->romHeader.nds_region & 0x80) {
				// iQue DS.
				idx |= (1 << 3);
			}
		}
	}

	return sysNames[idx];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> NintendoDS::supportedFileExtensions_static(void)
{
	vector<const rp_char*> ret;
	ret.reserve(1);
	ret.push_back(_RP(".nds"));
	return ret;
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> NintendoDS::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t NintendoDS::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t NintendoDS::supportedImageTypes(void) const
{
	return supportedImageTypes_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int NintendoDS::loadFieldData(void)
{
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!m_file) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Nintendo DS ROM header.
	const NintendoDSPrivate::NDS_RomHeader *romHeader = &d->romHeader;

	// Game title.
	m_fields->addData_string(latin1_to_rp_string(romHeader->title, sizeof(romHeader->title)));

	// Game ID and publisher.
	m_fields->addData_string(latin1_to_rp_string(romHeader->id6, sizeof(romHeader->id6)));

	// Look up the publisher.
	const rp_char *publisher = NintendoPublishers::lookup(romHeader->company);
	m_fields->addData_string(publisher ? publisher : _RP("Unknown"));

	// ROM version.
	m_fields->addData_string_numeric(romHeader->rom_version, RomFields::FB_DEC, 2);

	// Hardware type.
	// NOTE: DS_HW_DS is inverted bit0; DS_HW_DSi is normal bit1.
	uint32_t hw_type = (romHeader->unitcode & NintendoDSPrivate::DS_HW_DS) ^ NintendoDSPrivate::DS_HW_DS;
	hw_type |= (romHeader->unitcode & NintendoDSPrivate::DS_HW_DSi);
	if (hw_type == 0) {
		// 0x01 is invalid. Assume DS.
		hw_type = NintendoDSPrivate::DS_HW_DS;
	}
	m_fields->addData_bitfield(hw_type);

	// TODO: Combine DS Region and DSi Region into one bitfield?

	// DS Region.
	uint32_t nds_region = 0;
	if (romHeader->nds_region & 0x80) {
		nds_region |= NintendoDSPrivate::NDS_REGION_CHINA;
	}
	if (romHeader->nds_region & 0x40) {
		nds_region |= NintendoDSPrivate::NDS_REGION_SKOREA;
	}
	if (nds_region == 0) {
		// No known region flags.
		// Note that the Sonic Colors demo has 0x02 here.
		nds_region = NintendoDSPrivate::NDS_REGION_FREE;
	}
	m_fields->addData_bitfield(nds_region);

	// DSi Region.
	// Maps directly to the header field.
	if (hw_type & NintendoDSPrivate::DS_HW_DSi) {
		m_fields->addData_bitfield(romHeader->dsi_region);
	} else {
		// No DSi region.
		m_fields->addData_invalid();
	}

	// Finished reading the field data.
	return (int)m_fields->count();
}

/**
 * Convert an RGB555 pixel to ARGB32.
 * @param px16 RGB555 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t RGB555_to_ARGB32(uint16_t px16)
{
	uint32_t px32;

	// BGR555: xBBBBBGG GGGRRRRR
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	px32 = ((((px16 << 19) & 0xF80000) | ((px16 << 17) & 0x070000))) |	// Red
	       ((((px16 <<  6) & 0x00F800) | ((px16 <<  1) & 0x000700))) |	// Green
	       ((((px16 >>  7) & 0x0000F8) | ((px16 >> 12) & 0x000007)));	// Blue

	// No alpha channel.
	px32 |= 0xFF000000U;
	return px32;
}

/**
 * Blit a tile to a linear image buffer.
 * @tparam pixel	[in] Pixel type.
 * @tparam tileW	[in] Tile width.
 * @tparam tileH	[in] Tile height.
 * @param imgBuf	[out] Linear image buffer.
 * @param pitch		[in] Pitch of image buffer, in pixels.
 * @param tileBuf	[in] Tile buffer.
 * @param tileX		[in] Horizontal tile number.
 * @param tileY		[in] Vertical tile number.
 */
template<typename pixel, int tileW, int tileH>
static inline void BlitTile(pixel *imgBuf, int pitch,
			    const pixel *tileBuf, int tileX, int tileY)
{
	// Go to the first pixel for this tile.
	imgBuf += ((tileY * tileH * pitch) + (tileX * tileW));

	for (int y = tileH; y != 0; y--) {
		memcpy(imgBuf, tileBuf, (tileW * sizeof(pixel)));
		imgBuf += pitch;
		tileBuf += tileW;
	}
}

/**
 * Load an internal image.
 * Called by RomData::image() if the image data hasn't been loaded yet.
 * @param imageType Image type to load.
 * @return 0 on success; negative POSIX error code on error.
 */
int NintendoDS::loadInternalImage(ImageType imageType)
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

	// Nintendo DS ROM header.
	const NintendoDSPrivate::NDS_RomHeader *romHeader = &d->romHeader;

	// Get the address of the icon/title information.
	uint32_t icon_offset = le32_to_cpu(romHeader->icon_offset);

	// Read the icon data.
	// TODO: Also store titles?
	static_assert(sizeof(NintendoDSPrivate::NDS_IconTitleData) == NDS_IconTitleData_SIZE,
		      "NDS_IconTitleData is not 9,152 bytes.");
	NintendoDSPrivate::NDS_IconTitleData nds_icon_title;
	m_file->seek(icon_offset);
	size_t size = m_file->read(&nds_icon_title, sizeof(nds_icon_title));
	// Make sure we have up to the icon plus two titles.
	if (size < (offsetof(NintendoDSPrivate::NDS_IconTitleData, title) + 0x200))
		return -EIO;

	// Convert the icon from DS format to 256-color.
	rp_image *icon = new rp_image(32, 32, rp_image::FORMAT_CI8);

	// Convert the palette first.
	// TODO: Make sure there's at least 16 entries?
	uint32_t *palette = icon->palette();
	palette[0] = 0; // Color 0 is always transparent.
	icon->set_tr_idx(0);
	for (int i = 1; i < 16; i++) {
		// DS color format is BGR555.
		palette[i] = RGB555_to_ARGB32(le16_to_cpu(nds_icon_title.icon_pal[i]));
	}

	// 2 bytes = 4 pixels
	// Image is composed of 8x8px tiles.
	// 4 tiles wide, 4 tiles tall.
	uint8_t tileBuf[8*8];
	const uint8_t *src = nds_icon_title.icon_data;

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			// Convert each tile to 8-bit color manually.
			for (int i = 0; i < 8*8; i += 2, src++) {
				tileBuf[i+0] = *src & 0x0F;
				tileBuf[i+1] = *src >> 4;
			}

			// Blit the tile to the main image buffer.
			BlitTile<uint8_t, 8, 8>((uint8_t*)icon->bits(), 32, tileBuf, x, y);
		}
	}

	// Finished decoding the icon.
	m_images[imageType] = icon;
	return 0;
}

}
