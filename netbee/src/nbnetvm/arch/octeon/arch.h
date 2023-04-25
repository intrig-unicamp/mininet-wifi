/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#ifndef __OCTEON_ARCH_H__
#define __OCTEON_ARCH_H__

/* Include config.h if needed */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define OCTEON_MODEL OCTEON_CN38XX_PASS2
#include "octeon_runtime.h"
#include "octeon_interpreter.h"

/*
XXX errbuf was removed from the definition of arch_GetExbuf and
arch_ReleaseExbuf. Bu we use it in the octeon implementation, so for the moment
let me reintroduce it here. This is what you call quick and dirty.
*/

#define arch_AllocRTObject			octRT_AllocRTObject
#define arch_ReleaseRTObject		octRT_ReleaseRTObject
#define arch_CreateExbufPool		octRT_CreateExbufPool
#define arch_GetExbuf(A)			octRT_GetExbuf(A)
#define arch_ReleaseExbuf(A,B)		octRT_ReleaseExbuf(A, B)
#define arch_AllocPEMemory			octRT_AllocPEMemory
#define arch_CreatePhysInterfList	octRT_CreatePhysInterfList
#define arch_SetTStamp				octRT_SetTStamp
#define arch_WriteExBuff			octRT_WriteExBuff
#define arch_Execute_Handlers		octRT_Execute_Handlers
#define arch_ReadExBuff				octRT_ReadExBuff
#define arch_ReadFromPhys			octRT_ReadFromPhys

#ifdef RTE_PROFILE_COUNTERS
#define arch_SetTime_Now			octRT_SetTime_Now
#define arch_SampleDeltaTicks		octRT_SampleDeltaTicks
#endif
#define arch_nvmInit				octRT_nvmInit
#define arch_ReleaseVM				octRT_ReleaseVM
#define arch_OutPhysicIn			octRT_OutPhysicIn
#define arch_OpenPhysIn				octRT_OpenPhysIn
#endif		//__OCTEON_ARCH_H__
