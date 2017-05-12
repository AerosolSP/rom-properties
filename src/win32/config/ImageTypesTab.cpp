/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ImageTypesTab.cpp: Image type priorities tab. (Part of ConfigDialog.)   *
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

#include "stdafx.h"
#include "ImageTypesTab.hpp"

#include "libromdata/RpWin32.hpp"
#include "libromdata/config/Config.hpp"
#include "libromdata/RomData.hpp"
using LibRomData::Config;
using LibRomData::RomData;

#include "WinUI.hpp"
#include "resource.h"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
using std::wstring;

class ImageTypesTabPrivate
{
	public:
		ImageTypesTabPrivate();

	private:
		RP_DISABLE_COPY(ImageTypesTabPrivate)

	public:
		// Property for "D pointer".
		// This points to the ImageTypesTabPrivate object.
		static const wchar_t D_PTR_PROP[];

	public:
		// Image type data.
		static const int SYS_COUNT = 8;
		static const rp_char *const imageTypeNames[RomData::IMG_EXT_MAX+1];
		static const rp_char *const sysNames[SYS_COUNT];

		/**
		 * Create the grid of static text and combo boxes.
		 */
		void createGrid(void);

		/**
		 * Reset the grid to the current configuration.
		 */
		void reset(void);

		/**
		 * Save the configuration.
		 */
		void save(void);

	public:
		/**
		 * Dialog procedure.
		 * @param hDlg
		 * @param uMsg
		 * @param wParam
		 * @param lParam
		 */
		static INT_PTR CALLBACK dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

		/**
		 * Property sheet callback procedure.
		 * @param hWnd
		 * @param uMsg
		 * @param ppsp
		 */
		static UINT CALLBACK callbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

	public:
		// Property sheet.
		HPROPSHEETPAGE hPropSheetPage;
		HWND hWndPropSheet;

		// Has the user changed anything?
		bool changed;
};

/** ImageTypesTabPrivate **/

ImageTypesTabPrivate::ImageTypesTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, changed(false)
{ }

// Property for "D pointer".
// This points to the ImageTypesTabPrivate object.
const wchar_t ImageTypesTabPrivate::D_PTR_PROP[] = L"ImageTypesTabPrivate";

// Image type names.
const rp_char *const ImageTypesTabPrivate::imageTypeNames[RomData::IMG_EXT_MAX+1] = {
	_RP("Internal\nIcon"),
	_RP("Internal\nBanner"),
	_RP("Internal\nMedia"),
	_RP("External\nMedia"),
	_RP("External\nCover"),
	_RP("External\n3D Cover"),
	_RP("External\nFull Cover"),
	_RP("External\nBox"),
};

// System names.
const rp_char *const ImageTypesTabPrivate::sysNames[SYS_COUNT] = {
	_RP("amiibo"),
	_RP("Dreamcast Saves"),
	_RP("GameCube / Wii"),
	_RP("GameCube Saves"),
	_RP("Nintendo DS(i)"),
	_RP("Nintendo 3DS"),
	_RP("PlayStation Saves"),
	_RP("Wii U"),
};

/**
 * Create the grid of static text and combo boxes.
 */
void ImageTypesTabPrivate::createGrid()
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// Get the dialog margin.
	// 7x7 DLU margin is recommended by the Windows UX guidelines.
	// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
	RECT dlgMargin = {7, 7, 8, 8};
	MapDialogRect(hWndPropSheet, &dlgMargin);

	// Get the font of the parent dialog.
	HFONT hFontDlg = GetWindowFont(GetParent(hWndPropSheet));
	assert(hFontDlg != nullptr);
	if (!hFontDlg) {
		// No font?!
		return;
	}

	// Get the dimensions of IDC_IMAGETYPES_DESC2.
	HWND lblDesc2 = GetDlgItem(hWndPropSheet, IDC_IMAGETYPES_DESC2);
	assert(lblDesc2 != nullptr);
	if (!lblDesc2) {
		// Label is missing...
		return;
	}
	RECT rect_lblDesc2;
	GetWindowRect(lblDesc2, &rect_lblDesc2);
	MapWindowPoints(HWND_DESKTOP, GetParent(lblDesc2), (LPPOINT)&rect_lblDesc2, 2);

	// Determine the size of the largest image type label.
	SIZE sz_lblImageType = {0, 0};
	for (int i = RomData::IMG_EXT_MAX; i >= 0; i--) {
		SIZE szCur;
		WinUI::measureTextSize(hWndPropSheet, hFontDlg, RP2W_c(imageTypeNames[i]), &szCur);
		if (szCur.cx > sz_lblImageType.cx) {
			sz_lblImageType.cx = szCur.cx;
		}
		if (szCur.cy > sz_lblImageType.cy) {
			sz_lblImageType.cy = szCur.cy;
		}
	}

	// Determine the size of the largest system name label.
	// TODO: Height needs to match the combo boxes.
	SIZE sz_lblSysName = {0, 0};
	for (int i = SYS_COUNT-1; i >= 0; i--) {
		SIZE szCur;
		WinUI::measureTextSize(hWndPropSheet, hFontDlg, RP2W_c(sysNames[i]), &szCur);
		if (szCur.cx > sz_lblSysName.cx) {
			sz_lblSysName.cx = szCur.cx;
		}
		if (szCur.cy > sz_lblSysName.cy) {
			sz_lblSysName.cy = szCur.cy;
		}
	}

	// Create the image type labels.
	POINT curPt = {rect_lblDesc2.left + sz_lblSysName.cx + dlgMargin.right,
		rect_lblDesc2.bottom + dlgMargin.bottom};
	for (unsigned int i = 0; i <= RomData::IMG_EXT_MAX; i++) {
		HWND hStatic = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, RP2W_c(imageTypeNames[i]),
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_CENTER,
			curPt.x, curPt.y, sz_lblImageType.cx, sz_lblImageType.cy,
			hWndPropSheet, (HMENU)IDC_STATIC, nullptr, nullptr);
		SetWindowFont(hStatic, hFontDlg, FALSE);
		curPt.x += sz_lblImageType.cx;
	}

	// Create the system name labels.
	curPt.x = rect_lblDesc2.left;
	curPt.y += sz_lblImageType.cy;
	for (unsigned int i = 0; i < SYS_COUNT; i++) {
		HWND hStatic = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, RP2W_c(sysNames[i]),
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_LEFT,
			curPt.x, curPt.y, sz_lblSysName.cx, sz_lblSysName.cy,
			hWndPropSheet, (HMENU)IDC_STATIC, nullptr, nullptr);
		SetWindowFont(hStatic, hFontDlg, FALSE);
		curPt.y += sz_lblSysName.cy;
	}

	// Load the configuration.
	reset();
}

