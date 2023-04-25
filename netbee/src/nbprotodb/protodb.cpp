/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include "sax_parser.h"

#include "protodb_globals.h"

#include "../nbee/globals/utils.h"
#include "../nbee/globals/globals.h"
#include "../nbee/globals/debug.h"


//! Contains the NetPDL global database, organized for better parsing.
struct _nbNetPDLDatabase *NetPDLDatabase;

//
// This is a trick for creating automatically a structure that contains the list of NetPDL elements codes and their name
// The result is something like:
//		struct _ElementList
//		{
//			int Code;
//			const char * const Name;
//		} ElementList[]=
//					{      
//						nbNETPDL_IDEL_NETPDL, "netpdl",
//						nbNETPDL_IDEL_PROTO, "proto",
//						...
//						nbNETPDL_IDEL_INVALID, NULL,
//					};
//
// Please note that element IDs (i.e. nbNETPDL_IDEL_NETPDL ans such) corresponds
// to the values of their position in this array. Hence, we can access to the information stored
// in this array by simply asking for 
//
//      ElementList[nbNETPDL_IDEL_XXX].MemberValue
//
// Please note also that the last element of the list has the nbNETPDL_IDEL_INVALID code.
//
struct _ElementList ElementList[]=
	{      
		#define nbNETPDL_ELEMENT(ElementName, ElementStringID, ElementCodeID, PtrToAllocationFunction, PtrToCrossLinkFunction, PtrToCleanupFunction, PtrToSerializeFunction) \
					{ ElementCodeID, ElementName, PtrToAllocationFunction, PtrToCrossLinkFunction, PtrToCleanupFunction, PtrToSerializeFunction} ,
		#include <nbprotodb_elements_xml.h>
		#undef nbNETPDL_ELEMENT
	};


CNetPDLSAXParser *NetPDLDatabaseHandler;

struct _nbNetPDLDatabase *nbProtoDBXMLLoad(const char *FileName, int Flags, char *ErrBuf, int ErrBufSize)
{
	if (FileName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "The NetPDL protocol database does not exist.");
		return NULL;
	}

	// Sanity check; in case no flags are set, just go with the full NetPDL database and no validation.
	if (Flags == 0)
		Flags= nbPROTODB_FULL;

	NetPDLDatabaseHandler= new CNetPDLSAXParser;

	if (NetPDLDatabaseHandler == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	NetPDLDatabase= NetPDLDatabaseHandler->Initialize(FileName, Flags);

	if (NetPDLDatabase == NULL)
	{
		sstrncpy(ErrBuf, NetPDLDatabaseHandler->GetLastError(), ErrBufSize);
		return NULL;
	}

	return NetPDLDatabase;
}


void nbProtoDBXMLCleanup()
{
	NetPDLDatabaseHandler->Cleanup();
	delete NetPDLDatabaseHandler;
}



//int nbProtoDBXMLSave(const char *FileName, char *ErrBuf, int ErrBufSize)
//{
//	// FULVIO nbNetPDLDatabaseSave function not implemented
//	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Function not implemented.");
//	return nbFAILURE;
//}
