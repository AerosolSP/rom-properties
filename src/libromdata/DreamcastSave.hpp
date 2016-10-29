/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DreamcastSave.hpp: Sega Dreamcast save file reader.                     *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DREAMCASTSAVE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DREAMCASTSAVE_HPP__

#include <stdint.h>
#include <string>
#include "TextFuncs.hpp"

#include "RomData.hpp"

namespace LibRomData {

class DreamcastSavePrivate;
class DreamcastSave : public RomData
{
	public:
		/**
		 * Read a Sega Dreamcast save file.
		 *
		 * A save file must be opened by the caller. The file handle
		 * will be dup()'d and must be kept open in order to load
		 * data from the disc image.
		 *
		 * To close the file, either delete this object or call close().
		 *
		 * NOTE: Check isValid() to determine if this is a valid save file.
		 *
		 * @param file Open save file.
		 */
		explicit DreamcastSave(IRpFile *file);

		/**
		 * Read a Sega Dreamcast save file. (.VMI+.VMS pair)
		 *
		 * This constructor requires two files:
		 * - .VMS file (main save file)
		 * - .VMI file (directory entry)
		 *
		 * Both files will be dup()'d.
		 * The .VMS file will be used as the main file for the RomData class.
		 *
		 * To close the files, either delete this object or call close().
		 * NOTE: Check isValid() to determine if this is a valid save file.
		 *
		 * @param vms_file Open .VMS save file.
		 * @param vmi_file Open .VMI save file.
		 */
		DreamcastSave(IRpFile *vms_file, IRpFile *vmi_file);

		virtual ~DreamcastSave();

	private:
		typedef RomData super;
		DreamcastSave(const DreamcastSave &other);
		DreamcastSave &operator=(const DreamcastSave &other);

	private:
		friend class DreamcastSavePrivate;
		DreamcastSavePrivate *const d;

	public:
		/** ROM detection functions. **/

		/**
		 * Is a ROM image supported by this class?
		 * @param info DetectInfo containing ROM detection information.
		 * @return Class-specific system ID (>= 0) if supported; -1 if not.
		 */
		static int isRomSupported_static(const DetectInfo *info);

		/**
		 * Is a ROM image supported by this object?
		 * @param info DetectInfo containing ROM detection information.
		 * @return Class-specific system ID (>= 0) if supported; -1 if not.
		 */
		virtual int isRomSupported(const DetectInfo *info) const final;

		/**
		 * Get the name of the system the loaded ROM is designed for.
		 * @param type System name type. (See the SystemName enum.)
		 * @return System name, or nullptr if type is invalid.
		 */
		virtual const rp_char *systemName(uint32_t type) const final;

	public:
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
		static std::vector<const rp_char*> supportedFileExtensions_static(void);

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
		virtual std::vector<const rp_char*> supportedFileExtensions(void) const final;

		/**
		 * Get a bitfield of image types this class can retrieve.
		 * @return Bitfield of supported image types. (ImageTypesBF)
		 */
		static uint32_t supportedImageTypes_static(void);

		/**
		 * Get a bitfield of image types this class can retrieve.
		 * @return Bitfield of supported image types. (ImageTypesBF)
		 */
		virtual uint32_t supportedImageTypes(void) const final;

	protected:
		/**
		 * Load field data.
		 * Called by RomData::fields() if the field data hasn't been loaded yet.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int loadFieldData(void) final;

		/**
		 * Load an internal image.
		 * Called by RomData::image() if the image data hasn't been loaded yet.
		 * @param imageType Image type to load.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int loadInternalImage(ImageType imageType) final;

	public:
		/**
		 * Get the animated icon data.
		 *
		 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
		 * object has an animated icon.
		 *
		 * @return Animated icon data, or nullptr if no animated icon is present.
		 */
		virtual const IconAnimData *iconAnimData(void) const final;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DREAMCASTSAVE_HPP__ */
