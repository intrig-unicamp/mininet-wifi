/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <pcre.h>
#include <nbprotodb_defs.h>
#include "protodb_globals.h"
#include "expressions.h"
#include "../nbee/globals/globals.h"
#include "../nbee/globals/utils.h"
#include "../nbee/globals/debug.h"




int OrganizeElementGeneric(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
	return nbSUCCESS;
}


int OrganizeElementProto(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementBase *NetPDLTempElement;
struct _nbNetPDLElementProto *NetPDLElement= (struct _nbNetPDLElementProto *) NetPDLElementInfo;

	NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLElement->FirstChild);

	while (NetPDLTempElement)
	{
		switch (NetPDLTempElement->Type)
		{
			case nbNETPDL_IDEL_FORMAT:
			{
			struct _nbNetPDLElementBase *NetPDLFieldsElement;

				// Let's get a pointer to the 'fields' element
				NetPDLFieldsElement= NETPDL_GET_ELEMENT(NetPDLTempElement->FirstChild);

				if (NetPDLFieldsElement->Type == nbNETPDL_IDEL_FIELDS)
				{
					// Now, let's get the first field
					NetPDLElement->FirstField= NETPDL_GET_ELEMENT(NetPDLFieldsElement->FirstChild);
				}
			}; break;

			case nbNETPDL_IDEL_EXECUTECODE:
			{
			struct _nbNetPDLElementExecuteX *NetPDLExecuteXElement;
			struct _nbNetPDLElementExecuteX *LastVerifyElement;
			struct _nbNetPDLElementExecuteX *LastInitElement;
			struct _nbNetPDLElementExecuteX *LastBeforeElement;
			struct _nbNetPDLElementExecuteX *LastAfterElement;

				LastVerifyElement= NULL;
				LastInitElement= NULL;
				LastBeforeElement= NULL;
				LastAfterElement= NULL;

				NetPDLExecuteXElement= (struct _nbNetPDLElementExecuteX*) NETPDL_GET_ELEMENT(NetPDLTempElement->FirstChild);

				while (NetPDLExecuteXElement)
				{
					switch (NetPDLExecuteXElement->Type)
					{
						case nbNETPDL_IDEL_VERIFY:
						{
							if (LastVerifyElement == NULL)
							{
								NetPDLElement->FirstExecuteVerify= NetPDLExecuteXElement;
								LastVerifyElement= NetPDLExecuteXElement;
							}
							else
							{
								LastVerifyElement->NextExecuteElement= NetPDLExecuteXElement;
								LastVerifyElement= NetPDLExecuteXElement;
							}
						}; break;

						case nbNETPDL_IDEL_INIT:
						{
							if (LastInitElement == NULL)
							{
								NetPDLElement->FirstExecuteInit= NetPDLExecuteXElement;
								LastInitElement= NetPDLExecuteXElement;
							}
							else
							{
								LastInitElement->NextExecuteElement= NetPDLExecuteXElement;
								LastInitElement= NetPDLExecuteXElement;
							}
						}; break;

						case nbNETPDL_IDEL_AFTER:
						{
							if (LastAfterElement == NULL)
							{
								NetPDLElement->FirstExecuteAfter= NetPDLExecuteXElement;
								LastAfterElement= NetPDLExecuteXElement;
							}
							else
							{
								LastAfterElement->NextExecuteElement= NetPDLExecuteXElement;
								LastAfterElement= NetPDLExecuteXElement;
							}
						}; break;

						case nbNETPDL_IDEL_BEFORE:
						{
							if (LastBeforeElement == NULL)
							{
								NetPDLElement->FirstExecuteBefore= NetPDLExecuteXElement;
								LastBeforeElement= NetPDLExecuteXElement;
							}
							else
							{
								LastBeforeElement->NextExecuteElement= NetPDLExecuteXElement;
								LastBeforeElement= NetPDLExecuteXElement;
							}
						}; break;
					}

					NetPDLExecuteXElement= (struct _nbNetPDLElementExecuteX*) NETPDL_GET_ELEMENT(NetPDLExecuteXElement->NextSibling);
				}
			}; break;

			case nbNETPDL_IDEL_ENCAPSULATION:
			{
				NetPDLElement->FirstEncapsulationItem= NETPDL_GET_ELEMENT(NetPDLTempElement->FirstChild);
			}; break;
		}

		NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
	}

	// Map protocol to visualization templates
	if (NetPDLElement->ShowSumTemplateName)
	{
		for (unsigned int i= 0; i < NetPDLDatabase->ShowSumTemplateNItems; i++)
		{
			if (strcmp(NetPDLElement->ShowSumTemplateName, NetPDLDatabase->ShowSumTemplateList[i]->Name) == 0)
			{
				NetPDLElement->ShowSumTemplateInfo= NetPDLDatabase->ShowSumTemplateList[i];
				break;
			}
		}

		if (NetPDLElement->ShowSumTemplateInfo == NULL)
		{
			// If the template has not been found, let's print an error
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
				"Protocol %s points to showsumtemplate '%s' which does not exist.",
				NetPDLElement->Name, NetPDLElement->ShowSumTemplateName);

			return nbFAILURE;
		}
	}

	return nbSUCCESS;
}


int OrganizeElementUpdateLookupTable(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementLookupKeyData *LastKeyElement;
struct _nbNetPDLElementLookupKeyData *LastDataElement;
struct _nbNetPDLElementKeyData* LookupTableKeyDef;
struct _nbNetPDLElementKeyData* LookupTableDataDef;
struct _nbNetPDLElementUpdateLookupTable *NetPDLElement= (struct _nbNetPDLElementUpdateLookupTable *) NetPDLElementInfo;
struct _nbNetPDLElementLookupTable *LookupTable;
unsigned int ElementNumber;
int LookupTableFound;

	LookupTableFound= 0;

	for (ElementNumber= 1; ElementNumber < NetPDLDatabase->GlobalElementsListNItems; ElementNumber++)
	{
		if (NetPDLDatabase->GlobalElementsList[ElementNumber]->Type == nbNETPDL_IDEL_LOOKUPTABLE)
		{
			LookupTable= (struct _nbNetPDLElementLookupTable *) NetPDLDatabase->GlobalElementsList[ElementNumber];

			if (strcmp(NetPDLElement->TableName, LookupTable->Name) == 0)
			{
				LookupTableFound= 1;
				break;
			}
		}
	}

	if (LookupTableFound == 0)
	{
		// Table name doesn't match
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
								"Lookup table '%s' in protocol '%s' cannot be found in the NetPDL description.", NetPDLElement->TableName, GetProtoName(NetPDLElementInfo));
		return nbFAILURE;
	}

	// Let's scan for keys and data
	LastKeyElement= NULL;
	LastDataElement= NULL;

	LookupTableKeyDef= LookupTable->FirstKey;
	LookupTableDataDef= LookupTable->FirstData;


	struct _nbNetPDLElementLookupKeyData *NetPDLLookupKeyDataElement;

	NetPDLLookupKeyDataElement= (struct _nbNetPDLElementLookupKeyData *) NETPDL_GET_ELEMENT(NetPDLElement->FirstChild);

	while ((NetPDLLookupKeyDataElement) && (NetPDLLookupKeyDataElement->Type == nbNETPDL_IDEL_LOOKUPKEY))
	{
		if ((LookupTableKeyDef == NULL) || (LookupTableKeyDef->Type != nbNETPDL_IDEL_KEY))
		{
			// Different number of keys members
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
							"Lookup table '%s' in protocol '%s' contains a different number of keys compared to the ones found in the definition.",
							NetPDLElement->TableName, GetProtoName(NetPDLElementInfo));
			return nbFAILURE;
		}

		if (NetPDLLookupKeyDataElement->Mask)
			NetPDLElement->KeysHaveMasks= 1;

		if (LastKeyElement == NULL)
		{
			NetPDLElement->FirstKey= NetPDLLookupKeyDataElement;
			LastKeyElement= NetPDLLookupKeyDataElement;
		}
		else
		{
			LastKeyElement->NextKeyData= NetPDLLookupKeyDataElement;
			LastKeyElement= NetPDLLookupKeyDataElement;
		}

		// Check that the return type is correct
		if ((LookupTableKeyDef->KeyDataType == nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER) && 
			(NetPDLLookupKeyDataElement->ExprTree->ReturnType != nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER))
		{
			// Different number of keys or data members
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
								"Lookup table '%s' in protocol '%s' contains a key whose return type does not match definition of key '%s'.",
								NetPDLElement->TableName, GetProtoName(NetPDLElementInfo), LookupTableKeyDef->Name);

		};

		if ((LookupTableKeyDef->KeyDataType == nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER) && 
			(NetPDLLookupKeyDataElement->ExprTree->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER))
