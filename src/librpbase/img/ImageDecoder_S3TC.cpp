/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_S3TC.cpp: Image decoding functions. (S3TC)                 *
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

#include "un-premultiply.hpp"

// References:
// - http://www.matejtomcik.com/Public/KnowHow/DXTDecompression/
// - http://www.fsdeveloper.com/wiki/index.php?title=DXT_compression_explained
// - https://en.wikipedia.org/wiki/S3_Texture_Compression
// - https://www.khronos.org/opengl/wiki/S3_Texture_Compression

namespace LibRpBase {

// DXT1 block format.
struct dxt1_block {
	uint16_t color[2];	// Colors 0 and 1, in RGB565 format.
	uint32_t indexes;	// Two-bit color indexes.
};
ASSERT_STRUCT(dxt1_block, 8);

// decode_DXTn_tile_color_palette flags.
enum DXTn_Palette_Flags {
	DXTn_PALETTE_BIG_ENDIAN		= (1 << 0),
	DXTn_PALETTE_COLOR3_ALPHA	= (1 << 1),	// GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
	DXTn_PALETTE_COLOR0_LE_COLOR1	= (1 << 2),	// Assume color0 <= color1. (DXT2/DXT3)
};

/**
 * Decode a DXTn tile color palette.
 * @tparam flags Flags. (See DXTn_Palette_Flags)
 * @param pal		[out] Array of four argb32_t values.
 * @param dxt1_src	[in] DXT1 block.
 */
template<uint32_t flags>
static inline void decode_DXTn_tile_color_palette(argb32_t pal[4], const dxt1_block *dxt1_src)
{
	// Convert the first two colors from RGB565.
	uint16_t c0, c1;
	if (flags & DXTn_PALETTE_BIG_ENDIAN) {
		c0 = be16_to_cpu(dxt1_src->color[0]);
		c1 = be16_to_cpu(dxt1_src->color[1]);
	} else {
		c0 = le16_to_cpu(dxt1_src->color[0]);
		c1 = le16_to_cpu(dxt1_src->color[1]);
	}
	pal[0].u32 = ImageDecoderPrivate::RGB565_to_ARGB32(c0);
	pal[1].u32 = ImageDecoderPrivate::RGB565_to_ARGB32(c1);

	// Calculate the second two colors.
	if (!(flags & DXTn_PALETTE_COLOR0_LE_COLOR1) && (c0 > c1)) {
		// color0 > color1
		pal[2].r = ((2 * pal[0].r) + pal[1].r) / 3;
		pal[2].g = ((2 * pal[0].g) + pal[1].g) / 3;
		pal[2].b = ((2 * pal[0].b) + pal[1].b) / 3;
		pal[2].a = 0xFF;

		pal[3].r = ((2 * pal[1].r) + pal[0].r) / 3;
		pal[3].g = ((2 * pal[1].g) + pal[0].g) / 3;
		pal[3].b = ((2 * pal[1].b) + pal[0].b) / 3;
		pal[3].a = 0xFF;
	} else {
		// color0 <= color1
		pal[2].r = (pal[0].r + pal[1].r) / 2;
		pal[2].g = (pal[0].g + pal[1].g) / 2;
		pal[2].b = (pal[0].b + pal[1].b) / 2;
		pal[2].a = 0xFF;

		// Black and/or transparent.
		pal[3].u32 = ((flags & DXTn_PALETTE_COLOR3_ALPHA) ? 0x00000000 : 0xFF000000);
	}
}

/**
 * Decode the DXT5 alpha channel value.
 * @param a3	3-bit alpha selector code.
 * @param alpha	2-element alpha array from dxt5_block.
 * @return Alpha channel value.
 */
static inline uint8_t decode_DXT5_alpha(unsigned int a3, const uint8_t alpha[2])
{
	unsigned int a_ret = 255;

	if (alpha[0] > alpha[1]) {
		switch (a3 & 7) {
			case 0:
				a_ret = alpha[0];
				break;
			case 1:
				a_ret = alpha[1];
				break;
			case 2:
				a_ret = ((6 * alpha[0]) + (1 * alpha[1])) / 7;
				break;
			case 3:
				a_ret = ((5 * alpha[0]) + (2 * alpha[1])) / 7;
				break;
			case 4:
				a_ret = ((4 * alpha[0]) + (3 * alpha[1])) / 7;
				break;
			case 5:
				a_ret = ((3 * alpha[0]) + (4 * alpha[1])) / 7;
				break;
			case 6:
				a_ret = ((2 * alpha[0]) + (5 * alpha[1])) / 7;
				break;
			case 7:
				a_ret = ((1 * alpha[0]) + (6 * alpha[1])) / 7;
				break;
		}
	} else {
		switch (a3 & 7) {
			case 0:
				a_ret = alpha[0];
				break;
			case 1:
				a_ret = alpha[1];
				break;
			case 2:
				a_ret = ((4 * alpha[0]) + (1 * alpha[1])) / 5;
				break;
			case 3:
				a_ret = ((3 * alpha[0]) + (2 * alpha[1])) / 5;
				break;
			case 4:
				a_ret = ((2 * alpha[0]) + (3 * alpha[1])) / 5;
				break;
			case 5:
				a_ret = ((1 * alpha[0]) + (4 * alpha[1])) / 5;
				break;
			case 6:
				a_ret = 0;
				break;
			case 7:
				a_ret = 255;
				break;
		}
	}

	// Prevent overflow.
	return (uint8_t)(a_ret > 255 ? 255 : a_ret);
}

/**
 * Convert a GameCube DXT1 image to rp_image.
 * The GameCube variant has 2x2 block tiling in addition to 4x4 pixel tiling.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromDXT1_GCN(int width, int height,
	const uint8_t *img_buf, int img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) / 2));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) / 2))
	{
		return nullptr;
	}

	// DXT1 uses 2x2 blocks of 4x4 tiles.
	assert(width % 8 == 0);
	assert(height % 8 == 0);
	if (width % 8 != 0 || height % 8 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const unsigned int tilesX = (unsigned int)(width / 4);
	const unsigned int tilesY = (unsigned int)(height / 4);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	const dxt1_block *dxt1_src = reinterpret_cast<const dxt1_block*>(img_buf);

	// Temporary tile buffer.
	uint32_t tileBuf[4*4];

	// Tiles are arranged in 2x2 blocks.
	// Reference: https://github.com/nickworonekin/puyotools/blob/80f11884f6cae34c4a56c5b1968600fe7c34628b/Libraries/VrSharp/GvrTexture/GvrDataCodec.cs#L712
	for (unsigned int y = 0; y < tilesY; y += 2) {
	for (unsigned int x = 0; x < tilesX; x += 2) {
		for (unsigned int y2 = 0; y2 < 2; y2++) {
		for (unsigned int x2 = 0; x2 < 2; x2++, dxt1_src++) {
			// Decode the DXT1 tile palette.
			// TODO: Color 3 may be either black or transparent.
			// Figure out if there's a way to specify that in GVR.
			// Assuming transparent for now, since most GVR DXT1
			// textures use transparency.
			argb32_t pal[4];
			decode_DXTn_tile_color_palette<DXTn_PALETTE_BIG_ENDIAN | DXTn_PALETTE_COLOR3_ALPHA>(pal, dxt1_src);

			// Process the 16 color indexes.
			// NOTE: MSB has the left-most pixel of the *bottom* row.
			// LSB has the right-most pixel of the *top* row.
			static const uint8_t pxmap[16] = {3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12};
			// TODO: Pointer optimization.
			uint32_t indexes = dxt1_src->indexes;
			for (unsigned int i = 0; i < 16; i++, indexes >>= 2) {
				tileBuf[pxmap[i]] = pal[indexes & 3].u32;
			}

			// Blit the tile to the main image buffer.
			ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img, tileBuf, x+x2, y+y2);
		} }
	} }

	// Image has been converted.
	return img;
}

/**
 * Convert a DXT1 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromDXT1(int width, int height,
	const uint8_t *img_buf, int img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) / 2));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) / 2))
	{
		return nullptr;
	}

	// DXT1 uses 2x2 blocks of 4x4 tiles.
	assert(width % 8 == 0);
	assert(height % 8 == 0);
	if (width % 8 != 0 || height % 8 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const unsigned int tilesX = (unsigned int)(width / 4);
	const unsigned int tilesY = (unsigned int)(height / 4);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	const dxt1_block *dxt1_src = reinterpret_cast<const dxt1_block*>(img_buf);

	// Temporary tile buffer.
	uint32_t tileBuf[4*4];

	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++, dxt1_src++) {
		// Decode the DXT1 tile palette.
		// TODO: Color 3 may be either black or transparent.
		// Figure out if there's a way to specify that in DDS.
		// Assuming black for now, since that's standard DXT1.
		argb32_t pal[4];
		decode_DXTn_tile_color_palette<0>(pal, dxt1_src);

		// Process the 16 color indexes.
		uint32_t indexes = dxt1_src->indexes;
		for (unsigned int i = 0; i < 16; i++, indexes >>= 2) {
			tileBuf[i] = pal[indexes & 3].u32;
		}

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img, tileBuf, x, y);
	} }

	// Image has been converted.
	return img;
}

/**
 * Convert a DXT2 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT5 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromDXT2(int width, int height,
	const uint8_t *img_buf, int img_siz)
{
	// TODO: Completely untested. Needs testing!

	// Use fromDXT3(), then convert from premultiplied alpha
	// to standard alpha.
	rp_image *img = fromDXT3(width, height, img_buf, img_siz);
	if (!img) {
		return nullptr;
	}

	// Un-premultiply the image.
	int ret = un_premultiply_image(img);
	if (ret != 0) {
		delete img;
		img = nullptr;
	}
	return img;
}

/**
 * Convert a DXT3 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT5 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromDXT3(int width, int height,
	const uint8_t *img_buf, int img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= (width * height));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < (width * height))
	{
		return nullptr;
	}

	// DXT3 uses 2x2 blocks of 4x4 tiles.
	assert(width % 8 == 0);
	assert(height % 8 == 0);
	if (width % 8 != 0 || height % 8 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const unsigned int tilesX = (unsigned int)(width / 4);
	const unsigned int tilesY = (unsigned int)(height / 4);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);

	// DXT3 block format.
	struct dxt3_block {
		uint64_t alpha;		// Alpha values. (4-bit per pixel)
		dxt1_block colors;	// DXT1-style color block.
	};
	ASSERT_STRUCT(dxt3_block, 16);
	const dxt3_block *dxt3_src = reinterpret_cast<const dxt3_block*>(img_buf);

	// Temporary tile buffer.
	uint32_t tileBuf[4*4];

	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++, dxt3_src++) {
		// Decode the DXT3 tile palette.
		argb32_t pal[4];
		// FIXME: DXTn_PALETTE_COLOR0_LE_COLOR1 seems to result in garbage pixels.
		// https://github.com/kchapelier/decode-dxt/tree/master/lib has similar code
		// but handles DXT3 like both DXT1 and DXT5, so disable this for now.
		decode_DXTn_tile_color_palette<0/*DXTn_PALETTE_COLOR0_LE_COLOR1*/>(pal, &dxt3_src->colors);

