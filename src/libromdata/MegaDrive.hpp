/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDrive.hpp: Sega Mega Drive ROM reader.                              *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_MEGADRIVE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_MEGADRIVE_HPP__

#include <stdint.h>
#include <string>
#include "TextFuncs.hpp"

#include "RomData.hpp"

namespace LibRomData {

class MegaDrive : public RomData
{
	public:
		// TODO: Some abstraction to read the file directory
		// using a wrapper around FILE*, QFile, etc.
		// For now, just check the header.

		/**
		 * Read a Sega Mega Drive ROM.
		 * @param header ROM header. (Should be at least 65536+512 bytes.)
		 * @param size Header size.
		 *
		 * Check isValid() to determine if this is a valid ROM.
		 */
		MegaDrive(const uint8_t *header, size_t size);

	private:
		MegaDrive(const MegaDrive &);
		MegaDrive &operator=(const MegaDrive &);

	private:
		// Third-party company list.
		// Reference: http://segaretro.org/Third-party_T-series_codes
		struct MD_ThirdParty {
			unsigned int t_code;
			const rp_char *publisher;
		};
		static const MD_ThirdParty MD_ThirdParty_List[];
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_MEGADRIVE_HPP__ */
