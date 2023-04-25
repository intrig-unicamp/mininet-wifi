# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


PROJECT(NETVMCOMPILER)


# Set source files
SET(SOURCES
	netvmcompiler.c
)


# Default directories for include files
INCLUDE_DIRECTORIES (
	${NETVMCOMPILER_SOURCE_DIR}
	${NETVMCOMPILER_SOURCE_DIR}/../../../include
)


# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES(${NETVMCOMPILER_SOURCE_DIR}/../../../lib)
ELSE(WIN32)
	LINK_DIRECTORIES(${NETVMCOMPILER_SOURCE_DIR}/../../../bin)
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
	netvmcompiler
	${SOURCES}
)


TARGET_LINK_LIBRARIES(
	netvmcompiler
	nbnetvm
)



# Link the executable to the required libraries
IF(WIN32)
	TARGET_LINK_LIBRARIES(
		netvmcompiler
		nbnetvm
	)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(
		netvmcompiler
		nbnetvm
	)
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET netvmcompiler
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/netvmcompiler.exe ../../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET netvmcompiler
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/netvmcompiler ../../../bin/.
	)
ENDIF(WIN32)
