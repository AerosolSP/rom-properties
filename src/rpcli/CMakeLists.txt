PROJECT(rom-properties-rpcli)

# Library checks.
INCLUDE(CheckFunctionExists)
MACRO(CHECK_FUNCTION_EXISTS_OR_INLINE FUNCTION HEADER SAMPLE_CODE VARIABLE)
	# MinGW defines some reentrant functions as inline functions
	# that are actually wrappers around MSVC "secure" functions.
	# Check for the function as a regular function first, then check
	# if it's available in the specified header.
	IF(NOT DEFINED ${VARIABLE})

	CHECK_FUNCTION_EXISTS(${FUNCTION} ${VARIABLE})
	IF(NOT ${VARIABLE})
		# Function does not exist normally.
		# Check if it's an inline function in the specified header.
		MESSAGE(STATUS "Looking for ${FUNCTION} as an inline function")
		CHECK_C_SOURCE_COMPILES(
			"#define _POSIX_SOURCE
			#define _POSIX_C_SOURCE 1
			#include <${HEADER}>
			int main(void) { ${SAMPLE_CODE} }"
			${VARIABLE}_INLINE)
		IF(${VARIABLE}_INLINE)
			MESSAGE(STATUS "Looking for ${FUNCTION} as an inline function - found")
			SET(${VARIABLE} 1 CACHE INTERNAL "Have function ${FUNCTION} (inline)")
		ELSE(${VARIABLE}_INLINE)
			MESSAGE(STATUS "Looking for ${FUNCTION} as an inline function - not found")
			# ${VARIABLE} is already cached by CHECK_FUNCTION_EXISTS().
		ENDIF(${VARIABLE}_INLINE)
	ENDIF(NOT ${VARIABLE})

	ENDIF(NOT DEFINED ${VARIABLE})
ENDMACRO(CHECK_FUNCTION_EXISTS_OR_INLINE VARIABLE)

# Check for reentrant functions.
# NOTE: May be _gmtime32_s() or _gmtime64_s() on MSVC 2005+.
# The "inline" part will detect that.
# FIXME: CHECK_SYMBOL_EXISTS() works for macros like in MSVC,
# but fails with inline functions like in MinGW-w64.
CHECK_FUNCTION_EXISTS_OR_INLINE(gmtime_r "time.h" "time_t tm; gmtime_r(&tm, NULL);" HAVE_GMTIME_R)
IF(NOT HAVE_GMTIME_R)
	CHECK_FUNCTION_EXISTS_OR_INLINE(gmtime_s "time.h" "time_t tm; gmtime_s(NULL, &tm);" HAVE_GMTIME_S)
ENDIF(NOT HAVE_GMTIME_R)
CHECK_FUNCTION_EXISTS_OR_INLINE(localtime_r "time.h" "time_t tm; localtime_r(&tm, NULL);" HAVE_LOCALTIME_R)
IF(NOT HAVE_LOCALTIME_R)
	CHECK_FUNCTION_EXISTS_OR_INLINE(localtime_s "time.h" "time_t tm; localtime_s(NULL, &tm);" HAVE_LOCALTIME_S)
ENDIF(NOT HAVE_LOCALTIME_R)

# Write the config.h file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.rpcli.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.rpcli.h")

# Sources and headers.
SET(rom-properties-rpcli_SRCS
	rpcli.cpp
	properties.cpp
	bmp.cpp
	resource.rc
	)
SET(rom-properties-rpcli_H
	properties.hpp
	bmp.hpp
	)

#####################
# Build the plugin. #
#####################

ADD_EXECUTABLE(rpcli
		${rom-properties-rpcli_SRCS}
		${rom-properties-rpcli_H}
		)
SET_TARGET_PROPERTIES(rpcli PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(rpcli)
SET_WINDOWS_SUBSYSTEM(rpcli CONSOLE)
SET_WINDOWS_NO_MANIFEST(rpcli)
# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(rpcli
	PUBLIC	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	PRIVATE	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
	)
TARGET_LINK_LIBRARIES(rpcli romdata8)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# NOTE: Don't install libraries.
# That installs the import library, which isn't used
# for shell extensions.
INSTALL(TARGETS rpcli
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	LIBRARY DESTINATION "${DIR_INSTALL_DLL}"
	#ARCHIVE DESTINATION "${DIR_INSTALL_LIB}"
	COMPONENT "program"
	)
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME rpcli PDB)
	INSTALL(FILES "${DEBUG_FILENAME}"
		DESTINATION "${DIR_INSTALL_EXE_DEBUG}"
		COMPONENT "debug"
		)
	UNSET(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)