/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_IMG_IMAGEDECODER_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IMG_IMAGEDECODER_HPP__

#include "config.librpbase.h"
#include "common.h"

// C includes.
#include <stdint.h>

namespace LibRpBase {

class rp_image;

class ImageDecoder
{
	private:
		// ImageDecoder is a static class.
		ImageDecoder();
		~ImageDecoder();
		RP_DISABLE_COPY(ImageDecoder)

	public:
		/** Linear images **/

		// Pixel formats.
		enum PixelFormat {
			PXF_UNKNOWN,

			// 16-bit
			PXF_RGB565,	// xRRRRRGG GGGBBBBB
			PXF_BGR565,	// xBBBBBGG GGGRRRRR
			PXF_ARGB1555,	// ARRRRRGG GGGBBBBB
			PXF_ABGR1555,	// ABBBBBGG GGGRRRRR
			PXF_RGBA5551,	// RRRRRGGG GGBBBBBA
			PXF_BGRA5551,	// BBBBBGGG GGRRRRRA
			PXF_ARGB4444,	// AAAARRRR GGGGBBBB
			PXF_ABGR4444,	// AAAABBBB GGGGRRRR
			PXF_RGBA4444,	// RRRRGGGG BBBBAAAA
			PXF_BGRA4444,	// BBBBGGGG RRRRAAAA
			PXF_xRGB4444,	// xxxxRRRR GGGGBBBB
			PXF_xBGR4444,	// xxxxBBBB GGGGRRRR
			PXF_RGBx4444,	// RRRRGGGG BBBBxxxx
			PXF_BGRx4444,	// BBBBGGGG RRRRxxxx

			// GameCube-specific 16-bit
			PXF_RGB5A3,	// High bit determines RGB555 or ARGB4444.
			PXF_IA8,	// Intensity/Alpha.

			// 15-bit
			PXF_RGB555,
			PXF_BGR555,
			PXF_BGR555_PS1,	// Special transparency handling.

			// 24-bit
			PXF_RGB888,
			PXF_BGR888,

			// 32-bit with alpha channel.
			PXF_ARGB8888,
			PXF_ABGR8888,
			PXF_RGBA8888,
			PXF_BGRA8888,
			// 32-bit with unused alpha channel.
			PXF_xRGB8888,
			PXF_xBGR8888,
			PXF_RGBx8888,
			PXF_BGRx8888,

			// Uncommon 32-bit formats.
			PXF_G16R16,

			// Endian-specific ARGB32 definitions.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
			PXF_HOST_ARGB32 = PXF_ARGB8888,
			PXF_HOST_ABGR32 = PXF_ABGR8888,
			PXF_HOST_xRGB32 = PXF_xRGB8888,
			PXF_HOST_RGBx32 = PXF_RGBx8888,
			PXF_SWAP_ARGB32 = PXF_BGRA8888,
			PXF_SWAP_ABGR32 = PXF_RGBA8888,
			PXF_SWAP_xRGB32 = PXF_BGRx8888,
			PXF_SWAP_RGBx32 = PXF_xBGR8888,
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
			PXF_HOST_ARGB32 = PXF_BGRA8888,
			PXF_HOST_ABGR32 = PXF_RGBA8888
			PXF_HOST_xRGB32 = PXF_BGRx8888,
			PXF_HOST_RGBx32 = PXF_xBGR8888,
			PXF_SWAP_ARGB32 = PXF_ARGB8888,
			PXF_SWAP_ABGR32 = PXF_ABGR8888,
			PXF_SWAP_xRGB32 = PXF_xRGB8888,
			PXF_SWAP_RGBx32 = PXF_RGBx8888,
#endif
		};

		/**
		 * Convert a linear CI4 image to rp_image with a little-endian 16-bit palette.
		 * @tparam px_format Palette pixel format.
		 * @tparam msn_left If true, most-significant nybble is the left pixel.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf CI4 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)/2]
		 * @param pal_buf Palette buffer.
		 * @param pal_siz Size of palette data. [must be >= 16*2]
		 * @return rp_image, or nullptr on error.
		 */
		template<PixelFormat px_format, bool msn_left>
		static rp_image *fromLinearCI4(int width, int height,
			const uint8_t *img_buf, int img_siz,
			const uint16_t *pal_buf, int pal_siz);

		/**
		 * Convert a linear CI8 image to rp_image with a little-endian 16-bit palette.
		 * @tparam px_format Palette pixel format.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf CI8 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)]
		 * @param pal_buf Palette buffer.
		 * @param pal_siz Size of palette data. [must be >= 256*2]
		 * @return rp_image, or nullptr on error.
		 */
		template<PixelFormat px_format>
		static rp_image *fromLinearCI8(int width, int height,
			const uint8_t *img_buf, int img_siz,
			const uint16_t *pal_buf, int pal_siz);

