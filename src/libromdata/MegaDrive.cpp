/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDrive.cpp: Sega Mega Drive ROM reader.                              *
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

#include "MegaDrive.hpp"
#include "MegaDrivePublishers.hpp"
#include "MegaDriveRegions.hpp"
#include "md_structs.h"
#include "CopierFormats.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstring>

// C++ includes.
#include <vector>
using std::vector;

namespace LibRomData {

class MegaDrivePrivate
{
	public:
		MegaDrivePrivate();

	private:
		MegaDrivePrivate(const MegaDrivePrivate &other);
		MegaDrivePrivate &operator=(const MegaDrivePrivate &other);

	public:
		/** RomFields **/

		// I/O support. (RFT_BITFIELD)
		enum MD_IOSupport {
			MD_IO_JOYPAD_3		= (1 << 0),	// 3-button joypad
			MD_IO_JOYPAD_6		= (1 << 1),	// 6-button joypad
			MD_IO_JOYPAD_SMS	= (1 << 2),	// 2-button joypad (SMS)
			MD_IO_TEAM_PLAYER	= (1 << 3),	// Team Player
			MD_IO_KEYBOARD		= (1 << 4),	// Keyboard
			MD_IO_SERIAL		= (1 << 5),	// Serial (RS-232C)
			MD_IO_PRINTER		= (1 << 6),	// Printer
			MD_IO_TABLET		= (1 << 7),	// Tablet
			MD_IO_TRACKBALL		= (1 << 8),	// Trackball
			MD_IO_PADDLE		= (1 << 9),	// Paddle
			MD_IO_FDD		= (1 << 10),	// Floppy Drive
			MD_IO_CDROM		= (1 << 11),	// CD-ROM
			MD_IO_ACTIVATOR		= (1 << 12),	// Activator
			MD_IO_MEGA_MOUSE	= (1 << 13),	// Mega Mouse
		};
		static const rp_char *const md_io_bitfield_names[];
		static const RomFields::BitfieldDesc md_io_bitfield;

		// Region code. (RFT_BITFIELD)
		static const rp_char *const md_region_code_bitfield_names[];
		static const RomFields::BitfieldDesc md_region_code_bitfield;

		// Monospace string formatting.
		static const RomFields::StringDesc md_string_monospace;

		// ROM fields.
		static const struct RomFields::Desc md_fields[];

		/** Internal ROM data. **/

		/**
		 * Parse the I/O support field.
		 * @param io_support I/O support field.
		 * @param size Size of io_support.
		 * @return io_support bitfield.
		 */
		uint32_t parseIOSupport(const char *io_support, int size);

	public:
		enum MD_RomType {
			ROM_UNKNOWN = -1,		// Unknown ROM type.

			// Low byte: System ID.
			// (TODO: MCD Boot ROMs, other specialized types?)
			ROM_SYSTEM_MD = 0,		// Mega Drive
			ROM_SYSTEM_MCD = 1,		// Mega CD
			ROM_SYSTEM_32X = 2,		// Sega 32X
			ROM_SYSTEM_MCD32X = 3,		// Sega CD 32X
			ROM_SYSTEM_PICO = 4,		// Sega Pico
			ROM_SYSTEM_MAX = ROM_SYSTEM_PICO,
			ROM_SYSTEM_UNKNOWN = 0xFF,
			ROM_SYSTEM_MASK = 0xFF,

			// High byte: Image format.
			ROM_FORMAT_CART_BIN = (0 << 8),		// Cartridge: Binary format.
			ROM_FORMAT_CART_SMD = (1 << 8),		// Cartridge: SMD format.
			ROM_FORMAT_DISC_2048 = (2 << 8),	// Disc: 2048-byte sectors. (ISO)
			ROM_FORMAT_DISC_2352 = (3 << 8),	// Disc: 2352-byte sectors. (BIN)
			ROM_FORMAT_MAX = ROM_FORMAT_DISC_2352,
			ROM_FORMAT_UNKNOWN = (0xFF << 8),
			ROM_FORMAT_MASK = (0xFF << 8),
		};

		int romType;		// ROM type.
		uint32_t md_region;	// MD hexadecimal region code.

		// SMD bank size.
		static const uint32_t SMD_BLOCK_SIZE = 16384;

