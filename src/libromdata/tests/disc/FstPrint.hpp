/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * FstPrint.hpp: FST printer.                                              *
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

// C++ includes.
#include <ostream>

namespace LibRpBase {
	class IFst;
}

namespace LibRomData {

struct FstFileCount {
	unsigned int dirs;
	unsigned int files;
};

/**
 * Print an FST to an ostream.
 * @param fst	[in] FST to print.
 * @param os	[in,out] ostream.
 * @param fc	[out,opt] Pointer to FstFileCount struct.
 *
 * If fc is nullptr, file count is printed to os.
 * Otherwise, file count is stored in fc.
 *
 * @return 0 on success; negative POSIX error code on error.
 */
int fstPrint(LibRpBase::IFst *fst, std::ostream &os, FstFileCount *fc = nullptr);

};
