/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#ifndef __JIT_INTERFACE_H__
#define __JIT_INTERFACE_H__




/** @file jit_interface.h
 * \brief This file contains the prototypes of the functions exported by the NetVM JIT to the NetVM runtime
 */


#include "nbnetvm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Compiles the current NetVM application into native code (with JIT)
 * \param 	NetVM pointer to the netvm object
 * \param 	RTObj pointer to the runtim object
 * \param	BackendID number identifying the backend used to compile the application (first backend is '0')
 * \param 	JitFlags flags for the compiler selecting what to do
 * \param	OptLevel optimization level for the compiler
 * \param	OutputFilePrefix string to prepend to filenames emitted (containing the native asm code)
 * \param	Errbuf buffer of for error messages
 */
int32_t nvmNetCompileApplication(
	nvmNetVM *NetVM, 
	nvmRuntimeEnvironment* RTObj, 
	uint32_t BackendID, 
	uint32_t JitFlags, 
	uint32_t OptLevel, 
	const char *OutputFilePrefix, 
	char *Errbuf);


/*!
 * \brief Verifies the current NetVM application (checks that everything is ok, independently from the backend that will be used)
 * \param 	NetVM pointer to the netvm object
 * \param 	RTObj pointer to the runtim object
 * \param 	JitFlags flags for the compiler selecting what to do
 * \param	OptLevel optimization level for the compiler
 * \param	OutputFilePrefix string to prepend to filenames emitted (containing the native asm code)
 * \param	Errbuf buffer of for error messages
 */
int32_t nvmVerifyApplication(nvmNetVM *NetVM,
	nvmRuntimeEnvironment* RTObj,	
	uint32_t JitFlags,
	uint32_t OptLevel,
	const char *OutputFilePrefix,
	char *Errbuf);

#ifdef __cplusplus
}
#endif


#endif
