/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * exe_structs.h: DOS/Windows executable structures.                       *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

// Based on w32api's winnt.h.
// References:
// - https://github.com/MaxKellermann/w32api/blob/440c229960e782831d01c6638661f1c40cadbeb5/include/winnt.h
// - https://github.com/MaxKellermann/w32api/blob/440c229960e782831d01c6638661f1c40cadbeb5/include/winver.h
// - http://www.brokenthorn.com/Resources/OSDevPE.html

#ifndef __ROMPROPERTIES_LIBROMDATA_EXE_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_EXE_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMAGE_DOS_SIGNATURE 0x5A4D
#define IMAGE_OS2_SIGNATURE 0x454E
#define IMAGE_OS2_SIGNATURE_LE 0x454C
#define IMAGE_VXD_SIGNATURE 0x454C
#define IMAGE_NT_SIGNATURE 0x00004550
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20b
#define IMAGE_ROM_OPTIONAL_HDR_MAGIC 0x107
#define IMAGE_SEPARATE_DEBUG_SIGNATURE 0x4944
#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16
#define IMAGE_SIZEOF_ROM_OPTIONAL_HEADER 56
#define IMAGE_SIZEOF_STD_OPTIONAL_HEADER 28
#define IMAGE_SIZEOF_NT_OPTIONAL_HEADER 224
#define IMAGE_SIZEOF_SHORT_NAME 8
#define IMAGE_SIZEOF_SECTION_HEADER 40
#define IMAGE_SIZEOF_SYMBOL 18
#define IMAGE_SIZEOF_AUX_SYMBOL 18
#define IMAGE_SIZEOF_RELOCATION 10
#define IMAGE_SIZEOF_BASE_RELOCATION 8
#define IMAGE_SIZEOF_LINENUMBER 6
#define IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR 60
#define SIZEOF_RFPO_DATA 16

#define IMAGE_SIZEOF_FILE_HEADER 20
typedef enum {
	IMAGE_FILE_RELOCS_STRIPPED		= 1,
	IMAGE_FILE_EXECUTABLE_IMAGE		= 2,
	IMAGE_FILE_LINE_NUMS_STRIPPED		= 4,
	IMAGE_FILE_LOCAL_SYMS_STRIPPED		= 8,
	IMAGE_FILE_AGGRESIVE_WS_TRIM 		= 16,
	IMAGE_FILE_LARGE_ADDRESS_AWARE		= 32,
	IMAGE_FILE_BYTES_REVERSED_LO		= 128,
	IMAGE_FILE_32BIT_MACHINE		= 256,
	IMAGE_FILE_DEBUG_STRIPPED		= 512,
	IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP	= 1024,
	IMAGE_FILE_NET_RUN_FROM_SWAP		= 2048,
	IMAGE_FILE_SYSTEM			= 4096,
	IMAGE_FILE_DLL				= 8192,
	IMAGE_FILE_UP_SYSTEM_ONLY		= 16384,
	IMAGE_FILE_BYTES_REVERSED_HI		= 32768,
} PE_Characteristics;

