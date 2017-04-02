/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NCCHReader.cpp: Nintendo 3DS NCCH reader.                               *
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

#include "NCCHReader.hpp"
#include "config.libromdata.h"

#include "byteswap.h"
#include "n3ds_structs.h"

#include "file/IRpFile.hpp"
#include "PartitionFile.hpp"

#ifdef ENABLE_DECRYPTION
#include "crypto/AesCipherFactory.hpp"
#include "crypto/IAesCipher.hpp"
#include "crypto/KeyManager.hpp"
#endif /* ENABLE_DECRYPTION */

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <memory>
#include <vector>
using std::vector;
using std::unique_ptr;

namespace LibRomData {

class NCCHReaderPrivate
{
	public:
		NCCHReaderPrivate(NCCHReader *q, IRpFile *file,
			uint8_t media_unit_shift,
			int64_t ncch_offset, uint32_t ncch_length,
			const N3DS_Ticket_t *ticket = nullptr,
			uint16_t tmd_content_index = 0);
		~NCCHReaderPrivate();

	private:
		RP_DISABLE_COPY(NCCHReaderPrivate)
	protected:
		NCCHReader *const q_ptr;

	public:
		IRpFile *file;		// 3DS ROM image.

		// NCCH offsets.
		const int64_t ncch_offset;	// NCCH start offset, in bytes.
		const uint32_t ncch_length;	// NCCH length, in bytes.
		const uint8_t media_unit_shift;

		// Current read position within the NCCH.
		// pos = 0 indicates the beginning of the NCCH header.
		// NOTE: This cannot be more than 4 GB,
		// so we're using uint32_t.
		uint32_t pos;

		// Loaded headers.
		enum HeadersPresent {
			HEADER_NONE	= 0,
			HEADER_NCCH	= (1 << 0),
			HEADER_EXHEADER	= (1 << 1),
			HEADER_EXEFS	= (1 << 2),
		};
		uint32_t headers_loaded;	// HeadersPresent

		// NCCH header.
		N3DS_NCCH_Header_t ncch_header;
		// NCCH ExHeader.
		N3DS_NCCH_ExHeader_t ncch_exheader;
		// ExeFS header.
		N3DS_ExeFS_Header_t exefs_header;

		/**
		 * Load the NCCH Extended Header.
		 * @return 0 on success; non-zero on error.
		 */
		int loadExHeader(void);

#ifdef ENABLE_DECRYPTION
		// Title ID. Used for AES-CTR initialization.
		// (Big-endian format)
		uint64_t tid_be;

		// Encryption keys.
		// TODO: Use correct key index depending on file.
		// For now, only supporting NoCrypto and FixedCryptoKey
		// with a zero key.
		uint8_t ncch_keys[2][16];

		// NCCH cipher.
		IAesCipher *cipher_ncch;	// NCCH cipher.

		// CIA cipher.
		uint8_t title_key[16];		// Decrypted title key.
		IAesCipher *cipher_cia;		// CIA cipher.

		union ctr_t {
			uint8_t u8[16];
			uint32_t u32[4];
			uint64_t u64[2];
		};

		/**
		 * Initialize an AES-CTR counter using the Title ID.
		 * @param ctr AES-CTR counter.
		 * @param section NCCH section.
		 * @param offset Partition offset, in bytes.
		 */
		inline void init_ctr(ctr_t *ctr, uint8_t section, uint32_t offset)
		{
			ctr->u64[0] = tid_be;
			ctr->u8[8] = section;
			ctr->u8[9] = 0;
			ctr->u8[10] = 0;
			ctr->u8[11] = 0;
			offset /= 16;
			ctr->u32[3] = cpu_to_be32(offset);
		}

		/**
		 * Initialize an AES-CBC IV using the TMD content index.
		 * Used for decrypting CIAs.
		 * @param iv AES-CBC IV.
		 */
		inline void init_cia_cbc_iv(ctr_t *iv)
		{
			iv->u8[0] = tmd_content_index >> 8;
			iv->u8[1] = tmd_content_index & 0xFF;
			memset(&iv->u8[2], 0, sizeof(iv->u8)-2);
		}

		// Encrypted section addresses.
		struct EncSection {
			uint32_t address;	// Relative to ncch_offset.
			uint32_t ctr_base;	// Base address for the AES-CTR counter.
			uint32_t length;
			uint8_t keyIdx;		// ncch_keys[] index
			uint8_t section;	// N3DS_NCCH_Sections

			EncSection(uint32_t address, uint32_t ctr_base,
				uint32_t length, uint8_t keyIdx, uint8_t section)
				: address(address)
				, ctr_base(ctr_base)
				, length(length)
				, keyIdx(keyIdx)
				, section(section)
				{ }

