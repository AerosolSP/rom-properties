/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_p.hpp: Image decoding functions. (PRIVATE CLASS)           *
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

#include "common.h"
#include "img/rp_image.hpp"
#include "byteswap.h"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

namespace LibRpBase {

class ImageDecoderPrivate
{
	private:
		// ImageDecoderPrivate is a static class.
		ImageDecoderPrivate();
		~ImageDecoderPrivate();
		RP_DISABLE_COPY(ImageDecoderPrivate)

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
		 * Blit a CI4 tile to a CI8 rp_image.
		 * NOTE: Left pixel is the least significant nybble.
		 * NOTE: No bounds checking is done.
		 * @tparam tileW	[in] Tile width.
		 * @tparam tileH	[in] Tile height.
		 * @param img		[out] rp_image.
		 * @param tileBuf	[in] Tile buffer.
		 * @param tileX		[in] Horizontal tile number.
		 * @param tileY		[in] Vertical tile number.
		 */
		template<int tileW, int tileH>
		static inline void BlitTile_CI4_LeftLSN(rp_image *img, const uint8_t *tileBuf, int tileX, int tileY);

		/**
		 * Convert a BGR555 pixel to ARGB32.
		 * @param px16 BGR555 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t BGR555_to_ARGB32(uint16_t px16);

		/**
		 * Convert an RGB5A3 pixel to ARGB32. (GameCube/Wii)
		 * @param px16 RGB5A3 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t RGB5A3_to_ARGB32(uint16_t px16);

		/**
		 * Convert an ARGB4444 pixel to ARGB32. (Dreamcast)
		 * @param px16 ARGB4444 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t ARGB4444_to_ARGB32(uint16_t px16);

		/**
		 * Convert an RGB565 pixel to ARGB32. (Dreamcast)
		 * @param px16 RGB565 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t RGB565_to_ARGB32(uint16_t px16);
};

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
inline void ImageDecoderPrivate::BlitTile(rp_image *img, const pixel *tileBuf, int tileX, int tileY)
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
	pixel *imgBuf = static_cast<pixel*>(img->scanLine(tileY * tileH));
	imgBuf += (tileX * tileW);

	for (int y = tileH; y > 0; y--) {
		memcpy(imgBuf, tileBuf, (tileW * sizeof(pixel)));
		imgBuf += stride_px;
		tileBuf += tileW;
	}
}

/**
 * Blit a CI4 tile to a CI8 rp_image.
 * NOTE: Left pixel is the least significant nybble.
 * NOTE: No bounds checking is done.
 * @tparam tileW	[in] Tile width.
 * @tparam tileH	[in] Tile height.
 * @param img		[out] rp_image.
 * @param tileBuf	[in] Tile buffer.
 * @param tileX		[in] Horizontal tile number.
 * @param tileY		[in] Vertical tile number.
 */
template<int tileW, int tileH>
inline void ImageDecoderPrivate::BlitTile_CI4_LeftLSN(rp_image *img, const uint8_t *tileBuf, int tileX, int tileY)
{
	assert(img->format() == rp_image::FORMAT_CI8);
	assert(img->width() % 2 == 0);
	assert(tileW % 2 == 0);

	// Go to the first pixel for this tile.
	uint8_t *imgBuf = static_cast<uint8_t*>(img->scanLine(tileY * tileH));
	imgBuf += (tileX * tileW);

	const int stride_px_adj = img->stride() - tileW;
	for (int y = tileH; y > 0; y--) {
		// Expand CI4 pixels to CI8 before writing.
		for (int x = tileW; x > 0; x -= 2) {
			imgBuf[0] = (*tileBuf & 0x0F);
			imgBuf[1] = (*tileBuf >> 4);
			imgBuf += 2;
			tileBuf++;
		}

		// Next line.
		imgBuf += stride_px_adj;
	}
}

/**
 * Convert a BGR555 pixel to ARGB32.
 * @param px16 BGR555 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::BGR555_to_ARGB32(uint16_t px16)
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
		// BGR555: xRRRRRGG GGGBBBBB
		// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
		px32 |= (((px16 << 3) & 0x0000F8) | ((px16 >> 2) & 0x000007));	// B
		px32 |= (((px16 << 6) & 0x00F800) | ((px16 << 1) & 0x000700));	// G
		px32 |= (((px16 << 9) & 0xF80000) | ((px16 << 4) & 0x070000));	// R
		px32 |= 0xFF000000U; // no alpha channel
	} else {
		// RGB4A3
		px32  =  (px16 & 0x000F);	// B
		px32 |= ((px16 & 0x00F0) << 4);	// G
		px32 |= ((px16 & 0x0F00) << 8);	// R
		px32 |= (px32 << 4);		// Copy to the top nybble.

		// Calculate the alpha channel.
		uint8_t a = ((px16 >> 7) & 0xE0);
		a |= (a >> 3);
		a |= (a >> 3);

		// Apply the alpha channel.
		px32 |= (a << 24);
	}

	return px32;
}

/**
 * Convert an ARGB4444 pixel to ARGB32. (Dreamcast)
 * @param px16 ARGB4444 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::ARGB4444_to_ARGB32(uint16_t px16)
{
	uint32_t px32;
	px32  =  (px16 & 0x000F);		// B
	px32 |= ((px16 & 0x00F0) << 4);		// G
	px32 |= ((px16 & 0x0F00) << 8);		// R
	px32 |= ((px16 & 0xF000) << 12);	// A
	px32 |=  (px32 << 4);			// Copy to the top nybble.
	return px32;
}

/**
 * Convert an RGB565 pixel to ARGB32. (Dreamcast)
 * @param px16 RGB565 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::RGB565_to_ARGB32(uint16_t px16)
{
	// NOTE: px16 has already been byteswapped.
	uint32_t px32;

	// RGB555: RRRRRGGG GGGBBBBB
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	px32 = ((((px16 <<  8) & 0xF80000) | ((px16 <<  3) & 0x070000))) |	// Red
	       ((((px16 <<  5) & 0x00FC00) | ((px16 >>  1) & 0x000300))) |	// Green
	       ((((px16 <<  3) & 0x0000F8) | ((px16 >>  2) & 0x000007)));	// Blue

	// No alpha channel.
	px32 |= 0xFF000000U;
	return px32;
}

}
