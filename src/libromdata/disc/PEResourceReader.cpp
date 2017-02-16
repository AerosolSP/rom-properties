/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PEResourceReader.cpp: Portable Executable resource reader.              *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "PEResourceReader.hpp"
#include "../exe_structs.h"

#include "byteswap.h"
#include "../file/IRpFile.hpp"

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <memory>
#include <vector>
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class PEResourceReaderPrivate
{
	public:
		PEResourceReaderPrivate(PEResourceReader *q, IRpFile *file,
			uint32_t rsrc_addr, uint32_t rsrc_size);

	private:
		RP_DISABLE_COPY(PEResourceReaderPrivate)
	protected:
		PEResourceReader *const q_ptr;

	public:
		// EXE file.
		IRpFile *file;

		// .rsrc section.
		uint32_t rsrc_addr;
		uint32_t rsrc_size;

		// Read position.
		int64_t pos;

		// Resource directory entry.
		struct ResDirEntry {
			uint16_t id;	// Resource ID.
			uint32_t addr;	// Address of the IMAGE_RESOURCE_DIRECTORY or
					// IMAGE_RESOURCE_DATA_ENTRY, relative to rsrc_addr.
					// NOTE: If the high bit is set, this is a subdirectory.
		};
		typedef ao::uvector<ResDirEntry> rsrc_dir_t;

		// Resource types. (Top-level directory.)
		rsrc_dir_t res_types;

		/**
		 * Load a resource directory.
		 *
		 * NOTE: Only numeric resources and/or subdirectories are loaded.
		 * Named resources and/or subdirectores are ignored.
		 *
		 * @param addr	[in] Starting address of the directory. (relative to the start of .rsrc)
		 * @param dir	[out] Resource directory.
		 * @return Number of entries loaded, or negative POSIX error code on error.
		 */
		int loadResDir(uint32_t addr, rsrc_dir_t &dir);
};

/** PEResourceReaderPrivate **/

PEResourceReaderPrivate::PEResourceReaderPrivate(
		PEResourceReader *q, IRpFile *file,
		uint32_t rsrc_addr, uint32_t rsrc_size)
	: q_ptr(q)
	, file(file)
	, rsrc_addr(rsrc_addr)
	, rsrc_size(rsrc_size)
	, pos(0)
{
	if (!file) {
		this->file = nullptr;
		q->m_lastError = -EBADF;
		return;
	} else if (rsrc_addr == 0 || rsrc_size == 0) {
		this->file = nullptr;
		q->m_lastError = -EIO;
		return;
	}

	// Validate the starting address and size.
	const int64_t fileSize = file->size();
	if ((int64_t)rsrc_addr >= fileSize) {
		// Starting address is past the end of the file.
		this->file = nullptr;
		q->m_lastError = -EIO;
		return;
	} else if (((int64_t)rsrc_addr + (int64_t)rsrc_size) > fileSize) {
		// Resource ends past the end of the file.
		this->file = nullptr;
		q->m_lastError = -EIO;
		return;
	}

	// Load the root resource directory.
	int ret = loadResDir(0, res_types);
	if (ret <= 0) {
		// No resources, or an error occurred.
		this->file = nullptr;
	}
}

/**
 * Load a resource directory.
 *
 * NOTE: Only numeric resources and/or subdirectories are loaded.
 * Named resources and/or subdirectores are ignored.
 *
 * @param addr	[in] Starting address of the directory. (relative to the start of .rsrc)
 * @param dir	[out] Resource directory.
 * @return Number of entries loaded, or negative POSIX error code on error.
 */
int PEResourceReaderPrivate::loadResDir(uint32_t addr, rsrc_dir_t &dir)
{
	RP_Q(PEResourceReader);

	IMAGE_RESOURCE_DIRECTORY root;
	int ret = file->seek(rsrc_addr + addr);
	if (ret != 0) {
		// Seek error.
		q->m_lastError = file->lastError();
		file = nullptr;
		return q->m_lastError;
	}
	size_t size = file->read(&root, sizeof(root));
	if (size != sizeof(root)) {
		// Read error;
		q->m_lastError = file->lastError();
		file = nullptr;
		return q->m_lastError;
	}

	// Total number of entries.
	unsigned int entryCount = le16_to_cpu(root.NumberOfNamedEntries) + le16_to_cpu(root.NumberOfIdEntries);
	assert(entryCount <= 64);
	if (entryCount > 64) {
		// Sanity check; constrain to 64 entries.
		entryCount = 64;
	}
	uint32_t szToRead = (uint32_t)(entryCount * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));
	unique_ptr<IMAGE_RESOURCE_DIRECTORY_ENTRY[]> irdEntries(new IMAGE_RESOURCE_DIRECTORY_ENTRY[entryCount]);
	size = file->read(irdEntries.get(), szToRead);
	if (size != szToRead) {
		// Read error.
		q->m_lastError = file->lastError();
		file = nullptr;
		return q->m_lastError;
	}

	// Read each directory header.
	uint32_t startOfResourceSection = addr + sizeof(root);
	dir.resize(entryCount);
	const IMAGE_RESOURCE_DIRECTORY_ENTRY *irdEntry = irdEntries.get();
	unsigned int entriesRead = 0;
	for (unsigned int i = 0; i < entryCount; i++, irdEntry++) {
		// Skipping any root directory entry that isn't an ID.
		uint32_t id = le32_to_cpu(irdEntry->Name);
		if (id > 0xFFFF) {
			// Not an ID.
			continue;
		}

		// Entry data.
		auto &entry = dir[entriesRead];
		entry.id = (uint16_t)id;
		// addr points to IMAGE_RESOURCE_DIRECTORY
		// or IMAGE_RESOURCE_DATA_ENTRY.
		entry.addr = startOfResourceSection + le32_to_cpu(irdEntry->OffsetToData);

		// Next entry.
		entriesRead++;
	}

	// Shrink the vector in case we skipped some types.
	dir.resize(entriesRead);
	return (int)entriesRead;
}