		// Process the 16 color indexes and apply alpha.
		uint32_t indexes = dxt3_src->colors.indexes;
		uint64_t alpha = dxt3_src->alpha;
		for (unsigned int i = 0; i < 16; i++, indexes >>= 2, alpha >>= 4) {
			argb32_t color = pal[indexes & 3];
			// TODO: Verify alpha value handling for DXT3.
			color.a = (alpha & 0xF) | ((alpha & 0xF) << 4);
			tileBuf[i] = color.u32;
		}

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img, tileBuf, x, y);
	} }

	// Image has been converted.
	return img;
}

/**
 * Convert a DXT4 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT5 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromDXT4(int width, int height,
	const uint8_t *img_buf, int img_siz)
{
	// TODO: Completely untested. Needs testing!

	// Use fromDXT5(), then convert from premultiplied alpha
	// to standard alpha.
	rp_image *img = fromDXT5(width, height, img_buf, img_siz);
	if (!img) {
		return nullptr;
	}

	// Un-premultiply the image.
	int ret = un_premultiply_image(img);
	if (ret != 0) {
		delete img;
		img = nullptr;
	}
	return img;
}

/**
 * Convert a DXT5 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT5 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromDXT5(int width, int height,
	const uint8_t *img_buf, int img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= (width * height));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < (width * height))
	{
		return nullptr;
	}

	// DXT5 uses 2x2 blocks of 4x4 tiles.
	assert(width % 8 == 0);
	assert(height % 8 == 0);
	if (width % 8 != 0 || height % 8 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const unsigned int tilesX = (unsigned int)(width / 4);
	const unsigned int tilesY = (unsigned int)(height / 4);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);

	// DXT5 block format.
	struct dxt5_block {
		uint8_t alpha[2];	// Alpha values.
		uint16_t codes[3];	// Alpha operation codes. (48-bit unsigned; 3-bit per pixel)
		dxt1_block colors;	// DXT1-style color block.
	};
	ASSERT_STRUCT(dxt5_block, 16);
	const dxt5_block *dxt5_src = reinterpret_cast<const dxt5_block*>(img_buf);

	// Temporary tile buffer.
	uint32_t tileBuf[4*4];

	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++, dxt5_src++) {
		// Decode the DXT5 tile palette.
		argb32_t pal[4];
		decode_DXTn_tile_color_palette<0>(pal, &dxt5_src->colors);

		// Decode the DXT5 alpha values.
		// NOTE: Combining the alpha values into a uint64_t first
		// in order to make it easier to manage.
		uint64_t alpha48 =  (uint64_t)dxt5_src->codes[0] |
				   ((uint64_t)dxt5_src->codes[1] << 16) |
				   ((uint64_t)dxt5_src->codes[2] << 32);

		// Process the 16 color and alpha indexes.
		uint32_t indexes = dxt5_src->colors.indexes;
		for (unsigned int i = 0; i < 16; i++, indexes >>= 2, alpha48 >>= 3) {
			argb32_t color = pal[indexes & 3];
			// Decode the alpha channel value.
			color.a = decode_DXT5_alpha(alpha48 & 7, dxt5_src->alpha);
			tileBuf[i] = color.u32;
		}

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img, tileBuf, x, y);
	} }

	// Image has been converted.
	return img;
}

}
