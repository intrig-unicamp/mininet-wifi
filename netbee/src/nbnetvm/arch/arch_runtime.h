/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#ifndef __ARCH_RUNTIME_H__
#define __ARCH_RUNTIME_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Include config.h if needed */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//#define ARCH_RUNTIME_OCTEON

#ifdef ARCH_RUNTIME_X86
#include "./x86/arch.h"
#elif defined ARCH_RUNTIME_OCTEON
#include "./octeon/arch.h"
#elif defined ARCH_RUNTIME_X11-PPC
#include "./X11-PPC/arch.h"
#else
#include "./generic/arch.h"
#endif


#ifdef __cplusplus
}
#endif

#endif		//__ARCH_RUNTIME_H__
