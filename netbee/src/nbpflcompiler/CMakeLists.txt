PROJECT(NBPFLCOMPILER)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
INCLUDE(CMakeDependentOption)

SET(NETVM_COMMON_DIR ../nbnetvm/netvm_ir)
SET(NETVM_OPT_DIR ../nbnetvm/jit/opt)

OPTION(
	_DEBUG_SSA
	"turns on ssa debugging"
	OFF
	)
IF(_DEBUG_SSA)
	ADD_DEFINITIONS(-D_DEBUG_SSA)
ENDIF(_DEBUG_SSA)

OPTION(
	_DEBUG_OPTIMIZER
	"turns on optimizer debugging"
	OFF
	)
IF(_DEBUG_OPTIMIZER)
	ADD_DEFINITIONS(-D_DEBUG_OPTIMIZER)
ENDIF(_DEBUG_OPTIMIZER)

OPTION(
	_DEBUG_REASS
	"turns on optimizer debugging"
	OFF
	)
IF(_DEBUG_REASS)
	ADD_DEFINITIONS(-D_DEBUG_REASS)
ENDIF(_DEBUG_REASS)

OPTION(
	_ENABLE_PFLC_PROFILING
	"Turns on NetPFLCompiler profiling data"
	OFF
)

OPTION(
	ENABLE_COMPILER_PROFILING
	"Turns on printing of some informations about the compiler"
	OFF
)

OPTION(
	ENABLE_FIELD_OPT
	"Turns off field compact-optimization"
	ON
)

CMAKE_DEPENDENT_OPTION(
	ENABLE_PFLFRONTEND_PROFILING
	"Turns on some code that measures the execution time of the NetPFL frontend"
	OFF
	"NOT ENABLE_PFLFRONTEND_PROFILING_USEC"
  	OFF
)


CMAKE_DEPENDENT_OPTION(
	ENABLE_PFLFRONTEND_PROFILING_USEC
	"Turns on some code that measures the execution time of the NetPFL frontend (used for Ivano's paper)"
	OFF
	"NOT ENABLE_PFLFRONTEND_PROFILING"
  	OFF
)

CMAKE_DEPENDENT_OPTION(
	ENABLE_PEG_AND_xpFSA_PROFILING
	"Turns on some code that counts the number of states, transitions and self loops of the PEG and the xpFSA"
	OFF
	"NOT ENABLE_PFLFRONTEND_PROFILING AND NOT ENABLE_PFLFRONTEND_PROFILING_USEC"
  	OFF
)

CMAKE_DEPENDENT_OPTION(
  ENABLE_PFLFRONTEND_DBGFILES
  "Enables the generation of DOT files and other files used for debug (forcefully DISABLED if profiling)"
  OFF
  "NOT ENABLE_PFLFRONTEND_PROFILING AND NOT ENABLE_PFLFRONTEND_PROFILING_USEC"
  OFF
)

CMAKE_DEPENDENT_OPTION(
  ENABLE_STATISTICS
  "Enables statistics about the protocols (forcefully DISABLED if profiling)"
  OFF
  "NOT ENABLE_PFLFRONTEND_PROFILING AND NOT ENABLE_PFLFRONTEND_PROFILING_USEC"
  OFF
)

IF(_ENABLE_PFLC_PROFILING)
	ADD_DEFINITIONS(-D_ENABLE_PFLC_PROFILING)
ENDIF(_ENABLE_PFLC_PROFILING)

IF(ENABLE_COMPILER_PROFILING)
	ADD_DEFINITIONS(-DENABLE_COMPILER_PROFILING)
ENDIF(ENABLE_COMPILER_PROFILING)

IF(ENABLE_PFLFRONTEND_PROFILING)
  ADD_DEFINITIONS(-DENABLE_PFLFRONTEND_PROFILING)
ENDIF(ENABLE_PFLFRONTEND_PROFILING)

IF(ENABLE_PFLFRONTEND_PROFILING_USEC)
  ADD_DEFINITIONS(-DENABLE_PFLFRONTEND_PROFILING_USEC)
ENDIF(ENABLE_PFLFRONTEND_PROFILING_USEC)

IF(ENABLE_FIELD_OPT)
  ADD_DEFINITIONS(-DENABLE_FIELD_OPT)
