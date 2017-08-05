/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * AboutTab.hpp: About tab for rp-config.                                  *
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

#include "config.librpbase.h"

#include "AboutTab.hpp"
#include "config.version.h"
#include "git.h"

// librpbase
#include "librpbase/config/Config.hpp"
using LibRpBase::Config;

// C includes. (C++ namespace)
#include <cassert>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
// KIO version.
// NOTE: Only available as a compile-time constant.
#include <kio_version.h>
#else
// kdelibs version.
#include <kdeversion.h>
#endif

// Other libraries.
#ifdef HAVE_ZLIB
# include <zlib.h>
#endif
#ifdef HAVE_PNG
# include "librpbase/img/APNG_dlopen.h"
# include <png.h>
#endif
#ifdef ENABLE_DECRYPTION
# ifdef HAVE_NETTLE_VERSION_H
#  include "nettle/version.h"
# endif
#endif

#include "ui_AboutTab.h"
class AboutTabPrivate
{
	public:
		AboutTabPrivate() { }

	private:
		Q_DISABLE_COPY(AboutTabPrivate)

	public:
		Ui::AboutTab ui;

		/**
		 * Initialize the program title text.
		 */
		void initProgramTitleText(void);

		/**
		 * Is APNG supported?
		 * @return True if APNG is supported; false if not.
		 */
		static bool is_APNG_supported(void);

		/**
		 * Initialize the "Libraries" tab.
		 */
		void initLibrariesTab(void);
};

/** AboutTabPrivate **/

/**
 * Initialize the program title text.
 */
void AboutTabPrivate::initProgramTitleText(void)
{
	// lblTitle is RichText.

	// Program icon.
	// TODO: Make a custom icon instead of reusing the system icon.
	// TODO: Fallback for older Qt?
#if QT_VERSION >= QT_VERSION_CHECK(4,6,0)
	QIcon icon = QIcon::fromTheme(QLatin1String("media-flash"));
	if (!icon.isNull()) {
		// Get the 128x128 icon.
		// TODO: Determine the best size.
		ui.lblLogo->setPixmap(icon.pixmap(128, 128));
	} else {
		// No icon...
		ui.lblLogo->hide();
	}
#endif /* QT_VERSION >= QT_VERSION_CHECK(4,6,0) */

	// Useful strings.
	const QString b_start = QLatin1String("<b>");
	const QString b_end = QLatin1String("</b>");
	const QString br = QLatin1String("<br/>\n");

	QString sPrgTitle;
	sPrgTitle.reserve(4096);
	sPrgTitle = b_start +
		AboutTab::tr("ROM Properties Page") + b_end + br +
		AboutTab::tr("Shell Extension") + br + br +
		AboutTab::tr("Version %1")
			.arg(QLatin1String(RP_VERSION_STRING));
#ifdef RP_GIT_VERSION
	sPrgTitle += br + QString::fromUtf8(RP_GIT_VERSION);
# ifdef RP_GIT_DESCRIBE
	sPrgTitle += br + QString::fromUtf8(RP_GIT_DESCRIBE);
# endif /* RP_GIT_DESCRIBE */
#endif /* RP_GIT_VERSION */

	ui.lblTitle->setText(sPrgTitle);
}

/**
 * Initialize the "Libraries" tab.
 */