		/**
		 * Convert a linear monochrome image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf Monochrome image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)/8]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromLinearMono(int width, int height,
			const uint8_t *img_buf, int img_siz);

		/**
		 * Convert a linear 16-bit RGB image to rp_image.
		 * @param px_format	[in] 16-bit pixel format.
		 * @param width		[in] Image width.
		 * @param height	[in] Image height.
		 * @param img_buf	[in] 16-bit image buffer.
		 * @param img_siz	[in] Size of image data. [must be >= (w*h)*2]
		 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromLinear16(PixelFormat px_format,
			int width, int height,
			const uint16_t *img_buf, int img_siz, int stride = 0);

		/**
		 * Convert a linear 24-bit RGB image to rp_image.
		 * @param px_format	[in] 24-bit pixel format.
		 * @param width		[in] Image width.
		 * @param height	[in] Image height.
		 * @param img_buf	[in] Image buffer. (must be byte-addressable)
		 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
		 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromLinear24(PixelFormat px_format,
			int width, int height,
			const uint8_t *img_buf, int img_siz, int stride = 0);

		/**
		 * Convert a linear 32-bit RGB image to rp_image.
		 * @param px_format	[in] 32-bit pixel format.
		 * @param width		[in] Image width.
		 * @param height	[in] Image height.
		 * @param img_buf	[in] 32-bit image buffer.
		 * @param img_siz	[in] Size of image data. [must be >= (w*h)*2]
		 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromLinear32(PixelFormat px_format,
			int width, int height,
			const uint32_t *img_buf, int img_siz, int stride = 0);

		/** GameCube **/

		/**
		 * Convert a GameCube 16-bit image to rp_image.
		 * @tparam px_format 16-bit pixel format.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf RGB5A3 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)*2]
		 * @return rp_image, or nullptr on error.
		 */
		template<PixelFormat px_format>
		static rp_image *fromGcn16(int width, int height,
			const uint16_t *img_buf, int img_siz);

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
		static rp_image *fromGcnCI8(int width, int height,
			const uint8_t *img_buf, int img_siz,
			const uint16_t *pal_buf, int pal_siz);

		/** Nintendo DS **/

		/**
		 * Convert a Nintendo DS CI4 image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf CI4 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)/2]
		 * @param pal_buf Palette buffer.
		 * @param pal_siz Size of palette data. [must be >= 16*2]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromNDS_CI4(int width, int height,
			const uint8_t *img_buf, int img_siz,
			const uint16_t *pal_buf, int pal_siz);

		/** Nintendo 3DS **/

		/**
		 * Convert a Nintendo 3DS RGB565 tiled icon to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf RGB565 tiled image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)*2]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromN3DSTiledRGB565(int width, int height,
			const uint16_t *img_buf, int img_siz);

		/* S3TC */

		/**
		 * Convert a GameCube DXT1 image to rp_image.
		 * The GameCube variant has 2x2 block tiling in addition to 4x4 pixel tiling.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf DXT1 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)/2]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromDXT1_GCN(int width, int height,
			const uint8_t *img_buf, int img_siz);

		/**
		 * Convert a DXT1 image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf DXT1 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)/2]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromDXT1(int width, int height,
			const uint8_t *img_buf, int img_siz);

		/**
		 * Convert a DXT2 image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf DXT2 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromDXT2(int width, int height,
			const uint8_t *img_buf, int img_siz);

		/**
		 * Convert a DXT3 image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf DXT3 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromDXT3(int width, int height,
			const uint8_t *img_buf, int img_siz);

		/**
		 * Convert a DXT4 image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf DXT4 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromDXT4(int width, int height,
			const uint8_t *img_buf, int img_siz);

		/**
		 * Convert a DXT5 image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf DXT5 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromDXT5(int width, int height,
			const uint8_t *img_buf, int img_siz);

		/* Dreamcast */

		/**
		 * Convert a Dreamcast square twiddled 16-bit image to rp_image.
		 * @tparam px_format 16-bit pixel format.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf 16-bit image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)*2]
		 * @return rp_image, or nullptr on error.
		 */
		template<PixelFormat px_format>
		static rp_image *fromDreamcastSquareTwiddled16(int width, int height,
			const uint16_t *img_buf, int img_siz);

		/**
		 * Convert a Dreamcast vector-quantized image to rp_image.
		 * @tparam smallVQ If true, handle this image as SmallVQ.
		 * @param px_format Palette pixel format.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf VQ image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)*2]
		 * @param pal_buf Palette buffer.
		 * @param pal_siz Size of palette data. [must be >= 1024*2; for SmallVQ, 64*2, 256*2, or 512*2]
		 * @return rp_image, or nullptr on error.
		 */
		template<bool smallVQ>
		static rp_image *fromDreamcastVQ16(PixelFormat px_format,
			int width, int height,
			const uint8_t *img_buf, int img_siz,
			const uint16_t *pal_buf, int pal_siz);

		/**
		 * Get the number of palette entries for Dreamcast SmallVQ textures.
		 * TODO: constexpr?
		 * @param width Texture width.
		 * @return Number of palette entries.
		 */
		static inline int calcDreamcastSmallVQPaletteEntries(int width);
};

/**
 * Get the number of palette entries for Dreamcast SmallVQ textures.
 * TODO: constexpr?
 * @param width Texture width.
 * @return Number of palette entries.
 */
inline int ImageDecoder::calcDreamcastSmallVQPaletteEntries(int width)
{
	if (width <= 16) {
		return 64;
	} else if (width <= 32) {
		return 256;
	} else if (width <= 64) {
		return 512;
	} else {
		return 1024;
	}
}

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IMG_IMAGEDECODER_HPP__ */
