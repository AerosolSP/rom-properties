/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SystemRegion.hpp: Get the system country code.                          *
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

#ifndef __LIBROMDATA_SYSTEMREGION_HPP__
#define __LIBROMDATA_SYSTEMREGION_HPP__

// C includes.
#include <stdint.h>

namespace LibRomData {

class SystemRegion
{
	private:
		// SystemRegion is a static class.
		SystemRegion();
		~SystemRegion();
		SystemRegion(const SystemRegion &other);
		SystemRegion &operator=(const SystemRegion &other);

	public:
		/**
		 * Get the system country code. (ISO-3166)
		 * This will always be an uppercase ASCII value.
		 *
		 * NOTE: Some newer country codes may use 3-character abbreviations.
		 * The abbreviation will always be aligned towards the LSB, e.g.
		 * 'US' will be 0x00005553.
		 *
		 * @return ISO-3166 country code as a uint32_t, or 0 on error.
		 */
		static uint32_t getCountryCode(void);
};

}

#endif /* __LIBROMDATA_SYSTEMREGION_HPP__ */
