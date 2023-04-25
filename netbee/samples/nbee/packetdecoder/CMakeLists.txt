# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


PROJECT(PACKETDECODER)


# Set source files
SET(SOURCES
	../common/measure.h
	packetdecoder.cpp
)


# Default directories for include files
INCLUDE_DIRECTORIES (
	${PACKETDECODER_SOURCE_DIR}
	${PACKETDECODER_SOURCE_DIR}/../../../include
	${PACKETDECODER_SOURCE_DIR}/../../../../WPdPack/Include
)


# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES(${PACKETDECODER_SOURCE_DIR}/../../../lib)
ELSE(WIN32)
	LINK_DIRECTORIES(${PACKETDECODER_SOURCE_DIR}/../../../bin)
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
	packetdecoder
	${SOURCES}
)


# Link the executable to the required libraries
IF(WIN32)
	TARGET_LINK_LIBRARIES(
		packetdecoder
		nbee
	)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(
		packetdecoder
		nbee
	)
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET packetdecoder
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/packetdecoder.exe ../../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET packetdecoder
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/packetdecoder ../../../bin/.
	)
ENDIF(WIN32)
