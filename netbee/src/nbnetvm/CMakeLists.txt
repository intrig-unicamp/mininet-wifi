PROJECT(nbnetvm)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# definition of helpful directories
SET(NETVM_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
#SET(TOOLS_OUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools/bin)
SET(NETVM_SRC_DIR ${NETVM_BASE_DIR})
SET(NETVM_COMMON_DIR ${NETVM_BASE_DIR}/netvm_ir)
SET(NETVM_ASM_DIR ${NETVM_SRC_DIR}/assembler)
SET(NETVM_JIT_DIR ${NETVM_SRC_DIR}/jit)


# MACOSX
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#	MESSAGE (STATUS "Mac OS X detected!")
	SET (CMAKE_C_FLAGS "-fno-common")
	SET (CMAKE_CXX_FLAGS "-fno-common")
	ADD_DEFINITIONS(-D__APPLE__)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


INCLUDE_DIRECTORIES(Assembler ${NETVM_BASE_DIR}/../../include ${NETVM_SRC_DIR} ${NETVM_BASE_DIR}/netvm_ir)

#IF (LIBPCAP)
#	MESSAGE(STATUS "Found Pcap library: ${LIBPCAP}")
#	ADD_DEFINITIONS(-DHAVE_PCAP)
#ENDIF (LIBPCAP)


IF (WIN32)
	INCLUDE_DIRECTORIES(${NETVM_BASE_DIR}/../../../WPdPack/Include)
	INCLUDE_DIRECTORIES(${NETVM_BASE_DIR}/../../contrib/pcre70/include)
	LINK_DIRECTORIES(${NETVM_BASE_DIR}/../../../WPdPack/Lib)
	LINK_DIRECTORIES(${NETVM_BASE_DIR}/../../contrib/pcre70/lib)
ELSE(WIN32)
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
		    /lib
		    /opt/local/lib
		    /opt/lib
		    ${PCRE_INCLUDE_DIR}/../lib
		)


		  IF(PCRE_LIBRARY)
		    SET( PCRE_LIBRARIES ${PCRE_LIBRARY} )
		    SET( PCRE_FOUND "YES" )
		  ENDIF(PCRE_LIBRARY)
	ENDIF(PCRE_INCLUDE_DIR)
ENDIF (WIN32)


###################################################################################
#
# OPTIONS for compilation
#

# Choose enabled backend
OPTION(
	ENABLE_X86_BACKEND
	"Turn on the Intel x86 backend (32 bits) for NetVM"
	OFF
)
OPTION(
	ENABLE_X64_BACKEND
	"Turn on the Intel x64 backend (64 bits) for NetVM"
	OFF
)
OPTION(
	ENABLE_X11_BACKEND
	"Turn on the Xelerated x11 backend for NetVM"
	OFF
)
OPTION(
	ENABLE_OCTEON_BACKEND
	"Turn on the Cavium Octeon backend for NetVM"
	OFF
)
OPTION(
	ENABLE_OCTEONC_BACKEND
	"Turn on the Cavium Octeon backend for NetVM in C"
	OFF
)

IF(ENABLE_X86_BACKEND)
	ADD_DEFINITIONS(-DENABLE_X86_BACKEND)
ENDIF(ENABLE_X86_BACKEND)
IF(ENABLE_X64_BACKEND)
	ADD_DEFINITIONS(-DENABLE_X64_BACKEND)
ENDIF(ENABLE_X64_BACKEND)
IF(ENABLE_X11_BACKEND)
	ADD_DEFINITIONS(-DENABLE_X11_BACKEND)
ENDIF(ENABLE_X11_BACKEND)
IF(ENABLE_OCTEON_BACKEND)
	ADD_DEFINITIONS(-DENABLE_OCTEON_BACKEND)
ENDIF(ENABLE_OCTEON_BACKEND)
IF(ENABLE_OCTEONC_BACKEND)
	ADD_DEFINITIONS(-DENABLE_OCTEONC_BACKEND)
ENDIF(ENABLE_OCTEONC_BACKEND)

# Choose profiling options
OPTION(
	ENABLE_COMPILER_PROFILING
	"Turn on printing of some informations about the compiler"
	OFF
)

OPTION(
	ENABLE_CODE_PROFILING
	"Turn on the code that allows to measure the execution time of the NetVM code (jit or native code)"
	OFF
)

OPTION(
	ENABLE_COUNTERS_PROFILING
	"Turn on COUNTERS PROFILING"
	OFF
)

OPTION(
	ENABLE_PROFILING_COUNTERS
	"Enable RTE profiling counters"
	OFF
)

OPTION(ENABLE_JIT_PROFILING
	"Enable profiling of the jit"
	OFF
)

OPTION(ENABLE_CODEVERIFY_PROFILING
	"Enable profiling of the code verification (in the interpreter)"
	# Please remember that this code works only when the interpreter is used
	OFF
)

# Enabling logging
OPTION(
	ENABLE_INTERNAL_LOGGING
	"Turn on some additional logging (e.g., dump of the executed netil code) on file for debuging purposes"
	OFF
)


IF(ENABLE_COMPILER_PROFILING)
	ADD_DEFINITIONS(-DENABLE_COMPILER_PROFILING)
	ADD_DEFINITIONS(-DENABLE_PROFILING)
