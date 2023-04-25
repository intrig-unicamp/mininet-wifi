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


uint32_t _gen_rotl(uint32_t value, uint32_t pos );
uint32_t _gen_rotr(uint32_t value, uint32_t pos );


/*! \addtogroup OcteonFuncts
	\{
*/

int32_t octRT_Execute_Handlers(nvmExchangeBuffer **exbuf, uint32_t port, nvmHandlerState *HandlerState);


/** \} */

#ifdef __cplusplus
}
#endif

