/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#ifndef __OCTEON_RUNTIME_H__
#define __OCTEON_RUNTIME_H__

/* Include config.h if needed */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <nbnetvm.h>
#include "../../rt_environment.h"
#include <int_structs.h>

#ifdef __cplusplus
extern "C" {
#endif



/*! \addtogroup OcteonFuncts
	\{
*/

/*! \brief Allocates a runtime object
*/
void *octRT_AllocRTObject(nvmRuntimeEnvironment *RTObj, uint32_t size, char *errbuf);

/*! \brief Releasees a runtime object
*/
void *octRT_ReleaseRTObject(nvmRuntimeEnvironment *RTObj);

nvmRESULT octRT_CreateExbufPool(nvmRuntimeEnvironment *RTObj, uint32_t numExBuf, uint32_t pkt_size, uint32_t info_size, char *errbuf);

nvmExchangeBuffer *octRT_GetExbuf(nvmRuntimeEnvironment *RTObj);

nvmMemDescriptor *octRT_AllocPEMemory(nvmRuntimeEnvironment *RTObj, uint32_t size, char *errbuf);

nvmRESULT octRT_CreatePhysInterfList(nvmRuntimeEnvironment *RTObj, char *errbuf);

nvmRESULT	octRT_SetTStamp(nvmExchangeBuffer *exbuf, char *errbuf);
nvmRESULT	octRT_ReadFromPhys(nvmExchangeBuffer *exbuf, nvmPhysInterface *PhysInterface,char *errbuf);
nvmRESULT	octRT_WriteExBuff(nvmExchangeBuffer *exbuf, uint32_t port, nvmHandlerState *HandlerState);
nvmRESULT octRT_ReleaseExbuf(nvmRuntimeEnvironment *RTObj, nvmExchangeBuffer *exbuf);
nvmRESULT	octRT_ReadFromPhys(nvmExchangeBuffer *exbuf, nvmPhysInterface *PhysInterface,char *errbuf);
nvmRESULT octRT_ReadExBuff(nvmExchangeBuffer **exbuf, uint32_t port, nvmHandlerState *HandlerState);
#ifdef RTE_PROFILE_COUNTERS
//void octRT_SetTime_Now(uint32_t type, nvmHandlerState *HandlerState);
#endif
nvmRESULT	octRT_nvmInit();
void *octRT_AllocFPAObject(nvmRuntimeEnvironment *RTObj, uint32_t size,uint64_t pool);

void *octRT_ReleaseFPAObject(nvmRuntimeEnvironment *RTObj);

/** \} */


#ifdef __cplusplus
}
#endif

#endif		//__OCTEON_RUNTIME_H__
