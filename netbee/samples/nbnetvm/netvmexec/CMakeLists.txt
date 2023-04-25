# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(netvmexec)

OPTION(
	ENABLE_MULTICORE
	"Turn on multicore support on Intel machines"
	OFF
)

IF(ENABLE_MULTICORE)
	ADD_DEFINITIONS( -DENABLE_MULTICORE )
ENDIF(ENABLE_MULTICORE)


# Set source files
SET(SOURCES
	netvmexec.c
	utils.h
	utils.c
	multicore.h
	multicore.c
)



SET(NBEE_BASE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${NBEE_BASE_DIR}/include)

LINK_DIRECTORIES(${NBEE_BASE_DIR}/lib)


IF(WIN32)
	INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../../../../WPdPack/Include)
	LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../../../../WPdPack/Lib)
ENDIF(WIN32)



# Platform-specific definitions
IF(WIN32)
	ADD_DEFINITIONS(
		-D_CRT_SECURE_NO_WARNINGS
		-D_CRT_SECURE_NO_DEPRECATE
		-DWIN32_LEAN_AND_MEAN
	)
ENDIF(WIN32)


ADD_EXECUTABLE(
	netvmexec
	${SOURCES}
)


IF(WIN32)
	TARGET_LINK_LIBRARIES(netvmexec nbnetvm wpcap ws2_32)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(netvmexec nbnetvm pcap rt)
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET netvmexec
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/netvmexec.exe ../../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET netvmexec
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/netvmexec ../../../bin/.
	)
ENDIF(WIN32)
