/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomData.cpp: ROM data base class.                                       *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth                                  *
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

#include "RomData.hpp"
#include "RomData_p.hpp"

#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"
#include "threads/Atomics.h"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <ctime>

// C++ includes.
#include <vector>
using std::vector;

namespace LibRpBase {

/** RomDataPrivate **/

/**
 * Initialize a RomDataPrivate storage class.
 *
 * @param q RomData class.
 * @param file ROM file.
 * @param fields Array of ROM Field descriptions.
 * @param count Number of ROM Field descriptions.
 */
RomDataPrivate::RomDataPrivate(RomData *q, IRpFile *file)
	: q_ptr(q)
	, ref_cnt(1)
	, isValid(false)
	, file(nullptr)
	, fields(new RomFields())
	, className(nullptr)
	, fileType(RomData::FTYPE_ROM_IMAGE)
{
	if (!file)
		return;

	// dup() the file.
	this->file = file->dup();
}

RomDataPrivate::~RomDataPrivate()
{
	delete fields;

	// Close the file if it's still open.
	delete this->file;
}

/** Convenience functions. **/

static inline int calc_frac_part(int64_t size, int64_t mask)
{
	float f = (float)(size & (mask - 1)) / (float)mask;
	int frac_part = (int)(f * 1000.0f);

	// MSVC added round() and roundf() in MSVC 2013.
	// Use our own rounding code instead.
	int round_adj = (frac_part % 10 > 5);
	frac_part /= 10;
	frac_part += round_adj;
	return frac_part;
}

/**
 * Format a file size.
 * @param size File size.
 * @return Formatted file size.
 */
rp_string RomDataPrivate::formatFileSize(int64_t size)
{
	const char *suffix;
	// frac_part is always 0 to 100.
	// If whole_part >= 10, frac_part is divided by 10.
	int whole_part, frac_part;

	// TODO: Optimize this?
	// TODO: Localize this?
	if (size < 0) {
		// Invalid size. Print the value as-is.
		suffix = "";
		whole_part = (int)size;
		frac_part = 0;
	} else if (size < (2LL << 10)) {
		suffix = (size == 1 ? " byte" : " bytes");
		whole_part = (int)size;
		frac_part = 0;
	} else if (size < (2LL << 20)) {
		suffix = " KB";
		whole_part = (int)(size >> 10);
		frac_part = calc_frac_part(size, (1LL << 10));
	} else if (size < (2LL << 30)) {
		suffix = " MB";
		whole_part = (int)(size >> 20);
		frac_part = calc_frac_part(size, (1LL << 20));
	} else if (size < (2LL << 40)) {
		suffix = " GB";
		whole_part = (int)(size >> 30);
		frac_part = calc_frac_part(size, (1LL << 30));
	} else if (size < (2LL << 50)) {
		suffix = " TB";
		whole_part = (int)(size >> 40);
		frac_part = calc_frac_part(size, (1LL << 40));
	} else if (size < (2LL << 60)) {
		suffix = " PB";
		whole_part = (int)(size >> 50);
		frac_part = calc_frac_part(size, (1LL << 50));
	} else /*if (size < (2ULL << 70))*/ {
		suffix = " EB";
		whole_part = (int)(size >> 60);
		frac_part = calc_frac_part(size, (1LL << 60));
	}

	if (size < (2LL << 10)) {
		// Bytes or negative value. No fractional part.
		return rp_sprintf("%d%s", whole_part, suffix);
	} else {
		// TODO: Localized decimal point?
		int frac_digits = 2;
		if (whole_part >= 10) {
			int round_adj = (frac_part % 10 > 5);
			frac_part /= 10;
			frac_part += round_adj;
			frac_digits = 1;
		}
		return rp_sprintf("%d.%0*d%s",
			whole_part, frac_digits, frac_part, suffix);
	}

	// Should not get here...
	assert(!"Invalid code path.");
	return _RP("QUACK");
}

/**
 * Get the GameTDB URL for a given game.
 * @param system System name.
 * @param type Image type.
 * @param region Region name.
 * @param gameID Game ID.
 * @param ext File extension, e.g. ".png" or ".jpg".
 * TODO: PAL multi-region selection?
 * @return GameTDB URL.
 */
LibRpBase::rp_string RomDataPrivate::getURL_GameTDB(
	const char *system, const char *type,
	const char *region, const char *gameID,
	const char *ext)
{
	return rp_sprintf("http://art.gametdb.com/%s/%s/%s/%s%s",
		system, type, region, gameID, ext);
}

/**
 * Get the GameTDB cache key for a given game.
 * @param system System name.
 * @param type Image type.
 * @param region Region name.
 * @param gameID Game ID.
 * @param ext File extension, e.g. ".png" or ".jpg".
 * TODO: PAL multi-region selection?
 * @return GameTDB cache key.
 */
LibRpBase::rp_string RomDataPrivate::getCacheKey_GameTDB(
	const char *system, const char *type,
	const char *region, const char *gameID,
	const char *ext)
{
	return rp_sprintf("%s/%s/%s/%s%s",
		system, type, region, gameID, ext);
}

/**
 * Select the best size for an image.
 * @param sizeDefs Image size definitions.
 * @param size Requested thumbnail dimension. (assuming a square thumbnail)
 * @return Image size definition, or nullptr on error.
 */
const RomData::ImageSizeDef *RomDataPrivate::selectBestSize(const std::vector<RomData::ImageSizeDef> &sizeDefs, int size)
{
	if (sizeDefs.empty() || size < RomData::IMAGE_SIZE_MIN_VALUE) {
		// No sizes, or invalid size value.
		return nullptr;
	} else if (sizeDefs.size() == 1) {
		// Only one size.
		return &sizeDefs[0];
	}

	// Check for a "special" size value.
	switch (size) {
		case RomData::IMAGE_SIZE_DEFAULT:
			// Default image.
			return &sizeDefs[0];

		case RomData::IMAGE_SIZE_SMALLEST: {
			// Find the smallest image.
			const RomData::ImageSizeDef *ret = &sizeDefs[0];
			int sz = std::min(ret->width, ret->height);
			for (auto iter = sizeDefs.begin()+1; iter != sizeDefs.end(); ++iter) {
				const RomData::ImageSizeDef *sizeDef = &(*iter);
				if (sizeDef->width < sz || sizeDef->height < sz) {
					ret = sizeDef;
					sz = std::min(sizeDef->width, sizeDef->height);
				}
			}
			return ret;
		}

		case RomData::IMAGE_SIZE_LARGEST: {
			// Find the largest image.
			const RomData::ImageSizeDef *ret = &sizeDefs[0];
			int sz = std::max(ret->width, ret->height);
			for (auto iter = sizeDefs.begin()+1; iter != sizeDefs.end(); ++iter) {
				const RomData::ImageSizeDef *sizeDef = &(*iter);
				if (sizeDef->width > sz || sizeDef->height > sz) {
					ret = sizeDef;
					sz = std::max(sizeDef->width, sizeDef->height);
				}
			}
			return ret;
		}

		default:
			break;
	}

	// Find the largest image that has at least one dimension that
	// is >= the requested size. If no image is >= the requested
	// size, use the largest image.
	// TODO: Check width/height separately?
	const RomData::ImageSizeDef *ret = &sizeDefs[0];
	int sz = std::max(ret->width, ret->height);
	if (sz == size) {
		// Found a match already.
		return ret;
	}
	for (auto iter = sizeDefs.cbegin()+1; iter != sizeDefs.cend(); ++iter) {
		const RomData::ImageSizeDef *sizeDef = &(*iter);
		const int szchk = std::max(sizeDef->width, sizeDef->height);
		if (sz >= size) {
			// We already found an image >= size.
			// Only use this image if its largest dimension is
			// >= size and < sz.
			if (szchk >= size && szchk < sz) {
				// Found a better match.
				sz = szchk;
				ret = sizeDef;
			}
		} else {
			// Use this image if its largest dimension is > sz.
			if (szchk > sz) {
				// Found a better match.
				sz = szchk;
				ret = sizeDef;
			}
		}

		if (sz == size) {
			// Exact match!
			// TODO: Verify width/height separately?
			break;
		}
	}

	return ret;
}

/**
 * Convert an ASCII release date in YYYYMMDD format to Unix time_t.
 * This format is used by Sega Saturn and Dreamcast.
 * @param ascii_date ASCII release date. (Must be 8 characters.)
 * @return Unix time_t, or -1 on error.
 */
time_t RomDataPrivate::ascii_yyyymmdd_to_unix_time(const char *ascii_date)
{
	// Release date format: "YYYYMMDD"

	// Convert the date to an unsigned integer first.
	unsigned int ymd = 0;
	for (unsigned int i = 0; i < 8; i++) {
		if (!isdigit(ascii_date[i])) {
			// Invalid digit.
			return -1;
		}
		ymd *= 10;
		ymd += (ascii_date[i] & 0xF);
	}

	// Sanity checks:
	// - Must be higher than 19000101.
	// - Must be lower than 99991231.
	if (ymd < 19000101 || ymd > 99991231) {
		// Invalid date.
		return -1;
	}

	// Convert to Unix time.
	// NOTE: struct tm has some oddities:
	// - tm_year: year - 1900
	// - tm_mon: 0 == January
	struct tm ymdtime;

	ymdtime.tm_year = (ymd / 10000) - 1900;
	ymdtime.tm_mon  = ((ymd / 100) % 100) - 1;
	ymdtime.tm_mday = ymd % 100;

	// Time portion is empty.
	ymdtime.tm_hour = 0;
	ymdtime.tm_min = 0;
	ymdtime.tm_sec = 0;

	// tm_wday and tm_yday are output variables.
	ymdtime.tm_wday = 0;
	ymdtime.tm_yday = 0;
	ymdtime.tm_isdst = 0;

	// If conversion fails, this will return -1.
	return timegm(&ymdtime);
}

/** RomData **/

/**
 * ROM data base class.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * In addition, subclasses must pass an array of RomFieldDesc structs.
 *
 * @param file ROM file.
 * @param fields Array of ROM Field descriptions.
 * @param count Number of ROM Field descriptions.
 */
RomData::RomData(IRpFile *file)
	: d_ptr(new RomDataPrivate(this, file))
{ }

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
 * In addition, subclasses must pass an array of RomFieldDesc structs
 * using an allocated RomDataPrivate subclass.
 *
 * @param d RomDataPrivate subclass.
 */
RomData::RomData(RomDataPrivate *d)
	: d_ptr(d)
{ }

RomData::~RomData()
{
	delete d_ptr;
}

/**
 * Take a reference to this RomData* object.
 * @return this
 */
LibRpBase::RomData *RomData::ref(void)
{
	RP_D(RomData);
	ATOMIC_INC_FETCH(&d->ref_cnt);
	return this;
}

/**
 * Unreference this RomData* object.
 * If ref_cnt reaches 0, the RomData* object is deleted.
 */
void RomData::unref(void)
{
	RP_D(RomData);
	assert(d->ref_cnt > 0);
	if (ATOMIC_DEC_FETCH(&d->ref_cnt) <= 0) {
		// All references removed.
		delete this;
	}
}

/**
 * Is this ROM valid?
 * @return True if it is; false if it isn't.
 */
bool RomData::isValid(void) const
{
	RP_D(const RomData);
	return d->isValid;
}

/**
 * Is the file open?
 * @return True if the file is open; false if it isn't.
 */
bool RomData::isOpen(void) const
{
	RP_D(RomData);
	return (d->file != nullptr);
}

/**
 * Close the opened file.
 */
void RomData::close(void)
{
	RP_D(RomData);
	delete d->file;
	d->file = nullptr;
}

/**
 * Get the class name for the user configuration.
 * @return Class name. (ASCII) (nullptr on error)
 */
const char *RomData::className(void) const
{
	RP_D(const RomData);
	assert(d->className != nullptr);
	return d->className;
}

/**
 * Get the general file type.
 * @return General file type.
 */
RomData::FileType RomData::fileType(void) const
{
	RP_D(const RomData);
	return d->fileType;
}

/**
 * Get the general file type as a string.
 * @return General file type as a string, or nullptr if unknown.
 */
const rp_char *RomData::fileType_string(void) const
{
	RP_D(const RomData);
	assert(d->fileType >= FTYPE_UNKNOWN && d->fileType < FTYPE_LAST);
	if (d->fileType <= FTYPE_UNKNOWN || d->fileType >= FTYPE_LAST) {
		return nullptr;
	}

	static const rp_char *const fileType_names[] = {
		nullptr,			// FTYPE_UNKNOWN
		_RP("ROM Image"),		// FTYPE_ROM_IMAGE
		_RP("Disc Image"),		// FTYPE_DISC_IMAGE
		_RP("Save File"),		// FTYPE_SAVE_FILE
		_RP("Embedded Disc Image"),	// FTYPE_EMBEDDED_DISC_IMAGE
		_RP("Application Package"),	// FTYPE_APPLICATION_PACKAGE
		_RP("NFC Dump"),		// FTYPE_NFC_DUMP
		_RP("Disk Image"),		// FTYPE_DISK_IMAGE
		_RP("Executable"),		// FTYPE_EXECUTABLE
		_RP("Dynamic Link Library"),	// FTYPE_DLL
		_RP("Device Driver"),		// FTYPE_DEVICE_DRIVER
		_RP("Resource Library"),	// FTYPE_RESOURCE_LIBRARY
		_RP("Icon File"),		// FTYPE_ICON_FILE
		_RP("Banner File"),		// FTYPE_BANNER_FILE
		_RP("Homebrew Application"),	// FTYPE_HOMEBREW
		_RP("eMMC Dump"),		// FTYPE_EMMC_DUMP
		_RP("Title Contents"),		// FTYPE_TITLE_CONTENTS
		_RP("Firmware Binary"),		// FTYPE_FIRMWARE_BINARY
		_RP("Texture File"),		// FTYPE_TEXTURE_FILE
	};
	static_assert(ARRAY_SIZE(fileType_names) == FTYPE_LAST,
		"fileType_names[] needs to be updated.");

	return fileType_names[d->fileType];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t RomData::supportedImageTypes(void) const
{
	// No images supported by default.
	return 0;
}

/**
 * Get a list of all available image sizes for the specified image type.
 *
 * The first item in the returned vector is the "default" size.
 * If the width/height is 0, then an image exists, but the size is unknown.
 *
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
std::vector<RomData::ImageSizeDef> RomData::supportedImageSizes(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);

	// No images supported by default.
	RP_UNUSED(imageType);
	return std::vector<ImageSizeDef>();
}

/**
 * Get image processing flags.
 *
 * These specify post-processing operations for images,
 * e.g. applying transparency masks.
 *
 * @param imageType Image type.
 * @return Bitfield of ImageProcessingBF operations to perform.
 */
uint32_t RomData::imgpf(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}

	// No imgpf by default.
	RP_UNUSED(imageType);
	return 0;
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Pointer to const rp_image* to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int RomData::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	assert(pImage != nullptr);
	if (!pImage) {
		// Invalid parameters.
		return -EINVAL;
	} else if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		*pImage = nullptr;
		return -ERANGE;
	}