/**
 * Reset the grid to the current configuration.
 */
void ImageTypesTabPrivate::reset(void)
{
	// TODO

	// No longer changed.
	changed = false;
}

/**
 * Save the configuration.
 */
void ImageTypesTabPrivate::save(void)
{
	// TODO

	// No longer changed.
	changed = false;
}

/**
 * Dialog procedure.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK ImageTypesTabPrivate::dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Get the pointer to the ImageTypesTabPrivate object.
			ImageTypesTabPrivate *const d = reinterpret_cast<ImageTypesTabPrivate*>(pPage->lParam);
			if (!d)
				return TRUE;

			assert(d->hWndPropSheet == nullptr);
			d->hWndPropSheet = hDlg;

			// Store the D object pointer with this particular page dialog.
			SetProp(hDlg, D_PTR_PROP, reinterpret_cast<HANDLE>(d));

			// Create the control grid.
			d->createGrid();
			return TRUE;
		}

		case WM_DESTROY: {
			// Remove the D_PTR_PROP property from the page.
			// The D_PTR_PROP property stored the pointer to the
			// ImageTypesTabPrivate object.
			RemoveProp(hDlg, D_PTR_PROP);
			return TRUE;
		}

		case WM_NOTIFY: {
			ImageTypesTabPrivate *d = static_cast<ImageTypesTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No ImageTypesTabPrivate. Can't do anything...
				return FALSE;
			}

			LPPSHNOTIFY lppsn = reinterpret_cast<LPPSHNOTIFY>(lParam);
			switch (lppsn->hdr.code) {
				case PSN_APPLY:
					// Save settings.
					if (d->changed) {
						d->save();
					}
					break;

				case NM_CLICK:
				case NM_RETURN:
					// SysLink control notification.
					if (lppsn->hdr.hwndFrom == GetDlgItem(hDlg, IDC_IMAGETYPES_CREDITS)) {
						// Open the URL.
						PNMLINK pNMLink = reinterpret_cast<PNMLINK>(lParam);
						ShellExecute(nullptr, L"open", pNMLink->item.szUrl, nullptr, nullptr, SW_SHOW);
					}
					break;

				default:
					break;
			}
			break;
		}

		default:
			break;
	}

	return FALSE; // Let system deal with other messages
}

/**
 * Property sheet callback procedure.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
UINT CALLBACK ImageTypesTabPrivate::callbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	switch (uMsg) {
		case PSPCB_CREATE: {
			// Must return TRUE to enable the page to be created.
			return TRUE;
		}

		case PSPCB_RELEASE: {
			// TODO: Do something here?
			break;
		}

		default:
			break;
	}

	return FALSE;
}

/** ImageTypesTab **/

ImageTypesTab::ImageTypesTab(void)
	: d_ptr(new ImageTypesTabPrivate())
{ }

ImageTypesTab::~ImageTypesTab()
{
	delete d_ptr;
}

/**
 * Create the HPROPSHEETPAGE for this tab.
 *
 * NOTE: This function can only be called once.
 * Subsequent invocations will return nullptr.
 *
 * @return HPROPSHEETPAGE.
 */
HPROPSHEETPAGE ImageTypesTab::getHPropSheetPage(void)
{
	RP_D(ImageTypesTab);
	assert(d->hPropSheetPage == nullptr);
	if (d->hPropSheetPage) {
		// Property sheet has already been created.
		return nullptr;
	}

	extern HINSTANCE g_hInstance;

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);	
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp.hInstance = g_hInstance;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_IMAGETYPES);
	psp.pszIcon = nullptr;
	psp.pszTitle = L"Image Types";
	psp.pfnDlgProc = ImageTypesTabPrivate::dlgProc;
	psp.lParam = reinterpret_cast<LPARAM>(d);
	psp.pcRefParent = nullptr;
	psp.pfnCallback = ImageTypesTabPrivate::callbackProc;

	d->hPropSheetPage = CreatePropertySheetPage(&psp);
	return d->hPropSheetPage;
}

/**
 * Reset the contents of this tab.
 */
void ImageTypesTab::reset(void)
{
	RP_D(ImageTypesTab);
	d->reset();
}

/**
 * Save the contents of this tab.
 */
void ImageTypesTab::save(void)
{
	RP_D(ImageTypesTab);
	d->save();
}
