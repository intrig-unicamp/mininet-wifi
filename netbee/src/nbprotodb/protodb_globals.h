/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



// Allow including this file only once
#pragma once


#include <nbprotodb.h>
#include <nbprotodb_exports.h>
#include "elements_create.h"
#include "elements_organize.h"
#include "elements_delete.h"
#include "elements_serialize.h"




struct _ElementList
{
	//! Code assigned to the given element
	int Type;
	//! Name of the NetPDL element
	//! This basically is similar to a \#define because the value assigned to the variable cannot be changed
	const char * const Name;
	//! Pointer to the function that allocates the structure related to the given element
	nbNetPDLElementCreateHandler NetPDLElementCreateHandler;
	//! Pointer to the function that links a given element with others, in order to create the most useful cross-links
	nbNetPDLElementOrganizeHandler NetPDLElementOrganizeHandler;
	//! Pointer to the function that deletes the structure related to the given element
	nbNetPDLElementDeleteHandler NetPDLElementDeleteHandler;
	//! Pointer to the function that serializes the structure related to the given element
	nbNetPDLElementSerializeHandler NetPDLElementSerializeHandler;
};


//! Currently, we do not support more that this number of elements in the NetPDL file.
#define NETPDL_MAX_NELEMENTS 50000

//! Short version of the nbNETPDL_GET_ELEMENT(), used only internally to this library
#define NETPDL_GET_ELEMENT(index) (NetPDLDatabase->GlobalElementsList[index])

//! It contains the NetPDL global database, organized for better parsing.
extern struct _nbNetPDLDatabase *NetPDLDatabase;

//! It contains the list of NetPDL element that have been recognized within the XML file.
extern struct _ElementList ElementList[];

