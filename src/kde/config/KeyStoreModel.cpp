/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyStoreModel.cpp: QAbstractListModel for KeyStore.                     *
 *                                                                         *
 * Copyright (c) 2012-2017 by David Korth.                                 *
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

#include "KeyStoreModel.hpp"
#include "KeyStore.hpp"

// Qt includes.
#include <QtGui/QFont>
#include <QtGui/QPixmap>
#include <QApplication>
#include <QStyle>

/** KeyStoreModelPrivate **/

class KeyStoreModelPrivate
{
	public:
		explicit KeyStoreModelPrivate(KeyStoreModel *q);

	protected:
		KeyStoreModel *const q_ptr;
		Q_DECLARE_PUBLIC(KeyStoreModel)
	private:
		Q_DISABLE_COPY(KeyStoreModelPrivate)

	public:
		KeyStore *keyStore;

		// Style variables.
		struct style_t {
			style_t() { init(); }

			/**
			 * Initialize the style variables.
			 */
			void init(void);

			// Monospace font.
			QFont fntMonospace;

			// Pixmaps for COL_ISVALID.
			// TODO: Hi-DPI support.
			static const int pxmIsValid_width = 16;
			static const int pxmIsValid_height = 16;
			QPixmap pxmIsValid_unknown;
			QPixmap pxmIsValid_invalid;
			QPixmap pxmIsValid_good;
		};
		style_t style;

		/**
		 * Cached copy of keyStore->totalKeyCount().
		 * This value is needed after the KeyStore is destroyed,
		 * so we need to cache it here, since the destroyed()
		 * slot might be run *after* the KeyStore is deleted.
		 */
		int totalKeyCount;
};

KeyStoreModelPrivate::KeyStoreModelPrivate(KeyStoreModel *q)
	: q_ptr(q)
	, keyStore(nullptr)
	, totalKeyCount(0)
{ }

/**
 * Initialize the style variables.
 */
void KeyStoreModelPrivate::style_t::init(void)
{
	// Monospace font.
	fntMonospace = QFont(QLatin1String("Monospace"));
	fntMonospace.setStyleHint(QFont::TypeWriter);

	// Initialize the COL_ISVALID pixmaps.
	// TODO: Handle SP_MessageBoxQuestion on non-Windows systems,
	// which usually have an 'i' icon here (except for GNOME).
	QStyle *const style = QApplication::style();
	pxmIsValid_unknown = style->standardIcon(QStyle::SP_MessageBoxQuestion)
				.pixmap(pxmIsValid_width, pxmIsValid_height);
	pxmIsValid_invalid = style->standardIcon(QStyle::SP_MessageBoxCritical)
				.pixmap(pxmIsValid_width, pxmIsValid_height);
	pxmIsValid_good    = style->standardIcon(QStyle::SP_DialogApplyButton)
				.pixmap(pxmIsValid_width, pxmIsValid_height);
}

/** KeyStoreModel **/

KeyStoreModel::KeyStoreModel(QObject *parent)
	: super(parent)
	, d_ptr(new KeyStoreModelPrivate(this))
{
	// TODO: Handle system theme changes.
	// On Windows, listen for WM_THEMECHANGED.
	// Not sure about other systems...
}

KeyStoreModel::~KeyStoreModel()
{
	delete d_ptr;
}


int KeyStoreModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	Q_D(const KeyStoreModel);
	return d->totalKeyCount;
}

int KeyStoreModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return COL_MAX;
}

QVariant KeyStoreModel::data(const QModelIndex& index, int role) const
{
	Q_D(const KeyStoreModel);
	if (!d->keyStore || !index.isValid())
		return QVariant();
	if (index.row() >= rowCount())
		return QVariant();

	// Get the key.
	const KeyStore::Key *key = d->keyStore->getKey(index.row());
	if (!key)
		return QVariant();

	switch (role) {
		case Qt::DisplayRole:
			// TODO
			switch (index.column()) {
				case COL_KEY_NAME:
					return key->name;
				case COL_VALUE:
					return key->value;
				default:
					break;
			}
			break;

		case Qt::DecorationRole:
			// Images must use Qt::DecorationRole.
			// FIXME: Add a QStyledItemDelegate to center-align the icon.
			switch (index.column()) {
				case COL_ISVALID:
					switch (key->status) {
						default:
						case KeyStore::Key::Status_Unknown:
							// Unknown...
							return d->style.pxmIsValid_unknown;
						case KeyStore::Key::Status_NotAKey:
							// The key data is not in the correct format.
							return d->style.pxmIsValid_unknown;
						case KeyStore::Key::Status_Empty:
							// Empty key.
							break;
						case KeyStore::Key::Status_Incorrect:
							// Key is incorrect.
							return d->style.pxmIsValid_invalid;
						case KeyStore::Key::Status_OK:
							// Key is correct.
							return d->style.pxmIsValid_good;
					}

				default:
					break;
			}
			break;

		case Qt::TextAlignmentRole:
			switch (index.column()) {
				case COL_VALUE:
					// Encryption key should be center-aligned.
					return (int)(Qt::AlignHCenter | Qt::AlignVCenter);
				default:
					// Other text should be left-aligned horizontally, center-aligned vertically.
					return (int)(Qt::AlignLeft | Qt::AlignVCenter);
			}
			break;

		case Qt::FontRole:
			switch (index.column()) {
				case COL_VALUE:
					// The key value should use a monospace font.
					return d->style.fntMonospace;

				default:
					break;
			}
			break;

		case Qt::SizeHintRole: {
			// Increase row height by 4px.
			if (index.column() == COL_ISVALID) {
				return QSize(d->style.pxmIsValid_width,
					     (d->style.pxmIsValid_height + 4));
			}
			break;
		}

		default:
			break;
	}

	// Default value.
	return QVariant();
}

