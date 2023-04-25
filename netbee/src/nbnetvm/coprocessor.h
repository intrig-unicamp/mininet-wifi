/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/** @file coprocessor.h
 * \brief This file contains functions implementing the various available coprocessors. If you need to export coprocessor
 * helper functions/structures to applications, see Include/netvm_coprocessors.h.
 *
 */

#pragma once

// Uncomment line below to serialize coprocessor access
// This is required in case of a multicore implementation of the NetVM,
// since we do not want to be called from multiple sources, thus impairing
// possible shared variables in the coprocessor
//#define COPRO_SYNCH

#ifdef COPRO_SYNCH
#include <semaphore.h>
#define COPRO_TMP_FNAME "/copro_synch"
#endif

#include "nbnetvm.h"
#include "netvmprofiling.h"

#ifdef __cplusplus
extern "C" {
#endif



/* Available coprocessors */
//TODO PV

typedef struct _CoprocessorMapEntry nvmCoprocessorStateMapEntry;
typedef struct _CoprocessorState nvmCoprocessorState;


/* If you change any of the following, change it also in compiler.h (TODO) */
#define MAX_COPRO_NUMBER 10
#define MAX_COPRO_NAME 64
#define MAX_COPRO_REGISTERS 32
#define MAX_COPRO_OPS 8

//! Function that alloc the internal data of the Coprocessor
typedef int32_t(nvmCoproCreateFunct)(nvmCoprocessorState *);


/*!
	\brief Structure keeping a mapping of coprocessor names vs. their exported structures.
*/
struct _CoprocessorMapEntry {
	char copro_name[MAX_COPRO_NAME];
	//nvmCoprocessorState *copro_struct;
	nvmCoproCreateFunct *copro_create;
};

//! Table of available coprocessors.
#ifndef AVAILABLE_COPROS_NO
#define AVAILABLE_COPROS_NO 4
#endif

#define AVAILABLE_COPROS_NUM AVAILABLE_COPROS_NO
extern nvmCoprocessorStateMapEntry nvm_copro_map[AVAILABLE_COPROS_NO];

//! Flags for registers
typedef enum {
	COPREG_READ = 0x01,
	COPREG_WRITE = 0x02
} nvmCoprocessorStateRegisterFlags;

#ifdef COPRO_SYNCH
	struct copro_semaphores {
		sem_t serializing_semaphore;
	};
#endif
	
//! Type for coprocessor init function
typedef int32_t (nvmCoproInitFunct)(nvmCoprocessorState *c, void *data);
//! Type for coprocessor I/O callback function
typedef int32_t (nvmInOutFunct)(nvmCoprocessorState *c, uint32_t reg, uint32_t *val);
//! Type for coprocessor invocation callback function
typedef int32_t (nvmInvokeFunct)(nvmCoprocessorState *c, uint32_t operation);

typedef int32_t (nvmCoproFunct)(nvmCoprocessorState *c);

/*!
	\brief	Structure keeping the state of a coprocessor.
*/
struct _CoprocessorState {
	/* Membri comuni a tutte le implementazioni */
	char name[MAX_COPRO_NAME];        		//!< Coprocessor name.
	uint32_t n_regs;        			//!< Number of registers.
	uint8_t reg_flags[MAX_COPRO_REGISTERS]; 	//!< Register flags (See nvmCoprocessorStateRegisterFlags).

	/* Membri validi per l'implementazione su arch. general purpose*/
	uint32_t registers[MAX_COPRO_REGISTERS]; 	//!< Registers.
	nvmCoproInitFunct *init;			//!< Init function.
	nvmInOutFunct *write;				//!< Register write callback function.
	nvmInOutFunct *read;				//!< Register read callback function.
	nvmInvokeFunct *invoke;				//!< Coprocessor invocation callback function.
	void *data;					//!< Private coprocessor data.

	nvmExchangeBuffer *xbuf;			//!< Exchange buffer passed from PE.

#ifdef _EXP_COPROCESSOR_MODEL
	uint32_t n_ops;					//!< Number of operations.
	void *OpFunctions[MAX_COPRO_OPS];
#endif
#ifdef RTE_PROFILE_COUNTERS
  	nvmCounter			*ProfCounter_tot;		//!< Profiling counters
  	nvmCounter			**ProfCounter;		//!< Profiling counters
#endif

#ifdef COPRO_SYNCH
	struct copro_semaphores *sems;
#endif
};



/* The following are standard register I/O functions that can be used by coprocessor implementations if they do not need
 * any particular operations. */
int32_t nvmCoproStandardRegWrite (nvmCoprocessorState *c, uint32_t reg, uint32_t *val);
int32_t nvmCoproStandardRegRead (nvmCoprocessorState *c, uint32_t reg, uint32_t *val);

typedef struct nvmRegExpCoproData_s nvmRegExpCoproData;


#ifdef __cplusplus
}
#endif

