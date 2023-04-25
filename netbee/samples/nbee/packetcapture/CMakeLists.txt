# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


PROJECT(PACKETCAPTURE)


# Set source files
SET(SOURCES
	../common/measure.h
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
	LINK_DIRECTORIES(${PACKETCAPTURE_SOURCE_DIR}/../../../../WPdPack/Lib)
ELSE(WIN32)
	LINK_DIRECTORIES(${PACKETCAPTURE_SOURCE_DIR}/../../../bin)
	LINK_DIRECTORIES(${PACKETCAPTURE_SOURCE_DIR}/../../../lib)
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
	packetcapture
	${SOURCES}
)


# Link the executable to the required libraries
IF(WIN32)
	TARGET_LINK_LIBRARIES(packetcapture nbee wpcap)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(packetcapture nbee pcap)
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
		TARGET PACKETCAPTURE
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/packetcapture ../../../bin/.
	)
ENDIF(WIN32)
