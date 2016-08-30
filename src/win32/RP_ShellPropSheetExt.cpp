/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt.hpp: IShellPropSheetExt implementation.            *
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

// References:
// - http://www.codeproject.com/Articles/338268/COM-in-C
// - https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
// - https://msdn.microsoft.com/en-us/library/ms677109(v=vs.85).aspx

#include "stdafx.h"
#include "RP_ShellPropSheetExt.hpp"
#include "RegKey.hpp"

// libromdata
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/RomData.hpp"
#include "libromdata/RomFields.hpp"
#include "libromdata/RpFile.hpp"
using LibRomData::RomDataFactory;
using LibRomData::RomData;
using LibRomData::RomFields;
using LibRomData::IRpFile;
using LibRomData::RpFile;
using LibRomData::rp_string;

// CLSID
const CLSID CLSID_RP_ShellPropSheetExt =
	{0x2443C158, 0xDF7C, 0x4352, {0xB4, 0x35, 0xBC, 0x9F, 0x88, 0x5F, 0xFD, 0x52}};

// IDC_STATIC might not be defined.
#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

// Property for "external pointer".
// This links the property sheet to the COM object.
#define EXT_POINTER_PROP L"RP_ShellPropSheetExt"

static inline LPWORD lpwAlign(LPWORD lpIn, ULONG_PTR dw2Power = 4)
{
	ULONG_PTR ul;

	ul = (ULONG_PTR)lpIn;
	ul += dw2Power-1;
	ul &= ~(dw2Power-1);
	return (LPWORD)ul;
}

RP_ShellPropSheetExt::RP_ShellPropSheetExt()
	: m_romData(nullptr)
{
	m_szSelectedFile[0] = 0;
}

RP_ShellPropSheetExt::~RP_ShellPropSheetExt()
{
	delete m_romData;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ShellPropSheetExt::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	// Always set out parameter to NULL, validating it first.
	if (!ppvObj)
		return E_INVALIDARG;
	*ppvObj = NULL;

	// Check if this interface is supported.
	// NOTE: static_cast<> is required due to vtable shenanigans.
	// Also, IID_IUnknown must always return the same pointer.
	// References:
	// - http://stackoverflow.com/questions/1742848/why-exactly-do-i-need-an-explicit-upcast-when-implementing-queryinterface-in-a
	// - http://stackoverflow.com/a/2812938
	if (riid == IID_IUnknown || riid == IID_IShellExtInit) {
		*ppvObj = static_cast<IShellExtInit*>(this);
	} else if (riid == IID_IShellPropSheetExt) {
		*ppvObj = static_cast<IShellPropSheetExt*>(this);
	} else {
		// Interface is not supported.
		return E_NOINTERFACE;
	}

	// Make sure we count this reference.
	AddRef();
	return NOERROR;
}