typedef enum {
	// TODO: Update from `file`.
	IMAGE_FILE_MACHINE_UNKNOWN	= 0x0000,
	IMAGE_FILE_MACHINE_I386		= 0x014C, /* Intel 386 or later processors 
						     and compatible processors */
	IMAGE_FILE_MACHINE_R3000_BE	= 0x0160, /* MIPS big endian */
	IMAGE_FILE_MACHINE_R3000	= 0x0162, /* MIPS little endian */
	IMAGE_FILE_MACHINE_R4000	= 0x0166, /* MIPS little endian */
	IMAGE_FILE_MACHINE_R10000	= 0x0168, /* MIPS little endian */
	IMAGE_FILE_MACHINE_WCEMIPSV2	= 0x0169, /* MIPS little-endian WCE v2 */
	IMAGE_FILE_MACHINE_ALPHA	= 0x0184, /* Alpha AXP */
	IMAGE_FILE_MACHINE_SH3		= 0x01A2, /* Hitachi SH3 */
	IMAGE_FILE_MACHINE_SH3DSP	= 0x01A3, /* Hitachi SH3 DSP */
	IMAGE_FILE_MACHINE_SH3E		= 0x01A4, /* Hitachi SH3E */
	IMAGE_FILE_MACHINE_SH4		= 0x01A6, /* Hitachi SH4 */
	IMAGE_FILE_MACHINE_SH5		= 0x01A8, /* Hitachi SH5 */
	IMAGE_FILE_MACHINE_ARM		= 0x01C0, /* ARM little endian */
	IMAGE_FILE_MACHINE_THUMB	= 0x01C2, /* Thumb */
	IMAGE_FILE_MACHINE_ARMNT	= 0x01C4, /* Thumb-2 */
	IMAGE_FILE_MACHINE_AM33		= 0x01D3, /* Matsushita AM33 */
	IMAGE_FILE_MACHINE_POWERPC	= 0x01F0, /* Power PC little endian */
	IMAGE_FILE_MACHINE_POWERPCFP	= 0x01F1, /* Power PC with floating point support */
	IMAGE_FILE_MACHINE_IA64		= 0x0200, /* Intel Itanium processor family */
	IMAGE_FILE_MACHINE_MIPS16	= 0x0266, /* MIPS16 */
	IMAGE_FILE_MACHINE_ALPHA64	= 0x0284, /* Alpha AXP (64-bit) */
	IMAGE_FILE_MACHINE_MIPSFPU	= 0x0366, /* MIPS with FPU */
	IMAGE_FILE_MACHINE_MIPSFPU16	= 0x0466, /* MIPS16 with FPU */
	IMAGE_FILE_MACHINE_AXP64	= IMAGE_FILE_MACHINE_ALPHA64, /* Alpha AXP (64-bit) */
	IMAGE_FILE_MACHINE_TRICORE	= 0x0520, /* Infinieon */
	IMAGE_FILE_MACHINE_CEF		= 0x0CEF, /* Common Executable Format (Windows CE) */
	IMAGE_FILE_MACHINE_EBC		= 0x0EBC, /* EFI byte code */
	IMAGE_FILE_MACHINE_AMD64	= 0x8664, /* x64 */
	IMAGE_FILE_MACHINE_M32R		= 0x9041, /* Mitsubishi M32R little endian */
	IMAGE_FILE_MACHINE_ARM64	= 0xAA64, /* ARM64 little-endian */
	IMAGE_FILE_MACHINE_CEE		= 0xC0EE, /* MSIL (.NET) */
} PE_Machine;

typedef enum {
	IMAGE_SUBSYSTEM_UNKNOWN			= 0,
	IMAGE_SUBSYSTEM_NATIVE			= 1,
	IMAGE_SUBSYSTEM_WINDOWS_GUI		= 2,
	IMAGE_SUBSYSTEM_WINDOWS_CUI		= 3,
	IMAGE_SUBSYSTEM_OS2_CUI			= 5,  /* Not in PECOFF v8 spec */
	IMAGE_SUBSYSTEM_POSIX_CUI		= 7,
	IMAGE_SUBSYSTEM_NATIVE_WINDOWS		= 8,  /* Not in PECOFF v8 spec */
	IMAGE_SUBSYSTEM_WINDOWS_CE_GUI		= 9,
	IMAGE_SUBSYSTEM_EFI_APPLICATION		= 10,
	IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER	= 11,
	IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER	= 12,
	IMAGE_SUBSYSTEM_EFI_ROM			= 13,
	IMAGE_SUBSYSTEM_XBOX			= 14,
} PE_Subsystem;

typedef enum {
	IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA	= 0x0020,	// Image can handle a high entropy 64-bit virtual address space.
	IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE		= 0x0040,	// DLL can move.
	IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY	= 0x0080,	// Code Integrity Image
	IMAGE_DLLCHARACTERISTICS_NX_COMPAT		= 0x0100,	// Image is NX compatible.
	IMAGE_DLLCHARACTERISTICS_NO_ISOLATION		= 0x0200,	// Image understands isolation and doesn't want it.
	IMAGE_DLLCHARACTERISTICS_NO_SEH			= 0x0400,	// Image does not use SEH.  No SE handler may reside in this image.
	IMAGE_DLLCHARACTERISTICS_NO_BIND		= 0x0800,	// Do not bind this image.
	IMAGE_DLLCHARACTERISTICS_APPCONTAINER		= 0x1000,	// Image is an App(r).
	IMAGE_DLLCHARACTERISTICS_WDM_DRIVER		= 0x2000,	// Driver uses WDM model.
	IMAGE_DLLCHARACTERISTICS_GUARD_CF		= 0x4000,	// Image supports Control Flow Guard.
	IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE	= 0x8000,
} PE_DLL_Characteristics;

