# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


PROJECT(PDMLREADER)


# Set source files
SET(SOURCES
	pdmlreader.cpp
)


# Default directories for include files
INCLUDE_DIRECTORIES (
	${PDMLREADER_SOURCE_DIR}
	${PDMLREADER_SOURCE_DIR}/../../../include
	${PDMLREADER_SOURCE_DIR}/../../../../WPdPack/Include
)


# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES(${PDMLREADER_SOURCE_DIR}/../../../lib)
	LINK_DIRECTORIES(${PDMLREADER_SOURCE_DIR}/../../../../WPdPack/Lib)
ELSE(WIN32)
	LINK_DIRECTORIES(${PDMLREADER_SOURCE_DIR}/../../../bin)
ENDIF(WIN32)


# Platform-specific definitions
IF(WIN32)
	ADD_DEFINITIONS(
		-D_CRT_SECURE_NO_WARNINGS
		-D_CRT_SECURE_NO_DEPRECATE
		-DWIN32_LEAN_AND_MEAN
		-DHAVE_REMOTE
	)
ENDIF(WIN32)


# Create executable
ADD_EXECUTABLE(
	pdmlreader
	${SOURCES}
)


# Link the executable to the required libraries
IF(WIN32)
	TARGET_LINK_LIBRARIES(
		pdmlreader
		nbee
		wpcap
	)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(
		pdmlreader
		nbee
		pcap
	)
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET pdmlreader
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/pdmlreader.exe ../../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET pdmlreader
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/pdmlreader ../../../bin/.
	)
ENDIF(WIN32)
