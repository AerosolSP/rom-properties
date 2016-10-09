/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * gtest_init.c: Google Test initialization.                               *
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

// C++ includes.
#include <locale>
using std::locale;

#ifdef _WIN32
#include <windows.h>

// DEP policy. (requires _WIN32_WINNT >= 0x0600)
#ifndef PROCESS_DEP_ENABLE
#define PROCESS_DEP_ENABLE 0x1
#endif
#ifndef PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION
#define PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION 0x2
#endif

// rp_image backend registration.
#include "libromdata/img/RpGdiplusBackend.hpp"
#include "libromdata/img/rp_image.hpp"
using LibRomData::RpGdiplusBackend;
using LibRomData::rp_image;

/**
 * Enable extra security options.
 * Reference: http://msdn.microsoft.com/en-us/library/bb430720.aspx
 * @return 0 on success; non-zero on error.
 */
static int Win32_SetSecurityOptions(void)
{
	HMODULE hKernel32 = LoadLibrary(L"kernel32.dll");
	if (!hKernel32)
		return -1;

	// Enable DEP/NX. (WinXP SP3, Vista, and later.)
	// NOTE: DEP/NX should be specified in the PE header
	// using ld's --nxcompat, but we'll set it manually here,
	// just in case the linker doesn't support it.
	typedef BOOL (WINAPI *PFNSETDEP)(DWORD dwFlags);
	PFNSETDEP pfnSetDep = (PFNSETDEP)GetProcAddress(hKernel32, "SetProcessDEPPolicy");
	if (pfnSetDep)
		pfnSetDep(PROCESS_DEP_ENABLE);

	// Remove the current directory from the DLL search path.
	typedef BOOL (WINAPI *PFNSETDLLDIRW)(LPCWSTR lpPathName);
	PFNSETDLLDIRW pfnSetDllDirectoryW = (PFNSETDLLDIRW)GetProcAddress(hKernel32, "SetDllDirectoryW");
	if (pfnSetDllDirectoryW)
		pfnSetDllDirectoryW(L"");

	// Terminate the process if heap corruption is detected.
	// NOTE: Parameter 2 is usually type enum HEAP_INFORMATION_CLASS,
	// but this type isn't present in older versions of MinGW, so we're
	// using int instead.
	typedef BOOL (WINAPI *PFNHSI)
		(HANDLE HeapHandle, int HeapInformationClass,
		 PVOID HeapInformation, SIZE_T HeapInformationLength);
	PFNHSI pfnHeapSetInformation = (PFNHSI)GetProcAddress(hKernel32, "HeapSetInformation");
	if (pfnHeapSetInformation) {
		// HeapEnableTerminationOnCorruption == 1
		pfnHeapSetInformation(nullptr, 1, nullptr, 0);
	}

	FreeLibrary(hKernel32);
	return 0;
}

#endif /* _WIN32 */

extern "C" int gtest_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
#ifdef _WIN32
	// Set Win32 security options.
	Win32_SetSecurityOptions();

	// Register RpGdiplusBackend.
	// TODO: Static initializer somewhere?
	rp_image::setBackendCreatorFn(RpGdiplusBackend::creator_fn);
#endif

	// Set the C and C++ locales.
	locale::global(locale(""));

	// Call the actual main function.
	return gtest_main(argc, argv);
}