QVariant KeyStoreModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(orientation);

	switch (role) {
		case Qt::DisplayRole:
			switch (section) {
				case COL_KEY_NAME:	return tr("Key Name");
				case COL_VALUE:		return tr("Value");
				case COL_ISVALID:	return tr("Valid?");

				default:
					break;
			}
			break;

		case Qt::TextAlignmentRole:
			// Center-align the text.
			return Qt::AlignHCenter;
	}

	// Default value.
	return QVariant();
}

/**
 * Set the KeyStore to use in this model.
 * @param keyStore KeyStore.
 */
void KeyStoreModel::setKeyStore(KeyStore *keyStore)
{
	Q_D(KeyStoreModel);
	if (d->keyStore == keyStore) {
		// No point in setting it to the same thing...
		return;
	}

	// If we have a KeyStore already, disconnect its signals.
	if (d->keyStore) {
		// Notify the view that we're about to remove all rows.
		// TODO: totalKeyCount should already be cached...
		const int totalKeyCount = d->keyStore->totalKeyCount();
		if (totalKeyCount > 0) {
			beginRemoveRows(QModelIndex(), 0, (totalKeyCount - 1));
		}

		// Disconnect the KeyStore's signals.
		disconnect(d->keyStore, SIGNAL(destroyed(QObject*)),
			   this, SLOT(keyStore_destroyed_slot(QObject*)));
		disconnect(d->keyStore, SIGNAL(keyChanged(int)),
			   this, SLOT(keyStore_keyChanged_slot(int)));
		disconnect(d->keyStore, SIGNAL(allKeysChanged()),
			   this, SLOT(keyStore_allKeysChanged_slot()));

		d->keyStore = nullptr;

		// Done removing rows.
		d->totalKeyCount = 0;
		if (totalKeyCount > 0) {
			endRemoveRows();
		}
	}

	if (keyStore) {
		// Notify the view that we're about to add rows.
		const int totalKeyCount = keyStore->totalKeyCount();
		if (totalKeyCount > 0) {
			beginInsertRows(QModelIndex(), 0, (totalKeyCount - 1));
		}

		// Set the KeyStore.
		d->keyStore = keyStore;
		// NOTE: totalKeyCount must be set here.
		d->totalKeyCount = totalKeyCount;

		// Connect the KeyStore's signals.
		connect(d->keyStore, SIGNAL(destroyed(QObject*)),
			this, SLOT(keyStore_destroyed_slot(QObject*)));
		connect(d->keyStore, SIGNAL(keyChanged(int)),
			this, SLOT(keyStore_keyChanged_slot(int)));
		connect(d->keyStore, SIGNAL(allKeysChanged()),
			this, SLOT(keyStore_allKeysChanged_slot()));

		// Done adding rows.
		if (totalKeyCount > 0) {
			endInsertRows();
		}
	}

	// KeyStore has been changed.
	emit keyStoreChanged();
}

/**
 * Get the KeyStore in use by this model.
 * @return KeyStore.
 */
KeyStore *KeyStoreModel::keyStore(void) const
{
	Q_D(const KeyStoreModel);
	return d->keyStore;
}

/** Private slots. **/

/**
 * KeyStore object was destroyed.
 * @param obj QObject that was destroyed.
 */
void KeyStoreModel::keyStore_destroyed_slot(QObject *obj)
{
	Q_D(KeyStoreModel);
	if (obj != d->keyStore)
		return;

	// Our KeyStore was destroyed.
	d->keyStore = nullptr;
	int old_totalKeyCount = d->totalKeyCount;
	if (old_totalKeyCount > 0) {
		beginRemoveRows(QModelIndex(), 0, (old_totalKeyCount - 1));
	}
	d->totalKeyCount = 0;
	if (old_totalKeyCount > 0) {
		endRemoveRows();
	}

	emit keyStoreChanged();
}

/**
 * A key in the KeyStore has changed.
 * @param keyIdx Flat key index.
 */
void KeyStoreModel::keyStore_keyChanged_slot(int idx)
{
	QModelIndex qmi_left = createIndex(idx, 0);
	QModelIndex qmi_right = createIndex(idx, COL_MAX);
	emit dataChanged(qmi_left, qmi_right);
}

/**
 * All keys in the KeyStore have changed.
 */
void KeyStoreModel::keyStore_allKeysChanged_slot(void)
{
	Q_D(KeyStoreModel);
	if (d->totalKeyCount <= 0)
		return;
	QModelIndex qmi_left = createIndex(0, 0);
	QModelIndex qmi_right = createIndex(d->totalKeyCount-1, COL_MAX);
	emit dataChanged(qmi_left, qmi_right);
}

/**
 * The system theme has changed.
 */
void KeyStoreModel::themeChanged_slot(void)
{
	// Reinitialize the style.
	Q_D(KeyStoreModel);
	d->style.init();

	// TODO: Force an update?
}
