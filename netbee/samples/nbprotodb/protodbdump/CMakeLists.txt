# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


PROJECT(PROTODBDUMP)


# Set source files
SET(SOURCES
	protodbdump.cpp
)


# Default directories for include files
INCLUDE_DIRECTORIES (
	${PROTODBDUMP_SOURCE_DIR}
	${PROTODBDUMP_SOURCE_DIR}/../../../include)


# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES(${PROTODBDUMP_SOURCE_DIR}/../../../lib)
ELSE(WIN32)
	LINK_DIRECTORIES(${PROTODBDUMP_SOURCE_DIR}/../../../bin)
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
	protodbdump
	${SOURCES}
)


# Link the executable to the required libraries
IF(WIN32)
	TARGET_LINK_LIBRARIES(
		protodbdump
		nbprotodb
	)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(
		protodbdump
		nbprotodb
	)
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET protodbdump
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/protodbdump.exe ../../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET protodbdump
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/protodbdump ../../../bin/.
	)
ENDIF(WIN32)
