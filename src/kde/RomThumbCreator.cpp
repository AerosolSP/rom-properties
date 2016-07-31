/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomThumbCreator.cpp: Thumbnail creator.                                 *
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

#include "RomThumbCreator.hpp"

// libromdata
#include "libromdata/RomData.hpp"
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/rp_image.hpp"
using namespace LibRomData;

#include <QtGui/QImage>
#include <QLabel>

/**
 * Factory method.
 * References:
 * - https://api.kde.org/4.x-api/kdelibs-apidocs/kio/html/classThumbCreator.html
 * - https://api.kde.org/frameworks/kio/html/classThumbCreator.html
 */
extern "C" {
	Q_DECL_EXPORT ThumbCreator *new_creator()
	{
		return new RomThumbCreator();
	}
}

/** RomThumbCreator **/

RomThumbCreator::~RomThumbCreator()
{ }

/**
 * Convert an rp_image to QImage.
 * @param rp_image rp_image.
 * @return QImage.
 */
QImage RomThumbCreator::rpToQImage(const rp_image *image)
{
	if (!image || !image->isValid())
		return QImage();

	// Determine the QImage format.
	QImage::Format fmt;
	switch (image->format()) {
		case rp_image::FORMAT_CI8:
			fmt = QImage::Format_Indexed8;
			break;
		case rp_image::FORMAT_ARGB32:
			fmt = QImage::Format_ARGB32;
			break;
		default:
			fmt = QImage::Format_Invalid;
			break;
	}
	if (fmt == QImage::Format_Invalid)
		return QImage();

	QImage img(image->width(), image->height(), fmt);

	// Copy the image data.
	memcpy(img.bits(), image->bits(), image->data_len());

	// Copy the palette data, if necessary.
	if (fmt == QImage::Format_Indexed8) {
		QVector<QRgb> palette(image->palette_len());
		memcpy(palette.data(), image->palette(), image->palette_len()*sizeof(QRgb));
		img.setColorTable(palette);
	}

	// Image converted successfully.
	return img;
}

/**
 * Create a thumbnail for a ROM image.
 * @param path Local pathname of the ROM image.
 * @param width Requested width.
 * @param height Requested height.
 * @param img Target image.
 * @return True if a thumbnail was created; false if not.
 */
bool RomThumbCreator::create(const QString &path, int width, int height, QImage &img)
{
	Q_UNUSED(width);
	Q_UNUSED(height);

	bool ret = false;

	// Attempt to open the ROM file.
	// TODO: rp_QFile() wrapper?
	// For now, using stdio.
	FILE *file = fopen(path.toUtf8().constData(), "rb");
	if (!file) {
		// Could not open the file.
		return false;
	}

	// Get the appropriate RomData class for this ROM.
	RomData *romData = RomDataFactory::getInstance(file);
	fclose(file);	// file is dup()'d by RomData.
	if (!romData) {
		// ROM is not supported.
		return false;
	}

	// TODO: Customize which ones are used per-system.
	// For now, check EXT MEDIA, then INT ICON.
	const rp_image *image = nullptr;

	uint32_t imgbf = romData->supportedImageTypes();
	if (imgbf & RomData::IMGBF_EXT_MEDIA) {
		// External media scan.
		// TODO
	}

	if (!image) {
		// No external media scan.
		if (imgbf & RomData::IMGBF_INT_ICON) {
			// Internal icon.
			image = romData->image(RomData::IMG_INT_ICON);
		}
	}

	if (image) {
		// Convert the icon to QImage.
		img = rpToQImage(image);
		if (!img.isNull())
			ret = true;
	}

	// We're done with romData.
	delete romData;

	return ret;
}
