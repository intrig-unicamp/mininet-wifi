PROJECT(SVCHELPERTEST)


# Set source files
SET(SOURCES
	svchelpertest.c
)


# Default directories for include files
INCLUDE_DIRECTORIES (
	${SVCHELPERTEST_SOURCE_DIR}
	${SVCHELPERTEST_SOURCE_DIR}/../../../include
)


# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES(${SVCHELPERTEST_SOURCE_DIR}/../../../lib)
ELSE(WIN32)
	LINK_DIRECTORIES(${SVCHELPERTEST_SOURCE_DIR}/../../../bin)
ENDIF(WIN32)


# Platform-specific definitions
IF(WIN32)
	ADD_DEFINITIONS(
		-D_CRT_SECURE_NO_WARNINGS
		-DWIN32_LEAN_AND_MEAN
	)
ENDIF(WIN32)


# Create executable
ADD_EXECUTABLE(
	svchelpertest
	${SOURCES}
)


# Link the executable to the required libraries
IF(WIN32)
	TARGET_LINK_LIBRARIES(
		svchelpertest
		nbsvchelper
	)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(
		svchelpertest
		libnbsvchelper.so
	)
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET svchelpertest
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/svchelpertest.exe ../../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET svchelpertest
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/svchelpertest ../../../bin/.
	)
ENDIF(WIN32)
