PROJECT(NBSVCINSTALLER)


#The whole project should be built only on Windows platforms
IF(WIN32) 
# Set source files
SET(SOURCES
	svcinstaller.c
	../utils/debug.h
	../utils/debug.c
	../utils/utils.h
	../utils/utils.c
	version.rc
)


# Default directories for include files
INCLUDE_DIRECTORIES (
	${NBSVCINSTALLER_SOURCE_DIR}
	${NBSVCINSTALLER_SOURCE_DIR}/../../include
)


# Platform-specific definitions
ADD_DEFINITIONS(
	-D_CRT_SECURE_NO_WARNINGS
	-D_CRT_SECURE_NO_DEPRECATE
	-DWIN32_LEAN_AND_MEAN
)



# Create executable
ADD_EXECUTABLE(
	nbsvcinstaller
	${SOURCES}
)


# Copy generated files in the right place
ADD_CUSTOM_COMMAND(
	TARGET nbsvcinstaller
	POST_BUILD
	COMMAND cp ${CMAKE_CFG_INTDIR}/nbsvcinstaller.exe ../../bin/.
)

ENDIF(WIN32)
