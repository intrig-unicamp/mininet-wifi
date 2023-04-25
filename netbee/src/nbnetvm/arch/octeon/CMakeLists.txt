PROJECT(nbnetvm)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# definition of helpful directories
SET(NETVM_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
SET(NETVMOCTEON_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
SET(NETVM_SRC_DIR ${NETVM_BASE_DIR})
SET(NETVM_COMMON_DIR ${NETVM_BASE_DIR}/netvm_ir)
SET(NETVM_ASM_DIR ${NETVM_SRC_DIR}/assembler)


INCLUDE_DIRECTORIES(Assembler ${NETVM_BASE_DIR}/../../include ${NETVM_SRC_DIR} ${NETVM_BASE_DIR}/netvm_ir)


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


#####################################################################################
#
# Coprocessors
#

# 3 for lookup, one for regex, one for string matching
SET(AVAILABLE_COPROS_NO 5)


####################################################################################
#
# String matching
#

INCLUDE_DIRECTORIES(
	${NETVM_SRC_DIR}/arch/generic/coprocessors/
)
	
SET(NETVM_SRCS ${NETVM_SRCS} 
	${NETVM_SRC_DIR}/arch/octeon/coprocessors/stringmatching2.c
	${NETVM_SRC_DIR}/arch/generic/coprocessors/acsmx2.c
	${NETVM_SRC_DIR}/arch/generic/coprocessors/acsmx2.h
)

SET(NETVM_HDRS ${NETVM_HDRS} 
	${NETVM_SRC_DIR}/arch/octeon/coprocessors/stringmatching.h
)
	

########################################################################
#
# Regexp
#

SET(NETVM_SRCS ${NETVM_SRCS} 
	${NETVM_SRC_DIR}/arch/octeon/coprocessors/regexp2.c
)




#################################################################################
#
# files for all runtime backend
#

SET(NETVM_SRCS ${NETVM_SRCS}
	${NETVM_SRC_DIR}/assembler/dump.c
	${NETVM_SRC_DIR}/assembler/scanner.c
	${NETVM_SRC_DIR}/assembler/nvm_gramm.tab.c
	${NETVM_SRC_DIR}/assembler/netil_assembler.c
	${NETVM_SRC_DIR}/assembler/hashtable.c
	${NETVM_SRC_DIR}/helpers.c
	${NETVM_SRC_DIR}/rt_environment.c
	${NETVM_SRC_DIR}/utils/slinkedlst.c
	${NETVM_SRC_DIR}/utils/dlinkedlst.c
	${NETVM_SRC_DIR}/utils/hashtbl.c
	${NETVM_SRC_DIR}/utils/hashtblgen.c
	${NETVM_SRC_DIR}/vm_application.c
	${NETVM_SRC_DIR}/coprocessor.c
	${NETVM_SRC_DIR}/arch/generic/coprocessors/lookup.c
	${NETVM_SRC_DIR}/arch/generic/coprocessors/lookup-new.c
	${NETVM_SRC_DIR}/arch/generic/coprocessors/lookup_ex.c
	${NETVM_ASM_DIR}/nvm_gramm.y
	${NETVM_ASM_DIR}/scanner-template.l
# The following is dynamically generated from the scanner-template.l above
#	${NETVM_ASM_DIR}/scanner.l
# Files for profiling
	${NETVM_SRC_DIR}/netvmprofiling.cpp
	${NETVM_SRC_DIR}/../nbee/globals/profiling-functions.c
	${NETVM_SRC_DIR}/../nbee/globals/profiling.cpp
# Files for debug and misc
	${NETVM_SRC_DIR}/../nbee/globals/debug.c
	${NETVM_SRC_DIR}/../nbee/globals/utils.c
#Opcode tables
	${NETVM_SRC_DIR}/opcodes.txt
	${NETVM_SRC_DIR}/newopcodes.txt
	${NETVM_SRC_DIR}/opcodestable.txt
	)

SET(NETVM_HDRS
	${NETVM_SRC_DIR}/../../include/nbnetvm.h
	${NETVM_SRC_DIR}/helpers.h
	${NETVM_SRC_DIR}/netvm_bytecode.h
	${NETVM_SRC_DIR}/rt_environment.h
	${NETVM_SRC_DIR}/coprocessor.h
	${NETVM_SRC_DIR}/int_structs.h
	${NETVM_SRC_DIR}/utils/lists.h
	${NETVM_SRC_DIR}/utils/hashtbl.h
	${NETVM_SRC_DIR}/netvm_ir/bitmatrix.h
	${NETVM_SRC_DIR}/assembler/hashtable.h
	${NETVM_SRC_DIR}/assembler/netvm_dump.h
# Files for profiling
	${NETVM_SRC_DIR}/netvmprofiling.h
	${NETVM_BASE_DIR}/../nbee/globals/profiling.h
	${NETVM_BASE_DIR}/../nbee/globals/profiling-functions.h
# Files for debug
	${NETVM_BASE_DIR}/../nbee/globals/debug.h
	)


##################################################################################
#
# files for octeon runtime
#


#EXEC_PROGRAM(make ${NETVM_SRC_DIR}/arch/octeon)

#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#workaroud for linking static executable
#see http://www.cmake.org/Bug/view.php?id=5087

FOREACH(type SHARED_LIBRARY SHARED_MODULE EXE)
	SET(CMAKE_${type}_LINK_STATIC_C_FLAGS "")
	SET(CMAKE_${type}_LINK_DYNAMIC_C_FLAGS "")
ENDFOREACH(type)
#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

SET(OCTEON_ROOT $ENV{OCTEON_ROOT})
	
INCLUDE_DIRECTORIES(
	${OCTEON_ROOT}/target/include
	${NETVM_SRC_DIR}/arch/octeon/
	${NETVM_SRC_DIR}/arch/octeon/config
)

ADD_DEFINITIONS(-DARCH_RUNTIME_OCTEON)
ADD_DEFINITIONS(-DWORDS_BIGENDIAN)

LINK_DIRECTORIES(${OCTEON_ROOT}/tools-gcc-4.1/mips64-octeon-linux-gnu/sys-root/usr/lib64/)
	
SET (CMAKE_AR "mips64-octeon-linux-gnu-ar")
SET (CMAKE_RANLIB "mips64-octeon-linux-gnu-ranlib")
SET (CMAKE_EXE_LINKER_FLAGS 
	 " -mabi=64 -lrt -static -Wl,-T,${OCTEON_ROOT}/target/lib/cvmx-shared-linux.ld")
SET (CMAKE_C_COMPILER mips64-octeon-linux-gnu-gcc)
SET (CMAKE_CXX_COMPILER mips64-octeon-linux-gnu-g++)

SET(NETVM_SRCS ${NETVM_SRCS}
	${NETVM_SRC_DIR}/arch/octeon/octeon_runtime.c
	${NETVM_SRC_DIR}/arch/octeon/octeon_interpreter.c
	${NETVM_SRC_DIR}/arch/octeon/octeon_coprocessors.c
	${OCTEON_EXEC_LIB}
)

SET(NETVM_HDRS ${NETVM_HDRS}
	${NETVM_SRC_DIR}/arch/octeon/arch.h
	${NETVM_SRC_DIR}/arch/octeon/octeon_runtime.h
	${NETVM_SRC_DIR}/arch/octeon/octeon_interpreter.h
	${NETVM_SRC_DIR}/arch/octeon/octeon_coprocessors.h
)



###############################################################################
#
# target defintion for runtime
#

ADD_LIBRARY(
  nbnetvm
  STATIC
  ${NETVM_SRCS} ${NETVM_HDRS}
)

# the following three commands are needed to build the Cavium Octeon runtime
ADD_CUSTOM_TARGET(
	cvmx
	COMMAND make -f Makefile.cvmx
	DEPENDS ${NETVM_SRC_DIR}/arch/octeon/Makefile.cvmx ${NETVM_SRC_DIR}/arch/octeon/config/executive-config.h
	WORKING_DIRECTORY ${NETVM_SRC_DIR}/arch/octeon
	COMMENT "Generating octeon runtime library libcvmx.a (don't panic... it may take some minutes)"
)

ADD_CUSTOM_COMMAND(
	TARGET cvmx
	POST_BUILD
	COMMAND cp ${NETVM_SRC_DIR}/arch/octeon/obj-linux_64/libcvmx.a ${NETVM_SRC_DIR}/../../lib/.
)

ADD_DEPENDENCIES(nbnetvm cvmx)

TARGET_LINK_LIBRARIES (
  nbnetvm
  rt
)


ADD_DEFINITIONS(-DAVAILABLE_COPROS_NO=${AVAILABLE_COPROS_NO})

# Win32-specific definitions
# Not really used since this project has bto be compiled on Linux, but useful
# in order to test if everythinh compiles (more or less) on Windows, which is
# Fulvio's main platform
IF(WIN32)
	ADD_DEFINITIONS(
		-D_CRT_SECURE_NO_WARNINGS
		-D_CRT_SECURE_NO_DEPRECATE
		-D_SCL_SECURE_NO_DEPRECATE
		-DWIN32_LEAN_AND_MEAN
		-DNETVM_EXPORTS
		-D__STDC__
	)
ENDIF(WIN32)



# Copy generated files in the right place
ADD_CUSTOM_COMMAND(
	TARGET nbnetvm
	POST_BUILD
	COMMAND cp ${NETVMOCTEON_BASE_DIR}/libnbnetvm* ${NETVM_SRC_DIR}/../../lib/.
	COMMAND cp ${NETVMOCTEON_BASE_DIR}/libnbnetvm* ${NETVM_SRC_DIR}/../../bin/.
)
