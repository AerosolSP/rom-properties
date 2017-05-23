/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ImageTypesTab.hpp: Image Types tab for rp-config.                       *
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

#ifndef __ROMPROPERTIES_KDE_CONFIG_IMAGETYPESTAB_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_IMAGETYPESTAB_HPP__

#include "ITab.hpp"

class ImageTypesTabPrivate;
class ImageTypesTab : public ITab
{
	Q_OBJECT

	public:
		ImageTypesTab(QWidget *parent = nullptr);
		virtual ~ImageTypesTab();

	private:
		typedef ITab super;
		ImageTypesTabPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(ImageTypesTab);
		Q_DISABLE_COPY(ImageTypesTab)

	protected:
		// State change event. (Used for switching the UI language at runtime.)
		void changeEvent(QEvent *event);

	public slots:
		/**
		 * Reset the configuration.
		 */
		virtual void reset(void) override final;

		/**
		 * Save the configuration.
		 * @param pSettings QSettings object.
		 */
		virtual void save(QSettings *pSettings) override final;

	protected slots:
		/**
		 * A QComboBox index has changed.
		 * @param cbid ComboBox ID.
		 */
		void cboImageType_currentIndexChanged(int cbid);
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_IMAGETYPESTAB_HPP__ */
