PROJECT(extlib)

# zlib: Assembly optimizations.
# TODO: Test on all archs and fix Z_PREFIX issues.
# TODO: Add more assembly files from contrib/ besides the
# default ones that were listed in zlib's CMakeLists.txt.
# FIXME: Broken on AppVeyor. (MSVC 2015)
IF(0)
STRING(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" arch)
IF(arch MATCHES "^(i.|x)86$|^x86_64$|^amd64$")
	# i386/amd64. Check sizeof(void*) for the actual architecture,
	# since building 32-bit on 64-bit isn't considered "cross-compiling",
	# so CMAKE_SYSTEM_PROCESSOR might not be accurate.
	# TODO: Optimize this, e.g. IF(MSVC AND CMAKE_CL_64)?
	IF(MSVC)
		# Check CMAKE_CL_64 instead of sizeof(void*) for MSVC builds.
		IF(CMAKE_CL_64)
			SET(AMD64 "ON")
		ELSE()
			SET(ASM686 "ON")
		ENDIF()
	ELSE()
		IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
			SET(AMD64 "ON")
		ELSE()
			SET(ASM686 "ON")
		ENDIF()
	ENDIF()
ENDIF(arch MATCHES "^(i.|x)86$|^x86_64$|^amd64$")
UNSET(arch)
ENDIF(0)

# Don't install any of these libraries.
SET(OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
SET(OLD_SKIP_INSTALL_LIBRARIES ${SKIP_INSTALL_LIBRARIES})
SET(OLD_SKIP_INSTALL_HEADERS ${SKIP_INSTALL_HEADERS})
SET(OLD_SKIP_INSTALL_FILES ${SKIP_INSTALL_FILES})
SET(OLD_SKIP_INSTALL_ALL ${SKIP_INSTALL_ALL})
SET(BUILD_SHARED_LIBS OFF)
SET(SKIP_INSTALL_LIBRARIES ON)
SET(SKIP_INSTALL_HEADERS ON)
SET(SKIP_INSTALL_FILES ON)
SET(SKIP_INSTALL_ALL ON)

IF(WIN32)
	# Enable internal zlib for Win32 only.
	SET(ZLIB_DEFINITIONS "-DZLIB_CONST")
	ADD_SUBDIRECTORY(zlib)
ENDIF(WIN32)

# Google Test
IF(BUILD_TESTING)
	# Reference: http://stackoverflow.com/questions/12540970/how-to-make-gtest-build-mdd-instead-of-mtd-by-default-using-cmake
	SET(gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll")
	ADD_SUBDIRECTORY(googletest)

	INCLUDE(SetMSVCDebugPath)
	FOREACH(_target
		gtest gtest_main gtest_no_exception gtest_main_no_exception
		gtest_main_no_rtti gtest_dll gtest_main_use_own_tuple
		gmock gmock_main gmock_main_no_exception
		gmock_main_no_rtti gmock_main_use_own_tuple)
		IF(TARGET ${_target})
			# Set the MSVC debug path.
			SET_MSVC_DEBUG_PATH(${_target})

			# Exclude from ALL and DEFAULT builds.
			SET_TARGET_PROPERTIES(${_target} PROPERTIES
				EXCLUDE_FROM_ALL TRUE
				EXCLUDE_FROM_DEFAULT_BUILD TRUE
				)
		ENDIF(TARGET ${_target})
	ENDFOREACH()
ENDIF(BUILD_TESTING)

# FIXME: Probably unnecessary, since these variables
# only apply to the extlib/ subdirectory.
SET(BUILD_SHARED_LIBS ${OLD_BUILD_SHARED_LIBS})
SET(SKIP_INSTALL_LIBRARIES ${OLD_SKIP_INSTALL_LIBRARIES})
SET(SKIP_INSTALL_HEADERS ${OLD_SKIP_INSTALL_HEADERS})
SET(SKIP_INSTALL_FILES ${OLD_SKIP_INSTALL_FILES})
SET(SKIP_INSTALL_ALL ${OLD_SKIP_INSTALL_ALL})
UNSET(OLD_BUILD_SHARED_LIBS)
UNSET(OLD_SKIP_INSTALL_LIBRARIES)
UNSET(OLD_SKIP_INSTALL_HEADERS)
UNSET(OLD_SKIP_INSTALL_FILES)
UNSET(OLD_SKIP_INSTALL_ALL)