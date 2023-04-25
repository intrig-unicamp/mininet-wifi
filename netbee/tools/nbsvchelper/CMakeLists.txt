PROJECT(NBSVCHELPER)

# Set source files
SET(SOURCES
	../utils/debug.h
	../utils/debug.c
	../utils/utils.h
	../utils/utils.c
	include/nbsvchelper.h
	src/getopt.c
	src/svchelper.c
	src/svcsignals.h
)

IF(WIN32)
	SET(SOURCES ${SOURCES} src/svcsignals-win.c)
ENDIF(WIN32)

# MACOSX
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	#MESSAGE (STATUS "Mac OS X detected!")
	SET (CMAKE_C_FLAGS "-fno-common")
	SET (CMAKE_CXX_FLAGS "-fno-common")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Default directories for include files
INCLUDE_DIRECTORIES(
	${NBSVCHELPER_SOURCE_DIR}/include
)

# Platform-specific definitions
IF(WIN32)
	ADD_DEFINITIONS(
		-D_CRT_SECURE_NO_WARNINGS
		-D_CRT_SECURE_NO_DEPRECATE
		-DWIN32_LEAN_AND_MEAN
		-DSVCHELPER_EXPORTS
	)
ENDIF(WIN32)


# Create output library
ADD_LIBRARY(
	nbsvchelper
	SHARED
	${SOURCES}
)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbsvchelper
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbsvchelper.dll ../../bin/.
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbsvchelper.lib ../../lib/.
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbsvchelper.exp ../../lib/.
#		COMMAND cp ${NBSVCHELPER_SOURCE_DIR}/include/nbsvchelper.h ../../include/
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbsvchelper
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/libnbsvchelper* ../../bin/.
#		COMMAND cp ${NBSVCHELPER_SOURCE_DIR}/include/nbsvchelper.h ../../include/
	)
ENDIF(WIN32)