/**
 * Register the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ShellPropSheetExt::Register(void)
{
	static const wchar_t description[] = L"ROM Properties Page - Property Sheet";
	extern const wchar_t RP_ProgID[];

	// Convert the CLSID to a string.
	wchar_t clsid_str[48];	// maybe only 40 is needed?
	LONG lResult = StringFromGUID2(__uuidof(RP_ShellPropSheetExt), clsid_str, sizeof(clsid_str)/sizeof(clsid_str[0]));
	if (lResult <= 0)
		return ERROR_INVALID_PARAMETER;

	// Register the COM object.
	lResult = RegKey::RegisterComObject(__uuidof(RP_ShellPropSheetExt), RP_ProgID, description);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// Register as an "approved" shell extension.
	lResult = RegKey::RegisterApprovedExtension(__uuidof(RP_ShellPropSheetExt), description);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// Register as a property sheet handler for this ProgID.
	// TODO: Register for 'all' types, like the various hash extensions?
	// Create/open the ProgID key.
	RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, RP_ProgID, KEY_WRITE, true);
	if (!hkcr_ProgID.isOpen())
		return hkcr_ProgID.lOpenRes();
	// Create/open the "ShellEx" key.
	RegKey hkcr_ShellEx(hkcr_ProgID, L"ShellEx", KEY_WRITE, true);
	if (!hkcr_ShellEx.isOpen())
		return hkcr_ShellEx.lOpenRes();
	// Create/open the PropertySheetHandlers key.
	RegKey hkcr_PropertySheetHandlers(hkcr_ShellEx, L"PropertySheetHandlers", KEY_WRITE, true);
	if (!hkcr_PropertySheetHandlers.isOpen())
		return hkcr_PropertySheetHandlers.lOpenRes();
	// Create/open the "rom-properties" property sheet handler key.
	RegKey hkcr_PropSheet_RomProperties(hkcr_PropertySheetHandlers, RP_ProgID, KEY_WRITE, true);
	if (!hkcr_PropSheet_RomProperties.isOpen())
		return hkcr_PropSheet_RomProperties.lOpenRes();
	// Set the default value to this CLSID.
	lResult = hkcr_PropSheet_RomProperties.write(nullptr, clsid_str);
	if (lResult != ERROR_SUCCESS)
		return lResult;
	hkcr_PropSheet_RomProperties.close();
	hkcr_PropertySheetHandlers.close();
	hkcr_ShellEx.close();

	// COM object registered.
	return ERROR_SUCCESS;
}

/**
 * Unregister the COM object.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ShellPropSheetExt::Unregister(void)
{
	extern const wchar_t RP_ProgID[];

	// Unegister the COM object.
	LONG lResult = RegKey::UnregisterComObject(__uuidof(RP_ShellPropSheetExt), RP_ProgID);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// TODO
	return ERROR_SUCCESS;
}

PCWSTR RP_ShellPropSheetExt::GetSelectedFile(void) const
{
	return this->m_szSelectedFile;
}

/** IShellExtInit **/

/** IShellPropSheetExt **/
// References:
// - https://msdn.microsoft.com/en-us/library/windows/desktop/bb775094(v=vs.85).aspx
IFACEMETHODIMP RP_ShellPropSheetExt::Initialize(
	LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	if (!pDataObj) {
		return E_INVALIDARG;
	}

	HRESULT hr = E_FAIL;
	FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stm;

	// The pDataObj pointer contains the objects being acted upon. In this 
	// example, we get an HDROP handle for enumerating the selected files and 
	// folders.
	if (SUCCEEDED(pDataObj->GetData(&fe, &stm))) {
		// Get an HDROP handle.
		HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
		if (hDrop != NULL) {
			// Determine how many files are involved in this operation. This 
			// code sample displays the custom context menu item when only 
			// one file is selected. 
			UINT nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
			if (nFiles == 1) {
				// Get the path of the file.
				if (0 != DragQueryFile(hDrop, 0, m_szSelectedFile, 
				    ARRAYSIZE(m_szSelectedFile)))
				{
					// Open the file.
					// FIXME: Use utf16_to_rp_string() after merging to master.
					IRpFile *file = new RpFile(
						rp_string(reinterpret_cast<const char16_t*>(m_szSelectedFile)),
						RpFile::FM_OPEN_READ);
					if (file && file->isOpen()) {
						// Get the appropriate RomData class for this ROM.
						m_romData = RomDataFactory::getInstance(file);
						if (m_romData) {
							// ROM is supported. Initialize the dialog.
							initDialog();
							hr = S_OK;
						} else {
							// ROM is not supported.
							hr = E_FAIL;
						}
					}

					// RomData classes dup() the file, so
					// we can close the original one.
					delete file;
				}
			}

			GlobalUnlock(stm.hGlobal);
		}

		ReleaseStgMedium(&stm);
	}

	// If any value other than S_OK is returned from the method, the property 
	// sheet is not displayed.
	return hr;
}

/**
 * Initialize the dialog for the open ROM data object.
 */
