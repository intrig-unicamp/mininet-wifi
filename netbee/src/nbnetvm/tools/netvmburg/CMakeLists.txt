# Set requirement for minimum version.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


# Set source files
SET(SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/netvmburg.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/netvmburg.h
	${CMAKE_CURRENT_SOURCE_DIR}/parser.cpp
	)

SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/parser.cpp PROPERTIES GENERATED true)

# There must be some better way of doing this...

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


# Create parser through bison
ADD_CUSTOM_COMMAND(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/parser.cpp
	COMMAND bison
	ARGS -y ${CMAKE_CURRENT_SOURCE_DIR}/netvmburg.y -o "${CMAKE_CURRENT_SOURCE_DIR}/parser.cpp"
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/netvmburg.y
	)


ADD_EXECUTABLE(
	netvmburg

	${SOURCES}	
	)

# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET netvmburg
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/netvmburg.exe ../bin/.
	)
ENDIF(WIN32)
