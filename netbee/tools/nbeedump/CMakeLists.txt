PROJECT(NBEEDUMP)


# Set source files
SET(SOURCES
	configparams.h
	configparams.cpp
	../utils/utils.h
	../utils/utils.c
	nbeedump.cpp
)


# Default directories for include files
INCLUDE_DIRECTORIES (
	${NBEEDUMP_SOURCE_DIR}
	${NBEEDUMP_SOURCE_DIR}/../../include
	${NBEEDUMP_SOURCE_DIR}/../../../WPdPack/Include
)


# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES(${NBEEDUMP_SOURCE_DIR}/../../lib)
	LINK_DIRECTORIES(${NBEEDUMP_SOURCE_DIR}/../../../WPdPack/Lib)
ELSE(WIN32)
	LINK_DIRECTORIES(${NBEEDUMP_SOURCE_DIR}/../../bin)
	LINK_DIRECTORIES(${NBEEDUMP_SOURCE_DIR}/../../lib)
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
	nbeedump
	${SOURCES}
)


# Link the executable to the required libraries
IF(WIN32)
	TARGET_LINK_LIBRARIES(
		nbeedump
		nbee
		nbnetvm
		nbpflcompiler
		wpcap
	)
ELSE(WIN32)
IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	TARGET_LINK_LIBRARIES(
		nbeedump
		nbee
		nbnetvm
		nbpflcompiler
		pcap
		compat
	)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	TARGET_LINK_LIBRARIES(
		nbeedump
		nbee
		nbnetvm
		nbpflcompiler
		pcap
	)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbeedump
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbeedump.exe ../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbeedump
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbeedump ../../bin/.
	)
ENDIF(WIN32)

# Set additional flags that are compiler-dependent
IF(WIN32)
	# In Windows, we have to force "Project -> Properties -> C/C++ -> Language -> "Treat wchar_t as built in type" to "No (/Zc:wchar_t-)"
	SET_TARGET_PROPERTIES(
		nbeedump
		PROPERTIES
		COMPILE_FLAGS "/Zc:wchar_t-"
		LINK_FLAGS "/NODEFAULTLIB:LIBCMT;LIBCMTD"
	)
ENDIF(WIN32)
