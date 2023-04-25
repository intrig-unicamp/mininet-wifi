/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




/* Include config.h if needed */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <nbnetvm.h>
#include <rt_environment.h>
#include <int_structs.h>

#ifdef __cplusplus
extern "C" {
#endif

//#ifdef _WIN32
//#ifdef _DEBUG
//#endif
//#else
uint32_t _gen_rotl(uint32_t value, uint32_t pos );

uint32_t _gen_rotr(uint32_t value, uint32_t pos );

//#endif


/*! \addtogroup GenRTFuncts
	\{
*/
/*
 \brief Function use by the interpreter. It reads the bytecode and execute the code
 */ 
int32_t genRT_Execute_Handlers(nvmExchangeBuffer **exbuf, uint32_t port, nvmHandlerState *HandlerState);


/** \} */

#ifdef __cplusplus
}
#endif