ENDIF(ENABLE_FIELD_OPT)

IF(ENABLE_PEG_AND_xpFSA_PROFILING)
  ADD_DEFINITIONS(-DENABLE_PEG_AND_xpFSA_PROFILING)
ENDIF(ENABLE_PEG_AND_xpFSA_PROFILING)


IF(ENABLE_PFLFRONTEND_DBGFILES)
  ADD_DEFINITIONS(-DENABLE_PFLFRONTEND_DBGFILES)
ENDIF(ENABLE_PFLFRONTEND_DBGFILES)

IF(ENABLE_STATISTICS)
  ADD_DEFINITIONS(-DSTATISTICS)
ENDIF(ENABLE_STATISTICS)

MARK_AS_ADVANCED(
	_DEBUG_SSA _DEBUG_OPTIMIZER _ENABLE_PFLC_PROFILING _DEBUG_REASS
)

ADD_DEFINITIONS(-D_PFL_BUILD)
# Set source files
SET(SOURCES
	../nbee/globals/globals.h
	../nbee/globals/debug.h
	../nbee/globals/debug.c
	../nbee/globals/utils.h
	../nbee/globals/utils.c
	../nbee/globals/profiling-functions.c
	../../include/nbpflcompiler.h
	../../include/nbpflcompiler_exports.h
	${NETVM_COMMON_DIR}/digraph.h
	${NETVM_COMMON_DIR}/irnode.h
	${NETVM_COMMON_DIR}/basicblock.h
	${NETVM_COMMON_DIR}/cfg.h
	adjmatrix.h
	bitmatrix.cpp
	bitmatrix.h
	${NETVM_COMMON_DIR}/bitvectorset.cpp
	${NETVM_COMMON_DIR}/bitvectorset.h
	pflcfg.cpp
	pflcfg.h
	cfg_edge_splitter.h
	${NETVM_COMMON_DIR}/cfgdom.h
	${NETVM_COMMON_DIR}/cfg_ssa.h
	${NETVM_COMMON_DIR}/cfg_ssa.cpp
	cfgvisitor.cpp
	cfgvisitor.h
	cfgwriter.cpp
	cfgwriter.h
	compile.h
	compilerconsts.cpp
	compilerconsts.h
	compunit.cpp
	compunit.h
	compfields.cpp
	compfields.h
	${NETVM_OPT_DIR}/deadcode_elimination_2.h
	${NETVM_OPT_DIR}/optimization_step.h
	defs.h
	dump.cpp
	dump.h
	errors.cpp
	errors.h
	
	filtersubgraph.cpp
	filtersubgraph.h
	
	encapgraph.cpp
	encapgraph.h

	encapfsa.cpp
	encapfsa.h
	
	globalinfo.cpp
	globalinfo.h
	globalsymbols.cpp
	globalsymbols.h
	gramm.y
	hashtable.h
	intsymboltable.h
	ircodegen.cpp
	ircodegen.h
	irlowering.cpp
	irlowering.h
	irnoderegrenamer.cpp
	irnoderegrenamer.h
	iroperators.h
	irops.h
	netilopcodes.h
	netiltracebuilder.cpp
	netiltracebuilder.h
	nbpflcompiler.cpp
	netpflfrontend.cpp
	netpflfrontend.h
	node_translation.h
	node_translation.cpp
	parser.cpp
	parser.hpp
	pdlparser.cpp
	pdlparser.h
	pflexpression.cpp
	pflexpression.h
	mironode.h
	mironode.cpp
	protographshaper.cpp
	protographshaper.h
	protographwriter.cpp
	protographwriter.h
	reassociation_fixer.cpp
	reassociation_fixer.h
	${NETVM_COMMON_DIR}/registers.h
	${NETVM_COMMON_DIR}/registers.cpp
	scanner.cpp
	scanner.l
	statements.cpp
	statements.h
	strsymboltable.h
	strutils.h
	symbols.h
	symbols.cpp
	symtabletree.cpp
	symtabletree.h
	${NETVM_COMMON_DIR}/taggable.h
	${NETVM_COMMON_DIR}/taggable.cpp
	pfl_trace_builder.h
	pfl_trace_builder.cpp
	tree.cpp
	tree.h
	typecheck.cpp
	typecheck.h
	unistd.h
	utils.cpp
	utils.h
	visit.cpp
	visit.h
	
	
#SFT files:
	./sft/sft_defs.h
	./sft/sft_traits.hpp
	./sft/sft_basic_iterator.hpp 
	./sft/sft_generic_set.hpp    
	./sft/sft.hpp
	./sft/sft_writer.hpp
	./sft/sft_tools.hpp
	./sft/digraph_stlplus/containers_fixes.hpp 
	./sft/digraph_stlplus/digraph.tpp
	./sft/digraph_stlplus/exceptions.hpp
	./sft/digraph_stlplus/safe_iterator.hpp
	./sft/digraph_stlplus/digraph.hpp
	./sft/digraph_stlplus/exceptions.cpp
	./sft/digraph_stlplus/safe_iterator.tpp
)