		/**
		 * Is this a disc?
		 * Discs don't have a vector table.
		 * @return True if this is a disc; false if not.
		 */
		inline bool isDisc(void)
		{
			int rfmt = romType & ROM_FORMAT_MASK;
			return (rfmt == ROM_FORMAT_DISC_2048 ||
				rfmt == ROM_FORMAT_DISC_2352);
		}

		/**
		 * Decode a Super Magic Drive interleaved block.
		 * @param dest	[out] Destination block. (Must be 16 KB.)
		 * @param src	[in] Source block. (Must be 16 KB.)
		 */
		static void decodeSMDBlock(uint8_t dest[SMD_BLOCK_SIZE], const uint8_t src[SMD_BLOCK_SIZE]);

	public:
		// ROM header.
		// NOTE: Must be byteswapped on access.
		M68K_VectorTable vectors;	// Interrupt vectors.
		MD_RomHeader romHeader;		// ROM header.
		SMD_Header smdHeader;		// SMD header.
};

/** MegaDrivePrivate **/

MegaDrivePrivate::MegaDrivePrivate()
	: romType(ROM_UNKNOWN)
	, md_region(0)
{ }

// I/O support bitfield.
const rp_char *const MegaDrivePrivate::md_io_bitfield_names[] = {
	_RP("Joypad"), _RP("6-button"), _RP("SMS Joypad"),
	_RP("Team Player"), _RP("Keyboard"), _RP("Serial I/O"),
	_RP("Printer"), _RP("Tablet"), _RP("Trackball"),
	_RP("Paddle"), _RP("Floppy Drive"), _RP("CD-ROM"),
	_RP("Activator"), _RP("Mega Mouse")
};

const RomFields::BitfieldDesc MegaDrivePrivate::md_io_bitfield = {
	ARRAY_SIZE(md_io_bitfield_names), 3, md_io_bitfield_names
};

// Region code.
const rp_char *const MegaDrivePrivate::md_region_code_bitfield_names[] = {
	_RP("Japan"), _RP("Asia"),
	_RP("USA"), _RP("Europe")
};

const RomFields::BitfieldDesc MegaDrivePrivate::md_region_code_bitfield = {
	ARRAY_SIZE(md_region_code_bitfield_names), 0, md_region_code_bitfield_names
};

// Monospace string formatting.
const RomFields::StringDesc MegaDrivePrivate::md_string_monospace = {
	RomFields::StringDesc::STRF_MONOSPACE
};

// ROM fields.
const struct RomFields::Desc MegaDrivePrivate::md_fields[] = {
	{_RP("System"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Copyright"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Publisher"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Domestic Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Export Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Serial Number"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Checksum"), RomFields::RFT_STRING, {&md_string_monospace}},
	{_RP("I/O Support"), RomFields::RFT_BITFIELD, {&md_io_bitfield}},
	{_RP("ROM Range"), RomFields::RFT_STRING, {&md_string_monospace}},
	{_RP("RAM Range"), RomFields::RFT_STRING, {&md_string_monospace}},
	{_RP("SRAM Range"), RomFields::RFT_STRING, {&md_string_monospace}},
	{_RP("Region Code"), RomFields::RFT_BITFIELD, {&md_region_code_bitfield}},
	{_RP("Entry Point"), RomFields::RFT_STRING, {&md_string_monospace}},
	{_RP("Initial SP"), RomFields::RFT_STRING, {&md_string_monospace}}
};

/** Internal ROM data. **/

/**
 * Parse the I/O support field.
 * @param io_support I/O support field.
 * @param size Size of io_support.
 * @return io_support bitfield.
 */
uint32_t MegaDrivePrivate::parseIOSupport(const char *io_support, int size)
{
	uint32_t ret = 0;
	for (int i = size-1; i >= 0; i--) {
		switch (io_support[i]) {
			case 'J':
				ret |= MD_IO_JOYPAD_3;
				break;
			case '6':
				ret |= MD_IO_JOYPAD_6;
				break;
			case '0':
				ret |= MD_IO_JOYPAD_SMS;
				break;
			case '4':
				ret |= MD_IO_TEAM_PLAYER;
				break;
			case 'K':
				ret |= MD_IO_KEYBOARD;
				break;
			case 'R':
				ret |= MD_IO_SERIAL;
				break;
			case 'P':
				ret |= MD_IO_PRINTER;
				break;
			case 'T':
				ret |= MD_IO_TABLET;
				break;
			case 'B':
				ret |= MD_IO_TRACKBALL;
				break;
			case 'V':
				ret |= MD_IO_PADDLE;
				break;
			case 'F':
				ret |= MD_IO_FDD;
				break;
			case 'C':
				ret |= MD_IO_CDROM;
				break;
			case 'L':
				ret |= MD_IO_ACTIVATOR;
				break;
			case 'M':
				ret |= MD_IO_MEGA_MOUSE;
				break;
			default:
				break;
		}
	}

	return ret;
}

/**
 * Decode a Super Magic Drive interleaved block.
 * @param dest	[out] Destination block. (Must be 16 KB.)
 * @param src	[in] Source block. (Must be 16 KB.)
 */
void MegaDrivePrivate::decodeSMDBlock(uint8_t dest[SMD_BLOCK_SIZE], const uint8_t src[SMD_BLOCK_SIZE])
{
	// TODO: Add the "restrict" keyword to both parameters?

	// First 8 KB of the source block is ODD bytes.
	const uint8_t *end_block = src + 8192;
	for (uint8_t *odd = dest + 1; src < end_block; odd += 16, src += 8) {
		odd[ 0] = src[0];
		odd[ 2] = src[1];
		odd[ 4] = src[2];
		odd[ 6] = src[3];
		odd[ 8] = src[4];
		odd[10] = src[5];
		odd[12] = src[6];
		odd[14] = src[7];
	}

	// Second 8 KB of the source block is EVEN bytes.
	end_block = src + 8192;
	for (uint8_t *even = dest; src < end_block; even += 16, src += 8) {
		even[ 0] = src[ 0];
		even[ 2] = src[ 1];
		even[ 4] = src[ 2];
		even[ 6] = src[ 3];
		even[ 8] = src[ 4];
		even[10] = src[ 5];
		even[12] = src[ 6];
		even[14] = src[ 7];
	}
}

/** MegaDrive **/

/**
 * Read a Sega Mega Drive ROM.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM file.
 */
MegaDrive::MegaDrive(IRpFile *file)
	: RomData(file, MegaDrivePrivate::md_fields, ARRAY_SIZE(MegaDrivePrivate::md_fields))
	, d(new MegaDrivePrivate())
{
	// TODO: Only validate that this is an MD ROM here.
	// Load fields elsewhere.
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Seek to the beginning of the file.
	m_file->rewind();

	// Read the ROM header. [0x400 bytes]
	static_assert(sizeof(MD_RomHeader) == MD_RomHeader_SIZE,
		"MD_RomHeader_SIZE is the wrong size. (Should be 256 bytes.)");
	static_assert(sizeof(M68K_VectorTable) == M68K_VectorTable_SIZE,
		"M68K_VectorTable is the wrong size. (Should be 256 bytes.)");
	uint8_t header[0x400];
	size_t size = m_file->read(header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = reinterpret_cast<const uint8_t*>(header);
	info.ext = nullptr;	// Not needed for MD.
	info.szFile = 0;	// Not needed for MD.
	d->romType = isRomSupported(&info);

	if (d->romType >= 0) {
		// Save the header for later.
		// TODO (remove before committing): Does gcc/msvc optimize this into a jump table?
		switch (d->romType & MegaDrivePrivate::ROM_FORMAT_MASK) {
			case MegaDrivePrivate::ROM_FORMAT_CART_BIN:
				m_fileType = FTYPE_ROM_IMAGE;

				// MD header is at 0x100.
				// Vector table is at 0.
				memcpy(&d->vectors,    header,        sizeof(d->vectors));
				memcpy(&d->romHeader, &header[0x100], sizeof(d->romHeader));
				break;

			case MegaDrivePrivate::ROM_FORMAT_CART_SMD: {
				m_fileType = FTYPE_ROM_IMAGE;

				// Save the SMD header.
				memcpy(&d->smdHeader, header, sizeof(d->smdHeader));

				// First bank needs to be deinterleaved.
				uint8_t smd_data[MegaDrivePrivate::SMD_BLOCK_SIZE];
				uint8_t bin_data[MegaDrivePrivate::SMD_BLOCK_SIZE];
				m_file->seek(512);
				size = m_file->read(smd_data, sizeof(smd_data));
				if (size != sizeof(smd_data)) {
					// Short read. ROM is invalid.
					d->romType = MegaDrivePrivate::ROM_UNKNOWN;
					break;
				}

				// Decode the SMD block.
				d->decodeSMDBlock(bin_data, smd_data);

				// MD header is at 0x100.
				// Vector table is at 0.
				memcpy(&d->vectors,    bin_data,        sizeof(d->vectors));
				memcpy(&d->romHeader, &bin_data[0x100], sizeof(d->romHeader));
				break;
			}

			case MegaDrivePrivate::ROM_FORMAT_DISC_2048:
				m_fileType = FTYPE_DISC_IMAGE;

				// MCD-specific header is at 0. [TODO]
				// MD-style header is at 0x100.
				// No vector table is present on the disc.
				memcpy(&d->romHeader, &header[0x100], sizeof(d->romHeader));
				break;

			case MegaDrivePrivate::ROM_FORMAT_DISC_2352:
				m_fileType = FTYPE_DISC_IMAGE;

				// MCD-specific header is at 0x10. [TODO]
				// MD-style header is at 0x110.
				// No vector table is present on the disc.
				memcpy(&d->romHeader, &header[0x110], sizeof(d->romHeader));
				break;

			case MegaDrivePrivate::ROM_FORMAT_UNKNOWN:
			default:
				m_fileType = FTYPE_UNKNOWN;
				d->romType = MegaDrivePrivate::ROM_UNKNOWN;
				break;
		}
	}

	m_isValid = (d->romType >= 0);
	if (m_isValid) {
		// Parse the MD region code.
		d->md_region = MegaDriveRegions::parseRegionCodes(
			d->romHeader.region_codes, sizeof(d->romHeader.region_codes));
	}
}

MegaDrive::~MegaDrive()
{
	delete d;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int MegaDrive::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 0x200)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// ROM header.
	const uint8_t *const pHeader = info->header.pData;

	// Magic strings.
	static const char sega_magic[4] = {'S','E','G','A'};
	static const char segacd_magic[16] =
		{'S','E','G','A','D','I','S','C','S','Y','S','T','E','M',' ',' '};;

	static const struct {
		const char system_name[16];
		uint32_t system_id;
	} cart_magic[] = {
		{{'S','E','G','A',' ','P','I','C','O',' ',' ',' ',' ',' ',' ',' '}, MegaDrivePrivate::ROM_SYSTEM_PICO},
		{{'S','E','G','A',' ','3','2','X',' ',' ',' ',' ',' ',' ',' ',' '}, MegaDrivePrivate::ROM_SYSTEM_32X},
		{{'S','E','G','A',' ','M','E','G','A',' ','D','R','I','V','E',' '}, MegaDrivePrivate::ROM_SYSTEM_MD},
		{{'S','E','G','A',' ','G','E','N','E','S','I','S',' ',' ',' ',' '}, MegaDrivePrivate::ROM_SYSTEM_MD},
	};

	if (info->header.size >= 0x200) {
		// Check for Sega CD.
		// TODO: Gens/GS II lists "ISO/2048", "ISO/2352",
		// "BIN/2048", and "BIN/2352". I don't think that's
		// right; there should only be 2048 and 2352.
		// TODO: Detect Sega CD 32X.
		if (!memcmp(&pHeader[0x0010], segacd_magic, sizeof(segacd_magic))) {
			// Found a Sega CD disc image. (2352-byte sectors)
			return MegaDrivePrivate::ROM_SYSTEM_MCD |
			       MegaDrivePrivate::ROM_FORMAT_DISC_2352;
		} else if (!memcmp(&pHeader[0x0000], segacd_magic, sizeof(segacd_magic))) {
			// Found a Sega CD disc image. (2048-byte sectors)
			return MegaDrivePrivate::ROM_SYSTEM_MCD |
			       MegaDrivePrivate::ROM_FORMAT_DISC_2048;
		}

		// Check for SMD format. (Mega Drive only)
		if (info->header.size >= 0x300) {
			// Check if "SEGA" is in the header in the correct place
			// for a plain binary ROM.
			if (memcmp(&pHeader[0x100], sega_magic, sizeof(sega_magic)) != 0 &&
			    memcmp(&pHeader[0x101], sega_magic, sizeof(sega_magic)) != 0)
			{
				// "SEGA" is not in the header. This might be SMD.
				const SMD_Header *smdHeader = reinterpret_cast<const SMD_Header*>(pHeader);
				if (smdHeader->id[0] == 0xAA && smdHeader->id[1] == 0xBB &&
				    smdHeader->smd.file_data_type == SMD_FDT_68K_PROGRAM &&
				    smdHeader->file_type == SMD_FT_SMD_GAME_FILE)
				{
					// This is an SMD-format ROM.
					// TODO: Show extended information from the SMD header,
					// including "split" and other stuff?
					return MegaDrivePrivate::ROM_SYSTEM_MD |
					       MegaDrivePrivate::ROM_FORMAT_CART_SMD;
				}
			}
		}

		// Check for other MD-based cartridge formats.
		for (int i = 0; i < ARRAY_SIZE(cart_magic); i++) {
			if (!memcmp(&pHeader[0x100], cart_magic[i].system_name, 16) ||
			    !memcmp(&pHeader[0x101], cart_magic[i].system_name, 15))
			{
				// Found a matching system name.
				return MegaDrivePrivate::ROM_FORMAT_CART_BIN | cart_magic[i].system_id;
			}
		}
	}

	// Not supported.
	return MegaDrivePrivate::ROM_UNKNOWN;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int MegaDrive::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *MegaDrive::systemName(uint32_t type) const
{
	if (!m_isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// FIXME: Lots of system names and regions to check.
	// Also, games can be region-free, so we need to check
	// against the host system's locale.
	// For now, just use the generic "Mega Drive".

	static_assert(SYSNAME_TYPE_MASK == 3,
		"MegaDrive::systemName() array index optimization needs to be updated.");

	uint32_t romSys = (d->romType & MegaDrivePrivate::ROM_SYSTEM_MASK);
	if (romSys > MegaDrivePrivate::ROM_SYSTEM_MAX) {
		// Invalid system type. Default to MD.
		romSys = MegaDrivePrivate::ROM_SYSTEM_MD;
	}

	// sysNames[] bitfield:
	// - Bits 0-1: Type. (short, long, abbreviation)
	// - Bits 2-4: System type.
	uint32_t idx = (romSys << 2) | (type & SYSNAME_TYPE_MASK);
	if (idx >= 20) {
		// Invalid index...
		idx &= SYSNAME_TYPE_MASK;
	}

	static_assert(SYSNAME_REGION_MASK == (1 << 2),
		"MegaDrive::systemName() region type optimization needs to be updated.");
	if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_GENERIC) {
		// Generic system name.
		static const rp_char *const sysNames[20] = {
			_RP("Sega Mega Drive"), _RP("Mega Drive"), _RP("MD"), nullptr,
			_RP("Sega Mega CD"), _RP("Mega CD"), _RP("MCD"), nullptr,
			_RP("Sega 32X"), _RP("Sega 32X"), _RP("32X"), nullptr,
			_RP("Sega Mega CD 32X"), _RP("Mega CD 32X"), _RP("MCD32X"), nullptr,
			_RP("Sega Pico"), _RP("Pico"), _RP("Pico"), nullptr
		};
		return sysNames[idx];
	}

	// Get the system branding region.
	const MegaDriveRegions::MD_BrandingRegion md_bregion =
		MegaDriveRegions::getBrandingRegion(d->md_region);
	switch (md_bregion) {
		case MegaDriveRegions::MD_BREGION_JAPAN:
		default: {
			static const rp_char *const sysNames_JP[20] = {
				_RP("Sega Mega Drive"), _RP("Mega Drive"), _RP("MD"), nullptr,
				_RP("Sega Mega CD"), _RP("Mega CD"), _RP("MCD"), nullptr,
				_RP("Sega Super 32X"), _RP("Super 32X"), _RP("32X"), nullptr,
				_RP("Sega Mega CD 32X"), _RP("Mega CD 32X"), _RP("MCD32X"), nullptr,
				_RP("Sega Kids Computer Pico"), _RP("Kids Computer Pico"), _RP("Pico"), nullptr
			};
			return sysNames_JP[idx];
		}

		case MegaDriveRegions::MD_BREGION_USA: {
			static const rp_char *const sysNames_US[20] = {
				// TODO: "MD" or "Gen"?
				_RP("Sega Genesis"), _RP("Genesis"), _RP("MD"), nullptr,
				_RP("Sega CD"), _RP("Sega CD"), _RP("MCD"), nullptr,
				_RP("Sega 32X"), _RP("Sega 32X"), _RP("32X"), nullptr,
				_RP("Sega CD 32X"), _RP("Sega CD 32X"), _RP("MCD32X"), nullptr,
				_RP("Sega Pico"), _RP("Pico"), _RP("Pico"), nullptr
			};
			return sysNames_US[idx];
		}

		case MegaDriveRegions::MD_BREGION_EUROPE: {
			static const rp_char *const sysNames_EU[20] = {
				_RP("Sega Mega Drive"), _RP("Mega Drive"), _RP("MD"), nullptr,
				_RP("Sega Mega CD"), _RP("Mega CD"), _RP("MCD"), nullptr,
				_RP("Sega Mega Drive 32X"), _RP("Mega Drive 32X"), _RP("32X"), nullptr,
				_RP("Sega Mega CD 32X"), _RP("Sega Mega CD 32X"), _RP("MCD32X"), nullptr,
				_RP("Sega Pico"), _RP("Pico"), _RP("Pico"), nullptr
			};
			return sysNames_EU[idx];
		}

		case MegaDriveRegions::MD_BREGION_SOUTH_KOREA: {
			static const rp_char *const sysNames_KR[20] = {
				// TODO: "MD" or something else?
				_RP("Samsung Super Aladdin Boy"), _RP("Super Aladdin Boy"), _RP("MD"), nullptr,
				_RP("Samsung CD Aladdin Boy"), _RP("CD Aladdin Boy"), _RP("MCD"), nullptr,
				_RP("Samsung Super 32X"), _RP("Super 32X"), _RP("32X"), nullptr,
				_RP("Sega Mega CD 32X"), _RP("Sega Mega CD 32X"), _RP("MCD32X"), nullptr,
				_RP("Sega Pico"), _RP("Pico"), _RP("Pico"), nullptr
			};
			return sysNames_KR[idx];
		}

		case MegaDriveRegions::MD_BREGION_BRAZIL: {
			static const rp_char *const sysNames_BR[20] = {
				_RP("Sega Mega Drive"), _RP("Mega Drive"), _RP("MD"), nullptr,
				_RP("Sega CD"), _RP("Sega CD"), _RP("MCD"), nullptr,
				_RP("Sega Mega 32X"), _RP("Mega 32X"), _RP("32X"), nullptr,
				_RP("Sega CD 32X"), _RP("Sega CD 32X"), _RP("MCD32X"), nullptr,
				_RP("Sega Pico"), _RP("Pico"), _RP("Pico"), nullptr
			};
			return sysNames_BR[idx];
		}
	}

	// Should not get here...
	return nullptr;
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
vector<const rp_char*> MegaDrive::supportedFileExtensions_static(void)
{
	// NOTE: Not including ".md" due to conflicts with Markdown.
	// TODO: Add ".bin" later? (Too generic, though...)
	vector<const rp_char*> ret;
	ret.reserve(4);
	ret.push_back(_RP(".gen"));
	ret.push_back(_RP(".smd"));
	ret.push_back(_RP(".32x"));
	ret.push_back(_RP(".pco"));
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
vector<const rp_char*> MegaDrive::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int MegaDrive::loadFieldData(void)
{
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!m_file || !m_file->isOpen()) {
		// File isn't open.
		// NOTE: We already loaded the header,
		// so *maybe* this is okay?
		return -EBADF;
	} else if (!m_isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// MD ROM header, excluding the vector table.
	const MD_RomHeader *romHeader = &d->romHeader;

	// Read the strings from the header.
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->system, sizeof(romHeader->system)));
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->copyright, sizeof(romHeader->copyright)));