//		if ((LookupTableKeyDef->KeyDataType == nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER) && 
//			(NetPDLLookupKeyDataElement->ExprTree->ReturnType == nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER))
		{
			// Different number of keys or data members
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
								"Lookup table '%s' in protocol '%s' contains a key whose return type does not match definition of key '%s'.",
								NetPDLElement->TableName, GetProtoName(NetPDLElementInfo), LookupTableKeyDef->Name);

		};

		LookupTableKeyDef= LookupTableKeyDef->NextKeyData;
		NetPDLLookupKeyDataElement= (struct _nbNetPDLElementLookupKeyData *) NETPDL_GET_ELEMENT(NetPDLLookupKeyDataElement->NextSibling);
	}

	if (LookupTableKeyDef)
	{
		// Different number of keys members
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
						"Lookup table '%s' in protocol '%s' contains a different number of keys compared to the ones found in the definition.",
						NetPDLElement->TableName, GetProtoName(NetPDLElementInfo));
		return nbFAILURE;
	}


	// Let's start the check with data
	// Please take care that in some cases, the update-lookuptable does not have 'data' elements
	// (which are present only in case we're doing an ADD)
	if (NetPDLLookupKeyDataElement)
	{
		while ((NetPDLLookupKeyDataElement) && (NetPDLLookupKeyDataElement->Type == nbNETPDL_IDEL_LOOKUPDATA))
		{
			if ((LookupTableDataDef == NULL) || (LookupTableDataDef->Type != nbNETPDL_IDEL_DATA))
			{
				// Different number of keys members
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
								"Lookup table '%s' in protocol '%s' contains a different number of data compared to the ones found in the definition.",
								NetPDLElement->TableName, GetProtoName(NetPDLElementInfo));
				return nbFAILURE;
			}

			if (NetPDLLookupKeyDataElement->Mask)
				NetPDLElement->KeysHaveMasks= 1;

			if (LastDataElement == NULL)
			{
				NetPDLElement->FirstData= NetPDLLookupKeyDataElement;
				LastDataElement= NetPDLLookupKeyDataElement;
			}
			else
			{
				LastDataElement->NextKeyData= NetPDLLookupKeyDataElement;
				LastDataElement= NetPDLLookupKeyDataElement;
			}

			// Check that the return type is correct
			if ((LookupTableDataDef->KeyDataType == nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER) && 
				(NetPDLLookupKeyDataElement->ExprTree->ReturnType != nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER))
			{
				// Different number of keys or data members
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
								"Lookup table '%s' in protocol '%s' contains a data whose return type does not match definition of data '%s'.",
								NetPDLElement->TableName, GetProtoName(NetPDLElementInfo), LookupTableKeyDef->Name);

			};

			if ((LookupTableDataDef->KeyDataType == nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER) && 
				(NetPDLLookupKeyDataElement->ExprTree->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER))
//			if ((LookupTableDataDef->KeyDataType == nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER) && 
//				(NetPDLLookupKeyDataElement->ExprTree->ReturnType == nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER))
			{
				// Different number of keys or data members
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
								"Lookup table '%s' in protocol '%s' contains a data whose return type does not match definition of data '%s'.",
								NetPDLElement->TableName, GetProtoName(NetPDLElementInfo), LookupTableKeyDef->Name);

			};

			LookupTableDataDef= LookupTableDataDef->NextKeyData;
			NetPDLLookupKeyDataElement= (struct _nbNetPDLElementLookupKeyData *) NETPDL_GET_ELEMENT(NetPDLLookupKeyDataElement->NextSibling);
		}

		if (LookupTableDataDef)
		{
			// Different number of keys members
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
							"Lookup table '%s' in protocol '%s' contains a different number of data compared to the ones found in the definition.",
							NetPDLElement->TableName, GetProtoName(NetPDLElementInfo));
			return nbFAILURE;
		}
	}


	// Check that we're not trying to insert a dynamic entry into a table that was declared with only static entries
	if ((NetPDLElement->Validity != nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPFOREVER) &&
		(LookupTable->AllowDynamicEntries == 0))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
								"Lookup table '%s' in protocol '%s' cannot contain dynamic entries.",
								NetPDLElement->TableName, GetProtoName(NetPDLElementInfo));
		return nbFAILURE;
	}

	// Check if we have masks
	if ((NetPDLElement->Validity == nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_REPLACEONHIT) ||
		(NetPDLElement->Validity == nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_ADDONHIT))
	{
		if (NetPDLElement->KeysHaveMasks == 0)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
									"Lookup table '%s' in protocol '%s' contains an entry that must have masks.",
									NetPDLElement->TableName, GetProtoName(NetPDLElementInfo));
			return nbFAILURE;
		}
	}

	return nbSUCCESS;
}


int OrganizeElementShowTemplate(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementBase *NetPDLTempElement;
struct _nbNetPDLElementShowTemplate *NetPDLElement= (struct _nbNetPDLElementShowTemplate *) NetPDLElementInfo;

	NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLElement->FirstChild);

	while (NetPDLTempElement)
	{
		if (NetPDLTempElement->Type == nbNETPDL_IDEL_SHOWMAP)
			NetPDLElement->MappingTableInfo= (struct _nbNetPDLElementSwitch *) NETPDL_GET_ELEMENT(NetPDLTempElement->FirstChild);

		if (NetPDLTempElement->Type == nbNETPDL_IDEL_SHOWDTL)
			NetPDLElement->CustomTemplateFirstField= NETPDL_GET_ELEMENT(NetPDLTempElement->FirstChild);

		NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
	}

	return nbSUCCESS;
}


