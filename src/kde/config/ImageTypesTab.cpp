/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ImageTypesTab.cpp: Image Types tab for rp-config.                       *
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

#include "ImageTypesTab.hpp"
#include "RpQt.hpp"

// Qt includes.
#include <QComboBox>
#include <QLabel>

// TImageTypesConfig is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/config/TImageTypesConfig.cpp"
using LibRomData::TImageTypesConfig;

#include "ui_ImageTypesTab.h"
class ImageTypesTabPrivate : public TImageTypesConfig<QComboBox*>
{
	public:
		explicit ImageTypesTabPrivate(ImageTypesTab *q);
		~ImageTypesTabPrivate();

	private:
		ImageTypesTab *const q_ptr;
		Q_DECLARE_PUBLIC(ImageTypesTab)
		Q_DISABLE_COPY(ImageTypesTabPrivate)

	public:
		Ui::ImageTypesTab ui;

	protected:
		/** TImageTypesConfig functions. (protected) **/

		/**
		 * Create the labels in the grid.
		 */
		inline virtual void createGridLabels(void) override final;

		/**
		 * Create a ComboBox in the grid.
		 * @param cbid ComboBox ID.
		 */
		inline virtual void createComboBox(unsigned int cbid) override final;

		/**
		 * Add strings to a ComboBox in the grid.
		 * @param cbid ComboBox ID.
		 * @param max_prio Maximum priority value. (minimum is 1)
		 */
		inline virtual void addComboBoxStrings(unsigned int cbid, int max_prio) override final;

		/**
		 * Finish adding the ComboBoxes.
		 */
		inline virtual void finishComboBoxes(void) override final;

		/**
		 * Initialize the Save subsystem.
		 * This is needed on platforms where the configuration file
		 * must be opened with an appropriate writer class.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		inline virtual int saveStart(void) override final;

		/**
		 * Write an ImageType configuration entry.
		 * @param sysName System name.
		 * @param imageTypeList Image type list, comma-separated.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		inline virtual int saveWriteEntry(const rp_char *sysName, const rp_char *imageTypeList) override final;

		/**
		 * Close the Save subsystem.
		 * This is needed on platforms where the configuration file
		 * must be opened with an appropriate writer class.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		inline virtual int saveFinish(void) override final;

	protected:
		/** TImageTypesConfig functions. (public) **/

		/**
		 * Set a ComboBox's current index.
		 * This will not trigger cboImageType_priorityValueChanged().
		 * @param cbid ComboBox ID.
		 * @param prio New priority value. (0xFF == no)
		 */
		inline virtual void cboImageType_setPriorityValue(unsigned int cbid, unsigned int prio) override final;

	public:
		// Last ComboBox added.
		// Needed in order to set the correct
		// tab order for the credits label.
		QComboBox *cboImageType_lastAdded;
};

/** ImageTypesTabPrivate **/

ImageTypesTabPrivate::ImageTypesTabPrivate(ImageTypesTab* q)
	: q_ptr(q)
	, cboImageType_lastAdded(nullptr)
{ }

ImageTypesTabPrivate::~ImageTypesTabPrivate()
{
	// cboImageType_lastAdded should be nullptr.
	// (Cleared by finishComboBoxes().)
	assert(cboImageType_lastAdded == nullptr);
}

/** TImageTypesConfig functions. (protected) **/

/**
 * Create the labels in the grid.
 */
void ImageTypesTabPrivate::createGridLabels(void)
{
	Q_Q(ImageTypesTab);

	// TODO: Make sure that all columns except 0 have equal sizes.

	// Create the image type labels.
	const QString cssImageType = QLatin1String(
		"QLabel { margin-left: 0.2em; margin-right: 0.2em; margin-bottom: 0.1em; }");
	for (unsigned int i = 0; i < IMG_TYPE_COUNT; i++) {
		QLabel *const lblImageType = new QLabel(RP2Q(imageTypeNames[i]), q);
		lblImageType->setAlignment(Qt::AlignTop|Qt::AlignHCenter);
		lblImageType->setStyleSheet(cssImageType);
		ui.gridImageTypes->addWidget(lblImageType, 0, i+1);
	}

	// Create the system name labels.
	const QString cssSysName = QLatin1String(
		"QLabel { margin-right: 0.25em; }");
	for (unsigned int sys = 0; sys < SYS_COUNT; sys++) {
		QLabel *const lblSysName = new QLabel(RP2Q(sysData[sys].name), q);
		lblSysName->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
		lblSysName->setStyleSheet(cssSysName);
		ui.gridImageTypes->addWidget(lblSysName, sys+1, 0);
	}
}

/**
 * Create a ComboBox in the grid.
 * @param cbid ComboBox ID.
 */
