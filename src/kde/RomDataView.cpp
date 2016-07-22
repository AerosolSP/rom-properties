/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomDataView.cpp: Sega Mega Drive ROM viewer.                          *
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

#include "RomDataView.hpp"

#include "libromdata/RomData.hpp"
#include "libromdata/RomFields.hpp"
using LibRomData::RomData;
using LibRomData::RomFields;

// C includes. (C++ namespace)
#include <cassert>
#include <cstdio>

#include <QLabel>
#include <QCheckBox>

#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QSpacerItem>

class RomDataViewPrivate
{
	public:
		RomDataViewPrivate(RomDataView *q, RomData *romData);
		~RomDataViewPrivate();

	private:
		RomDataView *const q_ptr;
		Q_DECLARE_PUBLIC(RomDataView)
	private:
		Q_DISABLE_COPY(RomDataViewPrivate)

	public:
		struct Ui {
			void setupUi(QWidget *RomDataView);

			QFormLayout *formLayout;
			// TODO: Store the field widgets?
		};
		Ui ui;
		RomData *romData;

		/**
		 * Update the display widgets.
		 * FIXME: Allow running this multiple times?
		 */
		void updateDisplay(void);

		bool displayInit;
};

/** RomDataViewPrivate **/

RomDataViewPrivate::RomDataViewPrivate(RomDataView *q, RomData *romData)
	: q_ptr(q)
	, romData(romData)
	, displayInit(false)
{ }

RomDataViewPrivate::~RomDataViewPrivate()
{
	delete romData;
}

void RomDataViewPrivate::Ui::setupUi(QWidget *RomDataView)
{
	// Only the formLayout is initialized here.
	// Everything else is initialized in updateDisplay.
	formLayout = new QFormLayout(RomDataView);
}

// TODO: Move this elsehwere.
static inline QString rpToQS(const LibRomData::rp_string &rps)
{
#if defined(RP_UTF8)
	return QString::fromUtf8(rps.c_str(), (int)rps.size());
#elif defined(RP_UTF16)
	return QString::fromUtf16(reinterpret_cast<const ushort*>(rps.data()), (int)rps.size());
#else
#error Text conversion not available on this system.
#endif
}

static inline QString rpToQS(const rp_char *rps)
{
#if defined(RP_UTF8)
	return QString::fromUtf8(rps);
#elif defined(RP_UTF16)
	return QString::fromUtf16(reinterpret_cast<const ushort*>(rps));
#else
#error Text conversion not available on this system.
#endif
}

/**
 * Update the display widgets.
 * FIXME: Allow running this multiple times?
 */
void RomDataViewPrivate::updateDisplay(void)
{
	if (!romData || displayInit)
		return;
	displayInit = true;

	// Get the fields.
	const RomFields *fields = romData->fields();
	const int count = fields->count();

	// Make sure the underlying file handle is closed,
	// since we don't need it anymore.
	romData->close();

	// Create the UI widgets.
	Q_Q(RomDataView);
	for (int i = 0; i < count; i++) {
		const RomFields::Desc *desc = fields->desc(i);
		const RomFields::Data *data = fields->data(i);
		if (!desc || !data)
			continue;
		if (desc->type != data->type)
			continue;
		if (!desc->name || desc->name[0] == '\0')
			continue;

		QLabel *lblDesc = new QLabel(q);
		lblDesc->setTextFormat(Qt::PlainText);
		lblDesc->setText(RomDataView::tr("%1:").arg(rpToQS(desc->name)));

		switch (desc->type) {
			case RomFields::RFT_STRING: {
				// String type.
				// TODO: Monospace hint?
				QLabel *lblString = new QLabel(q);
				lblString->setTextFormat(Qt::PlainText);
				lblString->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);
				if (data->str) {
					lblString->setText(rpToQS(data->str));
				}
				ui.formLayout->addRow(lblDesc, lblString);
				break;
			}

			case RomFields::RFT_BITFIELD: {
				// Bitfield type. Create a grid of checkboxes.
				QGridLayout *gridLayout = new QGridLayout();
				int row = 0, col = 0;
				for (int i = 0; i < desc->bitfield.elements; i++) {
					const rp_char *name = desc->bitfield.names[i];
					if (!name)
						continue;
					// TODO: Prevent toggling; disable automatic alt key.
					QCheckBox *checkBox = new QCheckBox(q);
					checkBox->setText(rpToQS(name));
					if (data->bitfield & (1 << i)) {
						checkBox->setChecked(true);
					}
					gridLayout->addWidget(checkBox, row, col, 1, 1);
					col++;
					if (col == desc->bitfield.elemsPerRow) {
						row++;
						col = 0;
					}
				}
				ui.formLayout->addRow(lblDesc, gridLayout);
				break;
			}

			default:
				// Unsupported right now.
				assert(false);
				delete lblDesc;
				break;
		}
	}
}

/** RomDataView **/

RomDataView::RomDataView(RomData *rom, QWidget *parent)
	: super(parent)
	, d_ptr(new RomDataViewPrivate(this, rom))
{
	Q_D(RomDataView);
	d->ui.setupUi(this);

	// Update the display widgets.
	d->updateDisplay();
}

RomDataView::~RomDataView()
{
	delete d_ptr;
}
