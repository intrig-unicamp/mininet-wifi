/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#ifndef NULL
#define NULL 0
#endif

#include <stdio.h>



#ifdef _WIN32
#include <windows.h>
#ifndef snprintf
#define snprintf _snprintf
#endif

#endif

#ifndef INT_TYPES_DEFINED
typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned int	uint32;
typedef char			int8;
typedef short			int16;
typedef int				int32;
#define INT_TYPES_DEFINED	//!< Placeholder that confirms that the right integer types are defined in the current compilation environment
#endif


//#if 0
//enum ReturnValues
//{
//	SUCCESS = 0,
//	FAILURE = -1
//};
//#endif

#define CHECK_MEM_ALLOC(ptr)	do{\
									if (ptr == NULL)\
									throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");\
								}while(0);





#ifdef _WIN32
#define nbASSERT(cond, msg) \
	{\
		if (!(cond))\
		{\
			MessageBoxA(NULL,msg, "-- NETBEE ASSERTION --",0);\
			DebugBreak();\
		}\
	}

#else
//!todo OM: Questa la metto qui per comodita', ma e' da risistemare eventualmente dentro NetBEE
#define nbASSERT(cond, msg) \
	{\
		if (!(cond))\
		{\
                  fprintf(stderr, "NetBee ASSERTION: %s\n\tin %s:%d\n", msg, __FILE__, __LINE__); \
			abort();\
		}\
	}

#endif
