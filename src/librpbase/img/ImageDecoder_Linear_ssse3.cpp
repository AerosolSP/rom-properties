/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_Linear.cpp: Image decoding functions. (Linear)             *
 * SSSE3-optimized version.                                                *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "ImageDecoder.hpp"
#include "ImageDecoder_p.hpp"

// C includes. (C++ namespace)
#include <cassert>

// SSSE3 headers.
#include <xmmintrin.h>
#include <emmintrin.h>
#include <tmmintrin.h>

namespace LibRpBase {

/**
 * Convert a linear 24-bit RGB image to rp_image.
 * SSSE3-optimized version.
 * @param px_format	[in] 24-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] Image buffer. (must be byte-addressable)
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromLinear24_ssse3(PixelFormat px_format,
	int width, int height,
	const uint8_t *img_buf, int img_siz, int stride)
{
	ASSERT_ALIGNMENT(16, img_buf);
	static const int bytespp = 3;

	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) * bytespp));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) * bytespp))
	{
		return nullptr;
	}

	// Stride adjustment.
	int src_stride_adj = 0;
	assert(stride >= 0);
	if (stride > 0) {
		// Set src_stride_adj to the number of pixels we need to
		// add to the end of each line to get to the next row.
		assert(stride % bytespp == 0);
		if (stride % bytespp != 0) {
			// Invalid stride.
			return nullptr;
		}
		// Byte addressing, so keep it in units of bytespp.
		src_stride_adj = (width * bytespp) - stride;
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}
	const int dest_stride_adj = (img->stride() / sizeof(argb32_t)) - img->width();
	argb32_t *px_dest = static_cast<argb32_t*>(img->bits());

	// SSSE3-optimized version based on:
	// - https://stackoverflow.com/questions/2973708/fast-24-bit-array-32-bit-array-conversion
	// - https://stackoverflow.com/a/2974266

	// 24-bit RGB images don't have an alpha channel.
	__m128i alpha_mask = _mm_setr_epi8(0,0,0,-1, 0,0,0,-1, 0,0,0,-1, 0,0,0,-1);

	// Determine the byte shuffle mask.
	__m128i shuf_mask;
	switch (px_format) {
		case PXF_RGB888:
			shuf_mask = _mm_setr_epi8(0,1,2,-1, 3,4,5,-1, 6,7,8,-1, 9,10,11,-1);
			break;
		case PXF_BGR888:
			shuf_mask = _mm_setr_epi8(2,1,0,-1, 5,4,3,-1, 8,7,6,-1, 11,10,9,-1);
			break;
		default:
			assert(!"Unsupported 24-bit pixel format.");
			return nullptr;
	}

	for (unsigned int y = (unsigned int)height; y > 0; y--) {
		// Process 16 pixels per iteration using SSSE3.
		unsigned int x = (unsigned int)width;
		for (; x > 15; x -= 16, px_dest += 16, img_buf += 16*3) {
			const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
			__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

			__m128i sa = _mm_load_si128(xmm_src);
			__m128i sb = _mm_load_si128(xmm_src+1);
			__m128i sc = _mm_load_si128(xmm_src+2);

			__m128i val = _mm_shuffle_epi8(sa, shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(xmm_dest, val);
			val = _mm_shuffle_epi8(_mm_alignr_epi8(sb, sa, 12), shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(xmm_dest+1, val);
			val = _mm_shuffle_epi8(_mm_alignr_epi8(sc, sb, 8), shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(xmm_dest+2, val);
			val = _mm_shuffle_epi8(_mm_alignr_epi8(sc, sc, 4), shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(xmm_dest+3, val);
		}

		// Remaining pixels.
		switch (px_format) {
			case PXF_RGB888:
				for (; x > 0; x--, px_dest++, img_buf += 3) {
					px_dest->b = img_buf[0];
					px_dest->g = img_buf[1];
					px_dest->r = img_buf[2];
					px_dest->a = 0xFF;
				}
				break;

			case PXF_BGR888:
				for (; x > 0; x--, px_dest++, img_buf += 3) {
					px_dest->b = img_buf[2];
					px_dest->g = img_buf[1];
					px_dest->r = img_buf[0];
					px_dest->a = 0xFF;
				}
				break;

			default:
				assert(!"Unsupported 24-bit pixel format.");
				return nullptr;
		}

		// Next line.
		img_buf += src_stride_adj;
		px_dest += dest_stride_adj;
	}

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {8,8,8,0,0};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert a linear 32-bit RGB image to rp_image.
 * SSSE3-optimized version.
 * @param px_format	[in] 32-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 32-bit image buffer.(must be byte-addressable)
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromLinear32_ssse3(PixelFormat px_format,
	int width, int height,
	const uint32_t *img_buf, int img_siz, int stride)
{
	ASSERT_ALIGNMENT(16, img_buf);
	static const int bytespp = 4;

	// FIXME: Add support for these formats.
	// For now, redirect back to the C++ version.
	switch (px_format) {
		case PXF_G16R16:
		case PXF_A2R10G10B10:
		case PXF_A2B10G10R10:
			return fromLinear32_cpp(px_format, width, height, img_buf, img_siz, stride);

		default:
			break;
	}

	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) * bytespp));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) * bytespp))
	{
		return nullptr;
	}

	// Stride adjustment.
	int src_stride_adj = 0;
	assert(stride >= 0);
	if (stride > 0) {
		// Set src_stride_adj to the number of pixels we need to
		// add to the end of each line to get to the next row.
		assert(stride % bytespp == 0);
		if (stride % bytespp != 0) {
			// Invalid stride.
			return nullptr;
		}
		src_stride_adj = width - (stride / bytespp);
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}
	const int dest_stride_adj = (img->stride() / sizeof(uint32_t)) - img->width();
	uint32_t *px_dest = static_cast<uint32_t*>(img->bits());

	// sBIT for standard ARGB32.
	static const rp_image::sBIT_t sBIT_32 = {8,8,8,0,8};

	if (px_format == PXF_HOST_ARGB32) {
		// Host-endian ARGB32.
		// We can directly copy the image data without conversions.
		if (stride == img->stride()) {
			// Stride is identical. Copy the whole image all at once.
			memcpy(img->bits(), img_buf, stride * height);
		} else {
			// Stride is not identical. Copy each scanline.
			uint32_t *px_dest = static_cast<uint32_t*>(img->bits());
			const unsigned int copy_len = (unsigned int)width * bytespp;
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				memcpy(px_dest, img_buf, copy_len);
				img_buf += (stride / bytespp);
				px_dest += dest_stride_adj;
			}
		}
		// Set the sBIT metadata.
		img->set_sBIT(&sBIT_32);
		return img;
	}

	// TODO: PXF_HOST_xRGB32 - apply alpha mask but no shuffling.

	// SSSE3-optimized version based on:
	// - https://stackoverflow.com/questions/2973708/fast-24-bit-array-32-bit-array-conversion
	// - https://stackoverflow.com/a/2974266

	// 32-bit xRGB images don't have an alpha channel.
	//                                 B G R  A
	__m128i alpha_mask = _mm_setr_epi8(0,0,0,-1, 0,0,0,-1, 0,0,0,-1, 0,0,0,-1);

	// Determine the byte shuffle mask.
	__m128i shuf_mask;
	bool has_alpha;
	switch (px_format) {
		case PXF_HOST_ARGB32:
			assert(!"ARGB32 is handled separately.");
			return nullptr;
		case PXF_HOST_xRGB32:
			// TODO: Only apply the alpha mask instead of shuffling.
			shuf_mask = _mm_setr_epi8(0,1,2,3, 4,5,6,7, 8,9,10,11, 12,13,14,15);
			has_alpha = false;
			break;

		case PXF_HOST_RGBA32:
		case PXF_HOST_RGBx32:
			shuf_mask = _mm_setr_epi8(1,2,3,0, 5,6,7,4, 9,10,11,8, 13,14,15,12);
			has_alpha = (px_format == PXF_HOST_RGBA32);
			break;

		case PXF_SWAP_ARGB32:
		case PXF_SWAP_xRGB32:
			shuf_mask = _mm_setr_epi8(3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12);
			has_alpha = (px_format == PXF_SWAP_ARGB32);
			break;

		case PXF_SWAP_RGBA32:
		case PXF_SWAP_RGBx32:
			shuf_mask = _mm_setr_epi8(2,1,0,3, 6,5,4,7, 10,9,8,11, 14,13,12,15);
			has_alpha = (px_format == PXF_SWAP_RGBA32);
			break;

		default:
			assert(!"Unsupported 32-bit pixel format.");
			delete img;
			return nullptr;
	}

	for (unsigned int y = (unsigned int)height; y > 0; y--) {
		// Process 16 pixels per iteration using SSSE3.
		unsigned int x = (unsigned int)width;
		if (has_alpha) {
			for (; x > 15; x -= 16, px_dest += 16, img_buf += 16) {
				const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
				__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

				__m128i sa = _mm_load_si128(xmm_src);
				__m128i sb = _mm_load_si128(xmm_src+1);
				__m128i sc = _mm_load_si128(xmm_src+2);
				__m128i sd = _mm_load_si128(xmm_src+3);

				__m128i val = _mm_shuffle_epi8(sa, shuf_mask);
				_mm_store_si128(xmm_dest, val);

				val = _mm_shuffle_epi8(sb, shuf_mask);
				_mm_store_si128(xmm_dest+1, val);

				val = _mm_shuffle_epi8(sc, shuf_mask);
				_mm_store_si128(xmm_dest+2, val);

				val = _mm_shuffle_epi8(sd, shuf_mask);
				_mm_store_si128(xmm_dest+3, val);
			}
		} else {
				const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
				__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

				__m128i sa = _mm_load_si128(xmm_src);
				__m128i sb = _mm_load_si128(xmm_src+1);
				__m128i sc = _mm_load_si128(xmm_src+2);
				__m128i sd = _mm_load_si128(xmm_src+3);

				__m128i val = _mm_shuffle_epi8(sa, shuf_mask);
				val = _mm_or_si128(val, alpha_mask);
				_mm_store_si128(xmm_dest, val);

				val = _mm_shuffle_epi8(sb, shuf_mask);
				val = _mm_or_si128(val, alpha_mask);
				_mm_store_si128(xmm_dest+1, val);

				val = _mm_shuffle_epi8(sc, shuf_mask);
				val = _mm_or_si128(val, alpha_mask);
				_mm_store_si128(xmm_dest+2, val);

				val = _mm_shuffle_epi8(sd, shuf_mask);
				val = _mm_or_si128(val, alpha_mask);
				_mm_store_si128(xmm_dest+3, val);
		}

		// Remaining pixels.
		switch (px_format) {
			case PXF_HOST_RGBA32:
				// Host-endian RGBA32.
				// Pixel copy is needed, with shifting.
				for (; x > 0; x--) {
					*px_dest = (*img_buf >> 8) | (*img_buf << 24);
					img_buf++;
					px_dest++;
				}
				break;

			case PXF_HOST_xRGB32:
				// Host-endian XRGB32.
				// Pixel copy is needed, with alpha channel masking.
				for (; x > 0; x--) {
					*px_dest = *img_buf | 0xFF000000;
					img_buf++;
					px_dest++;
				}
				break;

			case PXF_HOST_RGBx32:
				// Host-endian RGBx32.
				// Pixel copy is needed, with a right shift.
				for (; x > 0; x--) {
					*px_dest = (*img_buf >> 8) | 0xFF000000;
					img_buf++;
					px_dest++;
				}
				break;

			case PXF_SWAP_ARGB32:
				// Byteswapped ARGB32.
				// Pixel copy is needed, with byteswapping.
				for (; x > 0; x--) {
					*px_dest = __swab32(*img_buf);
					img_buf++;
					px_dest++;
				}
				break;

			case PXF_SWAP_RGBA32:
				// Byteswapped ABGR32.
				// Pixel copy is needed, with shifting.
				for (; x > 0; x--) {
					const uint32_t px = __swab32(*img_buf);
					*px_dest = (px >> 8) | (px << 24);
					img_buf++;
					px_dest++;
				}
				break;

			case PXF_SWAP_xRGB32:
				// Byteswapped XRGB32.
				// Pixel copy is needed, with byteswapping and alpha channel masking.
				for (; x > 0; x--) {
					*px_dest = __swab32(*img_buf) | 0xFF000000;
					img_buf++;
					px_dest++;
				}
				break;

			case PXF_SWAP_RGBx32:
				// Byteswapped RGBx32.
				// Pixel copy is needed, with byteswapping and a right shift.
				for (; x > 0; x--) {
					*px_dest = (__swab32(*img_buf) >> 8) | 0xFF000000;
					img_buf++;
					px_dest++;
				}
				break;

			default:
				assert(!"Unsupported 32-bit pixel format.");
				delete img;
				return nullptr;
		}

		// Next line.
		img_buf += src_stride_adj;
		px_dest += dest_stride_adj;
	}

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT_x32 = {8,8,8,0,0};
	static const rp_image::sBIT_t sBIT_A32 = {8,8,8,0,8};
	img->set_sBIT(has_alpha ? &sBIT_A32 : &sBIT_x32);

	// Image has been converted.
	return img;
}

}