ENDIF(ENABLE_COMPILER_PROFILING)

IF(ENABLE_CODE_PROFILING)
	ADD_DEFINITIONS(-DCODE_PROFILING)
	ADD_DEFINITIONS(-DENABLE_PROFILING)
ENDIF(ENABLE_CODE_PROFILING)

IF(ENABLE_COUNTERS_PROFILING)
	ADD_DEFINITIONS(-DCODE_PROFILING)
	ADD_DEFINITIONS(-DCOUNTERS_PROFILING)
	ADD_DEFINITIONS(-DENABLE_PROFILING)
ENDIF(ENABLE_COUNTERS_PROFILING)

IF(ENABLE_PROFILING_COUNTERS)
	ADD_DEFINITIONS(-DRTE_PROFILE_COUNTERS)
	ADD_DEFINITIONS(-DENABLE_PROFILING)
ENDIF(ENABLE_PROFILING_COUNTERS)

IF(ENABLE_DYNAMIC_PROFILING)
	ADD_DEFINITIONS(-DRTE_DYNAMIC_PROFILE)
	ADD_DEFINITIONS(-DENABLE_PROFILING)
ENDIF(ENABLE_DYNAMIC_PROFILING)

IF(ENABLE_JIT_PROFILING)
	ADD_DEFINITIONS(-DJIT_RTE_PROFILE_COUNTERS)
	ADD_DEFINITIONS(-DENABLE_PROFILING)
ENDIF(ENABLE_JIT_PROFILING)

IF(ENABLE_CODEVERIFY_PROFILING)
	ADD_DEFINITIONS(-DCODEVERIFY_PROFILING)
	ADD_DEFINITIONS(-DENABLE_PROFILING)
ENDIF(ENABLE_CODEVERIFY_PROFILING)

IF(ENABLE_INTERNAL_LOGGING)
	ADD_DEFINITIONS(-DENABLE_NETVM_LOGGING)
ENDIF(ENABLE_INTERNAL_LOGGING)


# Choose debug options
OPTION(
	_DEBUG_JIT_IF
	"Turns on jit interface debugging"
	OFF
)
IF(_DEBUG_JIT_IF)
	ADD_DEFINITIONS(-D_DEBUG_JIT_IF)
ENDIF(_DEBUG_JIT_IF)

OPTION(
	_DEBUG_CFG_BUILD
	"Turns on cfg building debugging"
	OFF
)
IF(_DEBUG_CFG_BUILD)
	ADD_DEFINITIONS(-D_DEBUG_CFG_BUILD)
ENDIF(_DEBUG_CFG_BUILD)

OPTION(
	_DEBUG_SSA
	"turns on ssa debugging"
	OFF
	)
IF(_DEBUG_SSA)
	ADD_DEFINITIONS(-D_DEBUG_SSA)
ENDIF(_DEBUG_SSA)

OPTION(
	_DEBUG_BCHECK
	"turns on bcheck remove debugging"
	OFF
	)
IF(_DEBUG_BCHECK)
	ADD_DEFINITIONS(-D_DEBUG_BCHECK)
ENDIF(_DEBUG_BCHECK)

OPTION(
	_DEBUG_SWITCH_LOWERING
	"turns on switch lowering debugging"
	OFF
	)
IF(_DEBUG_SWITCH_LOWERING)
	ADD_DEFINITIONS(-D_DEBUG_SWITCH_LOWERING)
ENDIF(_DEBUG_SWITCH_LOWERING)

OPTION(
	_DEBUG_LIVENESS
	"turns on livness debugging"
	OFF
	)
IF(_DEBUG_LIVENESS)
	ADD_DEFINITIONS(-D_DEBUG_LIVENESS)
ENDIF(_DEBUG_LIVENESS)

OPTION(
	_DEBUG_REG_ALLOC
	"turns on register allocation debugging"
	OFF
	)
IF(_DEBUG_REG_ALLOC)
	ADD_DEFINITIONS(-D_DEBUG_REG_ALLOC)
ENDIF(_DEBUG_REG_ALLOC)

OPTION(
	_DEBUG_OPTIMIZER
	"turns on optimizer debugging"
	OFF
	)
IF(_DEBUG_OPTIMIZER)
	ADD_DEFINITIONS(-D_DEBUG_OPTIMIZER)
ENDIF(_DEBUG_OPTIMIZER)

OPTION(
	_DEBUG_ALGEBRAIC_SIMPLIFICATION
	"turns on algebraic simplification debugging"
	OFF
	)
IF(_DEBUG_ALGEBRAIC_SIMPLIFICATION)
	ADD_DEFINITIONS(-D_DEBUG_ALGEBRAIC_SIMPLIFICATION)
ENDIF(_DEBUG_ALGEBRAIC_SIMPLIFICATION)

OPTION(
	_DEBUG_DEAD_CODE_ELIMINATION
	"turns on dead code elimination debugging"
	OFF
	)
IF(_DEBUG_DEAD_CODE_ELIMINATION)
	ADD_DEFINITIONS(-D_DEBUG_DEAD_CODE_ELIMINATION)
ENDIF(_DEBUG_DEAD_CODE_ELIMINATION)

