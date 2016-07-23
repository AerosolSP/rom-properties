/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomData.hpp: ROM data base class.                                       *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_ROMDATA_HPP__
#define __ROMPROPERTIES_LIBROMDATA_ROMDATA_HPP__

#include "TextFuncs.hpp"
#include "RomFields.hpp"

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstdio>

// C++ includes.
#include <string>
#include <vector>

namespace LibRomData {

class rp_image;
class RomData
{
	protected:
		// TODO: Some abstraction to read the file directory
		// using a wrapper around FILE*, QFile, etc.
		// For now, just check the header.

		/**
		 * ROM data base class.
		 *
		 * A ROM file must be opened by the caller. The file handle
		 * will be dup()'d and must be kept open in order to load
		 * data from the ROM.
		 *
		 * To close the file, either delete this object or call close().
		 *
		 * NOTE: Check isValid() to determine if this is a valid ROM.
		 *
		 * In addition, subclasses must pass an array of RomFielDesc structs.
		 *
		 * @param file ROM file.
		 * @param fields Array of ROM Field descriptions.
		 * @param count Number of ROM Field descriptions.
		 */
		RomData(FILE *file, const RomFields::Desc *fields, int count);
	public:
		virtual ~RomData();

	private:
		RomData(const RomData &);
		RomData &operator=(const RomData &);

	public:
		/**
		 * Is this ROM valid?
		 * @return True if it is; false if it isn't.
		 */
		bool isValid(void) const;

		/**
		 * Close the opened file.
		 */
		void close(void);

	protected:
		/**
		 * Load field data.
		 * Called by RomData::fields() if the field data hasn't been loaded yet.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int loadFieldData(void) = 0;

		/**
		 * Load the internal icon.
		 * Called by RomData::icon() if the icon data hasn't been loaded yet.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int loadInternalIcon(void);

	public:
		/**
		 * Get the ROM Fields object.
		 * @return ROM Fields object.
		 */
		const RomFields *fields(void) const;

		/**
		 * Get the ROM's internal icon.
		 * @return Internal icon, or nullptr if the ROM doesn't have one.
		 */
		const rp_image *icon(void) const;

	protected:
		// TODO: Make a private class?
		bool m_isValid;			// Subclass must set this to true if the ROM is valid.
		FILE *m_file;			// Open file.
		RomFields *const m_fields;	// ROM fields.

		// Images.
		rp_image *m_icon;		// Internal icon.
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_ROMDATA_HPP__ */