/** PEResourceReader **/

/**
 * Construct a PEResourceReader with the specified IRpFile.
 *
 * NOTE: The IRpFile *must* remain valid while this
 * PEResourceReader is open.
 *
 * @param file IRpFile.
 * @param rsrc_addr .rsrc section start offset.
 * @param rsrc_size .rsrc section size.
 */
PEResourceReader::PEResourceReader(IRpFile *file, uint32_t rsrc_addr, uint32_t rsrc_size)
	: d_ptr(new PEResourceReaderPrivate(this, file, rsrc_addr, rsrc_size))
{ }

PEResourceReader::~PEResourceReader()
{
	delete d_ptr;
}

/** IDiscReader **/

/**
 * Is the partition open?
 * This usually only returns false if an error occurred.
 * @return True if the partition is open; false if it isn't.
 */
bool PEResourceReader::isOpen(void) const
{
	RP_D(PEResourceReader);
	return (d->file && d->file->isOpen());
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t PEResourceReader::read(void *ptr, size_t size)
{
	RP_D(PEResourceReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
		m_lastError = EBADF;
		return 0;
	}

	// Are we already at the end of the .rsrc section?
	if (d->pos >= (int64_t)d->rsrc_size) {
		// End of the .rsrc section.
		return 0;
	}

	// Make sure d->pos + size <= d->rsrc_size.
	// If it isn't, we'll do a short read.
	if (d->pos + (int64_t)size >= (int64_t)d->rsrc_size) {
		size = (size_t)((int64_t)d->rsrc_size - d->pos);
	}

	// Seek to the position.
	int ret = d->file->seek(d->pos);
	if (ret != 0) {
		// Seek error.
		m_lastError = d->file->lastError();
		return 0;
	}
	// Read the data.
	size_t read = d->file->read(ptr, size);
	d->pos += read;
	m_lastError = d->file->lastError();
	return read;
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int PEResourceReader::seek(int64_t pos)
{
	RP_D(PEResourceReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file ||  !d->file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// Handle out-of-range cases.
	// TODO: How does POSIX behave?
	if (pos < 0) {
		d->pos = 0;
	} else if (pos >= (int64_t)d->rsrc_size) {
		d->pos = (int64_t)d->rsrc_size;
	} else {
		d->pos = pos;
	}
	return 0;
}

/**
 * Seek to the beginning of the partition.
 */
void PEResourceReader::rewind(void)
{
	seek(0);
}

/**
 * Get the data size.
 * This size does not include the partition header,
 * and it's adjusted to exclude hashes.
 * @return Data size, or -1 on error.
 */
int64_t PEResourceReader::size(void)
{
	// TODO: Errors?
	RP_D(PEResourceReader);
	return (int64_t)d->rsrc_size;
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
int64_t PEResourceReader::partition_size(void) const
{
	// TODO: Errors?
	RP_D(const PEResourceReader);
	return (int64_t)d->rsrc_size;
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
int64_t PEResourceReader::partition_size_used(void) const
{
	RP_D(const PEResourceReader);
	return (int64_t)d->rsrc_size;
}

/** Resource access functions. **/

/**
 * Open a resource.
 * @param type Resource type ID.
 * @param id Resource ID. (-1 for "first entry")
 * @param lang Language ID. (-1 for "first entry")
 * @return IRpFile*, or nullptr on error.
 */
IRpFile *PEResourceReader::open(uint16_t type, int id, int lang)
{
	// TODO
	assert(!"PEResourceReader::open() isn't implemented.");
	m_lastError = -ENOTSUP;
	return nullptr;
}

}