int OrganizeElementIf(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementBase *NetPDLTempElement;
struct _nbNetPDLElementIf *NetPDLElement= (struct _nbNetPDLElementIf *) NetPDLElementInfo;

	NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLElement->FirstChild);

	while (NetPDLTempElement)
	{
		if (NetPDLTempElement->Type == nbNETPDL_IDEL_IFTRUE)
			NetPDLElement->FirstValidChildIfTrue= NETPDL_GET_ELEMENT(NetPDLTempElement->FirstChild);

		if (NetPDLTempElement->Type == nbNETPDL_IDEL_IFFALSE)
			NetPDLElement->FirstValidChildIfFalse= NETPDL_GET_ELEMENT(NetPDLTempElement->FirstChild);

		if (NetPDLTempElement->Type == nbNETPDL_IDEL_MISSINGPACKETDATA)
			NetPDLElement->FirstValidChildIfMissingPacketData= NETPDL_GET_ELEMENT(NetPDLTempElement->FirstChild);

		NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
	}

	return nbSUCCESS;
}


int OrganizeElementCase(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementBase *NetPDLNextSiblingElement;
struct _nbNetPDLElementCase *NetPDLElement= (struct _nbNetPDLElementCase *) NetPDLElementInfo;

	NetPDLNextSiblingElement= NETPDL_GET_ELEMENT(NetPDLElement->NextSibling);

	if ( (NetPDLNextSiblingElement) && (NetPDLNextSiblingElement->Type == nbNETPDL_IDEL_CASE))
		NetPDLElement->NextCase= (struct _nbNetPDLElementCase *) NetPDLNextSiblingElement;

	return nbSUCCESS;
}


int OrganizeElementSwitch(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementBase *NetPDLTempElement;
struct _nbNetPDLElementSwitch *NetPDLElement= (struct _nbNetPDLElementSwitch *) NetPDLElementInfo;

	NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLElement->FirstChild);

	while (NetPDLTempElement)
	{
		if (NetPDLTempElement->Type == nbNETPDL_IDEL_DEFAULT)
			NetPDLElement->DefaultCase= (struct _nbNetPDLElementCase *) NetPDLTempElement;

		if ((NetPDLTempElement->Type == nbNETPDL_IDEL_CASE) && (NetPDLElement->FirstCase == NULL))
			NetPDLElement->FirstCase= (struct _nbNetPDLElementCase *) NetPDLTempElement;

		NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
	}

	return nbSUCCESS;
}


int OrganizeElementLoop(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementBase *NetPDLTempElement;
struct _nbNetPDLElementLoop *NetPDLElement= (struct _nbNetPDLElementLoop *) NetPDLElementInfo;

	NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLElement->FirstChild);

	if(NetPDLTempElement!=NULL)
	{

		if (NetPDLTempElement->Type == nbNETPDL_IDEL_MISSINGPACKETDATA)
		{
			NetPDLElement->FirstValidChildIfMissingPacketData= NETPDL_GET_ELEMENT(NetPDLTempElement->FirstChild);
			NetPDLElement->FirstValidChildInLoop= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
		}
		else
		{
			NetPDLElement->FirstValidChildInLoop= NetPDLTempElement;
		}
		return nbSUCCESS;
	}
	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
	"Found a loop without any inner element.");

	return nbFAILURE;
}


int OrganizeElementBlock(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementBlock *NetPDLElement= (struct _nbNetPDLElementBlock *) NetPDLElementInfo;

	// Map protocol to visualization templates
	if (NetPDLElement->ShowSumTemplateName)
	{
		for (unsigned int i= 0; i < NetPDLDatabase->ShowSumTemplateNItems; i++)
		{
			if (strcmp(NetPDLElement->ShowSumTemplateName, NetPDLDatabase->ShowSumTemplateList[i]->Name) == 0)
			{
				NetPDLElement->ShowSumTemplateInfo= NetPDLDatabase->ShowSumTemplateList[i];
				break;
			}
		}

		if (NetPDLElement->ShowSumTemplateInfo == NULL)
		{
			// If the template has not been found, let's print an error
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
				"Block %s points to showsumtemplate '%s' which does not exist.",
				NetPDLElement->Name, NetPDLElement->ShowSumTemplateName);

			return nbFAILURE;
		}
	}

	return nbSUCCESS;
}


int OrganizeElementIncludeBlk(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementBase *FormatElement;
struct _nbNetPDLElementBase *BlockElement;
struct _nbNetPDLElementIncludeBlk *NetPDLElement= (struct _nbNetPDLElementIncludeBlk *) NetPDLElementInfo;

	// Locate the current 'format' section
	FormatElement= (struct _nbNetPDLElementBase *) NetPDLElement;
	while (FormatElement->Type != nbNETPDL_IDEL_FORMAT)
		FormatElement= NETPDL_GET_ELEMENT(FormatElement->Parent);

	// Let's check the list of blocks, which are child of the protocol
	BlockElement= NETPDL_GET_ELEMENT(FormatElement->FirstChild);

	while (BlockElement != nbNETPDL_INVALID_ELEMENT)
	{
		if (BlockElement->Type == nbNETPDL_IDEL_BLOCK)
		{
			if (strcmp(((struct _nbNetPDLElementBlock *) BlockElement)->Name, NetPDLElement->IncludedBlockName) == 0)
			{
				NetPDLElement->IncludedBlock= (struct _nbNetPDLElementBlock *) BlockElement;
				break;
			}
		}
		BlockElement= NETPDL_GET_ELEMENT(BlockElement->NextSibling);
	}

	if (BlockElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			" Element <includeblk name=%s> (in protocol '%s') points to a block that cannot be found.",
			NetPDLElement->IncludedBlockName, GetProtoName(FormatElement) ); //((struct _nbNetPDLElementProto *) FormatElement)->Name);
		return nbFAILURE;
	}

	return nbSUCCESS;
}


int OrganizeFieldBase(struct _nbNetPDLElementFieldBase *NetPDLElement, char *ErrBuf, int ErrBufSize)
{
	// Let's check the 'showtemplate'
	if ((NetPDLElement->ShowTemplateInfo == NULL) && NetPDLElement->ShowTemplateName)
	{
		for (unsigned int i= 0; i < NetPDLDatabase->ShowTemplateNItems; i++)
		{
			if (strcmp(NetPDLDatabase->ShowTemplateList[i]->Name, NetPDLElement->ShowTemplateName) == 0)
			{
				NetPDLElement->ShowTemplateInfo= NetPDLDatabase->ShowTemplateList[i];
				break;
			}
		}

		if (NetPDLElement->ShowTemplateInfo == NULL)
		{
			// If the template has not been found, let's print an error
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
				"%s %s points to showtemplate '%s' which does not exist.",
				NetPDLElement->ElementName, NetPDLElement->Name, NetPDLElement->ShowTemplateName);

			return nbFAILURE;
		}
	}

	return nbSUCCESS;
}

