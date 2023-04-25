/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <stdio.h>
#include "protodb_globals.h"
#include "../nbee/globals/globals.h"
#include "../nbee/globals/debug.h"
#include <nbprotodb.h>


int CountSpecificElement(uint16_t ElementType)
{
int WantedElements= 0;

	for (unsigned int i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
		if (NetPDLDatabase->GlobalElementsList[i]->Type == ElementType)
			WantedElements++;
	}

	return WantedElements;
}


int CountSpecificADTElement(uint16_t ElementType, uint16_t ContainerElementType)
{
int WantedElements= 0;

	for (unsigned int i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
		if (NetPDLDatabase->GlobalElementsList[i]->Type == ElementType)
		{
		struct _nbNetPDLElementBase *NetPDLElementCheckParent;

			NetPDLElementCheckParent= NETPDL_GET_ELEMENT(NetPDLDatabase->GlobalElementsList[i]->Parent);

			if (NetPDLElementCheckParent->Type == nbNETPDL_IDEL_ADTLIST)
			{
				NetPDLElementCheckParent= NETPDL_GET_ELEMENT(NetPDLElementCheckParent->Parent);

				if (NetPDLElementCheckParent->Type == ContainerElementType)
					WantedElements++;
			}
		}
	}

	return WantedElements;
}


