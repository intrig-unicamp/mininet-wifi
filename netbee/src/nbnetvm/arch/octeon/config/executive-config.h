/*************************************************************************
Copyright (c) 2003-2005, Cavium Networks. All rights reserved.

This Software is the property of Cavium Networks.  The Software and all
accompanying documentation are copyrighted.  The Software made available
here constitutes the proprietary information of Cavium Networks.  You
agree to take reasonable steps to prevent the disclosure, unauthorized use
or unauthorized distribution of the Software.  You shall use this Software
solely with Cavium hardware.

Except as expressly permitted in a separate Software License Agreement
between You and Cavium Networks, you shall not modify, decompile,
disassemble, extract, or otherwise reverse engineer this Software.  You
shall not make any copy of the Software or its accompanying documentation,
except for copying incident to the ordinary and intended use of the
Software and the Underlying Program and except for the making of a single
archival copy.

This Software, including technical data, may be subject to U.S.  export
control laws, including the U.S.  Export Administration Act and its
associated regulations, and may be subject to export or import regulations
in other countries.  You warrant that You will comply strictly in all
respects with all such regulations and acknowledge that you have the
responsibility to obtain licenses to export, re-export or import the
Software.

TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
AND WITH ALL FAULTS AND CAVIUM MAKES NO PROMISES, REPRESENTATIONS OR
WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH RESPECT
TO THE SOFTWARE, INCLUDING ITS CONDITION, ITS CONFORMITY TO ANY
REPRESENTATION OR DESCRIPTION, OR THE EXISTENCE OF ANY LATENT OR PATENT
DEFECTS, AND CAVIUM SPECIFICALLY DISCLAIMS ALL IMPLIED (IF ANY) WARRANTIES
OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A PARTICULAR
PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET
POSSESSION OR CORRESPONDENCE TO DESCRIPTION.  THE ENTIRE RISK ARISING OUT
OF USE OR PERFORMANCE OF THE SOFTWARE LIES WITH YOU.

*************************************************************************/

/*
 * File version info: $Id: executive-config.h,v 1.17 2006/12/11 22:03:46 pkapoor Exp $
 * $Name: SDK_1_5_0_build_187 $
 */
#ifndef __EXECUTIVE_CONFIG_H__
#define __EXECUTIVE_CONFIG_H__

/* Define to enable the use of simple executive DFA functions */
#define CVMX_ENABLE_DFA_FUNCTIONS

/* Define to enable the use of simple executive packet output functions.
** For packet I/O setup enable the helper functions below. 
*/ 
#define CVMX_ENABLE_PKO_FUNCTIONS

/* Define to enable the use of simple executive timer bucket functions. 
** Refer to cvmx-tim.[ch] for more information
*/
//#define CVMX_ENABLE_TIMER_FUNCTIONS

/* Define to enable the use of simple executive helper functions. These
** include many harware setup functions.  See cvmx-helper.[ch] for
** details.
*/
#define CVMX_ENABLE_HELPER_FUNCTIONS

/* CVMX_HELPER_FIRST_MBUFF_SKIP is the number of bytes to reserve before
** the beginning of the packet. If necessary, override the default  
** here.  See the IPD section of the hardware manual for MBUFF SKIP 
** details.*/ 
#define CVMX_HELPER_FIRST_MBUFF_SKIP 184

/* CVMX_HELPER_NOT_FIRST_MBUFF_SKIP is the number of bytes to reserve in each
** chained packet element. If necessary, override the default here */
#define CVMX_HELPER_NOT_FIRST_MBUFF_SKIP 0

/* CVMX_HELPER_ENABLE_BACK_PRESSURE controls whether back pressure is enabled
** for all input ports. This controls if IPD sends backpressure to all ports if
** Octeon's FPA pools don't have enough packet or work queue entries. Even when
** this is off, it is still possible to get backpressure from individual
** hardware ports. When configuring backpressure, also check
** CVMX_HELPER_DISABLE_*_BACKPRESSURE below. If necessary, override the default
** here */
#define CVMX_HELPER_ENABLE_BACK_PRESSURE 1