int OrganizeElementField(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementFieldBase *NetPDLBaseElement= (struct _nbNetPDLElementFieldBase *) NetPDLElementInfo;

	switch(NetPDLBaseElement->FieldType)
	{
	case nbNETPDL_ID_FIELD_BIT:
		{
			struct _nbNetPDLElementFieldBit *NetPDLElement;
			struct _nbNetPDLElementFieldBase *PreviousSibling;

			NetPDLElement= (struct _nbNetPDLElementFieldBit *) NetPDLElementInfo;

			// Let's get previous element
			PreviousSibling= (struct _nbNetPDLElementFieldBase *) NETPDL_GET_ELEMENT(NetPDLElementInfo->PreviousSibling);

			/*
				Let's check if this is the last field of a bitfield group.
				We have two heuristics:
				- if next field is not a bit field, last bitfield needed the IsLastBitField to 'true'
				- if next field is a bit field, but previous bit field had the mask covering the last bit,
				then also the previous field needed the IsLastBitField to 'true'
			*/

			// First, always set this element as the last one of the group
			NetPDLElement->IsLastBitField= 1;

			// Then, let's check if we have to unset the 'IsLastBitField' from previous element
			// This has to be done only if the previous field is still a bitfield
			if ((PreviousSibling) && (PreviousSibling->FieldType == nbNETPDL_ID_FIELD_BIT))
			{
				struct _nbNetPDLElementFieldBit *PreviousBitField;

				PreviousBitField= (struct _nbNetPDLElementFieldBit *) PreviousSibling;

				// If the last bit of the mask is '0', then previous bitfield is not the last of the group
				if ((PreviousBitField->BitMask % 2) == 0)
					PreviousBitField->IsLastBitField= 0;
			}
		}break;
	case nbNETPDL_ID_CFIELD_TLV:
		{
		struct _nbNetPDLElementCfieldTLV *NetPDLElement= (struct _nbNetPDLElementCfieldTLV *) NetPDLElementInfo;
		struct _nbNetPDLElementBase *NetPDLTempElement;

			NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLElementInfo->FirstChild);

			while (NetPDLTempElement)
			{
				switch (NetPDLTempElement->Type)
				{
				case nbNETPDL_IDEL_SUBFIELD:
					{
					struct _nbNetPDLElementSubfield *NetPDLSubfieldElement= (struct _nbNetPDLElementSubfield *) NetPDLTempElement;

						switch (NetPDLSubfieldElement->Portion)
						{
						case nbNETPDL_ID_SUBFIELD_PORTION_TLV_TYPE:
							{NetPDLElement->TypeSubfield= NetPDLTempElement;}break;
						case nbNETPDL_ID_SUBFIELD_PORTION_TLV_LENGTH:
							{NetPDLElement->LengthSubfield= NetPDLTempElement;}break;
						case nbNETPDL_ID_SUBFIELD_PORTION_TLV_VALUE:
							{NetPDLElement->ValueSubfield= NetPDLTempElement;}break;
						default:
							{
								// If the portion has not been found, let's print an error
								errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
									"Complex field %s declares a subfield '%s' which does not exist.",
									NetPDLElement->Name, NetPDLSubfieldElement->Name);

								return nbFAILURE;
							}
						}
					}break;
				//case nbNETPDL_IDEL_CHOICE:
				//	{
				//	}break;
				default:
					{
						// If the neither subfield nor choice has been found, let's print an error
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
							"Complex field %s contains '<%s>' element which is not allowed.",
							NetPDLElement->Name, NetPDLTempElement->ElementName);

						return nbFAILURE;
					}
				}

				NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
			}
		};break;
	case nbNETPDL_ID_CFIELD_HDRLINE:
		{
		struct _nbNetPDLElementCfieldHdrline *NetPDLElement= (struct _nbNetPDLElementCfieldHdrline *) NetPDLElementInfo;
		struct _nbNetPDLElementBase *NetPDLTempElement;

			NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLElement->FirstChild);

			while (NetPDLTempElement)
			{
				switch (NetPDLTempElement->Type)
				{
				case nbNETPDL_IDEL_SUBFIELD:
					{
					struct _nbNetPDLElementSubfield *NetPDLSubfieldElement= (struct _nbNetPDLElementSubfield *) NetPDLTempElement;

						switch (NetPDLSubfieldElement->Portion)
						{
						case nbNETPDL_ID_SUBFIELD_PORTION_HDRLINE_HNAME:
							{NetPDLElement->HeaderNameSubfield= NetPDLTempElement;}break;
						case nbNETPDL_ID_SUBFIELD_PORTION_HDRLINE_HVALUE:
							{NetPDLElement->HeaderValueSubfield= NetPDLTempElement;}break;
						default:
							{
								// If the portion has not been found, let's print an error
								errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
									"Complex field %s declares a subfield '%s' which does not exist.",
									NetPDLElement->Name, NetPDLSubfieldElement->Name);

								return nbFAILURE;
							}
						}
					}break;
				//case nbNETPDL_IDEL_CHOICE:
				//	{
				//	}break;
				default:
					{
						// If the neither subfield nor choice has been found, let's print an error
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
							"Complex field %s contains '<%s>' element which is not allowed.",
							NetPDLElement->Name, NetPDLTempElement->ElementName);

						return nbFAILURE;
					}
				}

				NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
			}
		};break;
	case nbNETPDL_ID_CFIELD_DYNAMIC:
		{
		struct _nbNetPDLElementCfieldDynamic *NetPDLElement= (struct _nbNetPDLElementCfieldDynamic *) NetPDLElementInfo;
		struct _nbNetPDLElementBase *NetPDLTempElement;

			NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLElement->FirstChild);

			while (NetPDLTempElement)
			{
				switch (NetPDLTempElement->Type)
				{
				case nbNETPDL_IDEL_SUBFIELD:
				//case nbNETPDL_IDEL_CHOICE:
					{
					struct _nbNetPDLElementSubfield *NetPDLSubfieldElement= (struct _nbNetPDLElementSubfield *) NetPDLTempElement;
					int PCREStringNumber;

						PCREStringNumber= pcre_get_stringnumber(
											(pcre *) NetPDLElement->PatternPCRECompiledRegExp, 
											NetPDLSubfieldElement->PortionName);

						switch (PCREStringNumber)
						{
						case PCRE_ERROR_NOSUBSTRING:
							{
								// If the portion has not been found, let's print an error
								errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
									"Complex field %s declares a subfield '%s' which does not exist.",
									NetPDLElement->Name, NetPDLSubfieldElement->Name);

								return nbFAILURE;
							}; break;
						default:
							{
								NetPDLElement->NamesList[PCREStringNumber-1]= NULL;
							}
						}
						
					}break;
				default:
					{
						// If the neither subfield nor choice has been found, let's print an error
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
							"Complex field %s contains '<%s>' element which is not allowed.",
							NetPDLElement->Name, NetPDLTempElement->ElementName);

						return nbFAILURE;
					}
				}

				NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
			}
		};break;
	case nbNETPDL_ID_CFIELD_XML:
		{
		struct _nbNetPDLElementCfieldXML *NetPDLElement= (struct _nbNetPDLElementCfieldXML *) NetPDLElementInfo;
		struct _nbNetPDLElementBase *NetPDLTempElement;

			NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLElement->FirstChild);

			while (NetPDLTempElement)
			{
				switch (NetPDLTempElement->Type)
				{
				case nbNETPDL_IDEL_MAP:
					{
					}break;
				default:
					{
						// If the neither subfield nor choice has been found, let's print an error
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
							"Complex field %s contains '<%s>' element which is not allowed.",
							NetPDLElement->Name, NetPDLTempElement->ElementName);

						return nbFAILURE;
					}
				}

				NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
			}
		};break;
	default:
		// Several nbNETPDL_ID_* missing; this 'default' is required only to prevent warnings in the compiler
		break;
	}

	return OrganizeFieldBase(NetPDLBaseElement, ErrBuf, ErrBufSize);
}