# These files are automatically generated, so we tell cmake not to complain if they are not in the source tree
SET_SOURCE_FILES_PROPERTIES(
	scanner.cpp 
	parser.hpp 
	parser.cpp 
	PROPERTIES GENERATED true
	)

# MACOSX
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	MESSAGE (STATUS "Mac OS X detected!")
	SET (CMAKE_C_FLAGS "-fno-common")
	SET (CMAKE_CXX_FLAGS "-fno-common")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Default directories for include files
INCLUDE_DIRECTORIES(
#Note OM: please leave the src directory in the default include directories, otherwise the fake unistd.h file required by scanner.cpp will not be found on Win32
	${NBPFLCOMPILER_SOURCE_DIR}
	${NBPFLCOMPILER_SOURCE_DIR}/sft
	${NBPFLCOMPILER_SOURCE_DIR}/sft/digraph_stlplus
	${NBPFLCOMPILER_SOURCE_DIR}/../../include
	${NBPFLCOMPILER_SOURCE_DIR}/../nbnetvm/netvm_ir
	${NETVM_OPT_DIR}
)

# Platform-specific definitions
IF(WIN32)
	ADD_DEFINITIONS(
		-D_CRT_SECURE_NO_WARNINGS
		-D_CRT_SECURE_NO_DEPRECATE
		-DWIN32_LEAN_AND_MEAN
		-DPFLCOMPILER_EXPORTS
	)
ENDIF(WIN32)




# Add libraries that are required for linking
IF(WIN32)
#	LINK_LIBRARIES(xerces-c_2.lib pcre.lib nbprotodb.lib)
	LINK_LIBRARIES(pcre.lib nbprotodb.lib)
ENDIF(WIN32)


# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES(${NBPFLCOMPILER_SOURCE_DIR}/../../lib)
	LINK_DIRECTORIES(${NBPFLCOMPILER_SOURCE_DIR}/../../contrib/pcre70/lib)
	LINK_DIRECTORIES(${NBPFLCOMPILER_SOURCE_DIR}/../../../xerces-c/lib)
ENDIF(WIN32)


# Create output library
ADD_LIBRARY(
	nbpflcompiler
	SHARED
	${SOURCES}
)


# Create grammar files
ADD_CUSTOM_COMMAND(
		OUTPUT scanner.cpp parser.cpp parser.hpp
		COMMAND flex -i -P"pfl_" -oscanner.cpp scanner.l
		# flex -d: run in debug mode
		COMMAND bison --verbose -d -p"pfl_" -oparser.cpp gramm.y
		# bison -t: run in debug mode
		# This command is required on MacOSX; it is not requiried in Windows
		DEPENDS scanner.l gramm.y
		WORKING_DIRECTORY ${NBPFLCOMPILER_SOURCE_DIR}
	)

# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbpflcompiler
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbpflcompiler.dll ../../bin/.
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbpflcompiler.lib ../../lib/.
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbpflcompiler.exp ../../lib/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbpflcompiler
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/libnbpflcompiler* ../../bin/.
	)
ENDIF(WIN32)


# Set additional flags that are compiler-dependent
IF(WIN32)
	# In Windows, we have to force "Project -> Properties -> C/C++ -> Language -> "Treat wchar_t as built in type" to "No (/Zc:wchar_t-)"
	SET_TARGET_PROPERTIES(
		nbpflcompiler
		PROPERTIES
		COMPILE_FLAGS "/Zc:wchar_t-"
	)
ENDIF(WIN32)
