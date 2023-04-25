/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#ifndef __GENERIC_ARCH_H__
#define __GENERIC_ARCH_H__

/* Include config.h if needed */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "generic_runtime.h"
#include "generic_interpreter.h"

#define arch_AllocRTObject			genRT_AllocRTObject
#define arch_ReleaseRTObject		genRT_ReleaseRTObject
#define arch_CreateExbufPool		genRT_CreateExbufPool

#define arch_GetExbuf(RTObj) RTObj->ExbufPool->Pool[--RTObj->ExbufPool->Top]
#define arch_ReleaseExbuf(RTObj, exbuf) { RTObj->ExbufPool->Pool[RTObj->ExbufPool->Top++] = exbuf; }

#define arch_AllocPEMemory			genRT_AllocPEMemory
#define arch_CreatePhysInterfList	genRT_CreatePhysInterfList
#define arch_SetTStamp				genRT_SetTStamp
#define arch_WriteExBuff			genRT_WriteExBuff
#define arch_Execute_Handlers		genRT_Execute_Handlers
#define arch_ReadExBuff				genRT_ReadExBuff
#define arch_ReadFromPhys			genRT_ReadFromPhys

#ifdef RTE_PROFILE_COUNTERS
#define arch_SampleDeltaTicks		nbProfilerGetTime
#endif

#define arch_nvmInit				genRT_nvmInit
#define arch_ReleaseVM				genRT_ReleaseVM
#define arch_OutPhysicIn			genRT_OutPhysicIn
#define arch_OpenPhysIn				genRT_OpenPhysIn
#endif		//__GENERIC_ARCH_H__