int OrganizeElementMap(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementMapBase *NetPDLElement= (struct _nbNetPDLElementMapBase *) NetPDLElementInfo;

	if (NetPDLElement->FirstChild)
	{
		// If the map element has at most one child, let's print an error
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Child elements are not allowed for map element %s.",
			NetPDLElement->Name);

		return nbFAILURE;
	}

	if (NetPDLElement->NextSibling)
	{
	struct _nbNetPDLElementBase *NetPDLTempElement;

		NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLElement->NextSibling);

		switch (NetPDLTempElement->Type)
		{
		case nbNETPDL_IDEL_MAP:
			{
				switch (NetPDLElement->FieldType)
				{
				case nbNETPDL_ID_FIELD_MAP_XML_DOCTYPE:
				case nbNETPDL_ID_FIELD_MAP_XML_ELEMENT:
					{
						if (((struct _nbNetPDLElementMapBase *) NetPDLTempElement)->FieldType != nbNETPDL_ID_FIELD_MAP_XML_ELEMENT)
						{
							// If the map element has at most one child, let's print an error
							errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
								"Map element %s is not followed by correct map element.",
								NetPDLElement->Name);

							return nbFAILURE;
						}
					}; break;
				default:
					// Several nbNETPDL_ID_* missing; this 'default' is required only to prevent warnings in the compiler
					break;

				}
			}; break;
		default:
			{
				// If the map element has at most one child, let's print an error
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
					"Map element %s is not followed by another map element.",
					NetPDLElement->Name);

				return nbFAILURE;
			}
		}
	}

	switch (NetPDLElement->FieldType)
	{
	case nbNETPDL_ID_FIELD_MAP_XML_PI:
		{
		char PCREString[2048] = {'\0'};
//		int PCREFlags= PCRE_DOTALL | PCRE_PARTIAL;
			
			if (sprintf(PCREString, "<\?%s.*?\?>", NetPDLElement->RefName) == -1)
				return nbFAILURE;
		}; break;
	case nbNETPDL_ID_FIELD_MAP_XML_DOCTYPE:
		break;
	case nbNETPDL_ID_FIELD_MAP_XML_ELEMENT:
		break;
	default:
		// Several nbNETPDL_ID_* missing; this 'default' is required only to prevent warnings in the compiler
		break;
	}

	return nbSUCCESS;
}


int OrganizeElementSubfield(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementSubfield *NetPDLElement= (struct _nbNetPDLElementSubfield *) NetPDLElementInfo;

	if (OrganizeFieldBase((struct _nbNetPDLElementFieldBase *) NetPDLElement, ErrBuf, ErrBufSize) == nbFAILURE)
		return nbFAILURE;

	if (NetPDLElement->ComplexSubfieldInfo)
	{
		// Let's organize data structure related to complex subfield
		switch(NetPDLElement->ComplexSubfieldInfo->FieldType)
		{
		case nbNETPDL_ID_CFIELD_TLV:
			{
			struct _nbNetPDLElementCfieldTLV *NetPDLFieldTLVElement= (struct _nbNetPDLElementCfieldTLV *) NetPDLElement->ComplexSubfieldInfo;
			struct _nbNetPDLElementBase *NetPDLTempElement;

				NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLElementInfo->FirstChild);

				while (NetPDLTempElement)
				{
					switch (NetPDLTempElement->Type)
					{
					case nbNETPDL_IDEL_SUBFIELD:
						{
						struct _nbNetPDLElementSubfield *NetPDLSubfieldElement= (struct _nbNetPDLElementSubfield *) NetPDLTempElement;

							switch (NetPDLSubfieldElement->Portion)
							{
							case nbNETPDL_ID_SUBFIELD_PORTION_TLV_TYPE:
								{NetPDLFieldTLVElement->TypeSubfield= NetPDLTempElement;}break;
							case nbNETPDL_ID_SUBFIELD_PORTION_TLV_LENGTH:
								{NetPDLFieldTLVElement->LengthSubfield= NetPDLTempElement;}break;
							case nbNETPDL_ID_SUBFIELD_PORTION_TLV_VALUE:
								{NetPDLFieldTLVElement->ValueSubfield= NetPDLTempElement;}break;
							default:
								{
									// If the portion has not been found, let's print an error
									errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
										"Complex subfield %s declares a subfield '%s' which does not exist.",
										NetPDLFieldTLVElement->Name, NetPDLSubfieldElement->Name);

									return nbFAILURE;
								}
							}
						}break;
					//// TEMP FABIO 27/06/2008: supporto SET,CHOICE per i subfield sospeso
					//case nbNETPDL_IDEL_CHOICE:
					//	{
					//	}break;
					default:
						{
							// If the neither subfield nor choice has been found, let's print an error
							errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
								"Complex subfield %s contains '<%s>' element which is not allowed.",
								NetPDLFieldTLVElement->Name, NetPDLTempElement->ElementName);

							return nbFAILURE;
						}
					}

					NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
				}
			}break;
		case nbNETPDL_ID_CFIELD_DELIMITED:
			{
			//struct _nbNetPDLElementCfieldDelimited *NetPDLFieldDelimitedElement= (struct _nbNetPDLElementCfieldDelimited *) NetPDLElement->ComplexSubfieldInfo;
			}break;
		case nbNETPDL_ID_CFIELD_LINE:
			{
			//struct _nbNetPDLElementCfieldLine *NetPDLFieldLineElement= (struct _nbNetPDLElementCfieldLine *) NetPDLElement->ComplexSubfieldInfo;
			};break;
		case nbNETPDL_ID_CFIELD_HDRLINE:
			{
			struct _nbNetPDLElementCfieldHdrline *NetPDLFieldHdrlineElement= (struct _nbNetPDLElementCfieldHdrline *) NetPDLElement->ComplexSubfieldInfo;
			struct _nbNetPDLElementBase *NetPDLTempElement;

				NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLFieldHdrlineElement->FirstChild);

				while (NetPDLTempElement)
				{
					switch (NetPDLTempElement->Type)
					{
					case nbNETPDL_IDEL_SUBFIELD:
						{
						struct _nbNetPDLElementSubfield *NetPDLSubfieldElement= (struct _nbNetPDLElementSubfield *) NetPDLTempElement;

							switch (NetPDLSubfieldElement->Portion)
							{
							case nbNETPDL_ID_SUBFIELD_PORTION_HDRLINE_HNAME:
								{NetPDLFieldHdrlineElement->HeaderNameSubfield= NetPDLTempElement;}break;
							case nbNETPDL_ID_SUBFIELD_PORTION_HDRLINE_HVALUE:
								{NetPDLFieldHdrlineElement->HeaderValueSubfield= NetPDLTempElement;}break;
							default:
								{
									// If the portion has not been found, let's print an error
									errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
										"Complex field %s declares a subfield '%s' which does not exist.",
										NetPDLFieldHdrlineElement->Name, NetPDLSubfieldElement->Name);

									return nbFAILURE;
								}
							}
						}break;
					//// TEMP FABIO 27/06/2008: supporto SET,CHOICE per i subfield sospeso
					//case nbNETPDL_IDEL_CHOICE:
					//	{
					//	}break;
					default:
						{
							// If the neither subfield nor choice has been found, let's print an error
							errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
								"Complex field %s contains '<%s>' element which is not allowed.",
								NetPDLFieldHdrlineElement->Name, NetPDLTempElement->ElementName);

							return nbFAILURE;
						}
					}

					NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
				}
			};break;
		case nbNETPDL_ID_CFIELD_DYNAMIC:
			{
			struct _nbNetPDLElementCfieldDynamic *NetPDLFieldDynamiclement= (struct _nbNetPDLElementCfieldDynamic *) NetPDLElement->ComplexSubfieldInfo;
			struct _nbNetPDLElementBase *NetPDLTempElement;

				NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLFieldDynamiclement->FirstChild);

				while (NetPDLTempElement)
				{
					switch (NetPDLTempElement->Type)
					{
					case nbNETPDL_IDEL_SUBFIELD:
					//// TEMP FABIO 27/06/2008: supporto SET,CHOICE per i subfield sospeso
					//case nbNETPDL_IDEL_CHOICE:
						{
						struct _nbNetPDLElementSubfield *NetPDLSubfieldElement= (struct _nbNetPDLElementSubfield *) NetPDLTempElement;
						int PCREStringNumber;

							PCREStringNumber= pcre_get_stringnumber(
												(pcre *) NetPDLFieldDynamiclement->PatternPCRECompiledRegExp, 
												NetPDLSubfieldElement->PortionName);

							switch (PCREStringNumber)
							{
							case PCRE_ERROR_NOSUBSTRING:
								{
									// If the portion has not been found, let's print an error
									errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
										"Complex subfield %s declares a subfield '%s' which does not exist.",
										NetPDLFieldDynamiclement->Name, NetPDLSubfieldElement->Name);

									return nbFAILURE;
								}; break;
							default:
								{
									NetPDLFieldDynamiclement->NamesList[PCREStringNumber-1]= NULL;
								}
							}
							
						}break;
					default:
						{
							// If the neither subfield nor choice has been found, let's print an error
							errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
								"Complex subfield %s contains '<%s>' element which is not allowed.",
								NetPDLFieldDynamiclement->Name, NetPDLTempElement->ElementName);

							return nbFAILURE;
						}
					}

					NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
				}
			}break;
		case nbNETPDL_ID_CFIELD_ASN1:
			{
			//struct _nbNetPDLElementCfieldASN1 *NetPDLFieldASN1Element= (struct _nbNetPDLElementCfieldASN1 *) NetPDLElement->ComplexSubfieldInfo;
			};break;
		//// TEMP FABIO 27/06/2008: XML under construction
		case nbNETPDL_ID_CFIELD_XML:
			{
			struct _nbNetPDLElementCfieldXML *NetPDLFieldXMLElement= (struct _nbNetPDLElementCfieldXML *) NetPDLElement->ComplexSubfieldInfo;
			struct _nbNetPDLElementBase *NetPDLTempElement;

				NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLFieldXMLElement->FirstChild);

				while (NetPDLTempElement)
				{
					switch (NetPDLTempElement->Type)
					{
					case nbNETPDL_IDEL_MAP:
						{
						}break;
					default:
						{
							// If the neither subfield nor choice has been found, let's print an error
							errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
								"Complex field %s contains '<%s>' element which is not allowed.",
								NetPDLFieldXMLElement->Name, NetPDLTempElement->ElementName);

							return nbFAILURE;
						}
					}

					NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
				}
			};break;
		default:
			{
				// If the field type has not been found, let's print an error
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
					"Complex subfield '%s' declares a complex type which is not allowed.",
					NetPDLElement->Name);

				return nbFAILURE;
			}
		}

		// Let's update data structure related to complex subfield properly
		NetPDLElement->ComplexSubfieldInfo->FirstChild= NetPDLElement->FirstChild;
		NetPDLElement->ComplexSubfieldInfo->Name= NetPDLElement->Name;
		NetPDLElement->ComplexSubfieldInfo->LongName= NetPDLElement->LongName;
		NetPDLElement->ComplexSubfieldInfo->ShowTemplateName= NetPDLElement->ShowTemplateName;
		NetPDLElement->ComplexSubfieldInfo->ShowTemplateInfo= NetPDLElement->ShowTemplateInfo;
	}

	return nbSUCCESS;
}


