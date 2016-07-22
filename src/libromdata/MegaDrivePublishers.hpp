/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDrivePublishers.cpp: Sega Mega Drive third-party publishers list.    *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_MEGADRIVEPUBLISHERS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_MEGADRIVEPUBLISHERS_HPP__

#include "TextFuncs.hpp"

namespace LibRomData {

class MegaDrivePublishers
{
	private:
		MegaDrivePublishers();
		~MegaDrivePublishers();
	private:
		MegaDrivePublishers(const MegaDrivePublishers &);
		MegaDrivePublishers &operator=(const MegaDrivePublishers &);

	private:
		/**
		 * Sega Mega Drive third-party publisher list.
		 * Reference: http://segaretro.org/Third-party_T-series_codes
		 */
		struct ThirdPartyList {
			unsigned int t_code;
			const rp_char *publisher;
		};
		static const ThirdPartyList ms_thirdPartyList[];

	public:
		/**
		 * Comparison function for bsearch().
		 * @param a
		 * @param b
		 * @return
		 */
		static int compar(const void *a, const void *b);

		/**
		 * Look up a company code.
		 * @param code Company code.
		 * @return Publisher, or nullptr if not found.
		 */
		static const rp_char *lookup(unsigned int code);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_NINTENDOPUBLISHERS_HPP__ */