			/**
			 * Comparison operator for std::sort().
			 */
			inline bool operator<(const EncSection& other) const
			{
				return (other.address < this->address);
			}
		};
		vector<EncSection> encSections;

		/**
		 * Find the encrypted section containing a given address.
		 * @param address Address.
		 * @return Index in encSections, or -1 if not encrypted.
		 */
		int findEncSection(uint32_t address) const;

		// KeyY index for title key encryption. (CIA only)
		uint8_t titleKeyEncIdx;
		// TMD content index.
		uint16_t tmd_content_index;
#endif /* ENABLE_DECRYPTION */
};

/** NCCHReaderPrivate **/

NCCHReaderPrivate::NCCHReaderPrivate(NCCHReader *q, IRpFile *file,
	uint8_t media_unit_shift,
	int64_t ncch_offset, uint32_t ncch_length,
	const N3DS_Ticket_t *ticket,
	uint16_t tmd_content_index)
	: q_ptr(q)
	, file(file)
	, ncch_offset(ncch_offset)
	, ncch_length(ncch_length)
	, media_unit_shift(media_unit_shift)
	, pos(0)
	, headers_loaded(0)
#ifdef ENABLE_DECRYPTION
	, tid_be(0)
	, cipher_ncch(nullptr)
	, cipher_cia(nullptr)
	, titleKeyEncIdx(0)
	, tmd_content_index(tmd_content_index)
#endif /* ENABLE_DECRYPTION */
{
#ifdef ENABLE_DECRYPTION
	// Check if this is a CIA with title key encryption.
	if (ticket) {
		// Check the ticket issuer.
		const char *key_prefix;
		if (!strncmp(ticket->issuer, N3DS_TICKET_ISSUER_RETAIL, sizeof(ticket->issuer))) {
			// Retail issuer..
			key_prefix = "ctr";
			titleKeyEncIdx = N3DS_TICKET_TITLEKEY_ISSUER_RETAIL;
		} else if (!strncmp(ticket->issuer, N3DS_TICKET_ISSUER_DEBUG, sizeof(ticket->issuer))) {
			// Debug issuer.
			key_prefix = "ctr-dev";
			titleKeyEncIdx = N3DS_TICKET_TITLEKEY_ISSUER_DEBUG;
		} else {
			// Unknown issuer.
			key_prefix = "ctr";
			titleKeyEncIdx = N3DS_TICKET_TITLEKEY_ISSUER_UNKNOWN;
		}

		// Check the KeyY index.
		// TODO: Handle invalid KeyY indexes?
		titleKeyEncIdx |= (ticket->keyY_index << 2);

		// Get the Key Manager instance.
		KeyManager *keyManager = KeyManager::instance();
		assert(keyManager != nullptr);

		// Try to get the KeyNormal.
		// TODO: Fall back to KeyX/KeyY if not found.
		char key_name[64];
		snprintf(key_name, sizeof(key_name), "%s-Slot0x3DKeyNormal-%d",
			key_prefix, titleKeyEncIdx >> 2);
		KeyManager::KeyData_t keyData;
		if (keyManager->get(key_name, &keyData) != 0) {
			// Could not retrieve the CIA key.
			// TODO: Encryption errors like WiiPartition.
			q->m_lastError = EIO;
			this->file = nullptr;
			return;
		} else if (!keyData.key || keyData.length != 16) {
			// Invalid key.
			// TODO: Encryption errors like WiiPartition.
			q->m_lastError = EIO;
			this->file = nullptr;
			return;
		}

		// Create the cipher.
		cipher_cia = AesCipherFactory::create();

		// Initialize parameters for title key decryption.
		// Parameters:
		// - Keyslot: 0x3D
		// - Chaining mode: CBC
		// - IV: Title ID (little-endian)
		cipher_cia->setChainingMode(IAesCipher::CM_CBC);
		cipher_cia->setKey(keyData.key, keyData.length);
		tid_be = ticket->title_id.id;	// already in BE
		ctr_t cia_iv;
		memcpy(cia_iv.u8, &tid_be, sizeof(tid_be));
		memset(&cia_iv.u8[8], 0, 8);
		cipher_cia->setIV(cia_iv.u8, sizeof(cia_iv.u8));

		// Decrypt the title key.
		memcpy(title_key, ticket->title_key, sizeof(title_key));
		cipher_cia->decrypt(title_key, sizeof(title_key));

		// Initialize parameters for CIA decryption.
		// Parameters:
		// - Key: Decrypted title key.
		// - Chaining mode: CBC
		// - IV: Content index from the TMD.
		cipher_cia->setKey(title_key, sizeof(title_key));
	} else {
		// No CIA encryption.
		memset(title_key, 0, sizeof(title_key));
	}
#endif /* ENABLE_DECRYPTION */

	// Read the NCCH header.
	// We're including the signature, since the first 16 bytes
	// are used for encryption in certain cases.
	int ret = file->seek(ncch_offset);
	if (ret != 0) {
		// Seek error.
		q->m_lastError = file->lastError();
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		this->file = nullptr;
		return;
	}
	size_t size = file->read(&ncch_header, sizeof(ncch_header));
	if (size != sizeof(ncch_header)) {
		// Read error.
		q->m_lastError = file->lastError();
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		this->file = nullptr;
		return;
	}
#ifdef ENABLE_DECRYPTION
	if (cipher_cia) {
		// Decrypt the NCCH header.
		// IV is the TMD content index.
		ctr_t cia_iv;
		init_cia_cbc_iv(&cia_iv);
		cipher_cia->setIV(cia_iv.u8, sizeof(cia_iv.u8));
		// TODO: Change decrypt()'s parameter to void*?
		// TODO: Verify decryption return value.
		cipher_cia->decrypt(reinterpret_cast<uint8_t*>(&ncch_header), sizeof(ncch_header));
	}
#endif /* ENABLE_DECRYPTION */
	// Verify the NCCH magic number.
	if (memcmp(ncch_header.hdr.magic, N3DS_NCCH_HEADER_MAGIC, sizeof(ncch_header.hdr.magic)) != 0) {
		// NCCH magic number is incorrect.
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		this->file = nullptr;
		return;
	}
	headers_loaded |= HEADER_NCCH;

#ifdef ENABLE_DECRYPTION
	// Byteswap the title ID. (It's used for the AES counter.)
	if (tid_be == 0) {
		tid_be = __swab64(ncch_header.hdr.program_id.id);
	}
#endif /* ENABLE_DECRYPTION */

	// Determine the keyset to use.
	// Crypto settings, in priority order:
	// 1. NoCrypto: AES key is all 0s. (FixedCryptoKey should also be set.)
	// 2. FixedCryptoKey: Fixed key is used.
	// 3. Neither: Standard key is used.
#ifdef ENABLE_DECRYPTION
	if (ncch_header.hdr.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto) {
		// No encryption.
		memset(ncch_keys, 0, sizeof(ncch_keys));
	} else if (ncch_header.hdr.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_FixedCryptoKey) {
		// Fixed key encryption.
		// TODO: Determine which keyset is in use.
		// For now, assuming TEST. (Zero-key) [FBI.3ds uses this]
		memset(ncch_keys, 0, sizeof(ncch_keys));
	} else {
		// Standard NCCH encryption.
		// TODO: Implement this.
		// For now, assuming TEST. (Zero-key)
		memset(ncch_keys, 0, sizeof(ncch_keys));
	}
#else /* !ENABLE_DECRYPTION */
	// Decryption is not available, so only NoCrypto is allowed.
	if (!(ncch_header.hdr.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto)) {
		// Unsupported.
		// TODO: Set an error like WiiPartition.
		q->m_lastError = EIO;
		this->file = nullptr;
		return;
	}
#endif /* ENABLE_DECRYPTION */

	// Load the ExeFS header.
	// NOTE: Checking for >= 16, since it has to be
	// non-zero and on a multiple of 16 for AES.
	// TODO: Verify length.
	const uint32_t exefs_offset = (le32_to_cpu(ncch_header.hdr.exefs_offset) << media_unit_shift);
	if (exefs_offset >= 16) {
#ifdef ENABLE_DECRYPTION
		ctr_t cia_iv;
		if (cipher_cia) {
			// Read the CIA IV, which is 16 bytes before the
			// start of the ExeFS header.
			ret = file->seek(ncch_offset + exefs_offset - 16);
			if (ret != 0) {
				// Seek error.
				q->m_lastError = file->lastError();
				this->file = nullptr;
				return;
			}
			size = file->read(&cia_iv, sizeof(cia_iv));
			if (size != sizeof(cia_iv)) {
				// Read error.
				q->m_lastError = file->lastError();
				this->file = nullptr;
				return;
			}
		} else
#endif /* ENABLE_DECRYPTION */
		{
			// Seek to the start of the ExeFS.
			ret = file->seek(ncch_offset + exefs_offset);
			if (ret != 0) {
				// Seek error.
				q->m_lastError = file->lastError();
				this->file = nullptr;
				return;
			}
		}
		// Read the ExeFS header.
		size = file->read(&exefs_header, sizeof(exefs_header));
		if (size != sizeof(exefs_header)) {
			// Read error.
			q->m_lastError = file->lastError();
			this->file = nullptr;
			return;
		}

		if (cipher_cia) {
			// Decrypt the ExeFS header.
			// TODO: Change decrypt()'s parameter to void*?
			// TODO: Verify decryption return value.
			cipher_cia->decrypt(reinterpret_cast<uint8_t*>(&exefs_header), sizeof(exefs_header));
		}

		headers_loaded |= HEADER_EXEFS;
	}

#ifdef ENABLE_DECRYPTION
	if (!(ncch_header.hdr.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto)) {
		// Initialize the AES cipher.
		// TODO: Check for errors.
		cipher_ncch = AesCipherFactory::create();
		cipher_ncch->setChainingMode(IAesCipher::CM_CTR);
		ctr_t ctr;

		if (headers_loaded & HEADER_EXEFS) {
			// Decrypt the ExeFS header.
			// ExeFS header uses ncchKey0.
			cipher_ncch->setKey(ncch_keys[0], sizeof(ncch_keys[0]));
			init_ctr(&ctr, N3DS_NCCH_SECTION_EXEFS, 0);
			cipher_ncch->setIV(ctr.u8, sizeof(ctr.u8));
			cipher_ncch->decrypt(reinterpret_cast<uint8_t*>(&exefs_header), sizeof(exefs_header));
		}

		// Initialize encrypted section handling.
		// Reference: https://github.com/profi200/Project_CTR/blob/master/makerom/ncch.c
		// Encryption details:
		// - ExHeader: ncchKey0, N3DS_NCCH_SECTION_EXHEADER
		// - acexDesc (TODO): ncchKey0, N3DS_NCCH_SECTION_EXHEADER
		// - ExeFS:
		//   - Header, "icon" and "banner": ncchKey0, N3DS_NCCH_SECTION_EXEFS
		//   - Other files: ncchKey1, N3DS_NCCH_SECTION_EXEFS
		// - RomFS (TODO): ncchKey1, N3DS_NCCH_SECTION_ROMFS

		// ExHeader
		encSections.push_back(EncSection(
			sizeof(N3DS_NCCH_Header_t),	// Address within NCCH.
			sizeof(N3DS_NCCH_Header_t),	// Counter base address.
			le32_to_cpu(ncch_header.hdr.exheader_size),
			0, N3DS_NCCH_SECTION_EXHEADER));

		if (headers_loaded & HEADER_EXEFS) {
			// ExeFS header
			encSections.push_back(EncSection(
				exefs_offset,	// Address within NCCH.
				exefs_offset,	// Counter base address.
				sizeof(N3DS_ExeFS_Header_t),
				0, N3DS_NCCH_SECTION_EXEFS));

			// ExeFS files
			for (int i = 0; i < ARRAY_SIZE(exefs_header.files); i++) {
				const N3DS_ExeFS_File_Header_t *file_header = &exefs_header.files[i];
				if (file_header->name[0] == 0)
					continue;	// or break?

				uint8_t keyIdx;
				if (!strncmp(file_header->name, "icon", sizeof(file_header->name)) ||
				    !strncmp(file_header->name, "banner", sizeof(file_header->name))) {
					// Icon and Banner use key 0.
					keyIdx = 0;
				} else {
					// All other files use key 1.
					keyIdx = 1;
				}

				encSections.push_back(EncSection(
					exefs_offset + sizeof(exefs_header) +	// Address within NCCH.
						le32_to_cpu(file_header->offset),
					exefs_offset,				// Counter base address.
					le32_to_cpu(file_header->size),
					keyIdx, N3DS_NCCH_SECTION_EXEFS));
			}
		}

		// RomFS
		if (le32_to_cpu(ncch_header.hdr.romfs_size) != 0) {
			const uint32_t romfs_offset = (le32_to_cpu(ncch_header.hdr.romfs_offset) << media_unit_shift);
			encSections.push_back(EncSection(
				romfs_offset,	// Address within NCCH.
				romfs_offset,	// Counter base address.
				(le32_to_cpu(ncch_header.hdr.romfs_size) << media_unit_shift),
				0, N3DS_NCCH_SECTION_ROMFS));
		}

		// Sort encSections by NCCH-relative address.
		// TODO: Check for overlap?
		std::sort(encSections.begin(), encSections.end());
	}
#endif /* ENABLE_DECRYPTION */
}

NCCHReaderPrivate::~NCCHReaderPrivate()
{
#ifdef ENABLE_DECRYPTION
	delete cipher_ncch;
	delete cipher_cia;
#endif /* ENABLE_DECRYPTION */
}

#ifdef ENABLE_DECRYPTION
/**
 * Find the encrypted section containing a given address.
 * @param address Address.
 * @return Index in encSections, or -1 if not encrypted.
 */
int NCCHReaderPrivate::findEncSection(uint32_t address) const
{
	for (int i = 0; i < (int)encSections.size(); i++) {
		const auto &section = encSections.at(i);
		if (address >= section.address &&
		    address < section.address + section.length)
		{
			// Found the section.
			return i;
		}
	}

	// Not an encrypted section.
	return -1;
}
#endif /* ENABLE_DECRYPTION */

/**
 * Load the NCCH Extended Header.
 * @return 0 on success; non-zero on error.
 */
int NCCHReaderPrivate::loadExHeader(void)
{
	if (headers_loaded & HEADER_EXHEADER) {
		// NCCH Extended Header is already loaded.
		return 0;
	}

	// TODO: Load the NCCH header if it isn't loaded?
	if (!(headers_loaded & HEADER_NCCH)) {
		// NCCH header wasn't loaded.
		return -1;
	}

	RP_Q(NCCHReader);
	if (!file || !file->isOpen()) {
		// File isn't open...
		q->m_lastError = EBADF;
		return -2;
	}

	// NOTE: Using NCCHReader functions instead of direct
	// file access, so all addresses are relative to the
	// start of the NCCH.

	// Load the ExHeader.
	// ExHeader is stored immediately after the main header.
	int64_t prev_pos = q->tell();
	int ret = q->seek(sizeof(N3DS_NCCH_Header_t));
	if (ret != 0) {
		// Seek error.
		q->m_lastError = file->lastError();
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		q->seek(prev_pos);
		return -3;
	}

	// Check the ExHeader length.
	uint32_t exheader_length = le32_to_cpu(ncch_header.hdr.exheader_size);
	if (exheader_length < N3DS_NCCH_EXHEADER_MIN_SIZE ||
	    exheader_length > sizeof(ncch_exheader))
	{
		// ExHeader is either too small or too big.
		q->m_lastError = EIO;
		return -4;
	}

	// Round up exheader_length to the nearest 16 bytes for decryption purposes.
	exheader_length = (exheader_length + 15) & ~15;

	size_t size = q->read(&ncch_exheader, exheader_length);
	if (size != exheader_length) {
		// Read error.
		q->m_lastError = file->lastError();
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		q->seek(prev_pos);
		return -5;
	}

	// If the ExHeader size is smaller than the maximum size,
	// clear the rest of the ExHeader.
	// TODO: Possible strict aliasing violation...
	if (exheader_length < sizeof(ncch_exheader)) {
		uint8_t *exzero = reinterpret_cast<uint8_t*>(&ncch_exheader) + exheader_length;
		memset(exzero, 0, sizeof(ncch_exheader) - exheader_length);
	}

	// ExHeader loaded.
	q->seek(prev_pos);
	headers_loaded |= HEADER_EXHEADER;
	return 0;
}

/** NCCHReader **/

/**
 * Construct an NCCHReader with the specified IRpFile.
 *
 * NOTE: The IRpFile *must* remain valid while this
 * NCCHReader is open.
 *
 * @param file 			[in] IRpFile.
 * @param media_unit_shift	[in] Media unit shift.
 * @param ncch_offset		[in] NCCH start offset, in bytes.
 * @param ncch_length		[in] NCCH length, in bytes.
 * @param ticket		[in,opt] Ticket for CIA decryption. (nullptr if NoCrypto)
 * @param tmd_content_index	[in,opt] TMD content index for CIA decryption.
 */
NCCHReader::NCCHReader(IRpFile *file, uint8_t media_unit_shift,
		int64_t ncch_offset, uint32_t ncch_length,
		const N3DS_Ticket_t *ticket,
		uint16_t tmd_content_index)
	: d_ptr(new NCCHReaderPrivate(this, file, media_unit_shift, ncch_offset, ncch_length, ticket, tmd_content_index))
{ }

NCCHReader::~NCCHReader()
{
	delete d_ptr;
}

/** IDiscReader **/

/**
 * Is the partition open?
 * This usually only returns false if an error occurred.
 * @return True if the partition is open; false if it isn't.
 */
bool NCCHReader::isOpen(void) const
{
	RP_D(NCCHReader);
	return (d->file && d->file->isOpen());
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t NCCHReader::read(void *ptr, size_t size)
{
	RP_D(NCCHReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
		m_lastError = EBADF;
		return 0;
	}

	// Are we already at the end of the file?
	if (d->pos >= d->ncch_length)
		return 0;

	// Make sure d->pos + size <= d->ncch_length.
	// If it isn't, we'll do a short read.
	if (d->pos + (int64_t)size >= d->ncch_length) {
		size = (size_t)(d->ncch_length - d->pos);
	}

	const int64_t start_addr = d->ncch_offset + d->pos;

#ifdef ENABLE_DECRYPTION
	NCCHReaderPrivate::ctr_t cia_iv;
	if (d->cipher_cia) {
		// Need to load the IV first.
		if (d->pos == 0) {
			// IV is the TMD content index.
			d->init_cia_cbc_iv(&cia_iv);
		} else {
			// IV is 16 bytes before d->pos.
			int ret = d->file->seek(start_addr - 16);
			if (ret != 0) {
				// Seek error.
				m_lastError = d->file->lastError();
				return 0;
			}
			size_t size = d->file->read(&cia_iv, sizeof(cia_iv));
			if (size != sizeof(cia_iv)) {
				// Read error.
				m_lastError = d->file->lastError();
				return 0;
			}
		}
		d->cipher_cia->setIV(cia_iv.u8, sizeof(cia_iv.u8));
	} else
#endif /* ENABLE_DECRYPTION */
	{
		// Seek to the starting address.
		int ret = d->file->seek(start_addr);
		if (ret != 0) {
			// Seek error.
			m_lastError = d->file->lastError();
			return 0;
		}
	}

	if (d->ncch_header.hdr.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto) {
		// No encryption. Read directly from the NCCH.
		size_t ret_sz = d->file->read(ptr, size);
		if (ret_sz != size) {
			// Possible error occurred...
			m_lastError = d->file->lastError();
		}
#ifdef ENABLE_DECRYPTION
		if (d->cipher_cia) {
			// CIA has outer encryption.
			d->cipher_cia->decrypt(static_cast<uint8_t*>(ptr), ret_sz);
		}
#endif /* ENABLE_DECRYPTION */
		return ret_sz;
	}

#ifdef ENABLE_DECRYPTION
	// TODO: Handle reads that aren't a multiple of 16 bytes.
	assert(d->pos % 16 == 0);
	assert(size % 16 == 0);
	if (d->pos % 16 != 0 || size % 16 != 0) {
		// Cannot read now.
		return 0;
	}

	// Seek to the starting position.
	int ret = d->file->seek(d->ncch_offset + d->pos);
	if (ret != 0) {
		// Seek error.
		m_lastError = d->file->lastError();
		return 0;
	}

	uint8_t *ptr8 = static_cast<uint8_t*>(ptr);
	size_t sz_total_read = 0;
	while (size > 0) {
		// Determine what section we're in.
		int sectIdx = d->findEncSection(d->pos);
		const NCCHReaderPrivate::EncSection *section = (
			sectIdx >= 0 ? &d->encSections.at(sectIdx) : nullptr);

		size_t sz_to_read = 0;
		uint32_t section_offset = 0;
		if (!section) {
			// Not in an encrypted section.
			// TODO: Find the next encrypted section.
			// For now, assuming the rest is plaintext.
			sz_to_read = size;
			assert(!"Reading in an unencrypted section...");
		} else {
			// We're in an encrypted section.
			section_offset = (uint32_t)(d->pos - section->address);
			if (section_offset + size <= section->length) {
				// Remainder of reading is in this section.
				sz_to_read = size;
			} else {
				// We're reading past the end of this section.
				sz_to_read = section->length - section_offset;
			}
		}

		size_t ret_sz = d->file->read(ptr8, sz_to_read);
		if (ret_sz != sz_to_read) {
			// Possible error occurred...
			m_lastError = d->file->lastError();
		}

		if (d->cipher_cia && ret_sz >= 16) {
			// CIA has outer encryption.
			// Save the last 16-byte block for the next IV.
			// TODO: Handle ret_sz < 16; error handling.
			memcpy(&cia_iv.u8, ptr8 + ret_sz - 16, sizeof(cia_iv.u8));
			d->cipher_cia->decrypt(ptr8, ret_sz);
			// Set the IV. (FIXME: Only needed for AesCAPI; fix this...)
			d->cipher_cia->setIV(cia_iv.u8, sizeof(cia_iv.u8));
		}

		if (section) {
			// Set the required key.
			// TODO: Don't set the key if it hasn't changed.
			d->cipher_ncch->setKey(d->ncch_keys[section->keyIdx], sizeof(d->ncch_keys[section->keyIdx]));

			// Initialize the counter based on section and offset.
			NCCHReaderPrivate::ctr_t ctr;
			d->init_ctr(&ctr, section->section, d->pos - section->ctr_base);
			d->cipher_ncch->setIV(ctr.u8, sizeof(ctr.u8));

			// Decrypt the data.
			// FIXME: Round up to 16 if a short read occurred?
			ret_sz = d->cipher_ncch->decrypt(static_cast<uint8_t*>(ptr), (unsigned int)ret_sz);
		}

		d->pos += (uint32_t)ret_sz;
		ptr8 += ret_sz;
		sz_total_read += ret_sz;
		size -= ret_sz;
		if (d->pos > d->ncch_length) {
			d->pos = d->ncch_length;
			break;
		}
		if (ret_sz != sz_to_read) {
			// Short read.
			break;
		}
	}

	return sz_total_read;
#else /* !ENABLE_DECRYPTION */
	// Decryption is not enabled.
	return 0;
#endif /* ENABLE_DECRYPTION */
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int NCCHReader::seek(int64_t pos)
{
	RP_D(NCCHReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file ||  !d->file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// Handle out-of-range cases.
	// TODO: How does POSIX behave?
	if (pos < 0)
		d->pos = 0;
	else if (pos >= d->ncch_length)
		d->pos = d->ncch_length;
	else
		d->pos = (uint32_t)pos;
	return 0;
}

/**
 * Seek to the beginning of the partition.
 */
void NCCHReader::rewind(void)
{
	seek(0);
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
int64_t NCCHReader::tell(void)
{
	RP_D(NCCHReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file ||  !d->file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	return d->pos;
}

/**
 * Get the data size.
 * This size does not include the NCCH header,
 * and it's adjusted to exclude hashes.
 * @return Data size, or -1 on error.
 */
int64_t NCCHReader::size(void)
{
	// TODO: Errors?
	RP_D(const NCCHReader);
	const int64_t ret = d->ncch_length - sizeof(N3DS_NCCH_Header_t);
	return (ret >= 0 ? ret : 0);
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
int64_t NCCHReader::partition_size(void) const
{
	// TODO: Errors?
	RP_D(const NCCHReader);
	return d->ncch_length;
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
int64_t NCCHReader::partition_size_used(void) const
{
	// TODO: Errors?
	// NOTE: For NCCHReader, this is the same as partition_size().
	RP_D(const NCCHReader);
	return d->ncch_length;
}

/** NCCHReader **/

/**
 * Get the NCCH header.
 * @return NCCH header, or nullptr if it couldn't be loaded.
 */
const N3DS_NCCH_Header_NoSig_t *NCCHReader::ncchHeader(void) const
{
	RP_D(const NCCHReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
		//m_lastError = EBADF;
		return nullptr;
	}

	if (!(d->headers_loaded & NCCHReaderPrivate::HEADER_NCCH)) {
		// NCCH header wasn't loaded.
		// TODO: Try to load it here?
		return nullptr;
	}

	return &d->ncch_header.hdr;
}

/**
 * Get the NCCH extended header.
 * @return NCCH extended header, or nullptr if it couldn't be loaded.
 */
const N3DS_NCCH_ExHeader_t *NCCHReader::ncchExHeader(void) const
{
	RP_D(const NCCHReader);
	if (!(d->headers_loaded & NCCHReaderPrivate::HEADER_EXHEADER)) {
		if (const_cast<NCCHReaderPrivate*>(d)->loadExHeader() != 0) {
			// Unable to load the ExHeader.
			return nullptr;
		}
	}

	return &d->ncch_exheader;
}

/**
 * Get the ExeFS header.
 * @return ExeFS header, or nullptr if it couldn't be loaded.
 */
const N3DS_ExeFS_Header_t *NCCHReader::exefsHeader(void) const
{
	RP_D(const NCCHReader);
	if (!(d->headers_loaded & NCCHReaderPrivate::HEADER_EXEFS)) {
		// ExeFS header wasn't loaded.
		// TODO: Try to load it here?
		return nullptr;
	}

	// TODO: Check if the ExeFS header was actually loaded.
	return &d->exefs_header;
}

/**
 * Get the NCCH crypto type.
 * @param pCryptoType	[out] Crypto type.
 * @param pNcchHeader	[in] NCCH header.
 * @return 0 on success; negative POSIX error code on error.
 */
int NCCHReader::cryptoType_static(CryptoType *pCryptoType, const N3DS_NCCH_Header_NoSig_t *pNcchHeader)
{
	assert(pCryptoType != nullptr);
	assert(pNcchHeader != nullptr);
	if (!pCryptoType || !pNcchHeader) {
		// Invalid argument(s).
		return -EINVAL;
	}

	// References:
	// - https://github.com/d0k3/GodMode9/blob/master/source/game/ncch.c
	// - https://github.com/d0k3/GodMode9/blob/master/source/game/ncch.h
	if (pNcchHeader->flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto) {
		// No encryption.
		pCryptoType->name = "NoCrypto";
		pCryptoType->encrypted = false;
		pCryptoType->keyslot = 0xFF;
		pCryptoType->seed = false;
		return 0;
	}

	// Encryption is enabled.
	pCryptoType->encrypted = true;
	if (pNcchHeader->flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_FixedCryptoKey) {
		// Fixed key encryption.
		// NOTE: Keyslot 0x11 is used, but that keyslot
		// isn't permanently assigned, so we're not setting
		// it here.
		pCryptoType->keyslot = 0xFF;
		pCryptoType->seed = false;
		// NOTE: Using GodMode9's fixed keyset determination.
		if (le32_to_cpu(pNcchHeader->program_id.hi) & 0x10) {
			// Using the fixed debug key.
			pCryptoType->name = "Fixed (Debug)";
		} else {
			// Using zero-key.
			pCryptoType->name = "Fixed (Zero)";
		}
		return 0;
	}

	// Check ncchflag[3].
	switch (pNcchHeader->flags[N3DS_NCCH_FLAG_CRYPTO_METHOD]) {
		case 0x00:
			pCryptoType->name = "Standard";
			pCryptoType->keyslot = 0x2C;
			break;
		case 0x01:
			pCryptoType->name = "v7.x";
			pCryptoType->keyslot = 0x25;
			break;
		case 0x0A:
			pCryptoType->name = "Secure3";
			pCryptoType->keyslot = 0x18;
			break;
		case 0x0B:
			pCryptoType->name = "Secure4";
			pCryptoType->keyslot = 0x1B;
			break;
		default:
			// TODO: Unknown encryption method...
			assert(!"Unknown NCCH encryption method.");
			pCryptoType->name = nullptr;
			pCryptoType->keyslot = 0xFF;
			break;
	}

	// Is SEED encryption in use?
	pCryptoType->seed = !!(pNcchHeader->flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_Fw96KeyY);

	// We're done here.
	return 0;
}

/**
 * Get the NCCH crypto type.
 * @param pCryptoType	[out] Crypto type.
 * @return 0 on success; negative POSIX error code on error.
 */
int NCCHReader::cryptoType(CryptoType *pCryptoType) const
{
	assert(pCryptoType != nullptr);
	if (!pCryptoType) {
		// Invalid argument.
		return -EINVAL;
	}

	RP_D(const NCCHReader);
	if (!(d->headers_loaded & NCCHReaderPrivate::HEADER_NCCH)) {
		// NCCH header wasn't loaded.
		// TODO: Try to load it here?
		return -EIO;
	}
	return cryptoType_static(pCryptoType, &d->ncch_header.hdr);
}

/**
 * Open a file. (read-only)
 *
 * NOTE: Only ExeFS is currently supported.
 *
 * @param section NCCH section.
 * @param filename Filename. (ASCII)
 * @return IRpFile*, or nullptr on error.
 */
IRpFile *NCCHReader::open(int section, const char *filename)
{
	RP_D(NCCHReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	assert(section == N3DS_NCCH_SECTION_EXEFS);
	assert(filename != nullptr);
	if (!d->file || !d->file->isOpen()) {
		m_lastError = EBADF;
		return nullptr;
	} else if (section != N3DS_NCCH_SECTION_EXEFS) {
		// Only ExeFS is currently supported.
		m_lastError = ENOTSUP;
		return nullptr;
	} else if (!filename) {
		// Invalid filename.
		m_lastError = EINVAL;
		return nullptr;
	}

	// Get the ExeFS header.
	const N3DS_ExeFS_Header_t *const exefs_header = exefsHeader();
	if (!exefs_header) {
		// Unable to get the ExeFS header.
		return nullptr;
	}

	const N3DS_ExeFS_File_Header_t *file_header = nullptr;
	for (int i = 0; i < ARRAY_SIZE(exefs_header->files); i++) {
		if (!strncmp(exefs_header->files[i].name, filename, sizeof(exefs_header->files[i].name))) {
			// Found "icon".
			file_header = &exefs_header->files[i];
			break;
		}
	}
	if (!file_header) {
		// File not found.
		m_lastError = ENOENT;
		return nullptr;
	}

	// Get the file offset.
	const uint32_t offset = (le32_to_cpu(d->ncch_header.hdr.exefs_offset) << d->media_unit_shift) +
		sizeof(d->exefs_header) +
		le32_to_cpu(file_header->offset);
	const uint32_t size = le32_to_cpu(file_header->size);
	if (offset >= d->ncch_length ||
	    ((int64_t)offset + size) > d->ncch_length)
	{
		// File offset/size is out of bounds.
		m_lastError = EIO;	// TODO: Better error code?
		return nullptr;
	}

	// TODO: Reference count opened PartitionFiles and
	// add assertions if they aren't closed correctly.

	// Create the PartitionFile.
	// This is an IRpFile implementation that uses an
	// IPartition as the reader and takes an offset
	// and size as the file parameters.
	return new PartitionFile(this, offset, size);
}

}
