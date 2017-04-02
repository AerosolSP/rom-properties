/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * KeyManager.cpp: Encryption key manager.                                 *
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

#include "KeyManager.hpp"

#include "RpAtomics.h"
#include "file/FileSystem.hpp"
#include "file/RpFile.hpp"

#include "IAesCipher.hpp"
#include "AesCipherFactory.hpp"

#ifdef _WIN32
// windows.h is needed for Sleep()
#include "RpWin32.hpp"
#endif /* _WIN32 */

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>

// C++ includes.
#include <memory>
#include <string>
#include <unordered_map>
using std::string;
using std::unique_ptr;
using std::unordered_map;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "../uvector.h"

namespace LibRomData {

class KeyManagerPrivate
{
	public:
		KeyManagerPrivate();

	private:
		RP_DISABLE_COPY(KeyManagerPrivate)

	public:
		// Static KeyManager instance.
		static KeyManager instance;

	public:
		// Encryption key data.
		// Managed as a single block in order to reduce
		// memory allocations.
		ao::uvector<uint8_t> vKeys;

		/**
		 * Map of key names to vKeys indexes.
		 * - Key: Key name.
		 * - Value: vKeys information.
		 *   - High byte: Key length.
		 *   - Low 3 bytes: Key index.
		 */
		unordered_map<string, uint32_t> mapKeyNames;

		/**
		 * Initialize KeyManager.
		 */
		void init(void);

		/**
		 * Process a configuration line.
		 * @param line_buf Configuration line.
		 */
		void processConfigLine(const string &line_buf);

