/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


/*!
	\file nbee.h

	Main include file of the NetBee library. You can just include this file in order to use all the features implemented in the NetBee library.
*/



// Please remember that the same #define are present in the "Globals.h" header
// So, please be careful to keep them aligned
#ifndef nbSUCCESS
	#define nbSUCCESS 0		//!< Return code for 'success'
	#define nbFAILURE -1	//!< Return code for 'failure'
	#define nbWARNING -2	//!< Return code for 'warning' (we can go on, but something wrong occurred)
#endif




#if defined(_DEBUG) && defined(_MSC_VER) && defined (_CRTDBG_MAP_ALLOC)
#pragma message( "Memory leaks checking turned on" )
#endif


// Include class definitions
#include <nbee_initcleanup.h>
#include <nbee_packetdecoder.h>
#include <nbee_packetdumpfiles.h>
#include <nbee_pxmlreader.h>
#include <nbee_netpdlutils.h>
#include <nbee_packetengine.h>
#include <nbee_profiler.h>

//[OM] temporarily removed the netvm API from netbee
//#include <nbee_packetprocess.h>

#include <nbprotodb.h>
#include <nbpflcompiler.h>
#include <nbsockutils.h>

#include <nbee_extractedfieldreader.h>



