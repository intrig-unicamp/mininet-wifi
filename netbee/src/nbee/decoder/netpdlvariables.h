/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



/*!
	\file netpdlvariables.h

	This file defines the class devoted to the management of the run-time variables of the NetPDL engine.
*/


#pragma once

#include <nbprotodb.h>
#include "netpdllookuptables.h"
#include "netpdlstandardvars.h"



/*!
	\brief This class is devoted to the management of the run-time variables of the NetPDL engine

	The NetPDL specification defines a set of run-time variables. Moreover, the user can make use
	of its own variables for other purposes.
	This class keeps track of all the run-time variables of the NetPDL engine.
*/
class CNetPDLVariables : public CNetPDLStandardVars, public CNetPDLLookupTables
{
public:
	CNetPDLVariables(char* ErrBuf, int ErrBufSize);
	virtual ~CNetPDLVariables();

	void DoGarbageCollection(int TimestampSec);
        using CNetPDLLookupTables::DoGarbageCollection;
};