void ImageTypesTabPrivate::createComboBox(unsigned int cbid)
{
	const unsigned int sys = sysFromCbid(cbid);
	const unsigned int imageType = imageTypeFromCbid(cbid);
	if (!validateSysImageType(sys, imageType))
		return;

	// Create the ComboBox.
	Q_Q(ImageTypesTab);
	QComboBox *const cbo = new QComboBox(q);
	ui.gridImageTypes->addWidget(cbo, sys+1, imageType+1);
	cboImageType[sys][imageType] = cbo;

	// TODO: QSignalMapper so we can record changes.

	// Adjust the tab order.
	if (cboImageType_lastAdded) {
		q->setTabOrder(cboImageType_lastAdded, cbo);
	}
	cboImageType_lastAdded = cbo;
}

/**
 * Add strings to a ComboBox in the grid.
 * @param cbid ComboBox ID.
 * @param max_prio Maximum priority value. (minimum is 1)
 */
void ImageTypesTabPrivate::addComboBoxStrings(unsigned int cbid, int max_prio)
{
	const unsigned int sys = sysFromCbid(cbid);
	const unsigned int imageType = imageTypeFromCbid(cbid);
	if (!validateSysImageType(sys, imageType))
		return;

	QComboBox *const cbo = cboImageType[sys][imageType];
	assert(cbo != nullptr);
	if (!cbo)
		return;

	// Dropdown strings.
	// NOTE: One more string than the total number of image types,
	// since we have a string for "No".
	static const char s_values[IMG_TYPE_COUNT+1][4] = {
		"No", "1", "2", "3", "4", "5", "6", "7", "8"
	};
	static_assert(ARRAY_SIZE(s_values) == IMG_TYPE_COUNT+1, "s_values[] is the wrong size.");

	// NOTE: Need to add one more than the total number,
	// since "No" counts as an entry.
	for (int i = 0; i <= max_prio; i++) {
		assert(s_values[i] != nullptr);
		cbo->addItem(QLatin1String(s_values[i]));
	}
}

/**
 * Finish adding the ComboBoxes.
 */
void ImageTypesTabPrivate::finishComboBoxes(void)
{
	if (!cboImageType_lastAdded) {
		// Nothing to do here.
		return;
	}

	// Set the tab order for the credits label.
	Q_Q(ImageTypesTab);
	q->setTabOrder(cboImageType_lastAdded, ui.lblCredits);
	cboImageType_lastAdded = nullptr;
}

/**
 * Initialize the Save subsystem.
 * This is needed on platforms where the configuration file
 * must be opened with an appropriate writer class.
 * @return 0 on success; negative POSIX error code on error.
 */
int ImageTypesTabPrivate::saveStart(void)
{
	// TODO for Qt
	return -ENOTSUP;
}

/**
 * Write an ImageType configuration entry.
 * @param sysName System name.
 * @param imageTypeList Image type list, comma-separated.
 * @return 0 on success; negative POSIX error code on error.
 */
int ImageTypesTabPrivate::saveWriteEntry(const rp_char *sysName, const rp_char *imageTypeList)
{
	// TODO for Qt
	return -ENOTSUP;
}

/**
 * Close the Save subsystem.
 * This is needed on platforms where the configuration file
 * must be opened with an appropriate writer class.
 * @return 0 on success; negative POSIX error code on error.
 */
int ImageTypesTabPrivate::saveFinish(void)
{
	// TODO for Qt
	return -ENOTSUP;
}

/** TImageTypesConfig functions. (public) **/

/**
 * Set a ComboBox's current index.
 * This will not trigger cboImageType_priorityValueChanged().
 * @param cbid ComboBox ID.
 * @param prio New priority value. (0xFF == no)
 */
void ImageTypesTabPrivate::cboImageType_setPriorityValue(unsigned int cbid, unsigned int prio)
{
	const unsigned int sys = sysFromCbid(cbid);
	const unsigned int imageType = imageTypeFromCbid(cbid);
	if (!validateSysImageType(sys, imageType))
		return;

	QComboBox *const cbo = cboImageType[sys][imageType];
	assert(cbo != nullptr);
	if (cbo) {
		cbo->setCurrentIndex(prio < IMG_TYPE_COUNT ? prio+1 : 0);
	}
}

/** Other ImageTypesTabPrivate functions. **/

// TODO

/** ImageTypesTab **/

ImageTypesTab::ImageTypesTab(QWidget *parent)
	: super(parent)
	, d_ptr(new ImageTypesTabPrivate(this))
{
	Q_D(ImageTypesTab);
	d->ui.setupUi(this);

	// Create the control grid.
	d->createGrid();
}

ImageTypesTab::~ImageTypesTab()
{
	delete d_ptr;
}

/**
 * Widget state has changed.
 * @param event State change event.
 */
void ImageTypesTab::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(ImageTypesTab);
		d->ui.retranslateUi(this);
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/**
 * Reset the configuration.
 */
void ImageTypesTab::reset(void)
{
	// TODO
}

/**
 * Save the configuration.
 */
void ImageTypesTab::save(void)
{
	// TODO
}
