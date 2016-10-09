/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpFile_Win32.cpp: Standard file object. (Win32 implementation)          *
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

#include "../RpFile.hpp"
#include "TextFuncs.hpp"
#include "RpWin32.hpp"

// C includes. (C++ namespace)
#include <cctype>

// C++ includes.
#include <string>
using std::string;
using std::wstring;

// Define this symbol to get XP themes. See:
// http://msdn.microsoft.com/library/en-us/dnwxp/html/xptheming.asp
// for more info. Note that as of May 2006, the page says the symbols should
// be called "SIDEBYSIDE_COMMONCONTROLS" but the headers in my SDKs in VC 6 & 7
// don't reference that symbol. If ISOLATION_AWARE_ENABLED doesn't work for you,
// try changing it to SIDEBYSIDE_COMMONCONTROLS
#define ISOLATION_AWARE_ENABLED 1

#include <windows.h>

namespace LibRomData {

// Deleter for std::unique_ptr<void> m_file.
struct myFile_deleter {
	void operator()(void *p) const {
		if (p != nullptr && p != INVALID_HANDLE_VALUE) {
			CloseHandle(p);
		}
	}
};

static inline int mode_to_win32(RpFile::FileMode mode, DWORD *pdwDesiredAccess, DWORD *pdwCreationDisposition)
{
	switch (mode) {
		case RpFile::FM_OPEN_READ:
			*pdwDesiredAccess = GENERIC_READ;
			*pdwCreationDisposition = OPEN_EXISTING;
			break;
		case RpFile::FM_OPEN_WRITE:
			*pdwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
			*pdwCreationDisposition = OPEN_EXISTING;
			break;
		case RpFile::FM_CREATE|RpFile::FM_READ /*RpFile::FM_CREATE_READ*/ :
		case RpFile::FM_CREATE_WRITE:
			*pdwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
			*pdwCreationDisposition = CREATE_ALWAYS;
			break;
		default:
			// Invalid mode.
			return -1;
	}

	// Mode converted successfully.
	return 0;
}

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename.
 * @param mode File mode.
 */
RpFile::RpFile(const rp_char *filename, FileMode mode)
	: super()
	, m_file(INVALID_HANDLE_VALUE, myFile_deleter())
	, m_mode(mode)
	, m_lastError(0)
{
	init(filename);
}

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename.
 * @param mode File mode.
 */
RpFile::RpFile(const rp_string &filename, FileMode mode)
	: IRpFile()
	, m_file(INVALID_HANDLE_VALUE, myFile_deleter())
	, m_mode(mode)
	, m_lastError(0)
{
	init(filename.c_str());
}

/**
 * Common initialization function for RpFile's constructors.
 * @param filename Filename.
 */
void RpFile::init(const rp_char *filename)
{
	// Determine the file mode.
	DWORD dwDesiredAccess, dwCreationDisposition;
	if (mode_to_win32(m_mode, &dwDesiredAccess, &dwCreationDisposition) != 0) {
		// Invalid mode.
		m_lastError = EINVAL;
		return;
	}

	// If this is an absolute path, make sure it starts with
	// "\\?\" in order to support filenames longer than MAX_PATH.
	wstring filenameW;
	if (iswascii(filename[0]) && iswalpha(filename[0]) &&
	    filename[1] == _RP_CHR(':') && filename[2] == _RP_CHR('\\'))
	{
		// Absolute path. Prepend "\\?\" to the path.
		filenameW = L"\\\\?\\";
		filenameW += RP2W_c(filename);
	} else {
		// Not an absolute path, or "\\?\" is already
		// prepended. Use it as-is.
		filenameW = RP2W_c(filename);
	}

	// Open the file.
	m_file.reset(CreateFile(filenameW.c_str(),
			dwDesiredAccess, FILE_SHARE_READ, nullptr,
			dwCreationDisposition, FILE_ATTRIBUTE_NORMAL,
			nullptr), myFile_deleter());
	if (!m_file || m_file.get() == INVALID_HANDLE_VALUE) {
		// Error opening the file.
		// TODO: More extensive conversion of GetLastError() to POSIX?
		DWORD dwError = GetLastError();
		m_lastError = (dwError == ERROR_FILE_NOT_FOUND ? ENOENT : EIO);
		return;
	}
}

RpFile::~RpFile()
{ }

/**
 * Copy constructor.
 * @param other Other instance.
 */
RpFile::RpFile(const RpFile &other)
	: IRpFile()
	, m_file(other.m_file)
	, m_mode(other.m_mode)
	, m_lastError(0)
{ }

/**
 * Assignment operator.
 * @param other Other instance.
 * @return This instance.
 */
RpFile &RpFile::operator=(const RpFile &other)
{
	m_file = other.m_file;
	m_mode = other.m_mode;
	m_lastError = other.m_lastError;
	return *this;
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpFile::isOpen(void) const
{
	return (m_file && m_file.get() != INVALID_HANDLE_VALUE);
}

/**
 * Get the last error.
 * @return Last POSIX error, or 0 if no error.
 */
int RpFile::lastError(void) const
{
	return m_lastError;
}

/**
 * Clear the last error.
 */
void RpFile::clearError(void)
{
	m_lastError = 0;
}

/**
 * dup() the file handle.
 * Needed because IRpFile* objects are typically
 * pointers, not actual instances of the object.
 * @return dup()'d file, or nullptr on error.
 */
IRpFile *RpFile::dup(void)
{
	return new RpFile(*this);
}

/**
 * Close the file.
 */
void RpFile::close(void)
{
	m_file.reset(INVALID_HANDLE_VALUE, myFile_deleter());
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpFile::read(void *ptr, size_t size)
{
	if (!m_file || m_file.get() == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return 0;
	}

	DWORD bytesRead;
	BOOL bRet = ReadFile(m_file.get(), ptr, (DWORD)size, &bytesRead, nullptr);
	if (bRet == 0) {
		// An error occurred.
		// TODO: Convert GetLastError() to POSIX?
		m_lastError = EIO;
		return 0;
	}

	return bytesRead;
}

/**
 * Write data to the file.
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t RpFile::write(const void *ptr, size_t size)
{
	if (!m_file || m_file.get() == INVALID_HANDLE_VALUE || !(m_mode & FM_WRITE)) {
		// Either the file isn't open,
		// or it's read-only.
		m_lastError = EBADF;
		return 0;
	}

	DWORD bytesWritten;
	BOOL bRet = WriteFile(m_file.get(), ptr, (DWORD)size, &bytesWritten, nullptr);
	if (bRet == 0) {
		// An error occurred.
		// TODO: Convert GetLastError() to POSIX?
		m_lastError = EIO;
		return 0;
	}

	return bytesWritten;
}

/**
 * Set the file position.
 * @param pos File position.
 * @return 0 on success; -1 on error.
 */
int RpFile::seek(int64_t pos)
{
	if (!m_file || m_file.get() == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return -1;
	}

	LARGE_INTEGER liSeekPos;
	liSeekPos.QuadPart = pos;
	BOOL bRet = SetFilePointerEx(m_file.get(), liSeekPos, nullptr, FILE_BEGIN);
	if (bRet == 0) {
		// TODO: Convert GetLastError() to POSIX?
		m_lastError = EIO;
	}
	return (bRet == 0 ? -1 : 0);
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
int64_t RpFile::tell(void)
{
	if (!m_file || m_file.get() == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return -1;
	}

	LARGE_INTEGER liSeekPos, liSeekRet;
	liSeekPos.QuadPart = 0;
	BOOL bRet = SetFilePointerEx(m_file.get(), liSeekPos, &liSeekRet, FILE_CURRENT);
	if (bRet == 0) {
		// TODO: Convert GetLastError() to POSIX?
		m_lastError = EIO;
	}
	return (bRet == 0 ? -1 : liSeekRet.QuadPart);
}

/**
 * Seek to the beginning of the file.
 */
void RpFile::rewind(void)
{
	if (!m_file || m_file.get() == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return;
	}

	LARGE_INTEGER liSeekPos;
	liSeekPos.QuadPart = 0;
	SetFilePointerEx(m_file.get(), liSeekPos, nullptr, FILE_BEGIN);
}

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
int64_t RpFile::fileSize(void)
{
	if (!m_file || m_file.get() == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return -1;
	}

	LARGE_INTEGER liFileSize;
	BOOL bRet = GetFileSizeEx(m_file.get(), &liFileSize);
	return (bRet != 0 ? liFileSize.QuadPart : -1);
}

}
