# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


PROJECT(FILTERCOMPILER)


# Set source files
SET(SOURCES
	filtercompiler.cpp
)


# Default directories for include files
INCLUDE_DIRECTORIES (
	${FILTERCOMPILER_SOURCE_DIR}
	${FILTERCOMPILER_SOURCE_DIR}/../../../include)


# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES(${FILTERCOMPILER_SOURCE_DIR}/../../../lib)
ELSE(WIN32)
	LINK_DIRECTORIES(${FILTERCOMPILER_SOURCE_DIR}/../../../bin)
ENDIF(WIN32)


# Platform-specific definitions
IF(WIN32)
	ADD_DEFINITIONS(
		-D_CRT_SECURE_NO_WARNINGS
		-DWIN32_LEAN_AND_MEAN
	)
ENDIF(WIN32)


# Create executable
ADD_EXECUTABLE(
	filtercompiler
	${SOURCES}
)


# Link the executable to the required libraries
IF(WIN32)
	TARGET_LINK_LIBRARIES(
		filtercompiler
		nbprotodb
		nbpflcompiler
	)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(
		filtercompiler
		nbprotodb
		nbpflcompiler
	)
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET filtercompiler
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/filtercompiler.exe ../../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET filtercompiler
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/filtercompiler ../../../bin/.
	)
ENDIF(WIN32)
