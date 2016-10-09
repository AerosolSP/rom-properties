/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IStreamWrapper.hpp: IStream wrapper for IRpFile. (Win32)                *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_FILE_WIN32_ISTREAMWRAPPER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_FILE_WIN32_ISTREAMWRAPPER_HPP__

#ifndef _WIN32
#error IStreamWrapper.hpp is Windows only.
#endif

#include "../IRpFile.hpp"
#include "../../RpWin32.hpp"
#include <objidl.h>

namespace LibRomData {

class IStreamWrapper : public IStream
{
	public:
		/**
		 * Create an IStream wrapper for IRpFile.
		 * The IRpFile is dup()'d.
		 * @param file IRpFile.
		 */
		IStreamWrapper(IRpFile *file);
		virtual ~IStreamWrapper();

	private:
		typedef IStream super;
		IStreamWrapper(const IStreamWrapper &other);
		IStreamWrapper &operator=(const IStreamWrapper &other);

	public:
		/**
		 * Get the IRpFile.
		 * NOTE: The IRpFile is still owned by this object.
		 * @return IRpFile.
		 */
		IRpFile *file(void) const;

		/**
		 * Set the IRpFile.
		 * @param file New IRpFile.
		 */
		void setFile(IRpFile *file);

	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) final;
		IFACEMETHODIMP_(ULONG) AddRef(void) final;
		IFACEMETHODIMP_(ULONG) Release(void) final;

		// ISequentialStream
		IFACEMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead) final;
		IFACEMETHODIMP Write(const void *pv, ULONG cb, ULONG *pcbWritten) final;

		// IStream
		IFACEMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition) final;
		IFACEMETHODIMP SetSize(ULARGE_INTEGER libNewSize) final;
		IFACEMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten) final;
		IFACEMETHODIMP Commit(DWORD grfCommitFlags) final;
		IFACEMETHODIMP Revert(void) final;
		IFACEMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) final;
		IFACEMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) final;
		IFACEMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag) final;
		IFACEMETHODIMP Clone(IStream **ppstm) final;

	private:
		/* References of this object. */
		volatile ULONG m_ulRefCount;

	protected:
		IRpFile *m_file;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_FILE_WIN32_ISTREAMWRAPPER_HPP__ */