		/**
		 * Load keys from the configuration file.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadKeys(void);

		// Initialization counter.
		int init_counter;
		volatile int is_init;

		// Temporary configuration loading variables.
		string cfg_curSection;
		bool cfg_isInKeysSection;

		// keys.conf status.
		rp_string conf_filename;
		bool conf_was_found;
		time_t conf_mtime;
};

/** KeyManagerPrivate **/

// Verification test string.
// NOTE: This string is NOT NULL-terminated!
const char KeyManager::verifyTestString[] = {
	'A','E','S','-','1','2','8','-',
	'E','C','B','-','T','E','S','T'
};

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
KeyManager KeyManagerPrivate::instance;

KeyManagerPrivate::KeyManagerPrivate()
	: init_counter(-1)
	, is_init(0)
	, cfg_isInKeysSection(false)
	, conf_was_found(false)
	, conf_mtime(0)
{ }

/**
 * Initialize KeyManager.
 */
void KeyManagerPrivate::init(void)
{
	// How this works:
	// - init_counter is initially -1.
	// - Incrementing it returns 0; this means that the
	//   directories have not been initialized yet.
	// - is_init is set when initializing.
	// - If the counter wraps around, the directories won't be
	//   reinitialized because dir_is_init will be set.
	if (ATOMIC_INC_FETCH(&init_counter) != 0) {
		// Function has already been called.
		// Wait for directories to be initialized.
		while (ATOMIC_OR_FETCH(&is_init, 0) == 0) {
			// TODO: Timeout counter?
#ifdef _WIN32
			Sleep(0);
#else /* !_WIN32 */
			usleep(0);
#endif /* _WIN32 */
		}
		return;
	}

	// Reserve 1 KB for the key store.
	vKeys.reserve(1024);

	// Configuration filename.
	conf_filename = FileSystem::getConfigDirectory();
	if (!conf_filename.empty()) {
		if (conf_filename.at(conf_filename.size()-1) != _RP_CHR(DIR_SEP_CHR)) {
			conf_filename += _RP_CHR(DIR_SEP_CHR);
		}
		conf_filename += _RP("keys.conf");
	}

	// Make sure the configuration directory exists.
	// NOTE: The filename portion MUST be kept in config_path,
	// since the last component is ignored by rmkdir().
	int ret = FileSystem::rmkdir(conf_filename);
	if (ret != 0) {
		// rmkdir() failed.
		conf_filename.clear();
	}

	// KeyManager has been initialized.
	is_init = 1;
}

/**
 * Process a configuration line.
 * @param line_buf Configuration line.
 */
void KeyManagerPrivate::processConfigLine(const string &line_buf)
{
	bool foundNonSpace = false;
	const char *sect_start = nullptr;
	const char *equals_pos = nullptr;

	const char *chr = line_buf.data();
	for (int i = 0; i < (int)line_buf.size(); i++, chr++) {
		if (!foundNonSpace) {
			// Check if the current character is still a space.
			if (isspace(*chr)) {
				// Space character.
				continue;
			} else if (*chr == '[') {
				// Start of section.
				sect_start = chr+1;
				foundNonSpace = true;
			} else if (*chr == '=' || *chr == ';' || *chr == '#') {
				// Equals with no key name, or comment line.
				// Skip this line.
				return;
			} else {
				// Regular key line.
				foundNonSpace = true;
			}
		} else {
			if (sect_start != nullptr) {
				// Section header.
				if (*chr == ';' || *chr == '#') {
					// Comment. Skip this line.
					return;
				} else if (*chr == ']') {
					// End of section header.
					if (sect_start == chr) {
						// Empty section header.
						return;
					}

					// Check the section.
					cfg_curSection = string(sect_start, chr - sect_start);
					if (!strncasecmp(cfg_curSection.data(), "Keys", cfg_curSection.size())) {
						// This is the "Keys" section.
						cfg_isInKeysSection = true;
					} else {
						// This is not the "Keys" section.
						cfg_isInKeysSection = false;
					}

					// Skip the rest of the line.
					return;
				}
			} else {
				// Key name/value.
				if (!equals_pos) {
					// We haven't found the equals symbol yet.
					if (*chr == ';' || *chr == '#') {
						// Comment. Skip this line.
						return;
					} else if (*chr == '=') {
						// Found the equals symbol.
						equals_pos = chr;
					}
				} else {
					// We found the equals symbol.
					if (*chr == ';' || *chr == '#') {
						// Comment. Skip the rest of the line.
						break;
					}
				}
			}
		}
	}

	// Parse the key and value.
	if (!equals_pos) {
		// No equals sign.
		return;
	}

	const char *end = line_buf.data() + line_buf.size();
	if (chr > end) {
		// Out of bounds. Assume the end of the line.
		chr = end - 1;
	}

	const char *value = equals_pos + 1;
	int value_len = (int)(end - 1 - equals_pos);
	if (value[0] == 0 || (value_len % 2 != 0)) {
		// Key is either empty, or is an odd length.
		// (Odd length means half a byte...)
		return;
	}

	string keyName(line_buf.data(), equals_pos - line_buf.data());
	if (keyName.empty()) {
		// Empty key name.
		return;
	}

	// Parse the value.
	unsigned int vKeys_start_pos = (unsigned int)vKeys.size();
	unsigned int vKeys_pos = vKeys_start_pos;
	// Reserve space for half of the key string.
	// Key string is ASCII hex, so two characters make up one byte.
	vKeys.resize(vKeys.size() + (value_len / 2));

	// ASCII to HEX lookup table.
	// Reference: http://codereview.stackexchange.com/questions/22757/char-hex-string-to-byte-array
	const uint8_t ascii_to_hex[0x100] = {
		//0     1     2     3     4     5     6    7      8     9     A     B     C     D     E     F
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//0
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//1
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//2
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//3
		0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//4
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//5
		0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//6
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//7

		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//8
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//9
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//A
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//B
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//C
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//D
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//E
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF //F
	};

	for (; value_len > 0; value_len -= 2, vKeys_pos++, value += 2) {
		// Process two characters at a time.
		// Two hexadecimal digits == one byte.
		char chr0 = ascii_to_hex[(uint8_t)value[0]];
		char chr1 = ascii_to_hex[(uint8_t)value[1]];
		if (chr0 > 0x0F || chr1 > 0x0F) {
			// Invalid character.
			vKeys.resize(vKeys_start_pos);
		}

		vKeys[vKeys_pos] = (chr0 << 4 | chr1);
	}

	// Value parsed successfully.
	uint32_t keyIdx = vKeys_start_pos;
	uint8_t len = (uint8_t)(vKeys_pos - vKeys_start_pos);
	keyIdx |= (len << 24);
	mapKeyNames.insert(std::make_pair(keyName, keyIdx));
}

/**
 * Load keys from the configuration file.
 * @return 0 on success; negative POSIX error code on error.
 */
int KeyManagerPrivate::loadKeys(void)
{
	// Open the configuration file.
	if (conf_filename.empty()) {
		// Configuration directory is invalid...
		return -ENOENT;
	}
	unique_ptr<IRpFile> file(new RpFile(conf_filename, RpFile::FM_OPEN_READ));
	if (!file || !file->isOpen()) {
		// Error opening the file.
		return -EIO;
	}

	// Clear the loaded keys.
	vKeys.clear();
	mapKeyNames.clear();

	// We're not in the keys section initially.
	cfg_curSection.clear();
	cfg_isInKeysSection = false;

	// Parse the file.
	// We're looking for the "[Keys]" section.
	string line_buf;
	static const int LINE_LENGTH_MAX = 256;
	line_buf.reserve(LINE_LENGTH_MAX);
	size_t sz;
	bool skipLine = false;
	do {
		char buf[4096];
		sz = file->read(buf, sizeof(buf));
		if (sz == 0)
			break;

		int lastStartPos = 0;
		for (int pos = 0; pos < (int)sz; pos++) {
			// Find the first '\r' or '\n', starting at pos.
			if (buf[pos] == '\r' || buf[pos] == '\n') {
				// Found a newline.
				if (lastStartPos == pos || skipLine) {
					// Empty line, or the length limit has been exceeded.
					// Continue to the next line.
					// Next line starts at the next character.
					lastStartPos = pos + 1;
					skipLine = false;
					continue;
				}

				const int data_size = (pos - lastStartPos);

				// Add the string from lastStartPos up to the previous
				// character to the line buffer.
				if (line_buf.size() + data_size > LINE_LENGTH_MAX) {
					// Line length limit exceeded.
					line_buf.clear();
					// Next line starts at the next character.
					lastStartPos = pos + 1;
					continue;
				}

				line_buf.append(&buf[lastStartPos], data_size);
				if (!line_buf.empty()) {
					// Process the line.
					processConfigLine(line_buf);
					line_buf.clear();
				}

				// Next line starts at the next character.
				lastStartPos = pos + 1;
			}
		}

		// If anything is still left in buf[],
		// add it to the line buffer.
		if (!skipLine && lastStartPos < (int)sz) {
			const int data_size = ((int)sz - lastStartPos);
			if (line_buf.size() + data_size > LINE_LENGTH_MAX) {
				// Line length limit exceeded.
				skipLine = true;
				line_buf.clear();
			} else {
				line_buf.append(&buf[lastStartPos], data_size);
			}
		}
	} while (sz != 0);

	// If any data is left in the line buffer (possibly due
	// to a missing trailing newline), process it.
	if (!skipLine && !line_buf.empty()) {
		// Process the line.
		processConfigLine(line_buf);
	}

	// Keys loaded.
	return 0;
}

/** KeyManager **/

KeyManager::KeyManager()
	: d_ptr(new KeyManagerPrivate())
{ }

KeyManager::~KeyManager()
{
	delete d_ptr;
}

/**
 * Get the KeyManager instance.
 * @return KeyManager instance.
 */
KeyManager *KeyManager::instance(void)
{
	// Singleton instance.
	return &KeyManagerPrivate::instance;
}

/**
 * Have the encryption keys been loaded yet?
 *
 * This function will *not* load the keys.
 * To load the keys, call get() with the requested key name.
 *
 * If this function returns false after calling get(),
 * keys.conf is probably missing.
 *
 * @return True if keys have been loaded; false if not.
 */
bool KeyManager::areKeysLoaded(void) const
{
	RP_D(const KeyManager);
	return d->conf_was_found;
}

/**
 * Reload keys if the key configuration file has changed.
 * @return 0 on success; negative POSIX error code on error.
 */
int KeyManager::reloadIfChanged(void)
{
	// NOTE: It's safe to check dir_is_init here, since it's
	// only ever set to 1 by our code.
	RP_D(KeyManager);
	if (!d->is_init) {
		d->init();
	}

	int ret = 0;
	if (!d->conf_was_found) {
		// keys.conf wasn't found.
		// Try loading it again.
		ret = const_cast<KeyManagerPrivate*>(d)->loadKeys();
		if (ret == 0) {
			// Keys loaded.
			// Get the mtime.
			time_t mtime;
			ret = FileSystem::get_mtime(d->conf_filename, &mtime);
			if (ret == 0) {
				d->conf_mtime = mtime;
			} else {
				// mtime error...
				// TODO: What do we do here?
				d->conf_mtime = 0;
			}
			d->conf_was_found = true;
		}
	} else {
		// Check if the timestamp has changed.
		// NOTE: If get_mtime() failed, we'll leave everything
		// as it is instead of clearing the loaded keys.
		// TODO: Set a flag to ignore the previous mtime if
		// a new file is found in case the file is swapped
		// underneath us?
		time_t mtime;
		int ret = FileSystem::get_mtime(d->conf_filename, &mtime);
		if (ret == 0) {
			if (mtime != d->conf_mtime) {
				// Timestmap has changed.
				// Reload the keys.
				ret = const_cast<KeyManagerPrivate*>(d)->loadKeys();
				if (ret == 0) {
					// Keys loaded.
					d->conf_mtime = mtime;
				} else {
					// Key load failed.
					d->conf_was_found = false;
					// Existing keys are still OK.
					ret = 0;
				}
			}
		}
	}

	return ret;
}

/**
 * Get an encryption key.
 * @param keyName	[in]  Encryption key name.
 * @param pKeyData	[out] Key data struct.
 * @return VerifyResult.
 */
KeyManager::VerifyResult KeyManager::get(const char *keyName, KeyData_t *pKeyData) const
{
	assert(keyName != nullptr);
	assert(keyName[0] != 0);
	if (!keyName || keyName[0] == 0) {
		// Invalid parameters.
		return VERIFY_INVALID_PARAMS;
	}

	// Check if keys.conf needs to be reloaded.
	const_cast<KeyManager*>(this)->reloadIfChanged();
	if (!areKeysLoaded()) {
		// Keys are not loaded.
		return VERIFY_KEY_DB_NOT_LOADED;
	}

	// Attempt to get the key from the map.
	RP_D(const KeyManager);
	unordered_map<string, uint32_t>::const_iterator iter = d->mapKeyNames.find(keyName);
	if (iter == d->mapKeyNames.end()) {
		// Key not found.
		return VERIFY_KEY_NOT_FOUND;
	}

	// Found the key.
	const uint32_t keyIdx = iter->second;
	const uint32_t idx = (keyIdx & 0xFFFFFF);
	const uint8_t len = ((keyIdx >> 24) & 0xFF);

	// Make sure the key index is valid.
	assert(idx + len <= d->vKeys.size());
	if (idx + len > d->vKeys.size()) {
		// Should not happen...
		return VERIFY_KEY_DB_ERROR;
	}

	if (pKeyData) {
		pKeyData->key = d->vKeys.data() + idx;
		pKeyData->length = len;
	}
	return VERIFY_OK;
}

/**
 * Verify and retrieve an encryption key.
 *
 * This will decrypt the specified block of data
 * using the key with AES-128-ECB, which will result
 * in the 16-byte string "AES-128-ECB-TEST".
 *
 * If the key is valid, pKeyData will be populated
 * with the key information, similar to get().
 *
 * @param keyName	[in] Encryption key name.
 * @param pKeyData	[out] Key data struct.
 * @param pVerifyData	[in] Verification data block.
 * @param verifyLen	[in] Length of pVerifyData. (Must be 16.)
 * @return VerifyResult.
 */
KeyManager::VerifyResult KeyManager::getAndVerify(const char *keyName, KeyData_t *pKeyData,
	const uint8_t *pVerifyData, unsigned int verifyLen) const
{
	assert(keyName);
	assert(pVerifyData);
	assert(verifyLen == 16);
	if (!keyName || !pVerifyData || verifyLen != 16) {
		// Invalid parameters.
		return VERIFY_INVALID_PARAMS;
	}

	// Temporary KeyData_t in case pKeyData is nullptr.
	KeyData_t tmp_key_data;
	if (!pKeyData) {
		pKeyData = &tmp_key_data;
	}

	// Get the key first.
	VerifyResult res = get(keyName, pKeyData);
	if (res != VERIFY_OK) {
		// Error obtaining the key.
		return res;
	} else if (!pKeyData->key || pKeyData->length == 0) {
		// Key is invalid.
		return VERIFY_KEY_INVALID;
	}

	// Decrypt the test data.
	// TODO: Keep this IAesCipher instance around?
	unique_ptr<IAesCipher> cipher(AesCipherFactory::create());
	if (!cipher) {
		// Unable to create the IAesCipher.
		return VERFIY_IAESCIPHER_INIT_ERR;
	}

	// Set cipher parameters.
	int ret = cipher->setChainingMode(IAesCipher::CM_ECB);
	if (ret != 0) {
		return VERFIY_IAESCIPHER_INIT_ERR;
	}
	ret = cipher->setKey(pKeyData->key, pKeyData->length);
	if (ret != 0) {
		return VERFIY_IAESCIPHER_INIT_ERR;
	}

	// Decrypt the test data.
	// NOTE: IAesCipher decrypts in place, so we need to
	// make a temporary copy.
	unique_ptr<uint8_t[]> tmpData(new uint8_t[verifyLen]);
	memcpy(tmpData.get(), pVerifyData, verifyLen);
	unsigned int size = cipher->decrypt(tmpData.get(), verifyLen);
	if (size != verifyLen) {
		// Decryption failed.
		return VERIFY_IAESCIPHER_DECRYPT_ERR;
	}

	// Verify the test data.
	if (memcmp(tmpData.get(), verifyTestString, verifyLen) != 0) {
		// Verification failed.
		return VERIFY_WRONG_KEY;
	}

	// Test data verified.
	return VERIFY_OK;
}

}
