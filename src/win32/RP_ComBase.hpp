/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ComBase.hpp: Base class for COM objects.                             *
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

#ifndef __ROMPROPERTIES_WIN32_RP_COMBASE_HPP__
#define __ROMPROPERTIES_WIN32_RP_COMBASE_HPP__

/**
 * COM base class. Handles reference counting and IUnknown.
 * References:
 * - http://www.codeproject.com/Articles/665/A-very-simple-COM-server-without-ATL-or-MFC
 * - http://www.codeproject.com/Articles/338268/COM-in-C
 * - http://stackoverflow.com/questions/17310733/how-do-i-re-use-an-interface-implementation-in-many-classes
 */

#include "libromdata/RpWin32.hpp"

// References of all objects.
extern volatile ULONG RP_ulTotalRefCount;

/**
 * Is an RP_ComBase object referenced?
 * @return True if RP_ulTotalRefCount > 0; false if not.
 */
static inline bool RP_ComBase_isReferenced(void)
{
	return (RP_ulTotalRefCount > 0);
}

#define RP_COMBASE_IMPL(name) \
{ \
	protected: \
		/* References of this object. */ \
		volatile ULONG m_ulRefCount; \
	\
	public: \
		name() : m_ulRefCount(1) \
		{ \
			InterlockedIncrement(&RP_ulTotalRefCount); \
		} \
		virtual ~name() { } \
	\
	public: \
		/** IUnknown **/ \
		IFACEMETHODIMP_(ULONG) AddRef(void) final \
		{ \
			InterlockedIncrement(&RP_ulTotalRefCount); \
			InterlockedIncrement(&m_ulRefCount); \
			return m_ulRefCount; \
		} \
		\
		IFACEMETHODIMP_(ULONG) Release(void) final \
		{ \
			ULONG ulRefCount = InterlockedDecrement(&m_ulRefCount); \
			if (ulRefCount == 0) { \
				/* No more references. */ \
				delete this; \
			} \
			\
			InterlockedDecrement(&RP_ulTotalRefCount); \
			return ulRefCount; \
		} \
}

template<class I>
class RP_ComBase : public I
	RP_COMBASE_IMPL(RP_ComBase);

template<class I1, class I2>
class RP_ComBase2 : public I1, public I2
	RP_COMBASE_IMPL(RP_ComBase2);

#endif /* __ROMPROPERTIES_WIN32_RP_COMBASE_HPP__ */