void RP_ShellPropSheetExt::initDialog(void)
{
	if (!m_romData) {
		// No ROM data loaded.
		m_dlgBuilder.clear();
		return;
	}

	// Get the fields.
	const RomFields *fields = m_romData->fields();
	if (!fields) {
		// No fields.
		// TODO: Show an error?
		m_dlgBuilder.clear();
		return;
	}
	const int count = fields->count();

	// Create the dialog.
	DLGTEMPLATE dlt;
	dlt.style = DS_SETFONT | DS_FIXEDSYS | WS_CHILD | WS_DISABLED | WS_CAPTION;
	dlt.dwExtendedStyle = 0;
	dlt.cdit = 0;   // automatically updated by DialogBuilder
	dlt.x = 0; dlt.y = 0;
	dlt.cx = PROP_MED_CXDLG; dlt.cy = PROP_MED_CYDLG;
	// TODO: Localization.
	m_dlgBuilder.init(&dlt, L"ROM Properties");

	// Determine the maximum length of all field names.
	// Assume each character is an average of 4x8 DLUs.
	// TODO: Line breaks?
	// TODO: Calculate actual font metrics and convert to DLUs?
	size_t maxLen = 0;
	for (int i = 0; i < count; i++) {
		const RomFields::Desc *desc = fields->desc(i);
		const RomFields::Data *data = fields->data(i);
		if (!desc || !data)
			continue;
		if (desc->type != data->type)
			continue;
		if (!desc->name || desc->name[0] == '\0')
			continue;

		size_t len = LibRomData::rp_strlen(desc->name);
		if (len > maxLen)
			maxLen = len;
	}

	// Create the ROM field widgets.
	// Each static control is ((maxLen+1)*4) DLUs wide,
	// and 8 DLUs tall, plus 4 vertical DLUs for spacing.
	// NOTE: Not using SIZE because dialogs use short, not LONG.
	const struct { short cx, cy; } descSize = {((((short)maxLen) + 1) * 4), 8+4};
	// Current position.
	// 7x7 DLU margin is recommended by the Windows UX guidelines.
	// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
	// NOTE: Not using POINT because dialogs use short, not LONG.
	struct { short x, y; } curPt = {7, 7};

	DLGITEMTEMPLATE dlit;
	for (int i = 0; i < count; i++) {
		const RomFields::Desc *desc = fields->desc(i);
		const RomFields::Data *data = fields->data(i);
		if (!desc || !data)
			continue;
		if (desc->type != data->type)
			continue;
		if (!desc->name || desc->name[0] == '\0')
			continue;

		// Append a ":" to the description.
		// TODO: Localization.
		rp_string desc_text(desc->name);
		desc_text += _RP_CHR(':');

		// Create the static text widget. (FIXME: Disable mnemonics?)
		dlit.style = WS_CHILD | WS_VISIBLE | SS_LEFT;
		dlit.dwExtendedStyle = 0;
		dlit.x = curPt.x; dlit.y = curPt.y;
		dlit.cx = descSize.cx; dlit.cy = descSize.cy;
		dlit.id = IDC_STATIC;
		m_dlgBuilder.add(&dlit, WC_ORD_STATIC, desc_text);

		// TODO: Actual value widget.

		// Next row.
		curPt.y += descSize.cy;
	}
}


/** IShellPropSheetExt **/

IFACEMETHODIMP RP_ShellPropSheetExt::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7

	LPCDLGTEMPLATE pDlgTemplate = m_dlgBuilder.get();
	if (!pDlgTemplate) {
		// No property sheet is available.
		return E_FAIL;
	}

	// Create a property sheet page.
	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_DLGINDIRECT;
	psp.hInstance = nullptr;
	psp.pResource = pDlgTemplate;
	psp.pszIcon = nullptr;
	psp.pszTitle = nullptr;
	psp.pfnDlgProc = DlgProc;
	psp.pcRefParent = nullptr;
	psp.pfnCallback = CallbackProc;
	psp.lParam = reinterpret_cast<LPARAM>(this);

	HPROPSHEETPAGE hPage = CreatePropertySheetPage(&psp);
	if (hPage == NULL) {
		return E_OUTOFMEMORY;
	}

	// The property sheet page is then added to the property sheet by calling 
	// the callback function (LPFNADDPROPSHEETPAGE pfnAddPage) passed to 
	// IShellPropSheetExt::AddPages.
	if (pfnAddPage(hPage, lParam)) {
		// By default, after AddPages returns, the shell releases its 
		// IShellPropSheetExt interface and the property page cannot access the
		// extension object. However, it is sometimes desirable to be able to use 
		// the extension object, or some other object, from the property page. So 
		// we increase the reference count and maintain this object until the 
		// page is released in PropPageCallbackProc where we call Release upon 
		// the extension.
		this->AddRef();
	} else {
		DestroyPropertySheetPage(hPage);
		return E_FAIL;
	}

	// If any value other than S_OK is returned from the method, the property 
	// sheet is not displayed.
	return S_OK;
}