	// Determine the publisher.
	// Formats in the copyright line:
	// - "(C)SEGA"
	// - "(C)T-xx"
	// - "(C)T-xxx"
	// - "(C)Txxx"
	const rp_char *publisher = nullptr;
	unsigned int t_code = 0;
	if (!memcmp(romHeader->copyright, "(C)SEGA", 7)) {
		// Sega first-party game.
		publisher = _RP("Sega");
	} else if (!memcmp(romHeader->copyright, "(C)T", 4)) {
		// Third-party game.
		int start = 4;
		if (romHeader->copyright[4] == '-')
			start++;
		char *endptr;
		t_code = strtoul(&romHeader->copyright[start], &endptr, 10);
		if (t_code != 0 &&
		    endptr > &romHeader->copyright[start] &&
		    endptr < &romHeader->copyright[start+3])
		{
			// Valid T-code. Look up the publisher.
			publisher = MegaDrivePublishers::lookup(t_code);
		}
	}

	if (publisher) {
		// Publisher identified.
		m_fields->addData_string(publisher);
	} else if (t_code > 0) {
		// Unknown publisher, but there is a valid T code.
		char buf[16];
		int len = snprintf(buf, sizeof(buf), "T-%u", t_code);
		if (len > (int)sizeof(buf))
			len = sizeof(buf);
		m_fields->addData_string(len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
	} else {
		// Unknown publisher.
		m_fields->addData_string(_RP("Unknown"));
	}

	// Titles, serial number, and checksum.
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->title_domestic, sizeof(romHeader->title_domestic)));
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->title_export, sizeof(romHeader->title_export)));
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->serial, sizeof(romHeader->serial)));
	if (!d->isDisc()) {
		m_fields->addData_string_numeric(be16_to_cpu(romHeader->checksum), RomFields::FB_HEX, 4);
	} else {
		// Checksum is not valid in Mega CD headers.
		m_fields->addData_invalid();
	}

	// Parse I/O support.
	uint32_t io_support = d->parseIOSupport(romHeader->io_support, sizeof(romHeader->io_support));
	m_fields->addData_bitfield(io_support);

	if (!d->isDisc()) {
		// ROM range.
		m_fields->addData_string_address_range(
				be32_to_cpu(romHeader->rom_start),
				be32_to_cpu(romHeader->rom_end), 8);

		// RAM range.
		m_fields->addData_string_address_range(
				be32_to_cpu(romHeader->ram_start),
				be32_to_cpu(romHeader->ram_end), 8);

		// SRAM range.
		// Info format: 'R', 'A', %1x1yz000, 0x20
		const uint32_t sram_info = be32_to_cpu(romHeader->sram_info);
		if ((sram_info & 0xFFFFA7FF) == 0x5241A020) {
			// SRAM is present.
			// x == 1 for backup (SRAM), 0 for not backup
			// yz == 10 for even addresses, 11 for odd addresses
			// TODO: Print the 'x' bit.
			const rp_char *suffix;
			switch ((sram_info >> (8+3)) & 0x03) {
				case 2:
					suffix = _RP("(even only)");
					break;
				case 3:
					suffix = _RP("(odd only)");
					break;
				default:
					// TODO: Are both alternates 16-bit?
					suffix = _RP("(16-bit)");
					break;
			}

			m_fields->addData_string_address_range(
					be32_to_cpu(romHeader->sram_start),
					be32_to_cpu(romHeader->sram_end),
					suffix, 8);
		} else {
			// TODO: Non-monospaced.
			m_fields->addData_string(_RP("None"));
		}
	} else {
		// ROM, RAM, and SRAM ranges are not valid in Mega CD headers.
		m_fields->addData_invalid();
		m_fields->addData_invalid();
		m_fields->addData_invalid();
	}

	// Region codes.
	// TODO: Validate the Mega CD security program?
	m_fields->addData_bitfield(d->md_region);

	// Vectors.
	if (!d->isDisc()) {
		m_fields->addData_string_numeric(
			be32_to_cpu(d->vectors.initial_pc), RomFields::FB_HEX, 8);
		m_fields->addData_string_numeric(
			be32_to_cpu(d->vectors.initial_sp), RomFields::FB_HEX, 8);
	} else {
		// Discs don't have vector tables.
		// Add dummy entries for the vectors.
		m_fields->addData_invalid();
		m_fields->addData_invalid();
	}

	// Finished reading the field data.
	return (int)m_fields->count();
}

}