struct _nbNetPDLElementProto **OrganizeListProto(int NumElementsInList, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementProto **NetPDLElementList;
int j= 0;

	if (NumElementsInList == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Error: the element list is empty.");
		return NULL;
	}

	NetPDLElementList= (struct _nbNetPDLElementProto **) malloc(sizeof(struct _nbNetPDLElementProto *) * NumElementsInList);

	if (NetPDLElementList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	for (unsigned int i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
		if (NetPDLDatabase->GlobalElementsList[i]->Type == nbNETPDL_IDEL_PROTO)
			NetPDLElementList[j++]= (struct _nbNetPDLElementProto *) NetPDLDatabase->GlobalElementsList[i];
	}

	return NetPDLElementList;
}


struct _nbNetPDLElementShowSumStructure **OrganizeListShowSumStructure(int NumElementsInList, int* Result, char* ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementShowSumStructure **NetPDLElementList;
int j= 0;

	*Result= nbSUCCESS;

	if (NumElementsInList == 0)
	{
		if (NetPDLDatabase->Flags & nbPROTODB_FULL)
		{
			*Result= nbFAILURE;
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Error: the element list '<%s>' is empty.", ElementList[nbNETPDL_IDEL_SHOWSUMSTRUCT].Name);
		}
		return NULL;
	}

	NetPDLElementList= (struct _nbNetPDLElementShowSumStructure **) malloc(sizeof(struct _nbNetPDLElementShowSumStructure *) * NumElementsInList);

	if (NetPDLElementList == NULL)
	{
		*Result= nbFAILURE;
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	for (unsigned int i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
		if (NetPDLDatabase->GlobalElementsList[i]->Type == nbNETPDL_IDEL_SUMSECTION)
			NetPDLElementList[j++]= (struct _nbNetPDLElementShowSumStructure *) NetPDLDatabase->GlobalElementsList[i];
	}

	return NetPDLElementList;
}


struct _nbNetPDLElementShowSumTemplate **OrganizeListShowSumTemplate(int NumElementsInList, int* Result, char* ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementShowSumTemplate **NetPDLElementList;
int j= 0;

	*Result= nbSUCCESS;

	if (NumElementsInList == 0)
	{
		if (NetPDLDatabase->Flags & nbPROTODB_FULL)
		{
			*Result= nbFAILURE;
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Error: the element list '<%s>' is empty.", ElementList[nbNETPDL_IDEL_SHOWSUMTEMPLATE].Name);
		}
		return NULL;
	}

	NetPDLElementList= (struct _nbNetPDLElementShowSumTemplate **) malloc(sizeof(struct _nbNetPDLElementShowSumTemplate *) * NumElementsInList);

	if (NetPDLElementList == NULL)
	{
		*Result= nbFAILURE;
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	for (unsigned int i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
		if (NetPDLDatabase->GlobalElementsList[i]->Type == nbNETPDL_IDEL_SHOWSUMTEMPLATE)
			NetPDLElementList[j++]= (struct _nbNetPDLElementShowSumTemplate *) NetPDLDatabase->GlobalElementsList[i];
	}

	return NetPDLElementList;
}


struct _nbNetPDLElementShowTemplate **OrganizeListShowTemplate(int NumElementsInList, int* Result, char* ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementShowTemplate **NetPDLElementList;
int j= 0;

	*Result= nbSUCCESS;

	if (NumElementsInList == 0)
	{
		if (NetPDLDatabase->Flags & nbPROTODB_FULL)
		{
			*Result= nbFAILURE;
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Error: no '<%s>' elements in the protocol database.", ElementList[nbNETPDL_IDEL_SHOWTEMPLATE].Name);
		}
		return NULL;
	}

	NetPDLElementList= (struct _nbNetPDLElementShowTemplate **) malloc(sizeof(struct _nbNetPDLElementShowTemplate *) * NumElementsInList);

	if (NetPDLElementList == NULL)
	{
		*Result= nbFAILURE;
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	for (unsigned int i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
		if (NetPDLDatabase->GlobalElementsList[i]->Type == nbNETPDL_IDEL_SHOWTEMPLATE)
			NetPDLElementList[j++]= (struct _nbNetPDLElementShowTemplate *) NetPDLDatabase->GlobalElementsList[i];
	}

	return NetPDLElementList;
}


struct _nbNetPDLElementAdt **OrganizeListGlobalAdt(int NumElementsInList, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementAdt **NetPDLElementList;
int j= 0;

	if (NumElementsInList == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Error: the element list is empty.");
		return NULL;
	}

	NetPDLElementList= (struct _nbNetPDLElementAdt **) malloc(sizeof(struct _nbNetPDLElementAdt *) * NumElementsInList);

	if (NetPDLElementList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	for (unsigned int i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
		if (NetPDLDatabase->GlobalElementsList[i]->Type == nbNETPDL_IDEL_ADT)
		{
		struct _nbNetPDLElementBase *NetPDLElementCheckParent;

			NetPDLElementCheckParent= NETPDL_GET_ELEMENT(NetPDLDatabase->GlobalElementsList[i]->Parent);

			if (NetPDLElementCheckParent->Type == nbNETPDL_IDEL_ADTLIST)
			{
				NetPDLElementCheckParent= NETPDL_GET_ELEMENT(NetPDLElementCheckParent->Parent);

				if (NetPDLElementCheckParent->Type != nbNETPDL_IDEL_NETPDL)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
						"Global ADT %s does not observe correct nesting rules.",
						((struct _nbNetPDLElementAdt *) NetPDLDatabase->GlobalElementsList[i])->ADTName, NetPDLElementCheckParent->ElementName);
					return NULL;
				}

				NetPDLElementList[j++]= (struct _nbNetPDLElementAdt *) NetPDLDatabase->GlobalElementsList[i];
			}
		}
	}

	NetPDLDatabase->ADTNItems= j;

	return NetPDLElementList;
}


struct _nbNetPDLElementAdt **OrganizeListLocalAdt(int NumElementsInList, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementAdt **NetPDLElementList;
int j= 0;

	if (NumElementsInList == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Error: the element list is empty.");
		return NULL;
	}

	NetPDLElementList= (struct _nbNetPDLElementAdt **) malloc(sizeof(struct _nbNetPDLElementAdt *) * NumElementsInList);

	if (NetPDLElementList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	for (unsigned int i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
		if (NetPDLDatabase->GlobalElementsList[i]->Type == nbNETPDL_IDEL_ADT)
		{
		struct _nbNetPDLElementBase *NetPDLElementCheckParent;

			NetPDLElementCheckParent= NETPDL_GET_ELEMENT(NetPDLDatabase->GlobalElementsList[i]->Parent);

			if (NetPDLElementCheckParent->Type == nbNETPDL_IDEL_ADTLIST)
			{
				NetPDLElementCheckParent= NETPDL_GET_ELEMENT(NetPDLElementCheckParent->Parent);

				if (NetPDLElementCheckParent->Type != nbNETPDL_IDEL_PROTO)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
						"Local ADT %s does not observe correct nesting rules.",
						((struct _nbNetPDLElementAdt *) NetPDLDatabase->GlobalElementsList[i])->ADTName, NetPDLElementCheckParent->ElementName);
					return NULL;
				}

				NetPDLElementList[j++]= (struct _nbNetPDLElementAdt *) NetPDLDatabase->GlobalElementsList[i];
			}
		}
	}

	NetPDLDatabase->ADTNItems= j;

	return NetPDLElementList;
}

