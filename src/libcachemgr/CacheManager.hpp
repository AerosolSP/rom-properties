/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * CacheManager.hpp: Local cache manager.                                  *
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

#ifndef __ROMPROPERTIES_LIBCACHEMGR_CACHEMANAGER_HPP__
#define __ROMPROPERTIES_LIBCACHEMGR_CACHEMANAGER_HPP__

#include "libromdata/config.libromdata.h"

namespace LibCacheMgr {

class IDownloader;
class CacheManager
{
	public:
		CacheManager();
		~CacheManager();

	private:
		CacheManager(const CacheManager &);
		CacheManager &operator=(const CacheManager &);

	public:
		/** Proxy server functions. **/
		// NOTE: This is only useful for downloaders that
		// can't retrieve the system proxy server normally.

		/**
		 * Get the proxy server.
		 * @return Proxy server URL.
		 */
		LibRomData::rp_string proxyUrl(void) const;

		/**
		 * Set the proxy server.
		 * @param proxyUrl Proxy server URL. (Use nullptr or blank string for default settings.)
		 */
		void setProxyUrl(const rp_char *proxyUrl);

		/**
		 * Set the proxy server.
		 * @param proxyUrl Proxy server URL. (Use blank string for default settings.)
		 */
		void setProxyUrl(const LibRomData::rp_string &proxyUrl);

	protected:
		/**
		 * Get the ROM Properties cache directory.
		 * @return Cache directory.
		 */
		const LibRomData::rp_string &cacheDir(void);

		/**
		 * Get a cache filename.
		 * @param cache_key Cache key.
		 * @return Cache filename, or empty string on error.
		 */
		LibRomData::rp_string getCacheFilename(const LibRomData::rp_string &cache_key);

	public:
		/**
		 * Download a file.
		 * @param url URL.
		 * @param cache_key Cache key.
		 * @param cache_key_fb Fallback cache key.
		 *
		 * If the file is present in the cache, the cached version
		 * will be retrieved. Otherwise, the file will be downloaded.
		 *
		 * If the file was not found on the server, or it was not found
		 * the last time it was requested, an empty string will be
		 * returned, and a zero-byte file will be stored in the cache.
		 *
		 * NOTE: If a fallback cache key is specified, this is checked
		 * if the main cache key is missing. If not found, then a regular
		 * download will be done.
		 *
		 * @return Absolute path to cached file.
		 */
		LibRomData::rp_string download(const LibRomData::rp_string &url,
				const LibRomData::rp_string &cache_key,
				const LibRomData::rp_string &cache_key_fb = LibRomData::rp_string());

	protected:
		LibRomData::rp_string m_proxyUrl;
		LibRomData::rp_string m_cacheDir;

		IDownloader *m_downloader;
};

}

#endif /* __ROMPROPERTIES_LIBCACHEMGR_CACHEMANAGER_HPP__ */
