CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0)

PROJECT(NBEXTRACTOR)

###################################################################################
#
# OPTIONS for compilation
#
OPTION(
	ENABLE_SQLITE3_DUMP
	"Turn on sqlite3 dump"
	OFF
)

OPTION(
	ENABLE_REDIS_DUMP
	"Turn on redis dump"
	OFF
)

IF(ENABLE_SQLITE3_DUMP)
	SET(SQLITE3_SOURCES
		dump-sqlite.h
		dump-sqlite.cpp
	)
	ADD_DEFINITIONS(
		-DENABLE_SQLITE3
	)

	IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		# FreBSD specific code: let's try to locate the sqlite3 libs

		FIND_PATH(SQLITE3_INCLUDE_DIR sqlite3.h
	  		/usr/local/include
	  		/usr/include
	  		/opt/local/include
	  		/opt/include
		)

		IF(SQLITE3_INCLUDE_DIR)
			FIND_LIBRARY(SQLITE3_LIBRARY_DIR
				NAMES
					sqlite3
				PATHS
					/usr/local/lib
					/usr/lib
					/opt/local/lib
					/opt/lib
			)
		ENDIF(SQLITE3_INCLUDE_DIR)

	ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")

ENDIF(ENABLE_SQLITE3_DUMP)

IF(ENABLE_REDIS_DUMP)
	SET(REDIS_SOURCES
		dump-redis.h
		dump-redis.cpp
	)
	ADD_DEFINITIONS(
		-DENABLE_REDIS
	)

	IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		# FreBSD specific code: let's try to locate the sqlite3 libs

		FIND_PATH(REDIS_INCLUDE_DIR hiredis.h
	  		/usr/local/include/hiredis
	  		/usr/include/hiredis
	  		/opt/local/include/hiredis
	  		/opt/include/hiredis
		)

		IF(REDIS_INCLUDE_DIR)
			FIND_LIBRARY(REDIS_LIBRARY_DIR
				NAMES
					hiredis	
				PATHS
					/usr/local/lib
					/usr/lib
					/opt/local/lib
					/opt/lib
			)
		ENDIF(REDIS_INCLUDE_DIR)

	ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")

	INCLUDE_DIRECTORIES (${REDIS_INCLUDE_DIR})

ENDIF(ENABLE_REDIS_DUMP)


# Set source files
SET(SOURCES
	${SQLITE3_SOURCES}
	${REDIS_SOURCES}
	configparams.h
	configparams.cpp
	anonimize-ip.h
	anonimize-ip.cpp
	fieldprinter.h
	fieldprinter.cpp
	../utils/utils.h
	../utils/utils.c
	nbextractor.cpp
)


# Default directories for include files
INCLUDE_DIRECTORIES (
	${NBEXTRACTOR_SOURCE_DIR}
	${NBEXTRACTOR_SOURCE_DIR}/../../include
)
IF(WIN32)
	INCLUDE_DIRECTORIES (${NBEXTRACTOR_SOURCE_DIR}/../../../WPdPack/Include	)
ENDIF(WIN32)

IF(ENABLE_SQLITE3_DUMP)
	IF(WIN32)
		INCLUDE_DIRECTORIES(${NBEXTRACTOR_SOURCE_DIR}/../../contrib/sqlite3)
	ENDIF(WIN32)
	IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		INCLUDE_DIRECTORIES($SQLITE3_INCLUDE_DIR)
	ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
ENDIF(ENABLE_SQLITE3_DUMP)




# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES(${NBEXTRACTOR_SOURCE_DIR}/../../lib)
	LINK_DIRECTORIES(${NBEXTRACTOR_SOURCE_DIR}/../../../WPdPack/Lib)
ELSE(WIN32)
	LINK_DIRECTORIES(${NBEXTRACTOR_SOURCE_DIR}/../../bin)
	LINK_DIRECTORIES(${NBEXTRACTOR_SOURCE_DIR}/../../lib)
ENDIF(WIN32)

IF(ENABLE_SQLITE3_DUMP)
	IF(WIN32)
		LINK_DIRECTORIES(${NBEXTRACTOR_SOURCE_DIR}/../../contrib/sqlite3)
	ENDIF(WIN32)
	IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		LINK_DIRECTORIES($SQLITE3_LIBRARY_DIR)
	ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
ENDIF(ENABLE_SQLITE3_DUMP)



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
	nbextractor
	${SOURCES}
)



# Link the executable to the required libraries
IF(ENABLE_SQLITE3_DUMP)
	TARGET_LINK_LIBRARIES(
		nbextractor
		sqlite3
	)
ENDIF(ENABLE_SQLITE3_DUMP)

IF(ENABLE_REDIS_DUMP)
	TARGET_LINK_LIBRARIES(
		nbextractor
		${REDIS_LIBRARY_DIR}
	)
ENDIF(ENABLE_REDIS_DUMP)

IF(WIN32)
	TARGET_LINK_LIBRARIES(
		nbextractor
		nbee
		nbnetvm
		nbpflcompiler
		nbsockutils
		wpcap
		ws2_32		#for gethostname()
	)
ELSE(WIN32)
IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	TARGET_LINK_LIBRARIES(
		nbextractor
		nbee
		nbnetvm
		nbpflcompiler
		nbsockutils
		pcap
		compat
	)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	TARGET_LINK_LIBRARIES(
		nbextractor
		nbee
		nbnetvm
		nbpflcompiler
		nbsockutils
		pcap
	)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbextractor
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbextractor.exe ../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbextractor
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbextractor ../../bin/.
	)
ENDIF(WIN32)