void AboutTabPrivate::initLibrariesTab(void)
{
	// lblLibraries is PlainText.

	// Useful strings.
	const QChar br = L'\n';
	const QString brbr = QLatin1String("\n\n");

	// NOTE: These strings can NOT be static.
	// Otherwise, they won't be retranslated if the UI language
	// is changed at runtime.

	//: Using an internal copy of a library.
	const QString sIntCopyOf = AboutTab::tr("Internal copy of %1.");
	//: Compiled with a specific version of an external library.
	const QString sCompiledWith = AboutTab::tr("Compiled with %1.");
	//: Using an external library, e.g. libpcre.so
	const QString sUsingDll = AboutTab::tr("Using %1.");
	//: License: (libraries with only a single license)
	const QString sLicense = AboutTab::tr("License: %1");
	//: Licenses: (libraries with multiple licenses)
	const QString sLicenses = AboutTab::tr("Licenses: %1");

	// Included libraries string.
	QString sLibraries;
	sLibraries.reserve(4096);

	/** Qt **/
	const QString qtVersion = QLatin1String("Qt ") + QLatin1String(qVersion());
#ifdef QT_IS_STATIC
	sLibraries += sIntCopyOf.arg(qtVersion);
#else
	QString qtVersionCompiled = QLatin1String("Qt " QT_VERSION_STR);
	sLibraries += sCompiledWith.arg(qtVersionCompiled) + QChar(L'\n');
	sLibraries += sUsingDll.arg(qtVersion);
#endif /* QT_IS_STATIC */
	sLibraries += br +
		QLatin1String("Copyright (C) 1995-2017 The Qt Company Ltd. and/or its subsidiaries.");
	// TODO: Check QT_VERSION at runtime?
#if QT_VERSION >= QT_VERSION_CHECK(4,5,0)
	sLibraries += br + sLicenses.arg(QLatin1String("GNU LGPL v2.1+, GNU GPL v2+"));
#else
	sLibraries += br + sLicense.arg(QLatin1String("GNU GPL v2+"));
#endif /* QT_VERSION */

	/** KDE **/
	sLibraries += brbr;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	// NOTE: Can't obtain the runtime version for KDE5 easily...
	sLibraries += sCompiledWith.arg(QLatin1String("KDE Frameworks " KIO_VERSION_STRING));
	sLibraries += br +
		QLatin1String("Copyright (C) 1996-2017 KDE contributors.");
	sLibraries += br + sLicense.arg(QLatin1String("GNU LGPL v2.1+"));
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
	const QString kdeVersion = QLatin1String("KDE Libraries ") + QLatin1String(KDE::versionString());
	sLibraries += sCompiledWith.arg(QLatin1String("KDE Libraries " KDE_VERSION_STRING));
	sLibraries += br + sUsingDll.arg(kdeVersion);
	sLibraries += br +
		QLatin1String("Copyright (C) 1996-2017 KDE contributors.");
	sLibraries += br + sLicense.arg(QLatin1String("GNU LGPL v2.1+"));
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

	/** zlib **/
#ifdef HAVE_ZLIB
	sLibraries += brbr;
	QString sZlibVersion = QLatin1String("zlib %1");
	sZlibVersion = sZlibVersion.arg(QLatin1String(zlibVersion()));

#if defined(USE_INTERNAL_ZLIB) && !defined(USE_INTERNAL_ZLIB_DLL)
	sLibraries += sIntCopyOf.arg(sZlibVersion)
#else
	QString sZlibVersionCompiled = QLatin1String("zlib " ZLIB_VERSION);
	sLibraries += sCompiledWith.arg(sZlibVersionCompiled) + br;
	sLibraries += sUsingDll.arg(sZlibVersion);
#endif
	sLibraries += br + QLatin1String(
			"Copyright (C) 1995-2017 Jean-loup Gailly and Mark Adler.\n"
			"http://www.zlib.net/\n");
	sLibraries += sLicense.arg(QLatin1String("zlib license"));
#endif /* HAVE_ZLIB */

	/** libpng **/
#ifdef HAVE_PNG
	// APNG suffix.
	const bool APNG_is_supported = (APNG_ref() == 0);
	if (APNG_is_supported) {
		// APNG is supported.
		// Unreference it to prevent leaks.
		APNG_unref();
	}

	const QString pngAPngSuffix = (APNG_is_supported
			? QLatin1String(" + APNG")
			: AboutTab::tr(" (No APNG support)"));

	sLibraries += brbr;
	const uint32_t png_version_number = png_access_version_number();
	QString pngVersion = QString::fromLatin1("libpng %1.%2.%3")
			.arg(png_version_number / 10000)
			.arg((png_version_number / 100) % 100)
			.arg(png_version_number % 100);
	pngVersion += pngAPngSuffix;

#if defined(USE_INTERNAL_PNG) && !defined(USE_INTERNAL_ZLIB_DLL)
	sLibraries += sIntCopyOf.arg(pngVersion);
#else
	// NOTE: Gentoo's libpng has "+apng" at the end of
	// PNG_LIBPNG_VER_STRING if APNG is enabled.
	// We have our own "+ APNG", so remove Gentoo's.
	QString pngVersionCompiled = QLatin1String("libpng " PNG_LIBPNG_VER_STRING);
	while (!pngVersionCompiled.isEmpty()) {
		int idx = pngVersionCompiled.size() - 1;
		const QChar chr = pngVersionCompiled[idx];
		if (chr.isDigit())
			break;
		pngVersionCompiled.resize(idx);
	}
	pngVersionCompiled += pngAPngSuffix;
	sLibraries += sCompiledWith.arg(pngVersionCompiled) + br;
	sLibraries += sUsingDll.arg(pngVersion);
#endif

	/**
	 * NOTE: MSVC does not define __STDC__ by default.
	 * If __STDC__ is not defined, the libpng copyright
	 * will not have a leading newline, and all newlines
	 * will be replaced with groups of 6 spaces.
	 */
	QString png_copyright = QLatin1String(png_get_copyright(nullptr));
	if (png_copyright.indexOf(QChar(L'\n')) < 0) {
		// Convert spaces to newlines.
		// TODO: QString::simplified() to remove other patterns,
		// or just assume all versions of libpng have the same
		// number of spaces?
		png_copyright.replace(QLatin1String("      "), QLatin1String("\n"));
		png_copyright.prepend(QChar(L'\n'));
		png_copyright.append(QChar(L'\n'));
	}
	sLibraries += png_copyright;
	sLibraries += sLicense.arg(QLatin1String("libpng license"));
#endif /* HAVE_PNG */

	/** nettle **/
#ifdef ENABLE_DECRYPTION
	sLibraries += brbr;
# ifdef HAVE_NETTLE_VERSION_H
	QString nettle_build_version = QLatin1String("GNU Nettle %1.%2");
	sLibraries += sCompiledWith.arg(
		nettle_build_version
			.arg(NETTLE_VERSION_MAJOR)
			.arg(NETTLE_VERSION_MINOR));
#  ifdef HAVE_NETTLE_VERSION_FUNCTIONS
	QString nettle_runtime_version = QLatin1String("GNU Nettle %1.%2");
	sLibraries += br + sUsingDll.arg(
		nettle_runtime_version
			.arg(nettle_version_major())
			.arg(nettle_version_minor()));
#  endif /* HAVE_NETTLE_VERSION_FUNCTIONS */
	sLibraries += br +
		QString::fromUtf8("Copyright (C) 2001-2016 Niels Möller.");
	sLibraries += br + sLicenses.arg(QLatin1String("GNU LGPL v3+, GNU GPL v2+"));
# else /* !HAVE_NETTLE_VERSION_H */
#  ifdef HAVE_NETTLE_3
	sLibraries += sCompiledWith.arg(QLatin1String("GNU Nettle 3.0"));
	sLibraries += br +
		QString::fromUtf8("Copyright (C) 2001-2014 Niels Möller.");
	sLibraries += br + sLicense.arg(QLatin1String("GNU LGPL v3+, GNU GPL v2+"));
#  else /* !HAVE_NETTLE_3 */
	sLibraries += sCompiledWith.arg(QLatin1String("GNU Nettle 2.x"));
	sLibraries += br +
		QString::fromUtf8("Copyright (C) 2001-2013 Niels Möller.");
	sLibraries += br + sLicense.arg(QLatin1String("GNU LGPL v2.1+"));
#  endif /* HAVE_NETTLE_3 */
# endif /* HAVE_NETTLE_VERSION_H */
#endif /* ENABLE_DECRYPTION */

	// We're done building the string.
	ui.lblLibraries->setText(sLibraries);
}

/** AboutTab **/

AboutTab::AboutTab(QWidget *parent)
	: super(parent)
	, d_ptr(new AboutTabPrivate())
{
	Q_D(AboutTab);
	d->ui.setupUi(this);

	// Initialize the title and tabs.
	d->initProgramTitleText();
	d->initLibrariesTab();
}

AboutTab::~AboutTab()
{
	delete d_ptr;
}

/**
 * Widget state has changed.
 * @param event State change event.
 */
void AboutTab::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(AboutTab);
		d->ui.retranslateUi(this);
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/**
 * Reset the configuration.
 */
void AboutTab::reset(void)
{
	// Nothing to do here.
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void AboutTab::loadDefaults(void)
{
	// Nothing to do here.
}

/**
 * Save the configuration.
 * @param pSettings QSettings object.
 */
void AboutTab::save(QSettings *pSettings)
{
	// Nothing to do here.
	Q_UNUSED(pSettings)
}
