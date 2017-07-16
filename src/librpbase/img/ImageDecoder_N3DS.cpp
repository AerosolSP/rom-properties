/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_GCN.cpp: Image decoding functions. (GameCube)              *
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

#include "ImageDecoder.hpp"
#include "ImageDecoder_p.hpp"

namespace LibRpBase {

/**
 * Convert a Nintendo 3DS RGB565 tiled icon to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf RGB565 tiled image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromN3DSTiledRGB565(int width, int height,
	const uint16_t *img_buf, int img_siz)
{
	// Verify parameters.
	if (!img_buf)
		return nullptr;
	else if (width < 0 || height < 0)
		return nullptr;
	else if (img_siz < ((width * height) * 2))
		return nullptr;

	// N3DS tiled images use 8x8 tiles.
	if (width % 8 != 0 || height % 8 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const int tilesX = (width / 8);
	const int tilesY = (height / 8);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);

	// N3DS uses 3-level Z-ordered tiling.
	// References:
	// - https://github.com/devkitPro/3dstools/blob/master/src/smdhtool.cpp
	// - https://en.wikipedia.org/wiki/Z-order_curve
	static const uint8_t tile_order[] = {
		 0,  1,  8,  9,  2,  3, 10, 11, 16, 17, 24, 25, 18, 19, 26, 27,
		 4,  5, 12, 13,  6,  7, 14, 15, 20, 21, 28, 29, 22, 23, 30, 31,
		32, 33, 40, 41, 34, 35, 42, 43, 48, 49, 56, 57, 50, 51, 58, 59,
		36, 37, 44, 45, 38, 39, 46, 47, 52, 53, 60, 61, 54, 55, 62, 63
	};

	// Temporary tile buffer.
	uint32_t tileBuf[8*8];

	for (int y = 0; y < tilesY; y++) {
		for (int x = 0; x < tilesX; x++) {
			// Convert each tile to ARGB32 manually.
			// TODO: Optimize using pointers instead of indexes?
			for (int i = 0; i < 8*8; i++, img_buf++) {
				tileBuf[tile_order[i]] = ImageDecoderPrivate::RGB565_to_ARGB32(le16_to_cpu(*img_buf));
			}

			// Blit the tile to the main image buffer.
			ImageDecoderPrivate::BlitTile<uint32_t, 8, 8>(img, tileBuf, x, y);
		}
	}

	// Image has been converted.
	return img;
}

}