OPTION(
	_DEBUG_CONTROL_FLOW_SIMPLIFICATION	
	"turns on control flow simplification debugging"
	OFF
	)
IF(_DEBUG_CONTROL_FLOW_SIMPLIFICATION)
	ADD_DEFINITIONS(-D_DEBUG_CONTROL_FLOW_SIMPLIFICATION)
ENDIF(_DEBUG_CONTROL_FLOW_SIMPLIFICATION)

OPTION(
	_DEBUG_REDISTRIBUTION
	"turns on redistribution debugging"
	OFF
	)
IF(_DEBUG_REDISTRIBUTION)
	ADD_DEFINITIONS(-D_DEBUG_REDISTRIBUTION)
ENDIF(_DEBUG_REDISTRIBUTION)


OPTION(
	_DEBUG_CONSTANT_PROPAGATION
	"turns on constant propagation debugging"
	OFF
	)
IF(_DEBUG_CONSTANT_PROPAGATION)
	ADD_DEFINITIONS(-D_DEBUG_CONSTANT_PROPAGATION)
ENDIF(_DEBUG_CONSTANT_PROPAGATION)

OPTION(
	_DEBUG_REASS
	"turns on optimizer debugging"
	OFF
	)
IF(_DEBUG_REASS)
	ADD_DEFINITIONS(-D_DEBUG_REASS)
ENDIF(_DEBUG_REASS)

OPTION(
	_DEBUG_SPILL
	"Turns on Spill debugging"
	OFF
	)
IF(_DEBUG_SPILL)
	ADD_DEFINITIONS(-D_DEBUG_SPILL)
ENDIF(_DEBUG_SPILL)


OPTION(
	_DEBUG_COMMON_BACKEND
	"Turns on the common backend debug"
	OFF
	)
IF(_DEBUG_COMMON_BACKEND)
	ADD_DEFINITIONS(-D_DEBUG_COMMON_BACKEND)
ENDIF(_DEBUG_COMMON_BACKEND)

OPTION(
	_DEBUG_X64_BACKEND
	"Turns on x64 backend debug"
	OFF
	)
IF(_DEBUG_X64_BACKEND)
	ADD_DEFINITIONS(-D_DEBUG_X64_BACKEND)
ENDIF(_DEBUG_X64_BACKEND)

OPTION(
	_DEBUG_OPTIMIZATION_ALGORITHMS
	"Turns on the optimization algorithms debug"
	OFF
	)
IF(_DEBUG_OPTIMIZATION_ALGORITHMS)
	ADD_DEFINITIONS(-D_DEBUG_OPTIMIZATION_ALGORITHMS)
ENDIF(_DEBUG_OPTIMIZATION_ALGORITHMS)

INCLUDE_DIRECTORIES(${NETVM_JIT_DIR})
ADD_DEFINITIONS(-DUSE_JIT)
ADD_DEFINITIONS(-D_JIT_BUILD)


MARK_AS_ADVANCED(
	_DEBUG_CFG_BUILD 
	_DEBUG_SPILL 
	_DEBUG_REG_ALLOC 
	_DEBUG_LIVENESS 
	_DEBUG_SSA 
	_DEBUG_BCHECK 
	_DEBUG_OPTIMIZER 
	_DEBUG_ALGEBRAIC_SIMPLIFICATION
	_DEBUG_CONSTANT_PROPAGATION
	_DEBUG_DEAD_CODE_ELIMINATION
	_DEBUG_CONTROL_FLOW_SIMPLIFICATION
	_DEBUG_REDISTRIBUTION
	_DEBUG_JIT_IF
	_DEBUG_REASS
	_DEBUG_SWITCH_LOWERING
)

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

SET(NETVM_SRCS ${NETVM_SRCS} 
	${NETVM_SRC_DIR}/arch/generic/coprocessors/stringmatching.c
	${NETVM_SRC_DIR}/arch/generic/coprocessors/acsmx2.c
	${NETVM_SRC_DIR}/arch/generic/coprocessors/acsmx2.h
)



########################################################################
#
# Regexp
#

