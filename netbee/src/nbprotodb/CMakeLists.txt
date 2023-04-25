PROJECT(NBPROTODB)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# Set source files
SET(SOURCES
	../nbee/globals/globals.h
	../nbee/globals/debug.h
	../nbee/globals/debug.c
	../nbee/globals/utils.h
	../nbee/globals/utils.c
	../nbee/utils/netpdlutils.h
	../nbee/utils/netpdlutils.c

	../../include/nbprotodb.h
	../../include/nbprotodb_defs.h
	../../include/nbprotodb_exports.h
	../../include/nbprotodb_elements_xml.h

	elements_create.h
	elements_create.cpp
	elements_delete.h
	elements_delete.cpp
	elements_organize.h
	elements_organize.cpp
	elements_serialize.h
	elements_serialize.cpp
	lists_organize.h
	lists_organize.cpp
	expressions.h
	expressions.cpp
	expr-scanner.l
	expr-grammar.y
	expr-scanner.c
	expr-grammar.tab.h
	expr-grammar.tab.c
	protodb_globals.h
	protodb.cpp
	sax_handler.h
	sax_handler.cpp
	sax_parser.h
	sax_parser.cpp
)


# These files are automatically generated, so please do not complain if they are not in the source tree
SET_SOURCE_FILES_PROPERTIES(
	expr-scanner.c
	expr-grammar.tab.h
	expr-grammar.tab.c
	PROPERTIES GENERATED true
)

# MACOSX
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	MESSAGE (STATUS "Mac OS X detected!")
	SET (CMAKE_C_FLAGS "-fno-common")
	SET (CMAKE_CXX_FLAGS "-fno-common")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

IF(WIN32)
	SET(XERCES_INCLUDE_DIR ${NBPROTODB_SOURCE_DIR}/../../../xerces-c/include)
	SET(XERCES_LIBRARY_DIR ${NBPROTODB_SOURCE_DIR}/../../../xerces-c/lib)
	SET(PCRE_INCLUDE_DIR ${NBPROTODB_SOURCE_DIR}/../../contrib/pcre70/include)
	SET(PCRE_LIBRARY_DIR ${NBPROTODB_SOURCE_DIR}/../../contrib/pcre70/lib)
	SET(XERCES_LIBRARIES xerces-c_3.lib)
	SET(PCRE_LIBRARIES pcre.lib)
ELSE(WIN32)
	IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		MESSAGE (STATUS "Mac OS X detected!")
		SET (CMAKE_C_FLAGS "-fno-common")
		SET (CMAKE_CXX_FLAGS "-fno-common")
	ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	FIND_PATH(XERCES_INCLUDE_DIR 
	  NAMES
		dom/DOM.hpp
	  PATHS
  		/usr/local/include
		/usr/include
		/opt/local/include
		/opt/include
	  PATH_SUFFIXES xercesc)

	FIND_LIBRARY(XERCES_LIBRARY
	  NAMES
		xerces-c
	  PATHS
		/usr/local/lib
		/usr/lib
		/opt/local/lib
		/opt/lib
	    ${XERCES_INCLUDE_DIR}/../lib
	)


	  IF(XERCES_LIBRARY)
	    SET( XERCES_LIBRARIES ${XERCES_LIBRARY} )
	    SET( XERCES_FOUND "YES" )
	  ENDIF(XERCES_LIBRARY)

	FIND_PATH(PCRE_INCLUDE_DIR pcre.h
  		/usr/local/include
  		/usr/include
  		/opt/local/include
  		/opt/include
	)

	IF(PCRE_INCLUDE_DIR)
		FIND_LIBRARY(PCRE_LIBRARY
		  NAMES
		    pcre
		    pcrecpp
		  PATHS
		    /usr/local/lib
		    /usr/lib
		    /opt/local/lib
		    /opt/lib
		    ${PCRE_INCLUDE_DIR}/../lib
		)


		  IF(PCRE_LIBRARY)
		    SET( PCRE_LIBRARIES ${PCRE_LIBRARY} )
		    SET( PCRE_FOUND "YES" )
		  ENDIF(PCRE_LIBRARY)
	ENDIF(PCRE_INCLUDE_DIR)
ENDIF(WIN32)


# Default directories for include files
INCLUDE_DIRECTORIES(
	${NBPROTODB_SOURCE_DIR}/../../include
	${XERCES_INCLUDE_DIR}
	${XERCES_INCLUDE_DIR}/../
	${PCRE_INCLUDE_DIR}
)


# Platform-specific definitions
IF(WIN32)
	ADD_DEFINITIONS(
		-D_CRT_SECURE_NO_WARNINGS
		-D_CRT_SECURE_NO_DEPRECATE
		-D_CRT_NONSTDC_NO_DEPRECATE 
		-D_SCL_SECURE_NO_DEPRECATE
#		-D_CRTDBG_MAP_ALLOC
		-DWIN32_LEAN_AND_MEAN
		-DYY_NO_UNISTD_H
		-DPROTODB_EXPORTS
	)
ENDIF(WIN32)


# Add libraries that are required for linking
LINK_LIBRARIES(
	${XERCES_LIBRARIES}
	${PCRE_LIBRARIES}
	)


# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES( ${XERCES_LIBRARY_DIR} )
	LINK_DIRECTORIES( ${PCRE_LIBRARY_DIR} )
ENDIF(WIN32)



# Create output library
ADD_LIBRARY(
	nbprotodb
	SHARED
	${SOURCES}
)


# Create grammar files
# PRE_BUILD works only for Visual Studio projects and fall down to PRE_LINK in 
# the other build environments, so we have to work it around
ADD_CUSTOM_COMMAND(
	OUTPUT
	expr-grammar.tab.c
	expr-grammar.tab.h
	expr-scanner.c
	COMMAND bison -p"pdl_" -d expr-grammar.y
	# bison -t: run in debug mode
	COMMAND flex -P"pdl_" -oexpr-scanner.c expr-scanner.l
	# flex -d: run in debug mode
	# This command is required on MacOSX; it is not requiried in Windows
	WORKING_DIRECTORY ${NBPROTODB_SOURCE_DIR}
)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbprotodb
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbprotodb.dll ../../bin/.
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbprotodb.lib ../../lib/.
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbprotodb.exp ../../lib/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbprotodb
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/libnbprotodb* ../../bin/.
	)
ENDIF(WIN32)

