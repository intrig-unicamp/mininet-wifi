# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


PROJECT(NETPDLUTILS)


# Set source files
SET(SOURCES
	netpdlutils.cpp
)


# Default directories for include files
INCLUDE_DIRECTORIES (
	${NETPDLUTILS_SOURCE_DIR}
	${NETPDLUTILS_SOURCE_DIR}/../../../include
	${NETPDLUTILS_SOURCE_DIR}/../../../../WPdPack/Include
)


# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES(${NETPDLUTILS_SOURCE_DIR}/../../../lib)
ELSE(WIN32)
	LINK_DIRECTORIES(${NETPDLUTILS_SOURCE_DIR}/../../../bin)
ENDIF(WIN32)


# Platform-specific definitions
IF(WIN32)
	ADD_DEFINITIONS(
		-D_CRT_SECURE_NO_WARNINGS
		-D_CRT_SECURE_NO_DEPRECATE
		-DWIN32_LEAN_AND_MEAN
	)
ENDIF(WIN32)


# Create executable
ADD_EXECUTABLE(
	netpdlutils
	${SOURCES}
)


# Link the executable to the required libraries
IF(WIN32)
	TARGET_LINK_LIBRARIES(
		netpdlutils
		nbee
	)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(
		netpdlutils
		nbee
	)
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET netpdlutils
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/netpdlutils.exe ../../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET netpdlutils
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/netpdlutils ../../../bin/.
	)
ENDIF(WIN32)
