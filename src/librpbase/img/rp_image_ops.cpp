/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * rp_image_ops.cpp: Image class. (operations)                             *
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

#include "rp_image.hpp"
#include "rp_image_p.hpp"
#include "rp_image_backend.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <algorithm>

// Workaround for RP_D() expecting the no-underscore, UpperCamelCase naming convention.
#define rp_imagePrivate rp_image_private

namespace LibRpBase {

/** Image operations. **/

/**
 * Duplicate the rp_image.
 * @return New rp_image with a copy of the image data.
 */
rp_image *rp_image::dup(void) const
{
	RP_D(const rp_image);
	const int width = d->backend->width;
	const int height = d->backend->height;
	const rp_image::Format format = d->backend->format;
	assert(width != 0);
	assert(height != 0);

	rp_image *img = new rp_image(width, height, format);
	if (!img->isValid()) {
		// Image is invalid. Return it immediately.
		return img;
	}

	// Copy the image.
	// NOTE: Using uint8_t* because stride is measured in bytes.
	uint8_t *dest = static_cast<uint8_t*>(img->bits());
	const uint8_t *src = static_cast<const uint8_t*>(d->backend->data());
	const int dest_stride = img->stride();
	const int src_stride = d->backend->stride;

	int row_bytes;
	switch (format) {
		case rp_image::FORMAT_CI8:
			row_bytes = width;
			break;
		case rp_image::FORMAT_ARGB32:
			row_bytes = width * sizeof(uint32_t);
			break;
		default:
			assert(!"Unsupported rp_image::Format.");
			delete img;
			return nullptr;
	}

	for (unsigned int y = height; y > 0; y--) {
		memcpy(dest, src, row_bytes);
		dest += dest_stride;
		src += src_stride;
	}

	// If CI8, copy the palette.
	if (format == rp_image::FORMAT_CI8) {
		int entries = std::min(img->palette_len(), d->backend->palette_len());
		uint32_t *const dest_pal = img->palette();
		memcpy(dest_pal, d->backend->palette(), entries * sizeof(uint32_t));
		if (img->palette_len() < d->backend->palette_len()) {
			// Zero the remaining entries.
			int zero_entries = d->backend->palette_len() - img->palette_len();
			memset(&dest_pal[entries], 0, zero_entries * sizeof(uint32_t));
		}
	}

	return img;
}

/**
 * Duplicate the rp_image, converting to ARGB32 if necessary.
 * @return New ARGB32 rp_image with a copy of the image data.
 */
rp_image *rp_image::dup_ARGB32(void) const
{
	RP_D(const rp_image);
	if (d->backend->format == FORMAT_ARGB32) {
		// Already in ARGB32.
		// Do a direct dup().
		return this->dup();
	} else if (d->backend->format != FORMAT_CI8) {
		// Only CI8->ARGB32 is supported right now.
		return nullptr;
	}

	const int width = d->backend->width;
	const int height = d->backend->height;
	const rp_image::Format format = d->backend->format;
	assert(width != 0);
	assert(height != 0);

	// TODO: Handle palette length smaller than 256.
	assert(d->backend->palette_len() == 256);
	if (d->backend->palette_len() != 256) {
		return nullptr;
	}

	rp_image *img = new rp_image(width, height, FORMAT_ARGB32);
	if (!img->isValid()) {
		// Image is invalid. Something went wrong.
		delete img;
		return nullptr;
	}

	// Copy the image, converting from CI8 to ARGB32.
	uint32_t *dest = static_cast<uint32_t*>(img->bits());
	const uint8_t *src = static_cast<const uint8_t*>(d->backend->data());
	const uint32_t *pal = d->backend->palette();
	const int dest_adj = (img->stride() / 4) - width;
	const int src_adj = d->backend->stride - width;

	for (unsigned int y = (unsigned int)height; y > 0; y--) {
		// Convert up to 4 pixels per loop iteration.
		unsigned int x;
		for (x = (unsigned int)width; x > 3; x -= 4) {
			dest[0] = pal[src[0]];
			dest[1] = pal[src[1]];
			dest[2] = pal[src[2]];
			dest[3] = pal[src[3]];
			dest += 4;
			src += 4;
		}
		// Remaining pixels.
		for (; x > 0; x--) {
			*dest = pal[*src];
			dest++;
			src++;
		}

		// Next line.
		dest += dest_adj;
		src += src_adj;
	}

	// Converted to ARGB32.
	return img;
}

/**
 * Square the rp_image.
 *
 * If the width and height don't match, transparent rows
 * and/or columns will be added to "square" the image.
 * Otherwise, this is the same as dup().
 *
 * @return New rp_image with a squared version of the original.
 */
rp_image *rp_image::squared(void) const
{
	// Windows doesn't like non-square icons.
	// Add extra transparent columns/rows before
	// converting to HBITMAP.
	RP_D(const rp_image);
	const int width = d->backend->width;
	const int height = d->backend->height;
	rp_image *sq_img = nullptr;

	if (width == height) {
		// Image is already square. dup() it.
		return this->dup();
	} else if (width > height) {
		// Image is wider. Add rows to the top and bottom.
		// TODO: 8bpp support?
		assert(d->backend->format == rp_image::FORMAT_ARGB32);
		if (d->backend->format != rp_image::FORMAT_ARGB32) {
			// Cannot resize this image.
			// Use dup() instead.
			return this->dup();
		}
		sq_img = new rp_image(width, width, rp_image::FORMAT_ARGB32);
		if (!sq_img->isValid()) {
			// Could not allocate the image.
			delete sq_img;
			return nullptr;
		}

		const int addToTop = (width-height)/2;
		const int addToBottom = addToTop + ((width-height)%2);

		// NOTE: Using uint8_t* because stride is measured in bytes.
		uint8_t *dest = static_cast<uint8_t*>(sq_img->bits());
		const uint8_t *src = static_cast<const uint8_t*>(d->backend->data());
		const int dest_stride = sq_img->stride();
		const int src_stride = d->backend->stride;

		// Clear the top rows.
		memset(dest, 0, addToTop * dest_stride);
		dest += (addToTop * dest_stride);

		// Copy the image data.
		const int row_bytes = width * sizeof(uint32_t);
		for (unsigned int y = height; y > 0; y--) {
			memcpy(dest, src, row_bytes);
			dest += dest_stride;
			src += src_stride;
		}

		// Clear the bottom rows.
		// NOTE: Last row may not be the full stride.
		memset(dest, 0, ((addToBottom-1) * dest_stride) + (width * sizeof(uint32_t)));
	} else if (width < height) {
		// Image is taller. Add columns to the left and right.
		// TODO: 8bpp support?
		assert(d->backend->format == rp_image::FORMAT_ARGB32);
		if (d->backend->format != rp_image::FORMAT_ARGB32) {
			// Cannot resize this image.
			// Use dup() instead.
			return this->dup();
		}
		sq_img = new rp_image(height, height, rp_image::FORMAT_ARGB32);
		if (!sq_img->isValid()) {
			// Could not allocate the image.
			delete sq_img;
			return nullptr;
		}

		// NOTE: Mega Man Gold amiibo is "shifting" by 1px when
		// refreshing in Win7. (switching from icon to thumbnail)
		// Not sure if this can be fixed easily.
		const int addToLeft = (height-width)/2;
		const int addToRight = addToLeft + ((height-width)%2);

		// NOTE: Using uint8_t* because stride is measured in bytes.
		uint8_t *dest = static_cast<uint8_t*>(sq_img->bits());
		const uint8_t *src = static_cast<const uint8_t*>(d->backend->data());
		// "Blanking" area is right border, potential unused space from stride, then left border.
		const int dest_blanking = sq_img->stride() - (width * sizeof(uint32_t));
		const int dest_stride = sq_img->stride();
		const int src_stride = d->backend->stride;

		// Clear the left part of the first row.
		memset(dest, 0, addToLeft * sizeof(uint32_t));
		dest += (addToLeft * sizeof(uint32_t));

		// Copy and clear all but the last line.
		const int row_bytes = width * sizeof(uint32_t);
		for (unsigned int y = (height-1); y > 0; y--) {
			memcpy(dest, src, row_bytes);
			memset(&dest[row_bytes], 0, dest_blanking);
			dest += dest_stride;
			src += src_stride;
		}

		// Copy the last line.
		// NOTE: Last row may not be the full stride.
		memcpy(dest, src, row_bytes);
		// Clear the end of the line.
		memset(&dest[row_bytes], 0, addToRight * sizeof(uint32_t));
	}

	return sq_img;
}

}