int OrganizeElementFieldmatch(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementFieldmatch *NetPDLElement= (struct _nbNetPDLElementFieldmatch *) NetPDLElementInfo;
struct _nbNetPDLElementBase *NetPDLParentElement;
struct _nbNetPDLElementFieldBase *NetPDLFieldElement;
struct _nbNetPDLElementBase *NetPDLTempElement;

	// Let's check if the parent element is one among <set> and <choice>
	NetPDLParentElement= NETPDL_GET_ELEMENT(NetPDLElementInfo->Parent);
	switch (NetPDLParentElement->Type)
	{
	case nbNETPDL_IDEL_SET:
		{
			NetPDLFieldElement= ((struct _nbNetPDLElementSet *) NetPDLParentElement)->FieldToDecode;
		};break;
	case nbNETPDL_IDEL_CHOICE:
		{
			NetPDLFieldElement= ((struct _nbNetPDLElementChoice *) NetPDLParentElement)->FieldToDecode;
		};break;
	default:
		{
			// If the parent element is not a <set>/<choice>, let's print an error
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
				"<fieldmatch> element %s with matching expression \n'%s'\n is not enclosed within an allowed parent element.",
				NetPDLElement->Name, NetPDLElement->MatchExprString);

			return nbFAILURE;
		}
	}

	// Let's check correctness for those types that must have subfields as children
	NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLElement->FirstChild);

	switch (NetPDLFieldElement->FieldType)
	{
	case nbNETPDL_ID_CFIELD_TLV:
		{
			while (NetPDLTempElement)
			{
				switch (NetPDLTempElement->Type)
				{
				case nbNETPDL_IDEL_SUBFIELD:
					{
					struct _nbNetPDLElementSubfield *NetPDLSubfieldElement= (struct _nbNetPDLElementSubfield *) NetPDLTempElement;

						switch (NetPDLSubfieldElement->Portion)
						{
						case nbNETPDL_ID_SUBFIELD_PORTION_TLV_TYPE:
						case nbNETPDL_ID_SUBFIELD_PORTION_TLV_LENGTH:
						case nbNETPDL_ID_SUBFIELD_PORTION_TLV_VALUE:
							break;
						default:
							{
								// If the portion has not been found, let's print an error
								errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
									"Complex field or subfield %s declares a subfield '%s' which does not exist.",
									NetPDLElement->Name, NetPDLSubfieldElement->Name);

								return nbFAILURE;
							}
						}
					}break;
				//case nbNETPDL_IDEL_CHOICE:
				//	{
				//	}break;
				default:
					{
						// If the neither subfield nor choice has been found, let's print an error
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
							"Complex field or subfield %s with matching expression \n'%s'\n contains '<%s>' element which is not allowed.",
							NetPDLElement->Name, NetPDLTempElement->ElementName);

						return nbFAILURE;
					}
				}

				NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
			}
		};break;
	case nbNETPDL_ID_CFIELD_HDRLINE:
		{
			while (NetPDLTempElement)
			{
				switch (NetPDLTempElement->Type)
				{
				case nbNETPDL_IDEL_SUBFIELD:
					{
					struct _nbNetPDLElementSubfield *NetPDLSubfieldElement= (struct _nbNetPDLElementSubfield *) NetPDLTempElement;

						switch (NetPDLSubfieldElement->Portion)
						{
						case nbNETPDL_ID_SUBFIELD_PORTION_HDRLINE_HNAME:
						case nbNETPDL_ID_SUBFIELD_PORTION_HDRLINE_HVALUE:
							break;
						default:
							{
								// If the portion has not been found, let's print an error
								errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
									"Complex field or subfield %s with matching expression \n'%s'\n declares a subfield '%s' which does not exist.",
									NetPDLElement->Name, NetPDLSubfieldElement->Name);

								return nbFAILURE;
							}
						}
					}break;
				//case nbNETPDL_IDEL_CHOICE:
				//	{
				//	}break;
				default:
					{
						// If the neither subfield nor choice has been found, let's print an error
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
							"Complex field or subfield %s with matching expression \n'%s'\n contains '<%s>' element which is not allowed.",
							NetPDLElement->Name, NetPDLTempElement->ElementName);

						return nbFAILURE;
					}
				}

				NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
			}
		}; break;
	case nbNETPDL_ID_CFIELD_DYNAMIC:
		{
			while (NetPDLTempElement)
			{
				switch (NetPDLTempElement->Type)
				{
				case nbNETPDL_IDEL_SUBFIELD:
				//case nbNETPDL_IDEL_CHOICE:
					{
					struct _nbNetPDLElementSubfield *NetPDLSubfieldElement= (struct _nbNetPDLElementSubfield *) NetPDLTempElement;
					int PCREStringNumber;

						PCREStringNumber= pcre_get_stringnumber(
								(pcre *) ((struct _nbNetPDLElementCfieldDynamic *) NetPDLFieldElement)->PatternPCRECompiledRegExp, 
								NetPDLSubfieldElement->PortionName);

						if (PCREStringNumber == PCRE_ERROR_NOSUBSTRING)
						{
							// If the portion has not been found, let's print an error
							errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
								"Complex field or subfield %s with matching expression \n'%s'\n declares a subfield '%s' which does not exist.",
								NetPDLElement->Name, NetPDLSubfieldElement->Name);

							return nbFAILURE;
						}
						
					}break;
				default:
					{
						// If the neither subfield nor choice has been found, let's print an error
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
							"Complex field or subfield %s with matching expression \n'%s'\n contains '<%s>' element which is not allowed.",
							NetPDLElement->Name, NetPDLTempElement->ElementName);

						return nbFAILURE;
					}
				}

				NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
			}
		}; break;
	case nbNETPDL_ID_CFIELD_XML:
		{
			while (NetPDLTempElement)
			{
				switch (NetPDLTempElement->Type)
				{
				case nbNETPDL_IDEL_MAP:
					{
					}break;
				default:
					{
						// If the neither subfield nor choice has been found, let's print an error
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
							"Complex field %s contains '<%s>' element which is not allowed.",
							NetPDLElement->Name, NetPDLTempElement->ElementName);

						return nbFAILURE;
					}
				}

				NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
			}
		};break;
	default:
		// Several nbNETPDL_ID_* missing; this 'default' is required only to prevent warnings in the compiler
		break;
	}

	return OrganizeFieldBase((struct _nbNetPDLElementFieldBase *) NetPDLElement, ErrBuf, ErrBufSize);
}