/* CVMX_HELPER_ENABLE_IPD controls if the IPD is enabled in the helper
**  function. Once it is enabled the hardware starts accepting packets. You
**  might want to skip the IPD enable if configuration changes are need
**  from the default helper setup. If necessary, override the default here */
#define CVMX_HELPER_ENABLE_IPD 1

/* CVMX_HELPER_INPUT_TAG_TYPE selects the type of tag that the IPD assigns
** to incoming packets. */
#define CVMX_HELPER_INPUT_TAG_TYPE CVMX_POW_TAG_TYPE_ORDERED

/* The following select which fields are used by the PIP to generate
** the tag on INPUT
** 0: don't include
** 1: include */
#define CVMX_HELPER_INPUT_TAG_IPV6_SRC_IP	0
#define CVMX_HELPER_INPUT_TAG_IPV6_DST_IP   	0
#define CVMX_HELPER_INPUT_TAG_IPV6_SRC_PORT 	0
#define CVMX_HELPER_INPUT_TAG_IPV6_DST_PORT 	0
#define CVMX_HELPER_INPUT_TAG_IPV6_NEXT_HEADER 	0
#define CVMX_HELPER_INPUT_TAG_IPV4_SRC_IP	0
#define CVMX_HELPER_INPUT_TAG_IPV4_DST_IP   	0
#define CVMX_HELPER_INPUT_TAG_IPV4_SRC_PORT 	0
#define CVMX_HELPER_INPUT_TAG_IPV4_DST_PORT 	0
#define CVMX_HELPER_INPUT_TAG_IPV4_PROTOCOL	0
#define CVMX_HELPER_INPUT_TAG_INPUT_PORT	1

/* Select skip mode for input ports */
#define CVMX_HELPER_INPUT_PORT_SKIP_MODE	CVMX_PIP_PORT_CFG_MODE_SKIPL2

/* Define the number of queues per output port */
#define CVMX_HELPER_PKO_QUEUES_PER_PORT_INTERFACE0	1
#define CVMX_HELPER_PKO_QUEUES_PER_PORT_INTERFACE1	1

/* Force backpressure to be disabled.  This overrides all other
** backpressure configuration */
#define CVMX_HELPER_DISABLE_RGMII_BACKPRESSURE 1

/* Disable the SPI4000's processing of backpressure packets and backpressure
** generation. When this is 1, the SPI4000 will not stop sending packets when
** receiving backpressure. It will also not generate backpressure packets when
** its internal FIFOs are full. */
#define CVMX_HELPER_DISABLE_SPI4000_BACKPRESSURE 1

/* Select the number of low latency memory ports (interfaces) that
** will be configured.  Valid values are 1 and 2.
*/
#define CVMX_LLM_CONFIG_NUM_PORTS 1

/* Enable the fix for PKI-100 errata ("Size field is 8 too large in WQE and next
** pointers"). If CVMX_ENABLE_LEN_M8_FIX is set to 0, the fix for this errata will 
** not be enabled. 
** 0: Fix is not enabled
** 1: Fix is enabled, if supported by hardware
*/
#define CVMX_ENABLE_LEN_M8_FIX  1

#if defined(CVMX_ENABLE_HELPER_FUNCTIONS) && !defined(CVMX_ENABLE_PKO_FUNCTIONS)
#define CVMX_ENABLE_PKO_FUNCTIONS
#endif

#ifdef CVMX_NVM_OBJ
cvmxconfig
{
	fpa CVMX_NVM_OBJ_POOL
		pool		= 4
		size        = 16
		priority    = 1
		protected   = false
		description = "NvmObject";
}
#endif


/* Executive resource descriptions provided in cvmx-resources.config */
#include "cvmx-resources.config"

#endif
