# Platform-specific functionality.

# Hack to remove -rdynamic from CFLAGS and CXXFLAGS
# See http://public.kitware.com/pipermail/cmake/2006-July/010404.html
IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
	SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
ENDIF()

# Don't embed rpaths in the executables.
SET(CMAKE_SKIP_RPATH ON)

# Check for visibility symbols.
IF(NOT CMAKE_VERSION VERSION_LESS 3.3.0)
	# CMake 3.3: Use CMake predefined variables.
	# NOTE: CMake 3.0-3.2 do not apply these settings
	# to static libraries, so we have to fall back to the
	# "deprecated" ADD_COMPILER_EXPORT_FLAGS().
	CMAKE_POLICY(SET CMP0063 NEW)
	SET(CMAKE_C_VISIBILITY_PRESET "hidden")
	SET(CMAKE_CXX_VISIBILITY_PRESET "hidden")
	SET(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
ELSE()
	# CMake 2.x, 3.0-3.2: Use ADD_COMPILER_EXPORT_FLAGS().
	# NOTE: 3.0+ will show a deprecation warning.
	INCLUDE(GenerateExportHeader)
	ADD_COMPILER_EXPORT_FLAGS(RP_COMPILER_EXPORT_FLAGS)
	SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${RP_COMPILER_EXPORT_FLAGS}")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${RP_COMPILER_EXPORT_FLAGS}")
	UNSET(RP_COMPILER_EXPORT_FLAGS)
ENDIF()

# Common flag variables:
# [common]
# - RP_C_FLAGS_COMMON
# - RP_CXX_FLAGS_COMMON
# - RP_EXE_LINKER_FLAGS_COMMON
# - RP_SHARED_LINKER_FLAGS_COMMON
# [debug]
# - RP_C_FLAGS_DEBUG
# - RP_CXX_FLAGS_DEBUG
# - RP_EXE_LINKER_FLAGS_DEBUG
# - RP_SHARED_LINKER_FLAGS_DEBUG
# [release]
# - RP_C_FLAGS_RELEASE
# - RP_CXX_FLAGS_RELEASE
# - RP_EXE_LINKER_FLAGS_RELEASE
# - RP_SHARED_LINKER_FLAGS_RELEASE
#
# DEBUG and RELEASE variables do *not* include COMMON.
IF(MSVC)
	INCLUDE(cmake/platform/msvc.cmake)
ELSE(MSVC)
	INCLUDE(cmake/platform/gcc.cmake)
ENDIF(MSVC)

# Platform-specific configuration.
IF(WIN32)
	INCLUDE(cmake/platform/win32.cmake)
ENDIF(WIN32)

# Set CMAKE flags.
# TODO: RelWithDebInfo / MinSizeRel?
# Common
SET(CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} ${RP_C_FLAGS_COMMON}")
SET(CMAKE_CXX_FLAGS			"${CMAKE_CXX_FLAGS} ${RP_CXX_FLAGS_COMMON}")
SET(CMAKE_EXE_LINKER_FLAGS		"${CMAKE_EXE_LINKER_FLAGS} ${RP_EXE_LINKER_FLAGS_COMMON}")
SET(CMAKE_SHARED_LINKER_FLAGS		"${CMAKE_SHARED_LINKER_FLAGS} ${RP_SHARED_LINKER_FLAGS_COMMON}")
SET(CMAKE_MODULE_LINKER_FLAGS		"${CMAKE_MODULE_LINKER_FLAGS} ${RP_MODULE_LINKER_FLAGS_COMMON}")
# Debug
SET(CMAKE_C_FLAGS_DEBUG			"${CMAKE_C_FLAGS_DEBUG} ${RP_C_FLAGS_DEBUG}")
SET(CMAKE_CXX_FLAGS_DEBUG		"${CMAKE_CXX_FLAGS_DEBUG} ${RP_CXX_FLAGS_DEBUG}")
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG	"${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${RP_EXE_LINKER_FLAGS_DEBUG}")
SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG	"${CMAKE_SHARED_LINKER_FLAGS_DEBUG} ${RP_SHARED_LINKER_FLAGS_DEBUG}")
SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG	"${CMAKE_MODULE_LINKER_FLAGS_DEBUG} ${RP_MODULE_LINKER_FLAGS_COMMON_DEBUG}")
# Release
SET(CMAKE_C_FLAGS_RELEASE		"${CMAKE_C_FLAGS_RELEASE} ${RP_C_FLAGS_RELEASE}")
SET(CMAKE_CXX_FLAGS_RELEASE		"${CMAKE_CXX_FLAGS_RELEASE} ${RP_CXX_FLAGS_RELEASE}")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE	"${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${RP_EXE_LINKER_FLAGS_RELEASE}")
SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE	"${CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${RP_SHARED_LINKER_FLAGS_RELEASE}")
SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE	"${CMAKE_MODULE_LINKER_FLAGS_RELEASE} ${RP_MODULE_LINKER_FLAGS_COMMON_RELEASE}")

# Unset temporary variables.
# Common
UNSET(RP_C_FLAGS_COMMON)
UNSET(RP_CXX_FLAGS_COMMON)
UNSET(RP_EXE_LINKER_FLAGS_COMMON)
UNSET(RP_SHARED_LINKER_FLAGS_COMMON)
UNSET(RP_MODULE_LINKER_FLAGS_COMMON)
# Debug
UNSET(RP_C_FLAGS_DEBUG)
UNSET(RP_CXX_FLAGS_DEBUG)
UNSET(RP_EXE_LINKER_FLAGS_DEBUG)
UNSET(RP_SHARED_LINKER_FLAGS_DEBUG)
UNSET(RP_MODULE_LINKER_FLAGS_DEBUG)
# Release
UNSET(RP_C_FLAGS_RELEASE)
UNSET(RP_CXX_FLAGS_RELEASE)
UNSET(RP_EXE_LINKER_FLAGS_RELEASE)
UNSET(RP_SHARED_LINKER_FLAGS_RELEASE)
UNSET(RP_MODULE_LINKER_FLAGS_RELEASE)
