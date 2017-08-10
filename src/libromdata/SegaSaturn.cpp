/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SegaSaturn.hpp: Sega Saturn disc image reader.                          *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "SegaSaturn.hpp"
#include "librpbase/RomData_p.hpp"

#include "data/SegaPublishers.hpp"
#include "saturn_structs.h"
#include "cdrom_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
using namespace LibRpBase;

// CD-ROM reader.
#include "disc/Cdrom2352Reader.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <ctime>
#include <cstring>

// C++ includes.
#include <memory>
#include <vector>
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class SegaSaturnPrivate : public RomDataPrivate
{
	public:
		SegaSaturnPrivate(SegaSaturn *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(SegaSaturnPrivate)

	public:
		/** RomFields **/

		// Peripherals. (RFT_BITFIELD)
		enum Saturn_Peripherals_Bitfield {
			SATURN_IOBF_CONTROL_PAD		= (1 << 0),	// Standard control pad
			SATURN_IOBF_ANALOG_CONTROLLER	= (1 << 1),	// Analog controller
			SATURN_IOBF_MOUSE		= (1 << 2),	// Mouse
			SATURN_IOBF_KEYBOARD		= (1 << 3),	// Keyboard
			SATURN_IOBF_STEERING		= (1 << 4),	// Steering controller
			SATURN_IOBF_MULTITAP		= (1 << 5),	// Multi-Tap
			SATURN_IOBF_LIGHT_GUN		= (1 << 6),	// Light Gun
			SATURN_IOBF_RAM_CARTRIDGE	= (1 << 7),	// RAM Cartridge
			SATURN_IOBF_3D_CONTROLLER	= (1 << 8),	// 3D Controller
			SATURN_IOBF_LINK_CABLE		= (1 << 9),	// Link Cable
			SATURN_IOBF_NETLINK		= (1 << 10),	// NetLink
			SATURN_IOBF_PACHINKO		= (1 << 11),	// Pachinko Controller
			SATURN_IOBF_FDD			= (1 << 12),	// Floppy Disk Drive
			SATURN_IOBF_ROM_CARTRIDGE	= (1 << 13),	// ROM Cartridge
			SATURN_IOBF_MPEG_CARD		= (1 << 14),	// MPEG Card
		};

		/** Internal ROM data. **/

		/**
		 * Parse the peripherals field.
		 * @param peripherals Peripherals field.
		 * @param size Size of peripherals.
		 * @return peripherals bitfield.
		 */
		static uint32_t parsePeripherals(const char *peripherals, int size);

	public:
		enum DiscType {
			DISC_UNKNOWN		= -1,	// Unknown ROM type.
			DISC_ISO_2048		= 0,	// ISO-9660, 2048-byte sectors.
			DISC_ISO_2352		= 1,	// ISO-9660, 2352-byte sectors.
		};

		// Disc type.
		int discType;

		// Disc header.
		Saturn_IP0000_BIN_t discHeader;

		/**
		 * Convert an ASCII release date to Unix time.
		 * @param ascii_date ASCII date. (Must be at least 8 chars.)
		 * @return Unix time, or -1 if an error occurred.
		 */
		static time_t ascii_release_date_to_unix_time(const char *ascii_date);
};

/** SegaSaturnPrivate **/

SegaSaturnPrivate::SegaSaturnPrivate(SegaSaturn *q, IRpFile *file)
	: super(q, file)
	, discType(DISC_UNKNOWN)
{
	// Clear the disc header struct.
	memset(&discHeader, 0, sizeof(discHeader));
}

/**
 * Parse the peripherals field.
 * @param peripherals Peripherals field.
 * @param size Size of peripherals.
 * @return peripherals bitfield.
 */
uint32_t SegaSaturnPrivate::parsePeripherals(const char *peripherals, int size)
{
	uint32_t ret = 0;
	for (int i = size-1; i >= 0; i--) {
		switch (peripherals[i]) {
			case SATURN_IO_CONTROL_PAD:
				ret |= SATURN_IOBF_CONTROL_PAD;
				break;
			case SATURN_IO_ANALOG_CONTROLLER:
				ret |= SATURN_IOBF_ANALOG_CONTROLLER;
				break;
			case SATURN_IO_MOUSE:
				ret |= SATURN_IOBF_MOUSE;
				break;
			case SATURN_IO_KEYBOARD:
				ret |= SATURN_IOBF_KEYBOARD;
				break;
			case SATURN_IO_STEERING:
				ret |= SATURN_IOBF_STEERING;
				break;
			case SATURN_IO_MULTITAP:
				ret |= SATURN_IOBF_MULTITAP;
				break;
			case SATURN_IO_LIGHT_GUN:
				ret |= SATURN_IOBF_LIGHT_GUN;
				break;
			case SATURN_IO_RAM_CARTRIDGE:
				ret |= SATURN_IOBF_RAM_CARTRIDGE;
				break;
			case SATURN_IO_3D_CONTROLLER:
				ret |= SATURN_IOBF_3D_CONTROLLER;
				break;
			case SATURN_IO_LINK_CABLE_JPN:
			case SATURN_IO_LINK_CABLE_USA:
				// TODO: Are these actually the same thing?
				ret |= SATURN_IOBF_LINK_CABLE;
				break;
			case SATURN_IO_NETLINK:
				ret |= SATURN_IOBF_NETLINK;
				break;
			case SATURN_IO_PACHINKO:
				ret |= SATURN_IOBF_PACHINKO;
				break;
			case SATURN_IO_FDD:
				ret |= SATURN_IOBF_FDD;
				break;
			case SATURN_IO_ROM_CARTRIDGE:
				ret |= SATURN_IOBF_ROM_CARTRIDGE;
				break;
			case SATURN_IO_MPEG_CARD:
				ret |= SATURN_IOBF_MPEG_CARD;
				break;
			default:
				break;
		}
	}

	return ret;
}

/**
 * Convert an ASCII release date to Unix time.
 * @param ascii_date ASCII date. (Must be at least 8 chars.)
 * @return Unix time, or -1 if an error occurred.
 */
time_t SegaSaturnPrivate::ascii_release_date_to_unix_time(const char *ascii_date)
{
	// Release date format: "YYYYMMDD"

	// Verify that all characters are digits.
	for (unsigned int i = 0; i < 8; i++) {
		if (!isdigit(ascii_date[i])) {
			// Invalid digit.
			return -1;
		}
	}
	// TODO: Verify that the remaining spaces are empty?

	// Convert from BCD to Unix time.
	// NOTE: struct tm has some oddities:
	// - tm_year: year - 1900
	// - tm_mon: 0 == January
	struct tm dctime;

	dctime.tm_year = ((ascii_date[0] & 0xF) * 1000) +
			 ((ascii_date[1] & 0xF) * 100) +
			 ((ascii_date[2] & 0xF) * 10) +
			  (ascii_date[3] & 0xF) - 1900;
	dctime.tm_mon  = ((ascii_date[4] & 0xF) * 10) +
			  (ascii_date[5] & 0xF) - 1;
	dctime.tm_mday = ((ascii_date[6] & 0xF) * 10) +
			  (ascii_date[7] & 0xF);

	// Time portion is empty.
	dctime.tm_hour = 0;
	dctime.tm_min = 0;
	dctime.tm_sec = 0;

	// tm_wday and tm_yday are output variables.
	dctime.tm_wday = 0;
	dctime.tm_yday = 0;
	dctime.tm_isdst = 0;

	// If conversion fails, this will return -1.
	return timegm(&dctime);
}

/** SegaSaturn **/

/**
 * Read a Sega Saturn disc image.
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
SegaSaturn::SegaSaturn(IRpFile *file)
	: super(new SegaSaturnPrivate(this, file))
{
	// This class handles disc images.
	RP_D(SegaSaturn);
	d->className = "SegaSaturn";
	d->fileType = FTYPE_DISC_IMAGE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the disc header.
	// NOTE: Reading 2352 bytes due to CD-ROM sector formats.
	CDROM_2352_Sector_t sector;
	d->file->rewind();
	size_t size = d->file->read(&sector, sizeof(sector));
	if (size != sizeof(sector))
		return;

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(sector);
	info.header.pData = reinterpret_cast<const uint8_t*>(&sector);
	info.ext = nullptr;	// Not needed for SegaSaturn.
	info.szFile = 0;	// Not needed for SegaSaturn.
	d->discType = isRomSupported_static(&info);

	if (d->discType < 0)
		return;

	switch (d->discType) {
		case SegaSaturnPrivate::DISC_ISO_2048:
			// 2048-byte sectors.
			// TODO: Determine session start address.
			memcpy(&d->discHeader, &sector, sizeof(d->discHeader));
			break;
		case SegaSaturnPrivate::DISC_ISO_2352:
			// 2352-byte sectors.
			// FIXME: Assuming Mode 1.
			memcpy(&d->discHeader, &sector.m1.data, sizeof(d->discHeader));
			break;
		default:
			// Unsupported.
			return;
	}
	d->isValid = true;
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int SegaSaturn::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(CDROM_2352_Sector_t))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for Sega Saturn HW and Maker ID.

	// 0x0000: 2048-byte sectors.
	const Saturn_IP0000_BIN_t *ip0000_bin = reinterpret_cast<const Saturn_IP0000_BIN_t*>(info->header.pData);
	if (!memcmp(ip0000_bin->hw_id, SATURN_IP0000_BIN_HW_ID, sizeof(ip0000_bin->hw_id))) {
		// Found HW ID at 0x0000.
		// This is a 2048-byte sector image.
		return SegaSaturnPrivate::DISC_ISO_2048;
	}

	// 0x0010: 2352-byte sectors;
	ip0000_bin = reinterpret_cast<const Saturn_IP0000_BIN_t*>(&info->header.pData[0x10]);
	if (!memcmp(ip0000_bin->hw_id, SATURN_IP0000_BIN_HW_ID, sizeof(ip0000_bin->hw_id))) {
		// Found HW ID at 0x0010.
		// Verify the sync bytes.
		if (Cdrom2352Reader::isDiscSupported_static(info->header.pData, info->header.size) >= 0) {
			// Found CD-ROM sync bytes.
			// This is a 2352-byte sector image.
			return SegaSaturnPrivate::DISC_ISO_2352;
		}
	}

	// TODO: Check for other formats, including CDI and NRG?

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int SegaSaturn::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *SegaSaturn::systemName(unsigned int type) const
{
	RP_D(const SegaSaturn);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Sega Saturn has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"SegaSaturn::systemName() array index optimization needs to be updated.");

	static const rp_char *const sysNames[4] = {
		_RP("Sega Saturn"), _RP("Saturn"), _RP("Sat"), nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const rp_char *const *SegaSaturn::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		_RP(".iso"),	// ISO-9660 (2048-byte)
		_RP(".bin"),	// Raw (2352-byte)

		// TODO: Add these formats?
		//_RP(".cdi"),	// DiscJuggler
		//_RP(".nrg"),	// Nero

		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const rp_char *const *SegaSaturn::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int SegaSaturn::loadFieldData(void)
{
	RP_D(SegaSaturn);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->discType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Sega Saturn disc header.
	const Saturn_IP0000_BIN_t *const discHeader = &d->discHeader;
	d->fields->reserve(8);	// Maximum of 8 fields.

	// Title. (TODO: Encoding?)
	d->fields->addField_string(_RP("Title"),
		latin1_to_rp_string(discHeader->title, sizeof(discHeader->title)),
		RomFields::STRF_TRIM_END);

	// Publisher.
	const rp_char *publisher = nullptr;
	if (!memcmp(discHeader->maker_id, SATURN_IP0000_BIN_MAKER_ID, sizeof(discHeader->maker_id))) {
		// First-party Sega title.
		publisher = _RP("Sega");
	} else if (!memcmp(discHeader->maker_id, "SEGA TP T-", 10)) {
		// This may be a third-party T-code.
		char *endptr;
		unsigned int t_code = (unsigned int)strtoul(&discHeader->maker_id[10], &endptr, 10);
		if (t_code != 0 &&
		    endptr > &discHeader->maker_id[10] &&
		    endptr <= &discHeader->maker_id[15])
		{
			// Valid T-code. Look up the publisher.
			publisher = SegaPublishers::lookup(t_code);
		}
	}

	if (publisher) {
		d->fields->addField_string(_RP("Publisher"), publisher);
	} else {
		// Unknown publisher.
		// List the field as-is.
		d->fields->addField_string(_RP("Publisher"),
			latin1_to_rp_string(discHeader->maker_id, sizeof(discHeader->maker_id)),
			RomFields::STRF_TRIM_END);
	}

	// TODO: Latin-1, cp1252, or Shift-JIS?

	// Product number.
	d->fields->addField_string(_RP("Product #"),
		latin1_to_rp_string(discHeader->product_number, sizeof(discHeader->product_number)),
		RomFields::STRF_TRIM_END);

	// Product version.
	d->fields->addField_string(_RP("Version"),
		latin1_to_rp_string(discHeader->product_version, sizeof(discHeader->product_version)),
		RomFields::STRF_TRIM_END);

	// Release date.
	time_t release_date = d->ascii_release_date_to_unix_time(discHeader->release_date);
	d->fields->addField_dateTime(_RP("Release Date"), release_date,
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_IS_UTC  // Date only.
	);

	// Region code.
	// Note that for Sega Saturn, each character is assigned to
	// a specific position, so European games will be "  E",
	// not "E  ".
	// FIXME: This doesn't seem to be correct...
	unsigned int region_code = 0;
	region_code  = (discHeader->area_symbols[0] == 'J');
	region_code |= (discHeader->area_symbols[1] == 'T') << 1;
	region_code |= (discHeader->area_symbols[2] == 'U') << 1;
	region_code |= (discHeader->area_symbols[3] == 'E') << 2;

	static const rp_char *const region_code_bitfield_names[] = {
		_RP("Japan"), _RP("Taiwan"), _RP("USA"), _RP("Europe")
	};
	vector<rp_string> *v_region_code_bitfield_names = RomFields::strArrayToVector(
		region_code_bitfield_names, ARRAY_SIZE(region_code_bitfield_names));
	d->fields->addField_bitfield(_RP("Region Code"),
		v_region_code_bitfield_names, 0, region_code);

	// Disc number.
	uint8_t disc_num = 0;
	uint8_t disc_total = 0;
	if (!memcmp(discHeader->device_info, "CD-", 3) &&
	    discHeader->device_info[4] == '/')
	{
		// "GD-ROM" is present.
		if (isdigit(discHeader->device_info[3]) &&
		    isdigit(discHeader->device_info[5]))
		{
			// Disc digits are present.
			disc_num = discHeader->device_info[3] & 0x0F;
			disc_total = discHeader->device_info[5] & 0x0F;
		}
	}

	if (disc_num != 0) {
		d->fields->addField_string(_RP("Disc #"),
			rp_sprintf("%u of %u", disc_num, disc_total));
	} else {
		d->fields->addField_string(_RP("Disc #"), _RP("Unknown"));
	}

	// Peripherals.
	static const rp_char *const peripherals_bitfield_names[] = {
		_RP("Control Pad"), _RP("Analog Controller"), _RP("Mouse"),
		_RP("Keyboard"), _RP("Steering Controller"), _RP("Multi-Tap"),
		_RP("Light Gun"), _RP("RAM Cartridge"), _RP("3D Controller"),
		_RP("Link Cable"), _RP("NetLink"), _RP("Pachinko"),
		_RP("Floppy Drive"), _RP("ROM Cartridge"), _RP("MPEG Card"),
	};
	vector<rp_string> *v_peripherals_bitfield_names = RomFields::strArrayToVector(
		peripherals_bitfield_names, ARRAY_SIZE(peripherals_bitfield_names));
	// Parse peripherals.
	uint32_t peripherals = d->parsePeripherals(discHeader->peripherals, sizeof(discHeader->peripherals));
	d->fields->addField_bitfield(_RP("Peripherals"),
		v_peripherals_bitfield_names, 3, peripherals);

	// Finished reading the field data.
	return (int)d->fields->count();
}

}
