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
#include "byteswap.h"
#include <cstring>

namespace LibRomData {

/**
 * Mega Drive ROM header.
 * This matches the MD ROM header format exactly.
 * 
 * NOTE: Strings are NOT null-terminated!
 */
struct MD_RomHeader {
	char system[16];
	char copyright[16];
	char title_domestic[48];	// Japanese ROM name.
	char title_export[48];	// US/Europe ROM name.
	char serial[14];
	uint16_t checksum;
	char io_support[16];

	// ROM/RAM address information.
	uint32_t rom_start;
	uint32_t rom_end;
	uint32_t ram_start;
	uint32_t ram_end;

	// Save RAM information.
	// Info format: 'R', 'A', %1x1yz000, 0x20
	// x == 1 for backup (SRAM), 0 for not backup
	// yz == 10 for even addresses, 11 for odd addresses
	uint32_t sram_info;
	uint32_t sram_start;
	uint32_t sram_end;

	// Miscellaneous.
	char modem_info[12];
	char notes[40];
	char region_codes[16];
};

/**
 * Read a Sega Mega Drive ROM.
 * @param header ROM header. (Should be at least 65536+512 bytes.)
 * @param size Header size.
 *
 * Check isValid() to determine if this is a valid ROM.
 */
MegaDrive::MegaDrive(const uint8_t *header, size_t size)
	: m_isValid(false)
{
	// TODO: Handle SMD and other interleaved formats.
	// TODO: Handle Sega CD.
	// TODO: Store specific system type.

	// Check for certain strings at $100 and/or $101.
	// TODO: Better checks.
	static const char strchk[][17] = {
		"SEGA MEGA DRIVE ",
		"SEGA GENESIS    ",
		"SEGA PICO       ",
		"SEGA 32X        ",
	};

	// Header must be at *least* 0x200 bytes.
	if (size < 0x200)
		return;

	const MD_RomHeader *romHeader = reinterpret_cast<const MD_RomHeader*>(&header[0x100]);
	for (int i = 0; i < 4; i++) {
		if (!strncmp(romHeader->system, strchk[i], 16) ||
		    !strncmp(&romHeader->system[1], strchk[i], 15))
		{
			// Found a Mega Drive ROM.
			m_isValid = true;
			break;
		}
	}

	if (!m_isValid)
		return;

	// Read the rest of the header.
	// TODO: Trim the strings.
	m_system = std::string(romHeader->system, sizeof(romHeader->system));
	m_copyright = std::string(romHeader->copyright, sizeof(romHeader->copyright));
	m_title_domestic = std::string(romHeader->title_domestic, sizeof(romHeader->title_domestic));
	m_title_export = std::string(romHeader->title_export, sizeof(romHeader->title_export));
	m_serial = std::string(romHeader->serial, sizeof(romHeader->serial));
	// TODO: Parse company from the copyright line.
	m_checksum = be16_to_cpu(romHeader->checksum);

	// Parse I/O support.
	m_io_support = 0;
	for (int i = (int)sizeof(romHeader->io_support)-1; i >= 0; i--) {
		switch (romHeader->io_support[i]) {
			case 'J':
				m_io_support |= IO_JOYPAD_3;
				break;
			case '6':
				m_io_support |= IO_JOYPAD_6;
				break;
			case '0':
				m_io_support |= IO_JOYPAD_SMS;
				break;
			case '4':
				m_io_support |= IO_TEAM_PLAYER;
				break;
			case 'K':
				m_io_support |= IO_KEYBOARD;
				break;
			case 'R':
				m_io_support |= IO_SERIAL;
				break;
			case 'P':
				m_io_support |= IO_PRINTER;
				break;
			case 'T':
				m_io_support |= IO_TABLET;
				break;
			case 'B':
				m_io_support |= IO_TRACKBALL;
				break;
			case 'V':
				m_io_support |= IO_PADDLE;
				break;
			case 'F':
				m_io_support |= IO_FDD;
				break;
			case 'C':
				m_io_support |= IO_CDROM;
				break;
			case 'L':
				m_io_support |= IO_ACTIVATOR;
				break;
			case 'M':
				m_io_support |= IO_MEGA_MOUSE;
				break;
			default:
				break;
		}
	}

	// ROM/RAM addresses.
	m_rom_start = be32_to_cpu(romHeader->rom_start);
	m_rom_end   = be32_to_cpu(romHeader->rom_end);
	m_ram_start = be32_to_cpu(romHeader->ram_start);
	m_ram_end   = be32_to_cpu(romHeader->ram_end);

	// SRAM. (TODO)
	m_sram_start = 0;
	m_sram_end = 0;

	// Vectors.
	const uint32_t *vectors = reinterpret_cast<const uint32_t*>(header);
	m_entry_point = be32_to_cpu(vectors[1]);
	m_initial_sp = be32_to_cpu(vectors[0]);
}

/**
 * Is this ROM recognized as a Sega Mega Drive ROM?
 * @return True if it is; false if it isn't.
 */
bool MegaDrive::isValid(void) const
{
	return m_isValid;
}

}
