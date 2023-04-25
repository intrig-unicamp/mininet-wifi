# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


PROJECT(PACKETCAPTURE)


# Set source files
SET(SOURCES
	packetcapture.cpp
)


# Default directories for include files
INCLUDE_DIRECTORIES (
	${PACKETCAPTURE_SOURCE_DIR}
	${PACKETCAPTURE_SOURCE_DIR}/../../../include
	${PACKETCAPTURE_SOURCE_DIR}/../../../../WPdPack/Include
)


# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES(${PACKETCAPTURE_SOURCE_DIR}/../../../lib)
ELSE(WIN32)
	LINK_DIRECTORIES(${PACKETCAPTURE_SOURCE_DIR}/../../../bin)
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
	packetcapture
	${SOURCES}
)


# Link the executable to the required libraries
IF(WIN32)
	TARGET_LINK_LIBRARIES(
		packetcapture
		nbee
	)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(
		packetcapture
		libnbee.so
	)
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET packetcapture
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/packetcapture.exe ../../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET packetcapture
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/packetcapture ../../../bin/.
	)
ENDIF(WIN32)
