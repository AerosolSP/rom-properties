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

#ifndef __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_HPP__
#define __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_HPP__

// References:
// - http://www.codeproject.com/Articles/338268/COM-in-C
// - https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
// - https://msdn.microsoft.com/en-us/library/ms677109(v=vs.85).aspx

#include "RP_ComBase.hpp"
#include "libromdata/config.libromdata.h"

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_ShellPropSheetExt;
}

// C++ includes.
#include <string>

class UUID_ATTR("{2443C158-DF7C-4352-B435-BC9F885FFD52}")
RP_ShellPropSheetExt : public RP_ComBase2<IShellExtInit, IShellPropSheetExt>
{
	public:
		RP_ShellPropSheetExt();
	private:
		typedef RP_ComBase2<IShellExtInit, IShellPropSheetExt> super;

	public:
		// IUnknown
		STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj) override;

	public:
		/**
		 * Register the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG Register(void);

		/**
		 * Unregister the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG Unregister(void);

 	protected:
		// Information from IShellExtInit.
		PCIDLIST_ABSOLUTE m_pidlFolder;
		IDataObject *m_pdtobj;
		HKEY m_hkeyProgID;

		// Selected file.
		wchar_t m_szSelectedFile[MAX_PATH];
		PCWSTR GetSelectedFile(void) const;

		// Indirect dialog template memory buffer.
		// NOTE: Cannot be read-only, and cannot be
		// temporarily allocated on the stack, since
		// the system doesn't copy the template.
		uint8_t m_dlgbuf[32768];

	public:
		// IShellExtInit
		IFACEMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID);

		// IShellPropSheetExt
		IFACEMETHODIMP AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
		IFACEMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam);

		// Property sheet callback functions.
		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static UINT CALLBACK CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ExtractImage, 0x84573bc0, 0x9502, 0x42f8, 0x80, 0x66, 0xcc, 0x52, 0x7d, 0x07, 0x79, 0xe5)
#endif

#endif /* __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_HPP__ */