/**
 * MS-DOS "MZ" header.
 * All other EXE-based headers have this one first.
 *
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _IMAGE_DOS_HEADER {
	uint16_t e_magic;	// "MZ"
	uint16_t e_cblp;
	uint16_t e_cp;
	uint16_t e_crlc;
	uint16_t e_cparhdr;
	uint16_t e_minalloc;
	uint16_t e_maxalloc;
	uint16_t e_ss;
	uint16_t e_sp;
	uint16_t e_csum;
	uint16_t e_ip;
	uint16_t e_cs;
	uint16_t e_lfarlc;
	uint16_t e_ovno;
	uint16_t e_res[4];
	uint16_t e_oemid;
	uint16_t e_oeminfo;
	uint16_t e_res2[10];
	uint32_t e_lfanew;	// Pointer to NE/LE/LX/PE headers.
} IMAGE_DOS_HEADER;
#pragma pack()
ASSERT_STRUCT(IMAGE_DOS_HEADER, 64);

/**
 * Standard PE header.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _IMAGE_FILE_HEADER {
	uint16_t Machine;		// See PE_Machine.
	uint16_t NumberOfSections;
	uint32_t TimeDateStamp;		// UNIX timestamp.
	uint32_t PointerToSymbolTable;
	uint32_t NumberOfSymbols;
	uint16_t SizeOfOptionalHeader;
	uint16_t Characteristics;
} IMAGE_FILE_HEADER;
#pragma pack()
ASSERT_STRUCT(IMAGE_FILE_HEADER, IMAGE_SIZEOF_FILE_HEADER);

/**
 * PE image data directory indexes.
 * Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms680305(v=vs.85).aspx
 */
#pragma pack(1)
typedef enum {
	IMAGE_DATA_DIRECTORY_EXPORT_TABLE		= 0,
	IMAGE_DATA_DIRECTORY_IMPORT_TABLE		= 1,
	IMAGE_DATA_DIRECTORY_RESOURCE_TABLE		= 2,
	IMAGE_DATA_DIRECTORY_EXCEPTION_TABLE		= 3,
	IMAGE_DATA_DIRECTORY_CERTIFICATE_TABLE		= 4,
	IMAGE_DATA_DIRECTORY_RELOCATION_TABLE		= 5,
	IMAGE_DATA_DIRECTORY_DEBUG_INFO			= 6,
	IMAGE_DATA_DIRECTORY_ARCH_SPECIFIC_DATA		= 7,
	IMAGE_DATA_DIRECTORY_GLOBAL_PTR_REG		= 8,
	IMAGE_DATA_DIRECTORY_TLS_TABLE			= 9,
	IMAGE_DATA_DIRECTORY_LOCAL_CFG_TABLE		= 10,
	IMAGE_DATA_DIRECTORY_BOUND_IMPORT_TABLE		= 11,
	IMAGE_DATA_DIRECTORY_IMPORT_ADDR_TABLE		= 12,
	IMAGE_DATA_DIRECTORY_DELAY_IMPORT_DESCRIPTOR	= 13,
	IMAGE_DATA_DIRECTORY_CLR_HEADER			= 14,
	IMAGE_DATA_DIRECTORY_RESERVED			= 15,
} ImageDataDirectoryIndex;

