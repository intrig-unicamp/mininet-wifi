/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


/** @file helpers.h
 * \brief
 *
 */

/*! \addtogroup HelperFuncts
	\{
*/

#ifdef __cplusplus
extern "C" {
#endif

/* Include config.h if needed */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdint.h>


//#define errsnprintf nvmsnprintf

int errsnprintf(char* buffer, int bufsize, const char *format, ...);

void *nvmAllocObject(uint32_t size, char *errbuf);
void nvmFreeObject(void *obj);

void hex_and_ascii_print_with_offset(FILE *outfile, const char *ident, register const uint8_t *cp,	register uint32_t length, register uint32_t oset);

/* End of Doxygen group */
/** \} */

#ifdef __cplusplus
}
#endif
