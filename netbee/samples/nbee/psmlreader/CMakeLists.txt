# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


PROJECT(PSMLREADER)


# Set source files
SET(SOURCES
	psmlreader.cpp
)


# Default directories for include files
INCLUDE_DIRECTORIES (
	${PSMLREADER_SOURCE_DIR}
	${PSMLREADER_SOURCE_DIR}/../../../include
	${PSMLREADER_SOURCE_DIR}/../../../../WPdPack/Include
)


# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES(${PSMLREADER_SOURCE_DIR}/../../../lib)
	LINK_DIRECTORIES(${PSMLREADER_SOURCE_DIR}/../../../../WPdPack/Lib)
ELSE(WIN32)
	LINK_DIRECTORIES(${PSMLREADER_SOURCE_DIR}/../../../bin)
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
	psmlreader
	${SOURCES}
)


# Link the executable to the required libraries
IF(WIN32)
	TARGET_LINK_LIBRARIES(
		psmlreader
		nbee
		wpcap
	)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(
		psmlreader
		nbee
		pcap
	)
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET psmlreader
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/psmlreader.exe ../../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET psmlreader
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/psmlreader ../../../bin/.
	)
ENDIF(WIN32)
