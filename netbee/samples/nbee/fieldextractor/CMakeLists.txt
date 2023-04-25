# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


PROJECT(FIELDEXTRACTOR)


# Set source files
SET(SOURCES
	../common/measure.h
	fieldextractor.cpp
)


# Default directories for include files
INCLUDE_DIRECTORIES (
	${FIELDEXTRACTOR_SOURCE_DIR}
	${FIELDEXTRACTOR_SOURCE_DIR}/../../../include
	${FIELDEXTRACTOR_SOURCE_DIR}/../../../../WPdPack/Include
)


# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES(${FIELDEXTRACTOR_SOURCE_DIR}/../../../lib)
	LINK_DIRECTORIES(${FIELDEXTRACTOR_SOURCE_DIR}/../../../../WPdPack/Lib)
ELSE(WIN32)
	LINK_DIRECTORIES(${FIELDEXTRACTOR_SOURCE_DIR}/../../../bin)
	LINK_DIRECTORIES(${FIELDEXTRACTOR_SOURCE_DIR}/../../../lib)
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
	fieldextractor
	${SOURCES}
)


# Link the executable to the required libraries
IF(WIN32)
	TARGET_LINK_LIBRARIES(fieldextractor nbee wpcap)
ELSE(WIN32)
IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	TARGET_LINK_LIBRARIES(fieldextractor nbee pcap nbnetvm nbpflcompiler compat)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	TARGET_LINK_LIBRARIES(fieldextractor nbee pcap
	)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET fieldextractor
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/fieldextractor.exe ../../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET fieldextractor
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/fieldextractor ../../../bin/.
	)
ENDIF(WIN32)
