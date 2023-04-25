# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


PROJECT(SOCKUTILSTEST)


# Set source files
SET(SOURCES
	sockutilstest.cpp
)


# Default directories for include files
INCLUDE_DIRECTORIES (
	${SOCKUTILSTEST_SOURCE_DIR}
	${SOCKUTILSTEST_SOURCE_DIR}/../../../include
)


# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES(${SOCKUTILSTEST_SOURCE_DIR}/../../../lib)
ELSE(WIN32)
	LINK_DIRECTORIES(${SOCKUTILSTEST_SOURCE_DIR}/../../../bin)
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
	sockutilstest
	${SOURCES}
)


# Link the executable to the required libraries
IF(WIN32)
	LINK_LIBRARIES(ws2_32.lib)
	TARGET_LINK_LIBRARIES(
		sockutilstest
		nbsockutils
	)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(
		sockutilstest
		nbsockutils
	)
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET sockutilstest
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/sockutilstest.exe ../../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET sockutilstest
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/sockutilstest ../../../bin/.
	)
ENDIF(WIN32)