int OrganizeElementSet(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementSet *NetPDLElement= (struct _nbNetPDLElementSet *) NetPDLElementInfo;
struct _nbNetPDLElementBase *NetPDLTempElement;
struct _nbNetPDLElementFieldmatch *LastFieldmatchElement= NULL;
struct _nbNetPDLElementFieldmatch *DefaultItemElement= NULL;

	NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLElement->FirstChild);

	// Let's check children elements for current 'SET' element
	while (NetPDLTempElement)
	{
		switch (NetPDLTempElement->Type)
		{
		case nbNETPDL_IDEL_MISSINGPACKETDATA:
			{
				if (NetPDLElement->FirstValidChildIfMissingPacketData)
				{
					// If the 'MISSING-PACKETDATA' element is defined twice, let's print an error
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
						"<set> element defines more than one <missing-packetdata> element.");

					return nbFAILURE;
				}
			
				NetPDLElement->FirstValidChildIfMissingPacketData= NETPDL_GET_ELEMENT(NetPDLTempElement->FirstChild);
			}; break;
		case nbNETPDL_IDEL_EXITWHEN:
			{
				if (NetPDLElement->ExitWhen)
				{
					// If the <exit-when> element is defined twice, let's print an error
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
						"<set> element defines more than one <exit-when> element.");

					return nbFAILURE;
				}

				NetPDLElement->ExitWhen= (struct _nbNetPDLElementExitWhen *) NetPDLTempElement;
			}; break;
		case nbNETPDL_IDEL_FIELDMATCH:
			{
				// Let's perform 'FIELDMATCH' elements list linking, if it is needed
				if (LastFieldmatchElement)
				{
					LastFieldmatchElement->NextFieldmatch= (struct _nbNetPDLElementFieldmatch *) NetPDLTempElement;
					LastFieldmatchElement= LastFieldmatchElement->NextFieldmatch;
				}

				// Let's update (only for first time) pointer to first 'FIELDMATCH' element of the list
				if (NetPDLElement->FirstMatchElement == NULL)
				{
					NetPDLElement->FirstMatchElement= (struct _nbNetPDLElementFieldmatch *) NetPDLTempElement;
					LastFieldmatchElement= (struct _nbNetPDLElementFieldmatch *) NetPDLTempElement;
				}
			};break;
		case nbNETPDL_IDEL_DEFAULTITEM:
			{
				if (DefaultItemElement != NULL)
				{
					// If the <exit-when> element is defined twice, let's print an error
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
						"<set> element defines more than one <default-item> element.");

					return nbFAILURE;
				}

				DefaultItemElement= (struct _nbNetPDLElementFieldmatch *) NetPDLTempElement;

				// Let's update (only for first time) pointer to first 'FIELDMATCH' element of the list
				if (NetPDLElement->FirstMatchElement == NULL)
					NetPDLElement->FirstMatchElement= (struct _nbNetPDLElementFieldmatch *) NetPDLTempElement;
			};break;
		default:
			{
				// If the current child has not been recognized, let's print an error
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
					"<set> element declares an inner element which is not allowed.");

				return nbFAILURE;

				if (NetPDLElement->FirstMatchElement == NULL)
					NetPDLElement->FirstMatchElement= (struct _nbNetPDLElementFieldmatch *) NetPDLTempElement;
			}
		}

		NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
	}

	// Let's check presence of mandatory 'EXIT-WHEN' element
	if (!NetPDLElement->ExitWhen)
	{
		// If the <exit-when> element is not present, let's print an error
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"<set> element does not define mandatory <exit-when> element.");

		return nbFAILURE;
	}

	// Let's check presence of mandatory 'DEFAULT-ITEM' element
	if (!DefaultItemElement)
	{
		// If the <default-item> element is not present, let's print an error
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"<set> element does not define mandatory <default-item> element.");

		return nbFAILURE;
	}

	// Let's enqueue the 'DEFAULT-ITEM' element to the 'FIELDMATCH' elements list, if it is needed
	if (NetPDLElement->FirstMatchElement != DefaultItemElement)
		LastFieldmatchElement->NextFieldmatch= DefaultItemElement;

	return OrganizeFieldBase((struct _nbNetPDLElementFieldBase *) NetPDLElement->FieldToDecode, ErrBuf, ErrBufSize);
}


int OrganizeElementDefaultItem(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementFieldmatch *NetPDLElement= (struct _nbNetPDLElementFieldmatch *) NetPDLElementInfo;

	return OrganizeFieldBase((struct _nbNetPDLElementFieldBase *) NetPDLElement, ErrBuf, ErrBufSize);
}


