/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiPartition.hpp: Wii partition reader.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_WIIPARTITION_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_WIIPARTITION_HPP__

#include "IPartition.hpp"
#include "GcnFst.hpp"

namespace LibRomData {

class WiiPartitionPrivate;
class WiiPartition : public IPartition
{
	public:
		/**
		 * Construct a WiiPartition with the specified IDiscReader.
		 * NOTE: The IDiscReader *must* remain valid while this
		 * WiiPartition is open.
		 * @param discReader IDiscReader.
		 * @param partition_offset Partition start offset.
		 */
		WiiPartition(IDiscReader *discReader, int64_t partition_offset);
		~WiiPartition();

	private:
		WiiPartition(const WiiPartition &);
		WiiPartition &operator=(const WiiPartition &);
	private:
		friend class WiiPartitionPrivate;
		WiiPartitionPrivate *const d;

	public:
		/** IDiscReader **/

		/**
		 * Is the partition open?
		 * This usually only returns false if an error occurred.
		 * @return True if the partition is open; false if it isn't.
		 */
		virtual bool isOpen(void) const override;

		/**
		 * Get the last error.
		 * @return Last POSIX error, or 0 if no error.
		 */
		virtual int lastError(void) const override;

		/**
		 * Clear the last error.
		 */
		virtual void clearError(void) override;

		/**
		 * Read data from the partition.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		virtual size_t read(void *ptr, size_t size) override;

		/**
		 * Set the partition position.
		 * @param pos Partition position.
		 * @return 0 on success; -1 on error.
		 */
		virtual int seek(int64_t pos) override;

		/**
		 * Seek to the beginning of the partition.
		 */
		virtual void rewind(void) override;

		/**
		 * Get the data size.
		 * This size does not include the partition header,
		 * and it's adjusted to exclude hashes.
		 * @return Data size, or -1 on error.
		 */
		virtual int64_t size(void) const override;

		/** IPartition **/

		/**
		 * Get the partition size.
		 * This size includes the partition header and hashes.
		 * @return Partition size, or -1 on error.
		 */
		virtual int64_t partition_size(void) const override;

		/** WiiPartition **/

		enum EncInitStatus {
			ENCINIT_OK = 0,
			ENCINIT_UNKNOWN,
			ENCINIT_DISABLED,		// ENABLE_DECRYPTION disabled.
			ENCINIT_INVALID_KEY_IDX,	// Invalid common key index in the disc header.
			ENCINIT_NO_KEYFILE,		// keys.conf not found.
			ENCINIT_MISSING_KEY,		// Required key not found.
			ENCINIT_CIPHER_ERROR,		// Could not initialize the cipher.
			ENCINIT_INCORRECT_KEY,		// Key is incorrect.
		};

		/**
		 * Encryption initialization status.
		 * @return Encryption initialization status.
		 */
		EncInitStatus encInitStatus(void) const;

		/** GcnFst wrapper functions. **/

		/**
		 * Open a directory.
		 * @param path	[in] Directory path. [TODO; always reads "/" right now.]
		 * @return FstDir*, or nullptr on error.
		 */
		GcnFst::FstDir *opendir(const rp_char *path);

		/**
		 * Open a directory.
		 * @param path	[in] Directory path. [TODO; always reads "/" right now.]
		 * @return FstDir*, or nullptr on error.
		 */
		inline GcnFst::FstDir *opendir(const LibRomData::rp_string &path)
		{
			return opendir(path.c_str());
		}

		/**
		 * Read a directory entry.
		 * @param dirp FstDir pointer.
		 * @return FstDirEntry*, or nullptr if end of directory or on error.
		 * (TODO: Add lastError()?)
		 */
		GcnFst::FstDirEntry *readdir(GcnFst::FstDir *dirp);

		/**
		 * Close an opened directory.
		 * @param dirp FstDir pointer.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int closedir(GcnFst::FstDir *dirp);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_WIIPARTITION_HPP__ */