SET(NETVM_SRCS ${NETVM_SRCS} 
	${NETVM_SRC_DIR}/arch/generic/coprocessors/regexp.c
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



#################################################################################
#
# files for generic runtime backend
#

SET(NETVM_SRCS ${NETVM_SRCS}
	${NETVM_SRC_DIR}/arch/generic/generic_runtime.c
	${NETVM_SRC_DIR}/arch/generic/generic_interpreter.c
	${NETVM_SRC_DIR}/arch/generic/coprocessors/coprocessors_main.c
	${NETVM_JIT_DIR}/bytecode_analyse.cpp
	${NETVM_JIT_DIR}/bytecode_segments.cpp
	${NETVM_JIT_DIR}/comperror.cpp
	${NETVM_JIT_DIR}/codetable.c
	${NETVM_JIT_DIR}/jit_interface.cpp
	${NETVM_JIT_DIR}/iltranslator.cpp
	${NETVM_JIT_DIR}/mem_translator.cpp
	${NETVM_JIT_DIR}/op_size.cpp
	${NETVM_COMMON_DIR}/bitvectorset.cpp
	${NETVM_JIT_DIR}/cfg_edge_splitter.cpp
	${NETVM_JIT_DIR}/genericbackend.cpp
	${NETVM_COMMON_DIR}/cfg_ssa.cpp
	${NETVM_JIT_DIR}/mirnode.cpp
	${NETVM_COMMON_DIR}/registers.cpp
	${NETVM_JIT_DIR}/copy_folding.cpp
	${NETVM_JIT_DIR}/opt/reassociation.cpp
	${NETVM_COMMON_DIR}/taggable.cpp
	${NETVM_JIT_DIR}/application.cpp
	${NETVM_JIT_DIR}/switch_lowering.cpp
	${NETVM_JIT_DIR}/opt/constant_folding.cpp
	${NETVM_JIT_DIR}/opt/constant_propagation.cpp
	${NETVM_JIT_DIR}/opt/controlflow_simplification.cpp
	${NETVM_JIT_DIR}/opt/redistribution.cpp
	${NETVM_JIT_DIR}/opt/copy_propagation.cpp
	${NETVM_JIT_DIR}/opt/optimizer_statistics.cpp
)

SET(NETVM_HDRS ${NETVM_HDRS}
	${NETVM_SRC_DIR}/arch/generic/arch.h
	${NETVM_SRC_DIR}/arch/generic/generic_runtime.h
	${NETVM_SRC_DIR}/arch/generic/generic_interpreter.h
	${NETVM_COMMON_DIR}/digraph.h
	${NETVM_COMMON_DIR}/basicblock.h
	${NETVM_COMMON_DIR}/bitvectorset.h
	${NETVM_JIT_DIR}/bytecode_analyse.h
	${NETVM_JIT_DIR}/bytecode_segments.h
	${NETVM_JIT_DIR}/cfg_builder.h
	${NETVM_JIT_DIR}/cfg_loop_analyzer.h
	${NETVM_JIT_DIR}/cfg_bytecode_builder.h
	${NETVM_COMMON_DIR}/cfgdom.h
	${NETVM_COMMON_DIR}/cfg.h
	${NETVM_COMMON_DIR}/cfg_liveness.h
	${NETVM_JIT_DIR}/cfg_printer.h
	${NETVM_COMMON_DIR}/cfg_ssa.h
	${NETVM_JIT_DIR}/codetable.h
	${NETVM_JIT_DIR}/comperror.h
	${NETVM_JIT_DIR}/iltranslator.h
	${NETVM_COMMON_DIR}/irnode.h
	${NETVM_JIT_DIR}/jit_interface.h
	${NETVM_JIT_DIR}/jit_internals.h
	${NETVM_JIT_DIR}/mem_translator.h
	${NETVM_JIT_DIR}/mirnode.h
	${NETVM_JIT_DIR}/netvmjitglobals.h
	${NETVM_JIT_DIR}/visitors.h
	${NETVM_JIT_DIR}/genericbackend.h
	${NETVM_COMMON_DIR}/register_mapping.h
	${NETVM_JIT_DIR}/op_size.h
	${NETVM_JIT_DIR}/copy_folding.h
	${NETVM_JIT_DIR}/cfg_copy.h
	${NETVM_JIT_DIR}/insselector.h
	${NETVM_COMMON_DIR}/registers.h
	${NETVM_JIT_DIR}/cfg_edge_splitter.h
	${NETVM_JIT_DIR}/offsets.h
	${NETVM_COMMON_DIR}/taggable.h
	${NETVM_JIT_DIR}/application.h
	${NETVM_COMMON_DIR}/ssa_graph.h
	${NETVM_JIT_DIR}/gc_regalloc.h
	${NETVM_JIT_DIR}/switch_lowering.h
	${NETVM_JIT_DIR}/opt/deadcode_elimination_2.h
	${NETVM_JIT_DIR}/opt/constant_folding.h
	${NETVM_JIT_DIR}/opt/nvm_optimizer.h
	${NETVM_JIT_DIR}/opt/optimization_step.h
	${NETVM_JIT_DIR}/opt/constant_propagation.h
	${NETVM_JIT_DIR}/opt/deadcode_elimination.h
	${NETVM_JIT_DIR}/opt/controlflow_simplification.h
	${NETVM_JIT_DIR}/opt/redistribution.h
	${NETVM_JIT_DIR}/opt/reassociation.h
	${NETVM_COMMON_DIR}/tracebuilder.h
	${NETVM_JIT_DIR}/opt/copy_propagation.h
	${NETVM_JIT_DIR}/opt/optimizer_statistics.h
	${NETVM_JIT_DIR}/opt/bcheck_remove.h
	${NETVM_ASM_DIR}/compiler.h
)

ADD_DEFINITIONS(-DHAVE_PCAP)

SET_SOURCE_FILES_PROPERTIES(
	${NETVM_ASM_DIR}/scanner.l
	${NETVM_ASM_DIR}/scanner.c
	${NETVM_ASM_DIR}/nvm_gramm.tab.c
	${NETVM_ASM_DIR}/nvm_gramm.tab.h
	${NETVM_SRC_DIR}/opcodestable.txt
	PROPERTIES GENERATED true
)


###########################################################################################
#
# file for compilers backends with generic runtime
#

#
# IA32
#

IF(ENABLE_X86_BACKEND)
	SET(X86DIS_SRC_DIR ${NETVM_JIT_DIR}/x86_x64_disasm )
	SET(NETVM_SRCS ${NETVM_SRCS}
		${NETVM_JIT_DIR}/ia32/x86-asm.cpp
		${NETVM_JIT_DIR}/ia32/x86-emit.cpp
		${NETVM_JIT_DIR}/ia32/x86-backend.cpp
		${NETVM_JIT_DIR}/ia32/inssel-ia32.cpp
		${NETVM_JIT_DIR}/ia32/x86-regalloc.cpp
		${NETVM_JIT_DIR}/ia32/x86_switch_lowering.cpp
		${NETVM_JIT_DIR}/ia32/x86_counters.cpp
		${NETVM_JIT_DIR}/ia32/inssel-ia32.brg
	)
	
	SET(NETVM_HDRS ${NETVM_HDRS}
		${NETVM_JIT_DIR}/ia32/x86-asm.h
		${NETVM_JIT_DIR}/ia32/x86-backend.h
		${NETVM_JIT_DIR}/ia32/x86-emit.h
		${NETVM_JIT_DIR}/ia32/inssel-ia32.h
		${NETVM_JIT_DIR}/ia32/x86-regalloc.h
		${NETVM_JIT_DIR}/ia32/x86_switch_lowering.h
		${NETVM_JIT_DIR}/ia32/x86_counters.h
	)
	
	
	# Set x86 disassembler source files
	SET(X86DIS_SRCS
		#${X86DIS_SRC_DIR}/decode.c
		#${X86DIS_SRC_DIR}/input.c
		#${X86DIS_SRC_DIR}/mnemonics.c
		#${X86DIS_SRC_DIR}/opcmap.c
		#${X86DIS_SRC_DIR}/syn-att.c
		#${X86DIS_SRC_DIR}/syn-intel.c
		#${X86DIS_SRC_DIR}/syn.c
		#${X86DIS_SRC_DIR}/udis86.c
		
		${X86DIS_SRC_DIR}/decode.c
		${X86DIS_SRC_DIR}/input.c
		${X86DIS_SRC_DIR}/itab.c
		${X86DIS_SRC_DIR}/syn.c
		${X86DIS_SRC_DIR}/syn-att.c
		${X86DIS_SRC_DIR}/syn-intel.c
		${X86DIS_SRC_DIR}/udis86.c
	)
	 
	SET(X86DIS_HDRS
		#${X86DIS_SRC_DIR}/extern.h
		#${X86DIS_SRC_DIR}/input.h
		#${X86DIS_SRC_DIR}/mnemonics.h
		#${X86DIS_SRC_DIR}/opcmap.h
		#${X86DIS_SRC_DIR}/syn.h
		#${X86DIS_SRC_DIR}/dis_types.h
		#${X86DIS_SRC_DIR}/udis86.h
		
		${X86DIS_SRC_DIR}/decode.h
		${X86DIS_SRC_DIR}/extern.h
		${X86DIS_SRC_DIR}/input.h
		${X86DIS_SRC_DIR}/itab.h
		${X86DIS_SRC_DIR}/syn.h
		${X86DIS_SRC_DIR}/types.h
		${X86DIS_SRC_DIR}/udis86.h
	)

	SET(NETVM_SRCS ${NETVM_SRCS}
		${X86DIS_SRCS})
		
	SET(NETVM_HDRS ${NETVM_HDRS}
		${X86DIS_HDRS})
		
	
	INCLUDE_DIRECTORIES(${NETVM_JIT_DIR}/ia32 ${X86DIS_SRC_DIR} )
	SET_SOURCE_FILES_PROPERTIES(
		${NETVM_JIT_DIR}/ia32/inssel-ia32.cpp
		${NETVM_JIT_DIR}/ia32/inssel-ia32.h
		PROPERTIES GENERATED true)
ENDIF(ENABLE_X86_BACKEND)

#
# X11
#
IF(ENABLE_X11_BACKEND)
	SET(NETVM_SRCS ${NETVM_SRCS}
		${NETVM_JIT_DIR}/x11/x11-ir.cpp
		${NETVM_JIT_DIR}/x11/x11-backend.cpp
		${NETVM_JIT_DIR}/x11/x11-emit.cpp
		${NETVM_JIT_DIR}/x11/x11-util.cpp
		${NETVM_JIT_DIR}/x11/x11-optimization.cpp
		${NETVM_JIT_DIR}/x11/x11-coprocessor.cpp
		${NETVM_JIT_DIR}/x11/inssel-x11.cpp
		${NETVM_JIT_DIR}/x11/inssel-x11.brg
	)

	SET(NETVM_HDRS ${NETVM_HDRS}
		${NETVM_JIT_DIR}/x11/x11-ir.h
		${NETVM_JIT_DIR}/x11/inssel-x11.h
		${NETVM_JIT_DIR}/x11/x11-ir.h
		${NETVM_JIT_DIR}/x11/x11-emit.h
		${NETVM_JIT_DIR}/x11/x11-optimization.h
		${NETVM_JIT_DIR}/x11/x11-coprocessor.h
		${NETVM_JIT_DIR}/x11/x11-util.h
		${NETVM_JIT_DIR}/x11/x11-backend.h
	)

	INCLUDE_DIRECTORIES(${NETVM_JIT_DIR}/x11)

	SET_SOURCE_FILES_PROPERTIES(
		${NETVM_JIT_DIR}/x11/inssel-x11.cpp
		${NETVM_JIT_DIR}/x11/inssel-x11.h
		PROPERTIES GENERATED true
	)
ENDIF(ENABLE_X11_BACKEND)

#
# octeon
#
IF(ENABLE_OCTEON_BACKEND)
	SET(NETVM_SRCS ${NETVM_SRCS}
		${NETVM_JIT_DIR}/octeon/octeon-asm.cpp
		${NETVM_JIT_DIR}/octeon/octeon-asm.inl
		${NETVM_JIT_DIR}/octeon/octeon-backend.cpp
		${NETVM_JIT_DIR}/octeon/octeon-regalloc.cpp
		${NETVM_JIT_DIR}/octeon/octeon-offset.cpp
		${NETVM_JIT_DIR}/octeon/octeon-emit.cpp
		${NETVM_JIT_DIR}/octeon/inssel-octeon.cpp
		${NETVM_JIT_DIR}/octeon/octeon-coprocessor.cpp
		${NETVM_JIT_DIR}/octeon/octeon-switch.cpp
		${NETVM_JIT_DIR}/octeon/inssel-octeon.brg
	)

	SET(NETVM_HDRS ${NETVM_HDRS}
		${NETVM_JIT_DIR}/octeon/octeon-asm.h
		${NETVM_JIT_DIR}/octeon/octeon-backend.h
		${NETVM_JIT_DIR}/octeon/octeon-regalloc.h
		${NETVM_JIT_DIR}/octeon/octeon-offset.h
		${NETVM_JIT_DIR}/octeon/octeon-emit.h
		${NETVM_JIT_DIR}/octeon/inssel-octeon.h
		${NETVM_JIT_DIR}/octeon/octeon-coprocessor.h
		${NETVM_JIT_DIR}/octeon/octeon-switch.h
	)

	INCLUDE_DIRECTORIES(${NETVM_JIT_DIR}/octeon)

	SET_SOURCE_FILES_PROPERTIES(
		${NETVM_JIT_DIR}/octeon/inssel-octeon.cpp
		${NETVM_JIT_DIR}/octeon/inssel-octeon.h
		PROPERTIES GENERATED true
	)
ENDIF(ENABLE_OCTEON_BACKEND)

#
# octeon-c
#

IF(ENABLE_OCTEONC_BACKEND)
	SET(NETVM_SRCS ${NETVM_SRCS}
		${NETVM_JIT_DIR}/octeonc/octeonc-asm.cpp
		${NETVM_JIT_DIR}/octeonc/octeonc-asm.inl
		${NETVM_JIT_DIR}/octeonc/octeonc-backend.cpp
		${NETVM_JIT_DIR}/octeonc/octeonc-emit.cpp
		${NETVM_JIT_DIR}/octeonc/inssel-octeonc.cpp
		${NETVM_JIT_DIR}/octeonc/inssel-octeonc.brg
		### the next one is from octeon
		${NETVM_JIT_DIR}/octeon/octeon-coprocessor.cpp
	)

	SET(NETVM_HDRS ${NETVM_HDRS}
		${NETVM_JIT_DIR}/octeonc/octeonc-asm.inl
		${NETVM_JIT_DIR}/octeonc/octeonc-asm.h
		${NETVM_JIT_DIR}/octeonc/octeonc-backend.h
		${NETVM_JIT_DIR}/octeonc/octeonc-emit.h
		${NETVM_JIT_DIR}/octeonc/inssel-octeonc.h
		### the next one is from octeon
		${NETVM_JIT_DIR}/octeon/octeon-coprocessor.h
	)

	INCLUDE_DIRECTORIES(${NETVM_JIT_DIR}/octeonc)
	INCLUDE_DIRECTORIES(${NETVM_JIT_DIR}/octeon)

	SET_SOURCE_FILES_PROPERTIES(
		${NETVM_JIT_DIR}/octeonc/inssel-octeonc.cpp
		${NETVM_JIT_DIR}/octeonc/inssel-octeonc.h
		PROPERTIES GENERATED true
		)
ENDIF(ENABLE_OCTEONC_BACKEND)

#
# X64
#
IF(ENABLE_X64_BACKEND)
	SET(X86DIS_SRC_DIR ${NETVM_JIT_DIR}/x86_x64_disasm )
	SET(NETVM_SRCS ${NETVM_SRCS}
		${NETVM_JIT_DIR}/x64/x64-asm.cpp
		${NETVM_JIT_DIR}/x64/x64-emit.cpp
		${NETVM_JIT_DIR}/x64/x64-backend.cpp
		${NETVM_JIT_DIR}/x64/x64-opt.cpp
		${NETVM_JIT_DIR}/x64/inssel-x64.cpp
		${NETVM_JIT_DIR}/x64/x64-regalloc.cpp
		${NETVM_JIT_DIR}/x64/x64_switch_lowering.cpp
		${NETVM_JIT_DIR}/x64/x64_counters.cpp
		${NETVM_JIT_DIR}/x64/inssel-x64.brg
	)

	SET(NETVM_HDRS ${NETVM_HDRS}
		${NETVM_JIT_DIR}/x64/x64-asm.h
		${NETVM_JIT_DIR}/x64/x64-backend.h
		${NETVM_JIT_DIR}/x64/x64-emit.h
		${NETVM_JIT_DIR}/x64/x64-opt.h
		${NETVM_JIT_DIR}/x64/inssel-x64.h
		${NETVM_JIT_DIR}/x64/x64-regalloc.h
		${NETVM_JIT_DIR}/x64/x64_switch_lowering.h
		${NETVM_JIT_DIR}/x64/x64_counters.h
	)

	# Set x86 disassembler source files
	SET(X86DIS_SRCS
		#${X86DIS_SRC_DIR}/decode.c
		#${X86DIS_SRC_DIR}/input.c
		#${X86DIS_SRC_DIR}/mnemonics.c
		#${X86DIS_SRC_DIR}/opcmap.c
		#${X86DIS_SRC_DIR}/syn-att.c
		#${X86DIS_SRC_DIR}/syn-intel.c
		#${X86DIS_SRC_DIR}/syn.c
		#${X86DIS_SRC_DIR}/udis86.c
		
		${X86DIS_SRC_DIR}/decode.c
		${X86DIS_SRC_DIR}/input.c
		${X86DIS_SRC_DIR}/itab.c
		${X86DIS_SRC_DIR}/syn.c
		${X86DIS_SRC_DIR}/syn-att.c
		${X86DIS_SRC_DIR}/syn-intel.c
		${X86DIS_SRC_DIR}/udis86.c
	)

	SET(X86DIS_HDRS
		#${X86DIS_SRC_DIR}/extern.h
		#${X86DIS_SRC_DIR}/input.h
		#${X86DIS_SRC_DIR}/mnemonics.h
		#${X86DIS_SRC_DIR}/opcmap.h
		#${X86DIS_SRC_DIR}/syn.h
		#${X86DIS_SRC_DIR}/dis_types.h
		#${X86DIS_SRC_DIR}/udis86.h
		
		${X86DIS_SRC_DIR}/decode.h
		${X86DIS_SRC_DIR}/extern.h
		${X86DIS_SRC_DIR}/input.h
		${X86DIS_SRC_DIR}/itab.h
		${X86DIS_SRC_DIR}/syn.h
		${X86DIS_SRC_DIR}/types.h
		${X86DIS_SRC_DIR}/udis86.h
	)

	SET(NETVM_SRCS ${NETVM_SRCS}
		${X86DIS_SRCS})

	SET(NETVM_HDRS ${NETVM_HDRS}
		${X86DIS_HDRS})

	INCLUDE_DIRECTORIES(${NETVM_JIT_DIR}/x64 ${X86DIS_SRC_DIR} )

	SET_SOURCE_FILES_PROPERTIES(
		${NETVM_JIT_DIR}/x64/inssel-x64.cpp
		${NETVM_JIT_DIR}/x64/inssel-x64.h
		PROPERTIES GENERATED true
	)

ENDIF(ENABLE_X64_BACKEND)



###############################################################################
#
# target defintion for runtime
#

ADD_LIBRARY(
	nbnetvm
	SHARED
	${NETVM_SRCS} ${NETVM_HDRS}
)

ADD_SUBDIRECTORY(${NETVM_BASE_DIR}/tools/makenetilscanner)
ADD_SUBDIRECTORY(${NETVM_BASE_DIR}/tools/makeopcodetable)
ADD_SUBDIRECTORY(${NETVM_BASE_DIR}/tools/netvmburg)

ADD_DEPENDENCIES(nbnetvm makenetilscanner makeopcodetable netvmburg)

SET(scanner_prefix nvmparser_)
GET_TARGET_PROPERTY(NETVM_ASM_SCANNER_EXE makenetilscanner LOCATION)
GET_TARGET_PROPERTY(MAKEOPCODETABLE_EXE makeopcodetable LOCATION)
GET_TARGET_PROPERTY(NETVMBURG_EXE netvmburg LOCATION)

ADD_CUSTOM_COMMAND(
	OUTPUT
	${NETVM_SRC_DIR}/opcodestable.txt
	COMMAND ${MAKEOPCODETABLE_EXE} "../../opcodes.txt" 1> "../../opcodestable.txt"
	DEPENDS
	${NETVM_SRC_DIR}/opcodes.txt makeopcodetable
	WORKING_DIRECTORY "${NETVM_BASE_DIR}/tools/bin"
	)

ADD_CUSTOM_COMMAND(
	OUTPUT
	${NETVM_ASM_DIR}/nvm_gramm.tab.c
	${NETVM_ASM_DIR}/nvm_gramm.tab.h
	COMMAND bison -bgramm -d -p "${scanner_prefix}" "../../assembler/nvm_gramm.y" -o "../../assembler/nvm_gramm.tab.c"
	DEPENDS ${NETVM_ASM_DIR}/nvm_gramm.y
	WORKING_DIRECTORY "${NETVM_BASE_DIR}/tools/bin"
	)

ADD_CUSTOM_COMMAND(
	OUTPUT
	${NETVM_ASM_DIR}/scanner.l
	${NETVM_ASM_DIR}/scanner.c

	COMMAND ${NETVM_ASM_SCANNER_EXE} "../../opcodes.txt" "../../assembler/scanner-template.l" > "../../assembler/scanner.l"

	COMMAND flex -P${scanner_prefix} -i -o"../../assembler/scanner.c" "../../assembler/scanner.l"
	DEPENDS makenetilscanner
	${NETVM_ASM_DIR}/scanner-template.l
	${NETVM_SRC_DIR}/opcodes.txt
	WORKING_DIRECTORY "${NETVM_BASE_DIR}/tools/bin"
	)

ADD_CUSTOM_COMMAND(
	OUTPUT ${NETVM_JIT_DIR}/ia32/inssel-ia32.cpp
	${NETVM_JIT_DIR}/ia32/inssel-ia32.h
	COMMAND ${NETVMBURG_EXE} 
	ARGS
#comment the following line to disable constant swapping
	-DSWAP_CONST
	-C IA32InsSelector -p -d ../../jit/ia32/inssel-ia32.h ../../jit/ia32/inssel-ia32.brg > ../../jit/ia32/inssel-ia32.cpp
	DEPENDS netvmburg
	${NETVM_JIT_DIR}/ia32/inssel-ia32.brg
	WORKING_DIRECTORY "${NETVM_BASE_DIR}/tools/bin"
	)

ADD_CUSTOM_COMMAND(
	OUTPUT ${NETVM_JIT_DIR}/x64/inssel-x64.cpp
	${NETVM_JIT_DIR}/x64/inssel-x64.h
	COMMAND ${NETVMBURG_EXE} 
	ARGS
#comment the following line to disable constant swapping
	-DSWAP_CONST
	-C X64InsSelector -p -d ../../jit/x64/inssel-x64.h ../../jit/x64/inssel-x64.brg > ../../jit/x64/inssel-x64.cpp
	DEPENDS netvmburg
	${NETVM_JIT_DIR}/x64/inssel-x64.brg
	WORKING_DIRECTORY "${NETVM_BASE_DIR}/tools/bin"
	)

ADD_CUSTOM_COMMAND(
	OUTPUT ${NETVM_JIT_DIR}/x11/inssel-x11.cpp
	${NETVM_JIT_DIR}/x11/inssel-x11.h
	COMMAND ${NETVMBURG_EXE} ARGS -C X11InsSelector -p -d ../../jit/x11/inssel-x11.h ../../jit/x11/inssel-x11.brg > ../../jit/x11/inssel-x11.cpp
	DEPENDS netvmburg
	${NETVM_JIT_DIR}/x11/inssel-x11.brg
	WORKING_DIRECTORY "${NETVM_BASE_DIR}/tools/bin"
	)

ADD_CUSTOM_COMMAND(
	OUTPUT ${NETVM_JIT_DIR}/octeon/inssel-octeon.cpp
	${NETVM_JIT_DIR}/octeon/inssel-octeon.h
	COMMAND ${NETVMBURG_EXE} ARGS -C OcteonInsSelector -p -d ../../jit/octeon/inssel-octeon.h ../../jit/octeon/inssel-octeon.brg > ../../jit/octeon/inssel-octeon.cpp
	DEPENDS netvmburg
	${NETVM_JIT_DIR}/octeon/inssel-octeon.brg
	WORKING_DIRECTORY "${NETVM_BASE_DIR}/tools/bin"
	)
    
ADD_CUSTOM_COMMAND(
	OUTPUT ${NETVM_JIT_DIR}/octeonc/inssel-octeonc.cpp
	${NETVM_JIT_DIR}/octeonc/inssel-octeonc.h
	COMMAND ${NETVMBURG_EXE} ARGS -C OcteoncInsSelector -p -d
    ../../jit/octeonc/inssel-octeonc.h ../../jit/octeonc/inssel-octeonc.brg >
    ../../jit/octeonc/inssel-octeonc.cpp
	DEPENDS netvmburg
	${NETVM_JIT_DIR}/octeonc/inssel-octeonc.brg
	WORKING_DIRECTORY "${NETVM_BASE_DIR}/tools/bin"
	)


# Platform-specific definitions
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

ADD_DEFINITIONS(-DAVAILABLE_COPROS_NO=${AVAILABLE_COPROS_NO})



######################################################################################################################
#
# libraries linked with netvm
#

IF(WIN32)
  TARGET_LINK_LIBRARIES(nbnetvm ws2_32.lib wpcap.lib pcre.lib)
ELSE(WIN32)
	TARGET_LINK_LIBRARIES(nbnetvm pcap ${PCRE_LIBRARIES})
	INCLUDE_DIRECTORIES(${PCRE_INCLUDE_DIR})
ENDIF(WIN32)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbnetvm
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbnetvm.lib ../../lib/.
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbnetvm.exp ../../lib/.
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbnetvm.dll ../../bin/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbnetvm
		POST_BUILD
		COMMAND cp ${NETVM_BASE_DIR}/libnbnetvm* ../../lib/.
		COMMAND cp ${NETVM_BASE_DIR}/libnbnetvm* ../../bin/.
	)
ENDIF(WIN32)