/**
 * PE image data directory.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _IMAGE_DATA_DIRECTORY {
	uint32_t VirtualAddress;
	uint32_t Size;
} IMAGE_DATA_DIRECTORY;
#pragma pack()
ASSERT_STRUCT(IMAGE_DATA_DIRECTORY, 8);

/**
 * "Optional" 32-bit PE header.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _IMAGE_OPTIONAL_HEADER32 {
	uint16_t Magic;
	uint8_t MajorLinkerVersion;
	uint8_t MinorLinkerVersion;
	uint32_t SizeOfCode;
	uint32_t SizeOfInitializedData;
	uint32_t SizeOfUninitializedData;
	uint32_t AddressOfEntryPoint;
	uint32_t BaseOfCode;
	uint32_t BaseOfData;
	uint32_t ImageBase;
	uint32_t SectionAlignment;
	uint32_t FileAlignment;
	uint16_t MajorOperatingSystemVersion;
	uint16_t MinorOperatingSystemVersion;
	uint16_t MajorImageVersion;
	uint16_t MinorImageVersion;
	uint16_t MajorSubsystemVersion;
	uint16_t MinorSubsystemVersion;
	uint32_t Win32VersionValue;
	uint32_t SizeOfImage;
	uint32_t SizeOfHeaders;
	uint32_t CheckSum;
	uint16_t Subsystem;		// See PE_Subsystem.
	uint16_t DllCharacteristics;
	uint32_t SizeOfStackReserve;
	uint32_t SizeOfStackCommit;
	uint32_t SizeOfHeapReserve;
	uint32_t SizeOfHeapCommit;
	uint32_t LoaderFlags;
	uint32_t NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32;
#pragma pack()
ASSERT_STRUCT(IMAGE_OPTIONAL_HEADER32, 224);

/**
 * "Optional" 64-bit PE header.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _IMAGE_OPTIONAL_HEADER64 {
	uint16_t Magic;
	uint8_t MajorLinkerVersion;
	uint8_t MinorLinkerVersion;
	uint32_t SizeOfCode;
	uint32_t SizeOfInitializedData;
	uint32_t SizeOfUninitializedData;
	uint32_t AddressOfEntryPoint;
	uint32_t BaseOfCode;
	uint64_t ImageBase;
	uint32_t SectionAlignment;
	uint32_t FileAlignment;
	uint16_t MajorOperatingSystemVersion;
	uint16_t MinorOperatingSystemVersion;
	uint16_t MajorImageVersion;
	uint16_t MinorImageVersion;
	uint16_t MajorSubsystemVersion;
	uint16_t MinorSubsystemVersion;
	uint32_t Win32VersionValue;
	uint32_t SizeOfImage;
	uint32_t SizeOfHeaders;
	uint32_t CheckSum;
	uint16_t Subsystem;		// See PE_Subsystem.
	uint16_t DllCharacteristics;
	uint64_t SizeOfStackReserve;
	uint64_t SizeOfStackCommit;
	uint64_t SizeOfHeapReserve;
	uint64_t SizeOfHeapCommit;
	uint32_t LoaderFlags;
	uint32_t NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64;
#pragma pack()
ASSERT_STRUCT(IMAGE_OPTIONAL_HEADER64, 240);

/**
 * 32-bit PE headers.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _IMAGE_NT_HEADERS32 {
	uint32_t Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32;
#pragma pack()
ASSERT_STRUCT(IMAGE_NT_HEADERS32, 248);

/**
 * 64-bit PE32+ headers.
 * All fields are little-endian.
 */
