/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include "netpdlvariables.h"



/*!
	\brief Default constructor

	\param ErrBuf: pointer to a user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer belongs to the class that creates this one.
	Data stored in this buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.
*/
CNetPDLVariables::CNetPDLVariables(char* ErrBuf, int ErrBufSize)
					: CNetPDLStandardVars(ErrBuf, ErrBufSize), CNetPDLLookupTables(ErrBuf, ErrBufSize)
{
}


//! Default destructor
CNetPDLVariables::~CNetPDLVariables()
{
}


/*!
	\brief Checks if there are old entries and it deletes them.

	This method checks all the entries in the lookup tables to see if they are too old and need to be deleted.
	In addition, it deletes also the standard variables whose validity is 'this packet'.

	\param TimestampSec: current timestamp (seconds, in UNIX time). In case of lookup tables,
	entries whose (Timestamp + Lifetime) is less than this values are deleted from the table.
*/
void CNetPDLVariables::DoGarbageCollection(int TimestampSec)
{
static int Counter;

	CNetPDLStandardVars::DoGarbageCollection(TimestampSec);

	// Do not do garbage collection for lookup tables each packet, in order to save precious resources
	// So, the 'aggressive scan' is done every 10 packets, while the complete scan is done every 100 packets
	if ((Counter % 10) == 0)
	{
		if ((Counter % 100) == 0)
			CNetPDLLookupTables::DoGarbageCollection(TimestampSec, 0 /* Normal scan */);
		else
			CNetPDLLookupTables::DoGarbageCollection(TimestampSec, 1 /* Aggressive scan */);
	}

	Counter++;
}