IFACEMETHODIMP RP_ShellPropSheetExt::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam)
{
	// Not used.
	((void)uPageID);
	((void)pfnReplaceWith);
	((void)lParam);
	return E_NOTIMPL;
}

/** Property sheet callback functions. **/

//
//   FUNCTION: FilePropPageDlgProc
//
//   PURPOSE: Processes messages for the property page.
//
INT_PTR CALLBACK RP_ShellPropSheetExt::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (pPage) {
				// Access the property sheet extension from property page.
				RP_ShellPropSheetExt *pExt = reinterpret_cast<RP_ShellPropSheetExt*>(pPage->lParam);
				if (pExt) {
					// Store the object pointer with this particular page dialog.
					SetProp(hWnd, EXT_POINTER_PROP, static_cast<HANDLE>(pExt));
				}
			}
			return TRUE;
		}

		case WM_COMMAND: {
			// TODO
#if 0
			switch (LOWORD(wParam)) {
				case IDC_CHANGEPROP_BUTTON:
				// User clicks the "Simulate Property Changing" button...

				// Simulate property changing. Inform the property sheet to 
				// enable the Apply button.
				SendMessage(GetParent(hWnd), PSM_CHANGED, 
					reinterpret_cast<WPARAM>(hWnd), 0);
				return TRUE;
			}
#endif
			break;
		}

		case WM_NOTIFY: {
			// TODO
#if 0
			switch ((reinterpret_cast<LPNMHDR>(lParam))->code) {
				case PSN_APPLY:
					// The PSN_APPLY notification code is sent to every page in the 
					// property sheet to indicate that the user has clicked the OK, 
					// Close, or Apply button and wants all changes to take effect. 

					// Get the property sheet extension object pointer that was 
					// stored in the page dialog (See the handling of WM_INITDIALOG 
					// in FilePropPageDlgProc).
					RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
						GetProp(hWnd, EXT_POINTER_PROP));

					// Access the property sheet extension object.
					// ...

					// Return PSNRET_NOERROR to allow the property dialog to close if 
					// the user clicked OK.
					SetWindowLong(hWnd, DWLP_MSGRESULT, PSNRET_NOERROR);

					return TRUE;
			}
#endif
			break;
		}

		case WM_DESTROY: {
			// Remove the EXT_POINTER_PROP property from the page. 
			// The EXT_POINTER_PROP property stored the pointer to the 
			// FilePropSheetExt object.
			RemoveProp(hWnd, EXT_POINTER_PROP);
			return TRUE;
		}

		default:
			break;
	}

	return FALSE; // Let system deal with other messages
}


//
//   FUNCTION: FilePropPageCallbackProc
//
//   PURPOSE: Specifies an application-defined callback function that a property
//            sheet calls when a page is created and when it is about to be 
//            destroyed. An application can use this function to perform 
//            initialization and cleanup operations for the page.
//
UINT CALLBACK RP_ShellPropSheetExt::CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	switch (uMsg) {
		case PSPCB_CREATE: {
			// Must return TRUE to enable the page to be created.
			return TRUE;
		}

		case PSPCB_RELEASE: {
			// When the callback function receives the PSPCB_RELEASE notification, 
			// the ppsp parameter of the PropSheetPageProc contains a pointer to 
			// the PROPSHEETPAGE structure. The lParam member of the PROPSHEETPAGE 
			// structure contains the extension pointer which can be used to 
			// release the object.

			// Release the property sheet extension object. This is called even 
			// if the property page was never actually displayed.
			RP_ShellPropSheetExt *pExt = reinterpret_cast<RP_ShellPropSheetExt*>(ppsp->lParam);
			if (pExt) {
				pExt->Release();
			}
			break;
		}

		default:
			break;
	}

	return FALSE;
}
