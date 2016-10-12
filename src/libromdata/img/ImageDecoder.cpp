/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ImageDecoder.cpp: Image decoding functions.                             *
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

#include "ImageDecoder.hpp"
#include "img/rp_image.hpp"
#include "byteswap.h"

// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData {

class ImageDecoderPrivate
{
	private:
		// ImageDecoderPrivate is a static class.
		ImageDecoderPrivate();
		~ImageDecoderPrivate();
		ImageDecoderPrivate(const ImageDecoderPrivate &other);
		ImageDecoderPrivate &operator=(const ImageDecoderPrivate &other);

	public:
		/**
		 * Blit a tile to an rp_image.
		 * NOTE: No bounds checking is done.
		 * @tparam pixel	[in] Pixel type.
		 * @tparam tileW	[in] Tile width.
		 * @tparam tileH	[in] Tile height.
		 * @param img		[out] rp_image.
		 * @param tileBuf	[in] Tile buffer.
		 * @param tileX		[in] Horizontal tile number.
		 * @param tileY		[in] Vertical tile number.
		 */
		template<typename pixel, int tileW, int tileH>
		static inline void BlitTile(rp_image *img, const pixel *tileBuf, int tileX, int tileY);

		/**
		 * Convert an RGB555 pixel to ARGB32.
		 * @param px16 RGB555 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t RGB555_to_ARGB32(uint16_t px16);

		/**
		 * Convert an RGB5A3 pixel to ARGB32. (GameCube/Wii)
		 * @param px16 RGB5A3 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t RGB5A3_to_ARGB32(uint16_t px16);
};

/** ImageDecoderPrivate **/

/**
 * Blit a tile to an rp_image.
 * NOTE: No bounds checking is done.
 * @tparam pixel	[in] Pixel type.
 * @tparam tileW	[in] Tile width.
 * @tparam tileH	[in] Tile height.
 * @param img		[out] rp_image.
 * @param tileBuf	[in] Tile buffer.
 * @param tileX		[in] Horizontal tile number.
 * @param tileY		[in] Vertical tile number.
 */
template<typename pixel, int tileW, int tileH>
inline void BlitTile(rp_image *img, const pixel *tileBuf, int tileX, int tileY)
{
	switch (sizeof(pixel)) {
		case 4:
			assert(img->format() == rp_image::FORMAT_ARGB32);
			break;
		case 1:
			assert(img->format() == rp_image::FORMAT_CI8);
			break;
		default:
			assert(!"Unsupported sizeof(pixel).");
			return;
	}

	// Go to the first pixel for this tile.
	const int stride_px = img->stride() / sizeof(pixel);
	pixel *imgBuf = reinterpret_cast<pixel*>(img->scanLine(tileY * tileH));
	imgBuf += (tileX * tileW);

	for (int y = tileH; y > 0; y--) {
		memcpy(imgBuf, tileBuf, (tileW * sizeof(pixel)));
		imgBuf += stride_px;
		tileBuf += tileW;
	}
}

/**
 * Convert an RGB555 pixel to ARGB32.
 * @param px16 RGB555 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::RGB555_to_ARGB32(uint16_t px16)
{
	// NOTE: px16 has already been byteswapped.
	uint32_t px32;

	// BGR555: xBBBBBGG GGGRRRRR
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	px32 = ((((px16 << 19) & 0xF80000) | ((px16 << 17) & 0x070000))) |	// Red
	       ((((px16 <<  6) & 0x00F800) | ((px16 <<  1) & 0x000700))) |	// Green
	       ((((px16 >>  7) & 0x0000F8) | ((px16 >> 12) & 0x000007)));	// Blue

	// No alpha channel.
	px32 |= 0xFF000000U;
	return px32;
}

/**
 * Convert an RGB5A3 pixel to ARGB32. (GameCube/Wii)
 * @param px16 RGB5A3 pixel. (Must be host-endian.)
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::RGB5A3_to_ARGB32(uint16_t px16)
{
	uint32_t px32 = 0;

	if (px16 & 0x8000) {
		// RGB555: xRRRRRGG GGGBBBBB
		// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
		px32 |= (((px16 << 3) & 0x0000F8) | ((px16 >> 2) & 0x000007));	// B
		px32 |= (((px16 << 6) & 0x00F800) | ((px16 << 1) & 0x000700));	// G
		px32 |= (((px16 << 9) & 0xF80000) | ((px16 << 4) & 0x070000));	// R
		px32 |= 0xFF000000U; // no alpha channel
	} else {
		// RGB4A3
		px32 |= (((px16 & 0x000F) << 4)  |  (px16 & 0x000F));		// B
		px32 |= (((px16 & 0x00F0) << 8)  | ((px16 & 0x00F0) << 4));	// G
		px32 |= (((px16 & 0x0F00) << 12) | ((px16 & 0x0F00) << 8));	// R

		// Calculate the alpha channel.
		uint8_t a = ((px16 >> 7) & 0xE0);
		a |= (a >> 3);
		a |= (a >> 3);

		// Apply the alpha channel.
		px32 |= (a << 24);
	}

	return px32;
}

/** ImageDecoder **/

/**
 * Convert a Nintendo DS 16-color + palette image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 0x20]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromNDS_CI4(int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz)
{
	// Verify parameters.
	if (!img_buf || !pal_buf)
		return nullptr;
	else if (width < 0 || height < 0)
		return nullptr;
	else if (img_siz < ((width * height) / 2) || pal_siz < 0x20)
		return nullptr;

	// NDS 16-color images use 4x4 tiles.
	if (width % 4 != 0 || height % 4 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const int tilesX = (width / 4);
	const int tilesY = (height / 4);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_CI8);

	// Convert the palette.
	// TODO: Optimize using pointers instead of indexes?
	uint32_t *palette = img->palette();
	assert(img->palette_len() >= 16);
	if (img->palette_len() < 16) {
		// Not enough colors...
		delete img;
		return nullptr;
	}

	palette[0] = 0; // Color 0 is always transparent.
	img->set_tr_idx(0);
	for (int i = 1; i < 16; i++) {
		// NDS color format is BGR555.
		palette[i] = ImageDecoderPrivate::RGB555_to_ARGB32(le16_to_cpu(pal_buf[i]));
	}

	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.

	// 2 bytes = 4 pixels
	// Image is composed of 8x8px tiles.
	// 4 tiles wide, 4 tiles tall.
	uint8_t tileBuf[8*8];

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			// Convert each tile to 8-bit color manually.
			for (int i = 0; i < 8*8; i += 2, img_buf++) {
				tileBuf[i+0] = (*img_buf & 0x0F);
				tileBuf[i+1] = (*img_buf >> 4);
			}

			// Blit the tile to the main image buffer.
			BlitTile<uint8_t, 8, 8>(img, tileBuf, x, y);
		}
	}

	// Image has been converted.
	return img;
}

/**
 * Convert a GameCube RGB5A3 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf RGB5A3 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromGcnRGB5A3(int width, int height,
	const uint16_t *img_buf, int img_siz)
{
	// Verify parameters.
	if (!img_buf)
		return nullptr;
	else if (width < 0 || height < 0)
		return nullptr;
	else if (img_siz < ((width * height) * 2))
		return nullptr;

	// RGB5A3 uses 4x4 tiles.
	if (width % 4 != 0 || height % 4 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const int tilesX = (width / 4);
	const int tilesY = (height / 4);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);

	// Temporary tile buffer.
	uint32_t tileBuf[4*4];

	for (int y = 0; y < tilesY; y++) {
		for (int x = 0; x < tilesX; x++) {
			// Convert each tile to ARGB888 manually.
			// TODO: Optimize using pointers instead of indexes?
			for (int i = 0; i < 4*4; i++, img_buf++) {
				tileBuf[i] = ImageDecoderPrivate::RGB5A3_to_ARGB32(be16_to_cpu(*img_buf));
			}

			// Blit the tile to the main image buffer.
			BlitTile<uint32_t, 4, 4>(img, tileBuf, x, y);
		}
	}

	// Image has been converted.
	return img;
}

}
