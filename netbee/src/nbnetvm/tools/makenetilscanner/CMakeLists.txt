# Set requirement for minimum version.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


# Set source files
SET(SOURCES
  makenetilscanner.c
)


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../../include)
 
# Platform-specific definitions
IF(WIN32)
	ADD_DEFINITIONS(
		-D_CRT_SECURE_NO_WARNINGS
		-D_CRT_SECURE_NO_DEPRECATE
		-D_SCL_SECURE_NO_DEPRECATE
#		-D_CRTDBG_MAP_ALLOC
		-DWIN32_LEAN_AND_MEAN
    -D__STDC__
	)
ENDIF(WIN32)


ADD_EXECUTABLE(
	makenetilscanner

  ${SOURCES}	
)

# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET makenetilscanner
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/makenetilscanner.exe ../bin/.
	)
ENDIF(WIN32)