	// No images supported by the base class.
	RP_UNUSED(imageType);
	*pImage = nullptr;
	return -ENOENT;
}

/**
 * Get the ROM Fields object.
 * @return ROM Fields object.
 */
const RomFields *RomData::fields(void) const
{
	RP_D(const RomData);
	if (!d->fields->isDataLoaded()) {
		// Data has not been loaded.
		// Load it now.
		int ret = const_cast<RomData*>(this)->loadFieldData();
		if (ret < 0)
			return nullptr;
	}
	return d->fields;
}

/**
 * Get an internal image from the ROM.
 *
 * NOTE: The rp_image is owned by this object.
 * Do NOT delete this object until you're done using this rp_image.
 *
 * @param imageType Image type to load.
 * @return Internal image, or nullptr if the ROM doesn't have one.
 */
const rp_image *RomData::image(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		return nullptr;
	}
	// TODO: Check supportedImageTypes()?

	// Load the internal image.
	// The subclass maintains ownership of the image.
	const rp_image *img;
	int ret = const_cast<RomData*>(this)->loadInternalImage(imageType, &img);
	return (ret == 0 ? img : nullptr);
}

/**
 * Get a list of URLs for an external image type.
 *
 * A thumbnail size may be requested from the shell.
 * If the subclass supports multiple sizes, it should
 * try to get the size that most closely matches the
 * requested size.
 *
 * @param imageType	[in]     Image type.
 * @param pExtURLs	[out]    Output vector.
 * @param size		[in,opt] Requested image size. This may be a requested
 *                               thumbnail size in pixels, or an ImageSizeType
 *                               enum value.
 * @return 0 on success; negative POSIX error code on error.
 */
