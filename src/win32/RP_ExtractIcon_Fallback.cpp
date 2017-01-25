/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon_Fallback.cpp: IExtractIcon implementation.               *
 * Fallback functions for unsupported files.                               *
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
#include "RP_ExtractIcon.hpp"
#include "RP_ExtractIcon_p.hpp"

#include "RegKey.hpp"

// C++ includes.
#include <string>
using std::wstring;

/**
 * Fallback icon handler function. (internal)
 * @param hkey_Assoc File association key to check.
 * @param phiconLarge Large icon.
 * @param phiconSmall Small icon.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon_Private::Fallback_int(RegKey &hkey_Assoc, HICON *phiconLarge, HICON *phiconSmall)
{
	// Is RP_Fallback present?
	RegKey hkey_RP_Fallback(hkey_Assoc, L"RP_Fallback", KEY_READ, false);
	if (!hkey_RP_Fallback.isOpen()) {
		return hkey_RP_Fallback.lOpenRes();
	}

	// Get the DefaultIcon key.
	DWORD dwType;
	wstring defaultIcon = hkey_RP_Fallback.read(L"DefaultIcon", &dwType);
	if (defaultIcon.empty()) {
		// No default icon.
		return ERROR_FILE_NOT_FOUND;
	} else if (defaultIcon == L"%1") {
		// TODO: Forward to another COM object.
		return ERROR_FILE_NOT_FOUND;
	}

	// DefaultIcon format: "C:\\Windows\\Some.DLL,1"
	// TODO: Can the filename be quoted?
	// TODO: Better error codes?
	int nIconIndex;
	size_t comma = defaultIcon.find_last_of(L',');
	if (comma != wstring::npos) {
		// Found the comma.
		if (comma > 0 && comma < defaultIcon.size()-1) {
			wchar_t *str_end;
			errno = 0;
			nIconIndex = (int)wcstol(&defaultIcon[comma+1], &str_end, 10);
			if (errno == ERANGE || *str_end != 0) {
				// strtol() failed.
				// DefaultIcon is invalid.
				return ERROR_FILE_NOT_FOUND;
			}
		} else {
			// Comma is the last character.
			return ERROR_FILE_NOT_FOUND;
		}

		// Remove the comma portion.
		defaultIcon.resize(comma);
	} else {
		// Assume the default icon index.
		nIconIndex = 0;
	}

	// If the registry key type is REG_EXPAND_SZ, expand it.
	// TODO: Move to RegKey?
	if (dwType == REG_EXPAND_SZ) {
		// cchExpand includes the NULL terminator.
		DWORD cchExpand = ExpandEnvironmentStrings(defaultIcon.c_str(), nullptr, 0);
		if (cchExpand == 0) {
			// Error expanding the strings.
			return GetLastError();
		}

		wchar_t *wbuf = static_cast<wchar_t*>(malloc(cchExpand*sizeof(wchar_t)));
		cchExpand = ExpandEnvironmentStrings(defaultIcon.c_str(), wbuf, cchExpand);
		defaultIcon = wstring(wbuf, cchExpand-1);
		free(wbuf);
	}

	// Extract the icon.
	UINT uResult = ExtractIconEx(defaultIcon.c_str(), nIconIndex, phiconLarge, phiconSmall, 1);
	if (uResult == 1 || uResult == 2) {
		// Either the large, small, or both icons
		// have been extracted.
		return ERROR_SUCCESS;
	}
	// No icons extracted.
	return ERROR_FILE_NOT_FOUND;
}

/**
 * Fallback icon handler function.
 * @param phiconLarge Large icon.
 * @param phiconSmall Small icon.
 * @return ERROR_SUCCESS on success; Win32 error code on error.
 */
LONG RP_ExtractIcon_Private::Fallback(HICON *phiconLarge, HICON *phiconSmall)
{
	// TODO: Check HKCU first.

	// Get the file extension.
	if (filename.empty()) {
		return ERROR_FILE_NOT_FOUND;
	}
	size_t dotpos = filename.find_last_of(L'.');
	size_t bslashpos = filename.find_last_of(L'\\');
	if (dotpos == wstring::npos ||
	    dotpos >= filename.size()-1 ||
	    dotpos <= bslashpos)
	{
		// Invalid or missing file extension.
		return ERROR_FILE_NOT_FOUND;
	}

	// Open the filetype key in HKCR.
	RegKey hkey_Assoc(HKEY_CLASSES_ROOT, RP2W_c(&filename[dotpos]), KEY_READ, false);
	if (!hkey_Assoc.isOpen()) {
		return hkey_Assoc.lOpenRes();
	}

	// If we have a ProgID, check it first.
	wstring progID = hkey_Assoc.read(nullptr);
	if (!progID.empty()) {
		// Custom ProgID is registered.
		// TODO: Get the correct top-level registry key.
		RegKey hkcr_ProgID(HKEY_CLASSES_ROOT, progID.c_str(), KEY_READ, false);
		if (hkcr_ProgID.isOpen()) {
			LONG lResult = Fallback_int(hkcr_ProgID, phiconLarge, phiconSmall);
			if (lResult == ERROR_SUCCESS) {
				// ProgID icon extracted.
				return lResult;
			}
		}
	}

	// Extract the icon from the filetype key.
	return Fallback_int(hkey_Assoc, phiconLarge, phiconSmall);
}
