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
#include <QtGui/QFontMetrics>
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
			QSize szValueHint;	// Size hint for the value column.

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

	// Size hint for the monospace column.
	QFontMetrics fm(fntMonospace);
	szValueHint = fm.size(Qt::TextSingleLine,
		QLatin1String("0123456789ABCDEF0123456789ABCDEF"));

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
			switch (index.column()) {
				case COL_KEY_NAME:
					return key->name;
				case COL_VALUE:
					return key->value;
				default:
					break;
			}
			break;

		case Qt::EditRole:
			switch (index.column()) {
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
			// Text should be left-aligned horizontally, center-aligned vertically.
			// NOTE: Center-aligning the encryption key causes
			// weirdness when editing, especially since if the
			// key is short, the editor will start in the middle
			// of the column instead of the left side.
			return (int)(Qt::AlignLeft | Qt::AlignVCenter);

		case Qt::FontRole:
			switch (index.column()) {
				case COL_VALUE:
					// The key value should use a monospace font.
					return d->style.fntMonospace;

				default:
					break;
			}
			break;

		case Qt::SizeHintRole:
			switch (index.column()) {
				case COL_VALUE:
					// Use the monospace size hint.
					return d->style.szValueHint;
				case COL_ISVALID:
					// Increase row height by 4px.
					return QSize(d->style.pxmIsValid_width,
						(d->style.pxmIsValid_height + 4));
				default:
					break;
			}
			break;

		default:
			break;
	}

	// Default value.
	return QVariant();
}

bool KeyStoreModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	Q_D(KeyStoreModel);
	if (!d->keyStore || !index.isValid())
		return false;
	if (index.row() >= rowCount())
		return false;

	// Only COL_VALUE can be edited, and only text.
	if (index.column() != COL_VALUE || role != Qt::EditRole)
		return false;

	// Edit the value.
	// TODO: Make sure it's hexadecimal, and verify the key.
	// KeyStore::setKey() will emit a signal if the value changes,
	// which will cause KeyStoreModel to emit dataChanged().
	d->keyStore->setKey(index.row(), value.toString());
	return true;
}

Qt::ItemFlags KeyStoreModel::flags(const QModelIndex &index) const
{
	Q_D(const KeyStoreModel);
	if (!d->keyStore || !index.isValid())
		return 0;
	if (index.row() >= rowCount())
		return 0;

	switch (index.column()) {
		case COL_VALUE:
			// Value can be edited.
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
		default:
			// Standard flags.
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}
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
	QModelIndex qmi_right = createIndex(idx, COL_MAX-1);
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
	QModelIndex qmi_right = createIndex(d->totalKeyCount-1, COL_MAX-1);
	emit dataChanged(qmi_left, qmi_right);
}

/**
 * The system theme has changed.
 */
void KeyStoreModel::themeChanged_slot(void)
{
	// Reinitialize the style.
	Q_D(KeyStoreModel);
	emit layoutAboutToBeChanged();
	d->style.init();
	emit layoutChanged();
}