#pragma pack(1)
typedef struct PACKED _IMAGE_NT_HEADERS64 {
	uint32_t Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64;
#pragma pack()
ASSERT_STRUCT(IMAGE_NT_HEADERS64, 264);

/**
 * Section header.
 */
#pragma pack(1)
typedef struct _IMAGE_SECTION_HEADER {
	char Name[IMAGE_SIZEOF_SHORT_NAME];
	union {
		uint32_t PhysicalAddress;
		uint32_t VirtualSize;
	} Misc;
	uint32_t VirtualAddress;
	uint32_t SizeOfRawData;
	uint32_t PointerToRawData;
	uint32_t PointerToRelocations;
	uint32_t PointerToLinenumbers;
	uint16_t NumberOfRelocations;
	uint16_t NumberOfLinenumbers;
	uint32_t Characteristics;
} IMAGE_SECTION_HEADER;
#pragma pack()
ASSERT_STRUCT(IMAGE_SECTION_HEADER, IMAGE_SIZEOF_SECTION_HEADER);

/** Win32 resources. **/

// Resource types.
typedef enum {
	RT_CURSOR	= 1,
	RT_BITMAP	= 2,
	RT_ICON		= 3,
	RT_MENU		= 4,
	RT_DIALOG	= 5,
	RT_STRING	= 6,
	RT_FONTDIR	= 7,
	RT_FONT		= 8,
	RT_ACCELERATOR	= 9,
	RT_RCDATA	= 10,
	RT_MESSAGETABLE	= 11,
	RT_VERSION	= 16,
	RT_DLGINCLUDE	= 17,
	RT_PLUGPLAY	= 19,
	RT_VXD		= 20,
	RT_ANICURSOR	= 21,
	RT_ANIICON	= 22,
	RT_HTML		= 23,
	RT_MANIFEST	= 24,
} ResourceType;

// Resource directory.
#pragma pack(1)
typedef struct PACKED _IMAGE_RESOURCE_DIRECTORY {
	uint32_t Characteristics;
	uint32_t TimeDateStamp;
	uint16_t MajorVersion;
	uint16_t MinorVersion;
	uint16_t NumberOfNamedEntries;
	uint16_t NumberOfIdEntries;
	// following this struct are named entries, then ID entries
} IMAGE_RESOURCE_DIRECTORY;
#pragma pack()

// Resource directory entry.
#pragma pack(1)
typedef struct PACKED _IMAGE_RESOURCE_DIRECTORY_ENTRY {
	// Name/ID field.
	// If bit 31 is set, this is an offset into the string table.
	// Otherwise, it's a 16-bit ID.
	uint32_t Name;

	// Offset to the resource data.
	// If bit 31 is set, this points to a subdirectory.
	// Otherwise, it's an actual resource.
	uint32_t OffsetToData;
} IMAGE_RESOURCE_DIRECTORY_ENTRY;
#pragma pack()

// Resource data entry.
#pragma pack(1)
typedef struct PACKED _IMAGE_RESOURCE_DATA_ENTRY {
	uint32_t OffsetToData;
	uint32_t Size;
	uint32_t CodePage;
	uint32_t Reserved;
} IMAGE_RESOURCE_DATA_ENTRY;
#pragma pack()

// Version flags.
//#define VS_FILE_INFO RT_VERSION	// TODO
#define VS_VERSION_INFO 1
#define VS_USER_DEFINED 100
#define VS_FFI_SIGNATURE 0xFEEF04BD
#define VS_FFI_STRUCVERSION 0x10000
#define VS_FFI_FILEFLAGSMASK 0x3F
typedef enum {
	VS_FF_DEBUG = 1,
	VS_FF_PRERELEASE = 2,
	VS_FF_PATCHED = 4,
	VS_FF_PRIVATEBUILD = 8,
	VS_FF_INFOINFERRED = 16,
	VS_FF_SPECIALBUILD = 32,
} VS_FileFlags;

typedef enum {
	VOS_UNKNOWN = 0,
	VOS_DOS = 0x10000,
	VOS_OS216 = 0x20000,
	VOS_OS232 = 0x30000,
	VOS_NT = 0x40000,
	VOS__BASE = 0,
	VOS__WINDOWS16 = 1,
	VOS__PM16 = 2,
	VOS__PM32 = 3,
	VOS__WINDOWS32 = 4,
	VOS_DOS_WINDOWS16 = 0x10001,
	VOS_DOS_WINDOWS32 = 0x10004,
	VOS_OS216_PM16 = 0x20002,
	VOS_OS232_PM32 = 0x30003,
	VOS_NT_WINDOWS32 = 0x40004,
} VS_OperatingSystem;

typedef enum {
	VFT_UNKNOWN = 0,
	VFT_APP = 1,
	VFT_DLL = 2,
	VFT_DRV = 3,
	VFT_FONT = 4,
	VFT_VXD = 5,
	VFT_STATIC_LIB = 7,
} VS_FileType;

typedef enum {
	VFT2_UNKNOWN = 0,
	VFT2_DRV_PRINTER = 1,
	VFT2_DRV_KEYBOARD = 2,
	VFT2_DRV_LANGUAGE = 3,
	VFT2_DRV_DISPLAY = 4,
	VFT2_DRV_MOUSE = 5,
	VFT2_DRV_NETWORK = 6,
	VFT2_DRV_SYSTEM = 7,
	VFT2_DRV_INSTALLABLE = 8,
	VFT2_DRV_SOUND = 9,
	VFT2_DRV_COMM = 10,
	VFT2_DRV_INPUTMETHOD = 11,
	VFT2_FONT_RASTER = 1,
	VFT2_FONT_VECTOR = 2,
	VFT2_FONT_TRUETYPE = 3,
} VS_FileSubtype;

/**
 * Version info resource. (fixed-size data section)
 */
#pragma pack(1)
typedef struct PACKED _VS_FIXEDFILEINFO {
	uint32_t dwSignature;
	uint32_t dwStrucVersion;
	uint32_t dwFileVersionMS;
	uint32_t dwFileVersionLS;
	uint32_t dwProductVersionMS;
	uint32_t dwProductVersionLS;
	uint32_t dwFileFlagsMask;
	uint32_t dwFileFlags;
	uint32_t dwFileOS;
	uint32_t dwFileType;
	uint32_t dwFileSubtype;
	uint32_t dwFileDateMS;
	uint32_t dwFileDateLS;
} _VS_FIXEDFILEINFO;
#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_EXE_STRUCTS_H__ */
