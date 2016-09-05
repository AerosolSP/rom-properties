/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * FileSystem.hpp: File system functions.                                  *
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

#ifndef __ROMPROPERTIES_LIBCACHEMGR_FILESYSTEM_HPP__
#define __ROMPROPERTIES_LIBCACHEMGR_FILESYSTEM_HPP__

#include "libromdata/config.libromdata.h"
#include <stdint.h>

// access() macros.
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

// Windows doesn't define X_OK, W_OK, or R_OK.
#ifndef F_OK
#define F_OK 0
#endif
#ifndef X_OK
#define X_OK 1
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef R_OK
#define R_OK 4
#endif

// Directory separator characters.
#ifdef _WIN32
#define DIR_SEP_CHR '\\'
#else
#define DIR_SEP_CHR '/'
#endif

namespace LibCacheMgr {
namespace FileSystem {

/**
 * Recursively mkdir() subdirectories.
 *
 * The last element in the path will be ignored, so if
 * the entire pathname is a directory, a trailing slash
 * must be included.
 *
 * NOTE: Only native separators ('\\' on Windows, '/' on everything else)
 * are supported by this function.
 *
 * @param path Path to recursively mkdir. (last component is ignored)
 * @return 0 on success; non-zero on error.
 */
int rmkdir(const LibRomData::rp_string &path);

/**
 * Does a file exist?
 * @param pathname Pathname.
 * @param mode Mode.
 * @return 0 if the file exists with the specified mode; non-zero if not.
 */
int access(const LibRomData::rp_string &pathname, int mode);

/**
 * Get a file's size.
 * @param filename Filename.
 * @return Size on success; -1 on error.
 */
int64_t filesize(const LibRomData::rp_string &filename);

/**
 * Get the user's cache directory.
 * This is usually one of the following:
 * - WinXP: %APPDATA%\Local Settings
 * - WinVista: %LOCALAPPDATA%
 * - Linux: ~/.cache/
 *
 * @return User's cache directory, or empty string on error.
 */
const LibRomData::rp_string &getCacheDirectory(void);

/**
 * Set the modification timestamp of a file.
 * @param mtime Modification time.
 * @return 0 on success; negative POSIX error code on error.
 */
int set_mtime(const LibRomData::rp_string &filename, time_t mtime);

} }

#endif /* __ROMPROPERTIES_LIBCACHEMGR_FILESYSTEM_HPP__ */
