/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Amiibo.cpp: Nintendo amiibo NFC dump reader.                            *
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

#include "Amiibo.hpp"
#include "nfp_structs.h"
#include "AmiiboData.hpp"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>
#include <cstddef>

// C++ includes.
#include <vector>
using std::vector;

namespace LibRomData {

class AmiiboPrivate
{
	public:
		AmiiboPrivate() { }

	private:
		AmiiboPrivate(const AmiiboPrivate &other);
		AmiiboPrivate &operator=(const AmiiboPrivate &other);

	public:
		/** RomFields **/

		// Monospace string formatting.
		static const RomFields::StringDesc nfp_string_monospace;

		// Credits string formatting.
		static const RomFields::StringDesc nfp_string_credits;

		// ROM fields.
		static const struct RomFields::Desc nfp_fields[];

	public:
		// NFC data. (TODO)
		NFP_Data_t nfpData;

		/**
		 * Calculate the check bytes from an NTAG215 serial number.
		 * @param serial	[in] NTAG215 serial number. (9 bytes)
		 * @param pCb0		[out] Check byte 0. (calculated)
		 * @param pCb1		[out] Check byte 1. (calculated)
		 * @return True if the serial number has valid check bytes; false if not.
		 */
		static bool calcCheckBytes(const uint8_t *serial, uint8_t *pCb0, uint8_t *pCb1);
};

/** AmiiboPrivate **/

// Monospace string formatting.
const RomFields::StringDesc AmiiboPrivate::nfp_string_monospace = {
	RomFields::StringDesc::STRF_MONOSPACE
};

// Credits string formatting.
const RomFields::StringDesc AmiiboPrivate::nfp_string_credits = {
	RomFields::StringDesc::STRF_CREDITS
};

// ROM fields.
const struct RomFields::Desc AmiiboPrivate::nfp_fields[] = {
	// NTAG215 data.
	{_RP("NTAG215 serial"), RomFields::RFT_STRING, {&nfp_string_monospace}},

	// TODO: More amiibo data.
	{_RP("amiibo ID"), RomFields::RFT_STRING, {&nfp_string_monospace}},
	{_RP("amiibo Type"), RomFields::RFT_STRING, {nullptr}},

	{_RP("Character Series"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Character Name"), RomFields::RFT_STRING, {nullptr}},

	{_RP("amiibo Series"), RomFields::RFT_STRING, {nullptr}},
	{_RP("amiibo Name"), RomFields::RFT_STRING, {nullptr}},
	{_RP("amiibo Wave #"), RomFields::RFT_STRING, {nullptr}},
	{_RP("amiibo Release #"), RomFields::RFT_STRING, {nullptr}},

	// Credits
	{_RP("Credits"), RomFields::RFT_STRING, {&nfp_string_credits}},
};

/**
 * Calculate the check bytes from an NTAG215 serial number.
 * @param serial	[in] NTAG215 serial number. (9 bytes)
 * @param pCb0		[out] Check byte 0. (calculated)
 * @param pCb1		[out] Check byte 1. (calculated)
 * @return True if the serial number has valid check bytes; false if not.
 */
bool AmiiboPrivate::calcCheckBytes(const uint8_t *serial, uint8_t *pCb0, uint8_t *pCb1)
{
	// Check Byte 0 = CT ^ SN0 ^ SN1 ^ SN2
	// Check Byte 1 = SN3 ^ SN4 ^ SN5 ^ SN6
	// NTAG215 uses Cascade Level 2, so CT = 0x88.
	*pCb0 = 0x88 ^ serial[0] ^ serial[1] ^ serial[2];
	*pCb1 = serial[4] ^ serial[5] ^ serial[6] ^ serial[7];
	return (*pCb0 == serial[3] && *pCb1 == serial[8]);
}

/** Amiibo **/

/**
 * Read a Nintendo amiibo NFC dump.
 *
 * An NFC dump must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the NFC dump.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open NFC dump.
 */
Amiibo::Amiibo(IRpFile *file)
	: super(file, AmiiboPrivate::nfp_fields, ARRAY_SIZE(AmiiboPrivate::nfp_fields))
	, d(new AmiiboPrivate())
{
	// This class handles NFC dumps.
	m_fileType = FTYPE_NFC_DUMP;

	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the NFC data.
	m_file->rewind();
	size_t size = m_file->read(&d->nfpData, sizeof(d->nfpData));
	if (size != sizeof(d->nfpData))
		return;

	// Check if the NFC data is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->nfpData);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->nfpData);
	info.ext = nullptr;	// Not needed for NFP.
	info.szFile = m_file->fileSize();
	m_isValid = (isRomSupported_static(&info) >= 0);
}

Amiibo::~Amiibo()
{
	delete d;
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Amiibo::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 540 ||
	    info->szFile != 540)
	{
		// Either no detection information was specified,
		// the header is too small, or the file is the
		// wrong size.
		return -1;
	}

	const NFP_Data_t *nfpData = reinterpret_cast<const NFP_Data_t*>(info->header.pData);

	// Validate the UID check bytes.
	uint8_t cb0, cb1;
	if (!AmiiboPrivate::calcCheckBytes(nfpData->serial, &cb0, &cb1)) {
		// Check bytes are invalid.
		// These are read-only, so something went wrong
		// when the tag was being dumped.
		return -1;
	}

	// Check the "must match" values.
	static const uint8_t lock_header[2] = {0x0F, 0xE0};
	static const uint8_t cap_container[4] = {0xF1, 0x10, 0xFF, 0xEE};
	static const uint8_t lock_footer[3] = {0x01, 0x00, 0x0F};
	static const uint8_t cfg0[4] = {0x00, 0x00, 0x00, 0x04};
	static const uint8_t cfg1[4] = {0x5F, 0x00, 0x00, 0x00};

	static_assert(sizeof(nfpData->lock_header)   == sizeof(lock_header),   "lock_header is the wrong size.");
	static_assert(sizeof(nfpData->cap_container) == sizeof(cap_container), "cap_container is the wrong size.");
	static_assert(sizeof(nfpData->lock_footer)   == sizeof(lock_footer)+1, "lock_footer is the wrong size.");
	static_assert(sizeof(nfpData->cfg0)          == sizeof(cfg0),          "cfg0 is the wrong size.");
	static_assert(sizeof(nfpData->cfg1)          == sizeof(cfg1),          "cfg1 is the wrong size.");

	if (memcmp(nfpData->lock_header,   lock_header,   sizeof(lock_header)) != 0 ||
	    memcmp(nfpData->cap_container, cap_container, sizeof(cap_container)) != 0 ||
	    memcmp(nfpData->lock_footer,   lock_footer,   sizeof(lock_footer)) != 0 ||
	    memcmp(nfpData->cfg0,          cfg0,          sizeof(cfg0)) != 0 ||
	    memcmp(nfpData->cfg1,          cfg1,          sizeof(cfg1)) != 0)
	{
		// Not an amiibo.
		return -1;
	}

	// Low byte of amiibo_id must be 0x02.
	if ((be32_to_cpu(nfpData->amiibo_id) & 0xFF) != 0x02) {
		// Incorrect amiibo ID.
		return -1;
	}

	// This is an amiibo.
	return 0;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Amiibo::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *Amiibo::systemName(uint32_t type) const
{
	if (!m_isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// The "correct" name is "Nintendo Figurine Platform".
	// It's unknown whether or not Nintendo will release
	// NFC-enabled figurines that aren't amiibo.

	// NFP has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Amiibo::systemName() array index optimization needs to be updated.");

	static const rp_char *const sysNames[4] = {
		_RP("Nintendo Figurine Platform"),
		_RP("Nintendo Figurine Platform"),
		_RP("NFP"),
		nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

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
vector<const rp_char*> Amiibo::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		//_RP(".bin"),	// TODO: Enable this?
		// NOTE: The following extensions are listed
		// for testing purposes on Windows, and may
		// be removed later.
		_RP(".nfc"),
		_RP(".nfp"),
	};
	return vector<const rp_char*>(exts, exts + ARRAY_SIZE(exts));
}

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
vector<const rp_char*> Amiibo::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Amiibo::supportedImageTypes_static(void)
{
       return IMGBF_EXT_MEDIA;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Amiibo::supportedImageTypes(void) const
{
       return supportedImageTypes_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Amiibo::loadFieldData(void)
{
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!m_file) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// NTAG215 data.

	// Serial number.

	// Convert the 7-byte serial number to ASCII.
	static const uint8_t hex_lookup[16] = {
		'0','1','2','3','4','5','6','7',
		'8','9','A','B','C','D','E','F'
	};
	char buf[64]; char *pBuf = buf;
	for (int i = 0; i < 8; i++, pBuf += 2) {
		if (i == 3) {
			// Byte 3 is CB0.
			i++;
		}
		pBuf[0] = hex_lookup[d->nfpData.serial[i] >> 4];
		pBuf[1] = hex_lookup[d->nfpData.serial[i] & 0x0F];
	}

	// Verify the check bytes.
	// TODO: Show calculated check bytes?
	uint8_t cb0, cb1;
	int len;
	if (d->calcCheckBytes(d->nfpData.serial, &cb0, &cb1)) {
		// Check bytes are valid.
		len = snprintf(pBuf, sizeof(buf) - (7*2), " (check: %02X %02X)",
			d->nfpData.serial[3], d->nfpData.serial[8]);
	} else {
		// Check bytes are NOT valid.
		// NOTE: Shouldn't show up anymore because invalid
		// serial numbers are discarded in isRomSupported_static().
		len = snprintf(pBuf, sizeof(buf) - (7*2), " (check ERR: %02X %02X)",
			d->nfpData.serial[3], d->nfpData.serial[8]);
	}

	len += (7*2);
	if (len > (int)sizeof(buf))
		len = (int)sizeof(buf);
	m_fields->addData_string(len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));

	// NFP data.
	const uint32_t char_id = be32_to_cpu(d->nfpData.char_id);
	const uint32_t amiibo_id = be32_to_cpu(d->nfpData.amiibo_id);

	// amiibo ID.
	// Represents the character and amiibo series.
	// TODO: Link to http://amiibo.life/nfc/%08X-%08X
	len = snprintf(buf, sizeof(buf), "%08X-%08X", char_id, amiibo_id);
	if (len > (int)sizeof(buf))
		len = (int)sizeof(buf);
	m_fields->addData_string(len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));

	// amiibo type.
	const rp_char *type = nullptr;
	switch (char_id & 0xFF) {
		case NFP_TYPE_FIGURINE:
			type = _RP("Figurine");
			break;
		case NFP_TYPE_CARD:
			type = _RP("Card");
			break;
		case NFP_TYPE_YARN:
			type = _RP("Yarn");
			break;
		default:
			break;
	}

	if (type) {
		m_fields->addData_string(type);
	} else {
		// Invalid amiibo type.
		char buf[24];
		int len = snprintf(buf, sizeof(buf), "Unknown (0x%02X)", (char_id & 0xFF));
		if (len > (int)sizeof(buf))
			len = sizeof(buf);
		m_fields->addData_string(len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
	}

	// Character series.
	const rp_char *const char_series = AmiiboData::lookup_char_series_name(char_id);
	m_fields->addData_string(char_series ? char_series : _RP("Unknown"));

	// Character name.
	const rp_char *const char_name = AmiiboData::lookup_char_name(char_id);
	m_fields->addData_string(char_name ? char_name : _RP("Unknown"));

	// amiibo series.
	const rp_char *const amiibo_series = AmiiboData::lookup_amiibo_series_name(amiibo_id);
	m_fields->addData_string(amiibo_series ? amiibo_series : _RP("Unknown"));

	// amiibo name, wave number, and release number.
	int wave_no, release_no;
	const rp_char *const amiibo_name = AmiiboData::lookup_amiibo_series_data(amiibo_id, &release_no, &wave_no);
	if (amiibo_name) {
		m_fields->addData_string(amiibo_name);
		if (wave_no != 0) {
			m_fields->addData_string_numeric(wave_no);
		} else {
			m_fields->addData_invalid();
		}
		if (release_no != 0) {
			m_fields->addData_string_numeric(release_no);
		} else {
			m_fields->addData_invalid();
		}
	} else {
		// Unknown name.
		m_fields->addData_string(_RP("Unknown"));
		m_fields->addData_invalid();
		m_fields->addData_invalid();
	}

	// Credits.
	m_fields->addData_string(
		_RP("amiibo images provided by <a href=\"http://amiibo.life/\">amiibo.life</a>,\n the Unofficial amiibo Database."));

	// Finished reading the field data.
	return (int)m_fields->count();
}

/**
 * Load URLs for an external media type.
 * Called by RomData::extURL() if the URLs haven't been loaded yet.
 * @param imageType Image type to load.
 * @return 0 on success; negative POSIX error code on error.
 */
int Amiibo::loadURLs(ImageType imageType)
{
	assert(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_EXT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}

	const int idx = imageType - IMG_EXT_MIN;
	std::vector<ExtURL> &extURLs = m_extURLs[idx];
	if (!extURLs.empty()) {
		// URLs *have* been loaded...
		return 0;
	} else if (!m_file || !m_file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid) {
		// Invalid file.
		return -EIO;
	}

	// Only the "media" scan is supported.
	// Note that "media" refers to a photo of
	// the figure and/or card.
	if (imageType != IMG_EXT_MEDIA) {
		// Unsupported image type.
		return -ENOENT;
	}

	ExtURL extURL;

	// Cache key. (amiibo ID)
	// TODO: "amiibo/" or "nfp/"?
	char amiibo_id_str[32];
	int len = snprintf(amiibo_id_str, sizeof(amiibo_id_str), "amiibo/%08X-%08X",
		be32_to_cpu(d->nfpData.char_id), be32_to_cpu(d->nfpData.amiibo_id));
	if (len > (int)sizeof(amiibo_id_str))
		len = (int)sizeof(amiibo_id_str);
	if (len <= 0) {
		// Invalid NFC ID.
		return -EINVAL;
	}
	extURL.cache_key = latin1_to_rp_string(amiibo_id_str, len);
	extURL.cache_key += _RP(".png");

	// URL.
	// Format: http://amiibo.life/nfc/[Page21]-[Page22]/image
	char url_str[64];
	len = snprintf(url_str, sizeof(url_str), "http://amiibo.life/nfc/%.17s/image", &amiibo_id_str[7]);
	if (len > (int)sizeof(url_str))
		len = (int)sizeof(url_str);
	if (len <= 0) {
		// Invalid URL.
		return -EINVAL;
	}
	extURL.url = latin1_to_rp_string(url_str, len);

	// Add the URL and we're done.
	extURLs.push_back(extURL);
	return (extURLs.empty() ? -ENOENT : 0);
}

}
