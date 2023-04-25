PROJECT(NBSOCKUTILS)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# Set source files
SET(SOURCES
	../nbee/globals/debug.h
	../nbee/globals/debug.c
	../nbee/globals/utils.h
	../nbee/globals/utils.c
	../../include/nbsockutils.h
	sockutils.c
)


# Default directories for include files
INCLUDE_DIRECTORIES(
	${NBSOCKUTILS_SOURCE_DIR}/../../include
)

# Platform-specific definitions
IF(WIN32)
	ADD_DEFINITIONS(
		-D_CRT_SECURE_NO_WARNINGS
		-D_CRT_SECURE_NO_DEPRECATE
		-DWIN32_LEAN_AND_MEAN
		-DSOCKUTILS_EXPORTS
	)
ENDIF(WIN32)


# Add libraries that are required for linking
IF(WIN32)
	LINK_LIBRARIES(ws2_32.lib)
ENDIF(WIN32)


# Create output library
ADD_LIBRARY(
	nbsockutils
	SHARED
	${SOURCES}
)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbsockutils
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbsockutils.dll ../../bin/.
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbsockutils.lib ../../lib/.
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbsockutils.exp ../../lib/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbsockutils
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/libnbsockutils* ../../bin/.
	)
ENDIF(WIN32)