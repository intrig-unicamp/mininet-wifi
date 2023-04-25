/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#ifndef __GENERIC_RUNTIME_H__
#define __GENERIC_RUNTIME_H__

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

//TODO [OM]: comment all these functions

/*! \addtogroup GenRTFuncts
	\{
*/

/*! \brief Allocates a runtime object
*/
void *genRT_AllocRTObject(nvmRuntimeEnvironment *RTObj, uint32_t size, char *errbuf);

/*! \brief Releases a runtime object
*/
void genRT_ReleaseRTObject(nvmRuntimeEnvironment *RTObj);

/*! \brief Creates the exbuf pool
*/
nvmRESULT genRT_CreateExbufPool(nvmRuntimeEnvironment *RTObj, uint32_t numExBuf, uint32_t pkt_size, uint32_t info_size, char *errbuf);

/*! \brief Get a exbuf taking it from the exbuf pool
*/
nvmExchangeBuffer *genRT_GetExbuf(nvmRuntimeEnvironment *RTObj, char *errbuf);

/*! \brief Release a exbuf 
*/
//void arch_ReleaseExbuf(nvmRuntimeEnvironment *RTObj, nvmExchangeBuffer *exbuf, char *errbuf);
void arch_ReleaseExbuf(nvmRuntimeEnvironment *RTObj, nvmExchangeBuffer *exbuf);

/*! \brief Allocates a PE
*/
nvmMemDescriptor *genRT_AllocPEMemory(nvmRuntimeEnvironment *RTObj, uint32_t size, char *errbuf);

/*! \brief Creates a list of physic interfaces using pcap
*/
nvmRESULT genRT_CreatePhysInterfList(nvmRuntimeEnvironment *RTObj, char *errbuf);

nvmRESULT	genRT_SetTStamp(nvmExchangeBuffer *exbuf, char *errbuf);

/*! \brief Infinite loop in which reads pkts from the physic interface using pcap
*/
nvmRESULT	genRT_ReadFromPhys(nvmExchangeBuffer *exbuf, nvmPhysInterface *PhysInterface,char *errbuf);
/*! \brief Call the interpreter or the jit function to execute the bytecode of the handler on the exbuf(that contain the pkt).Use it in push mode
*/
nvmRESULT	genRT_WriteExBuff(nvmExchangeBuffer *exbuf, uint32_t port, nvmHandlerState *HandlerState);
void genRT_ReleaseExbuf(nvmRuntimeEnvironment *RTObj, nvmExchangeBuffer *exbuf, char *errbuf);
//nvmRESULT	genRT_ReadFromPhys(nvmExchangeBuffer *exbuf, nvmPhysInterface *PhysInterface,char *errbuf);

/*! \brief Call the interpreter or the jit function to execute the bytecode of the handler on the exbuf(that contain the pkt).Use it in pull mode
*/
nvmRESULT genRT_ReadExBuff(nvmExchangeBuffer **exbuf, uint32_t port, nvmHandlerState *HandlerState);
#ifdef RTE_PROFILE_COUNTERS
/*! \brief Set the actual ticks of the processor, you can set START,FORWARDED or DISCARDED
*/
void genRT_SetTime_Now(uint32_t type, nvmHandlerState *HandlerState);
#endif
/*! \brief Init the architecture.Useless for generic
*/
nvmRESULT	genRT_nvmInit();

void genRT_FreeAllocData(nvmRuntimeEnvironment *RTObj);

void genRT_SampleDeltaTicks(uint64_t *delta);

int32_t genRT_OutPhysicIn (nvmExchangeBuffer *exbuf, uint32_t port, nvmHandlerState *HandlerState);

int32_t	genRT_OpenPhysIn(nvmPhysInterface *PhysInterface, char *errbuf);

/** \} */


#ifdef __cplusplus
}
#endif

#endif		//__GENERIC_RUNTIME_H__
