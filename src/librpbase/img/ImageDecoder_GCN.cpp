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
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) * 2));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) * 2))
	{
		return nullptr;
	}

	// GameCube RGB5A3 uses 4x4 tiles.
	assert(width % 4 == 0);
	assert(height % 4 == 0);
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
			// Convert each tile to ARGB32 manually.
			// TODO: Optimize using pointers instead of indexes?
			for (int i = 0; i < 4*4; i++, img_buf++) {
				tileBuf[i] = ImageDecoderPrivate::RGB5A3_to_ARGB32(be16_to_cpu(*img_buf));
			}

			// Blit the tile to the main image buffer.
			ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img, tileBuf, x, y);
		}
	}

	// Image has been converted.
	return img;
}

/**
 * Convert a GameCube CI8 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI8 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 256*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromGcnCI8(int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(pal_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= (width * height));
	assert(pal_siz >= 256*2);
	if (!img_buf || !pal_buf || width <= 0 || height <= 0 ||
	    img_siz < (width * height) || pal_siz < 256*2)
	{
		return nullptr;
	}

	// GameCube CI8 uses 8x4 tiles.
	assert(width % 8 == 0);
	assert(height % 4 == 0);
	if (width % 8 != 0 || height % 4 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const int tilesX = (width / 8);
	const int tilesY = (height / 4);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_CI8);

	// Convert the palette.
	// TODO: Optimize using pointers instead of indexes?
	uint32_t *palette = img->palette();
	assert(img->palette_len() >= 256);
	if (img->palette_len() < 256) {
		// Not enough colors...
		delete img;
		return nullptr;
	}

	int tr_idx = -1;
	for (int i = 0; i < 256; i++) {
		// GCN color format is RGB5A3.
		palette[i] = ImageDecoderPrivate::RGB5A3_to_ARGB32(be16_to_cpu(pal_buf[i]));
		if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
			// Found the transparent color.
			tr_idx = i;
		}
	}
	img->set_tr_idx(tr_idx);

	// Tile pointer.
	const uint8_t *tileBuf = img_buf;

	for (int y = 0; y < tilesY; y++) {
		for (int x = 0; x < tilesX; x++) {
			// Decode the current tile.
			ImageDecoderPrivate::BlitTile<uint8_t, 8, 4>(img, tileBuf, x, y);
			tileBuf += (8 * 4);
		}
	}

	// Image has been converted.
	return img;
}

}