int RomData::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	assert(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_EXT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}
	assert(pExtURLs != nullptr);
	if (!pExtURLs) {
		// No vector.
		return -EINVAL;
	}

	// No external URLs by default.
	RP_UNUSED(size);
	pExtURLs->clear();
	return -ENOENT;
}

/**
 * Scrape an image URL from a downloaded HTML page.
 * Needed if IMGPF_EXTURL_NEEDS_HTML_SCRAPING is set.
 * @param html HTML data.
 * @param size Size of HTML data.
 * @return Image URL, or empty string if not found or not supported.
 */
rp_string RomData::scrapeImageURL(const char *html, size_t size) const
{
	// Not supported in the base class.
	RP_UNUSED(html);
	RP_UNUSED(size);
	return rp_string();
}

/**
* Get name of an image type
* @param imageType Image type.
* @return String containing user-friendly name of an image type.
*/
const rp_char *RomData::getImageTypeName(ImageType imageType) {
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		return nullptr;
	}

	static const rp_char *const image_type_names[] = {
		// Internal
		_RP("Internal icon"),				// IMG_INT_ICON
		_RP("Internal banner"),				// IMG_INT_BANNER
		_RP("Internal media scan"),			// IMG_INT_MEDIA
		_RP("Internal image"),				// IMG_INT_IMAGE
		// External
		_RP("External media scan"),			// IMG_EXT_MEDIA
		_RP("External cover scan"),			// IMG_EXT_COVER
		_RP("External cover scan (3D version)"),	// IMG_EXT_COVER_3D
		_RP("External cover scan (front and back)"),	// IMG_EXT_COVER_FULL
		_RP("External box scan"),			// IMG_EXT_BOX
	};
	static_assert(ARRAY_SIZE(image_type_names) == IMG_EXT_MAX + 1,
		"image_type_names[] needs to be updated.");

	return image_type_names[imageType];
}

/**
 * Get the animated icon data.
 *
 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
 * object has an animated icon.
 *
 * @return Animated icon data, or nullptr if no animated icon is present.
 */
const IconAnimData *RomData::iconAnimData(void) const
{
	// No animated icon by default.
	return nullptr;
}

}
