/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DiscReader.cpp: Basic disc reader interface.                            *
 * This class is a "null" interface that simply passes calls down to       *
 * libc's stdio functions.                                                 *
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

#include "DiscReader.hpp"
#include "file/IRpFile.hpp"

// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData {

DiscReader::DiscReader(IRpFile *file)
	: m_file(nullptr)
{
	if (!file)
		return;
	m_file = file->dup();
}

DiscReader::~DiscReader()
{
	delete m_file;
}

/**
 * Is the disc image open?
 * This usually only returns false if an error occurred.
 * @return True if the disc image is open; false if it isn't.
 */
bool DiscReader::isOpen(void) const
{
	return (m_file != nullptr);
}

/**
 * Read data from the disc image.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t DiscReader::read(void *ptr, size_t size)
{
	assert(m_file != nullptr);
	if (!m_file)
		return 0;
	return m_file->read(ptr, size);
}

/**
 * Set the disc image position.
 * @param pos Disc image position.
 * @return 0 on success; -1 on error.
 */
int DiscReader::seek(int64_t pos)
{
	assert(m_file != nullptr);
	if (!m_file)
		return -1;
	return m_file->seek(pos);
}

/**
 * Seek to the beginning of the disc image.
 */
void DiscReader::rewind(void)
{
	assert(m_file != nullptr);
	if (m_file) {
		m_file->rewind();
	}
}

/**
 * Get the disc image size.
 * @return Disc image size, or -1 on error.
 */
int64_t DiscReader::size(void) const
{
	assert(m_file != nullptr);
	if (!m_file)
		return -1;
	return m_file->fileSize();
}

}