int OrganizeElementChoice(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementChoice *NetPDLElement= (struct _nbNetPDLElementChoice *) NetPDLElementInfo;
struct _nbNetPDLElementBase *NetPDLTempElement;
struct _nbNetPDLElementFieldmatch *LastFieldmatchElement= NULL;
struct _nbNetPDLElementFieldmatch *DefaultItemElement= NULL;

	NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLElement->FirstChild);

	// Let's check if remaining children element are allowed for current 'CHOICE' element
	while (NetPDLTempElement)
	{
		switch (NetPDLTempElement->Type)
		{
		case nbNETPDL_IDEL_MISSINGPACKETDATA:
			{
				if (NetPDLElement->FirstValidChildIfMissingPacketData)
				{
					// If the 'MISSING-PACKETDATA' element is defined twice, let's print an error
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
						"<choice> element defines more than one <missing-packetdata> element.");

					return nbFAILURE;
				}

				NetPDLElement->FirstValidChildIfMissingPacketData= NETPDL_GET_ELEMENT(NetPDLTempElement->FirstChild);
			}; break;
		case nbNETPDL_IDEL_FIELDMATCH:
			{
				// Let's perform 'FIELDMATCH' elements list linking, if it is needed
				if (LastFieldmatchElement)
				{
					LastFieldmatchElement->NextFieldmatch= (struct _nbNetPDLElementFieldmatch *) NetPDLTempElement;
					LastFieldmatchElement= LastFieldmatchElement->NextFieldmatch;
				}

				// Let's update (only for first time) pointer to first 'FIELDMATCH' element of the list
				if (NetPDLElement->FirstMatchElement == NULL)
				{
					NetPDLElement->FirstMatchElement= (struct _nbNetPDLElementFieldmatch *) NetPDLTempElement;
					LastFieldmatchElement= (struct _nbNetPDLElementFieldmatch *) NetPDLTempElement;
				}
			};break;
		case nbNETPDL_IDEL_DEFAULTITEM:
			{
				if (DefaultItemElement)
				{
					// If the 'DEFAULT-ITEM' element is defined twice, let's print an error
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
						"<choice> element defines more than one <default-item> element.");

					return nbFAILURE;
				}

				DefaultItemElement= (struct _nbNetPDLElementFieldmatch *) NetPDLTempElement;

				// Let's update (only for first time) pointer to first 'FIELDMATCH' element of the list
				if (NetPDLElement->FirstMatchElement == NULL)
					NetPDLElement->FirstMatchElement= (struct _nbNetPDLElementFieldmatch *) NetPDLTempElement;
			};break;
		default:
			{
				// If the current child has not been recognized, let's print an error
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
					"<choice> element declares an inner element which is not allowed.");

				return nbFAILURE;
			}
		}

		NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
	}

	// Let's enqueue the 'DEFAULT-ITEM' element to the 'FIELDMATCH' elements list, if it is needed
	if (NetPDLElement->FirstMatchElement != DefaultItemElement)
		LastFieldmatchElement->NextFieldmatch= DefaultItemElement;

	return OrganizeFieldBase((struct _nbNetPDLElementFieldBase *) NetPDLElement->FieldToDecode, ErrBuf, ErrBufSize);
}


int OrganizeElementAdt(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementAdt *NetPDLElement= (struct _nbNetPDLElementAdt *) NetPDLElementInfo;
struct _nbNetPDLElementBase *NetPDLTempParentElement= NetPDLElementInfo;

	// Let's get the protocol name (it goes only for local ADTs)
	while ((NetPDLTempParentElement) && (NetPDLTempParentElement->Type != nbNETPDL_IDEL_PROTO))
		NetPDLTempParentElement= NETPDL_GET_ELEMENT(NetPDLTempParentElement->Parent);

	if (NetPDLTempParentElement)
		NetPDLElement->ProtoName= ((struct _nbNetPDLElementProto *) NetPDLTempParentElement)->Name;	

	return OrganizeFieldBase((struct _nbNetPDLElementFieldBase *) NetPDLElement->ADTFieldInfo, ErrBuf, ErrBufSize);
}


int OrganizeElementAdtfield(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementAdtfield *NetPDLElement= (struct _nbNetPDLElementAdtfield *) NetPDLElementInfo;

	return OrganizeFieldBase((struct _nbNetPDLElementFieldBase *) NetPDLElement, ErrBuf, ErrBufSize);
}


int OrganizeElementReplace(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementReplace *NetPDLElement= (struct _nbNetPDLElementReplace *) NetPDLElementInfo;

	// Let's check the 'showtemplate'
	if (NetPDLElement->ShowTemplateName)
	{
		for (unsigned int i= 0; i < NetPDLDatabase->ShowTemplateNItems; i++)
		{
			if (strcmp(NetPDLDatabase->ShowTemplateList[i]->Name, NetPDLElement->ShowTemplateName) == 0)
			{
				NetPDLElement->ShowTemplateInfo= NetPDLDatabase->ShowTemplateList[i];
				break;
			}
		}

		if (NetPDLElement->ShowTemplateInfo == NULL)
		{
			// If the template has not been found, let's print an error
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
				"Rename statement provided for %s points to showtemplate '%s' which does not exist.",
				NetPDLElement->FieldToRename, NetPDLElement->ShowTemplateName);

			return nbFAILURE;
		}
	}

	return nbSUCCESS;
}


int OrganizeElementSection(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementSection *NetPDLElement= (struct _nbNetPDLElementSection *) NetPDLElementInfo;

	if ((NetPDLElement->Type == nbNETPDL_IDEL_SECTION) && (NetPDLElement->SectionName))
	{
		unsigned int SectionNumber;
		for (SectionNumber= 0; SectionNumber < NetPDLDatabase->ShowSumStructureNItems; SectionNumber++)
		{
	 	struct _nbNetPDLElementShowSumStructure *SectionItem;

			SectionItem= NetPDLDatabase->ShowSumStructureList[SectionNumber];

			// Check if the specified section name matches with one declared into the index structure
			if (strcmp(NetPDLElement->SectionName, SectionItem->Name) == 0) 
			{
				// Update the pointer only in case it is allowed (the number of sections is
				// defined at the beginning)
				if (SectionNumber < NetPDLDatabase->ShowSumStructureNItems )
				{
					NetPDLElement->SectionNumber= SectionNumber;
					break;
				}
				else
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
						"Node <section name='%s'> contains an unvalid section name.",
						NetPDLElement->SectionName);

					return nbFAILURE;
				}
			}
		}

		if (SectionNumber == NetPDLDatabase->ShowSumStructureNItems)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
				"Node <section name='%s'> contains an unvalid section name.",
				NetPDLElement->SectionName);

			return nbFAILURE;
		}
	}

	return nbSUCCESS;
}


const char *GetProtoName(struct _nbNetPDLElementBase *NetPDLElement)
{
	while ((NetPDLElement) && (NetPDLElement->Type != nbNETPDL_IDEL_PROTO))
		NetPDLElement= NETPDL_GET_ELEMENT(NetPDLElement->Parent);

	if (NetPDLElement)
	{
	struct _nbNetPDLElementProto *NetPDLElementProto;

		NetPDLElementProto= (struct _nbNetPDLElementProto *) NetPDLElement;

		return (NetPDLElementProto->Name);
	}
	else
		return "Protocol name not found";
}
