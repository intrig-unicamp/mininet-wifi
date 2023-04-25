/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <pcre.h>
#include <nbprotodb_defs.h>
#include <iostream>
#include "protodb_globals.h"
#include "expressions.h"
#include "../nbee/utils/netpdlutils.h"
#include "../nbee/globals/globals.h"
#include "../nbee/globals/debug.h"
#include "../nbee/globals/utils.h"


extern int CurrentElementType;

// Pointer to the last lookup table encountered in the parsing
struct _nbNetPDLElementLookupTable* LastLookupTable;

struct _nbNetPDLElementBase *CreateElementGeneric(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementBase *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLElementBase *) malloc(sizeof(struct _nbNetPDLElementBase));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementBase));

	return NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementNetPDL(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
char* Attribute;
int VersionMajor, VersionMinor;

	Attribute= GetXMLAttribute(Attributes, NETPDL_DATE_ATTR);

	NetPDLDatabase->CreationDate= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
	if (NetPDLDatabase->CreationDate == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	strcpy(NetPDLDatabase->CreationDate, Attribute);


	Attribute= GetXMLAttribute(Attributes, NETPDL_CREATOR_ATTR);

	NetPDLDatabase->Creator= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
	if (NetPDLDatabase->Creator == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	strcpy(NetPDLDatabase->Creator, Attribute);


	Attribute= GetXMLAttribute(Attributes, NETPDL_VERSION_ATTR);

	sscanf(Attribute, "%d.%d", &VersionMajor, &VersionMinor);
	NetPDLDatabase->VersionMajor= VersionMajor;
	NetPDLDatabase->VersionMinor= VersionMinor;

	return (CreateElementGeneric(Attributes, ErrBuf, ErrBufSize));
}


struct _nbNetPDLElementBase *CreateElementProto(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementProto *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementProto *) malloc(sizeof(struct _nbNetPDLElementProto));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementProto));

	// Save the protocol name
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_NAME);

	NetPDLElement->Name= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
	if (NetPDLElement->Name == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	strcpy(NetPDLElement->Name, Attribute);

	// Save the protocol longname
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_LONGNAME);
	if (Attribute)
	{
		NetPDLElement->LongName= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
		if (NetPDLElement->LongName == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
		strcpy(NetPDLElement->LongName, Attribute);
	}

	// Save the template name
	Attribute= GetXMLAttribute(Attributes, NETPDL_PROTO_ATTR_SHOWSUMTEMPLATE);

	if (Attribute)
	{
		NetPDLElement->ShowSumTemplateName= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
		if (NetPDLElement->ShowSumTemplateName == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
		strcpy(NetPDLElement->ShowSumTemplateName, Attribute);
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementExecuteX(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementExecuteX *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementExecuteX *) malloc(sizeof(struct _nbNetPDLElementExecuteX));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementExecuteX));

	// Let's retrieve the 'when' expression (which may be missing)
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_WHEN);
	if (Attribute)
	{
		if (CreateExpression(Attribute, CREATE_EXPR_ALLOW_NUMBER_ONLY, &(NetPDLElement->WhenExprString), &(NetPDLElement->WhenExprTree), ErrBuf, ErrBufSize) == nbFAILURE)
			return NULL;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementVariable(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementVariable* NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementVariable *) malloc(sizeof(struct _nbNetPDLElementVariable));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementVariable));


	// Let's retrieve the variable name
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_NAME);

	NetPDLElement->Name= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
	if (NetPDLElement->Name == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	strcpy(NetPDLElement->Name, Attribute);


	// Checking the variable type
	Attribute= GetXMLAttribute(Attributes, NETPDL_VARIABLE_ATTR_VARTYPE);

	if (strcmp(Attribute, NETPDL_VARIABLE_ATTR_VARTYPE_NUMBER) == 0)
		NetPDLElement->VariableDataType= nbNETPDL_VARIABLE_TYPE_NUMBER;
	if (strcmp(Attribute, NETPDL_VARIABLE_ATTR_VARTYPE_BUFFER) == 0)
		NetPDLElement->VariableDataType= nbNETPDL_VARIABLE_TYPE_BUFFER;
	if (strcmp(Attribute, NETPDL_VARIABLE_ATTR_VARTYPE_REFBUFFER) == 0)
		NetPDLElement->VariableDataType= nbNETPDL_VARIABLE_TYPE_REFBUFFER;
	if (strcmp(Attribute, NETPDL_VARIABLE_ATTR_VARTYPE_PROTOCOL) == 0)
		NetPDLElement->VariableDataType= nbNETPDL_VARIABLE_TYPE_PROTOCOL;


	// Checking the validity of this variable
	Attribute= GetXMLAttribute(Attributes, NETPDL_VARIABLE_ATTR_VALIDITY);

	if (strcmp(Attribute, NETPDL_VARIABLE_ATTR_VALIDITY_STATIC) == 0)
		NetPDLElement->Validity= nbNETPDL_VARIABLE_VALIDITY_STATIC;
	if (strcmp(Attribute, NETPDL_VARIABLE_ATTR_VALIDITY_THISPKT) == 0)
		NetPDLElement->Validity= nbNETPDL_VARIABLE_VALIDITY_THISPACKET;

	// Checking the size of this variable
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_SIZE);

	if (Attribute)
		NetPDLElement->Size= atoi(Attribute);

	if ((NetPDLElement->VariableDataType == nbNETPDL_VARIABLE_TYPE_NUMBER) 
		|| (NetPDLElement->VariableDataType == nbNETPDL_VARIABLE_TYPE_BUFFER))
	{
		// Get the initial value (if present)
		Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_VALUE);

		if (Attribute)
		{
			if (NetPDLElement->VariableDataType == nbNETPDL_VARIABLE_TYPE_NUMBER)
			{
				NetPDLElement->InitValueNumber= GetNumber(Attribute, (int) strlen(Attribute));
			}
			else
			{
				NetPDLElement->InitValueString= (unsigned char *) AllocateAsciiString(Attribute, &(NetPDLElement->InitValueStringSize), 0, 0);

				if (NetPDLElement->InitValueString == NULL)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
					return NULL;
				}
			}
		}
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementLookupTable(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
	struct _nbNetPDLElementLookupTable* NetPDLElement;
	char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementLookupTable *) malloc(sizeof(struct _nbNetPDLElementLookupTable));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementLookupTable));

	// Let's retrieve the variable name
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_NAME);

	NetPDLElement->Name= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
	if (NetPDLElement->Name == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	strcpy(NetPDLElement->Name, Attribute);


	// Checking the size of this variable
	Attribute= GetXMLAttribute(Attributes, NETPDL_LOOKUPTABLE_ATTR_EXACTENTRIES);
	NetPDLElement->NumExactEntries= atoi(Attribute);

	Attribute= GetXMLAttribute(Attributes, NETPDL_LOOKUPTABLE_ATTR_MASKENTRIES);
	if (Attribute)
		NetPDLElement->NumMaskEntries= atoi(Attribute);

	// Let's check if this table supports dynamic entries
	Attribute= GetXMLAttribute(Attributes, NETPDL_LOOKUPTABLE_ATTR_VALIDITY);

	if (strcmp(Attribute, NETPDL_LOOKUPTABLE_ATTR_VALIDITY_STATIC) == 0)
		NetPDLElement->AllowDynamicEntries= 0;
	if (strcmp(Attribute, NETPDL_LOOKUPTABLE_ATTR_VALIDITY_DYNAMIC) == 0)
		NetPDLElement->AllowDynamicEntries= 1;

	// Save a pointer to this element; it is used to organize keys and data
	LastLookupTable= NetPDLElement;

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementKeyData(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementKeyData *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementKeyData *) malloc(sizeof(struct _nbNetPDLElementKeyData));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementKeyData));


	// Let's retrieve the variable name
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_NAME);

	// Let's check: some names are reserved
	if ((strcmp(Attribute, NETPDL_LOOKUPTABLE_FIELDNAME_TIMESTAMP) == 0) ||
		(strcmp(Attribute, NETPDL_LOOKUPTABLE_FIELDNAME_LIFETIME) == 0))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"A lookuptable has a field whose name ('%s' or '%s') is reserved.",
			NETPDL_LOOKUPTABLE_FIELDNAME_TIMESTAMP, NETPDL_LOOKUPTABLE_FIELDNAME_LIFETIME);
		return NULL;
	}

	// Let's store the name
	NetPDLElement->Name= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
	if (NetPDLElement->Name == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	strcpy(NetPDLElement->Name, Attribute);


	// Let's retrieve the data type
	Attribute= GetXMLAttribute(Attributes, NETPDL_LOOKUPTABLE_KEYANDDATA_ATTR_TYPE);
	
	if (strcmp(Attribute, NETPDL_LOOKUPTABLE_KEYANDDATA_ATTR_TYPE_NUMBER) == 0)
		NetPDLElement->KeyDataType= nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER;
	if (strcmp(Attribute, NETPDL_LOOKUPTABLE_KEYANDDATA_ATTR_TYPE_BUFFER) == 0)
		NetPDLElement->KeyDataType= nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER;
	if (strcmp(Attribute, NETPDL_LOOKUPTABLE_KEYANDDATA_ATTR_TYPE_PROTOCOL) == 0)
		NetPDLElement->KeyDataType= nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_PROTOCOL;

	if (NetPDLElement->KeyDataType == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Wrong code in the data type: %s.", Attribute);
		return NULL;
	}

	// Checking the size of this variable; this is present only for 'buffer' types
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_SIZE);
	if (Attribute)
		NetPDLElement->Size= atoi(Attribute);

	if (CurrentElementType == nbNETPDL_IDEL_KEY)
	{
		// Add this element to the key/data list of the corresponding variable
		if (LastLookupTable->FirstKey == NULL)
			LastLookupTable->FirstKey= NetPDLElement;
		else
		{
		struct _nbNetPDLElementKeyData* TempKeyData;

			TempKeyData= LastLookupTable->FirstKey;

			while (TempKeyData)
			{
				if (TempKeyData->NextKeyData == NULL)
				{
					TempKeyData->NextKeyData= NetPDLElement;
					break;
				}
				TempKeyData= TempKeyData->NextKeyData;
			}
		}
	}
	else
	{
		// Add this element to the key/data list of the corresponding variable
		if (LastLookupTable->FirstData == NULL)
			LastLookupTable->FirstData= NetPDLElement;
		else
		{
		struct _nbNetPDLElementKeyData* TempKeyData;

			TempKeyData= LastLookupTable->FirstData;

			while (TempKeyData)
			{
				if (TempKeyData->NextKeyData == NULL)
				{
					TempKeyData->NextKeyData= NetPDLElement;
					break;
				}
				TempKeyData= TempKeyData->NextKeyData;
			}
		}
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementAssignVariable(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementAssignVariable *NetPDLElement;
char* Attribute;
int RetVal;
char *VariableName, *OffsetStartAtPtr, *OffsetSizePtr, *TmpPtr;
struct _nbNetPDLElementVariable *NetPDLVariableDef;
unsigned int i;

	NetPDLElement= (struct _nbNetPDLElementAssignVariable *) malloc(sizeof(struct _nbNetPDLElementAssignVariable));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementAssignVariable));


	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_NAME);

	// Let's split the variable in its components
	VariableName= Attribute;

	// Let's get if the variable has some offsets
	TmpPtr= strchr(VariableName, '[');
	if (TmpPtr)
	{
		*TmpPtr= 0;
		OffsetStartAtPtr= TmpPtr + 1;

		TmpPtr= strchr(OffsetStartAtPtr, ':');
		*TmpPtr= 0;
		OffsetSizePtr= TmpPtr + 1;

		TmpPtr= strchr(OffsetSizePtr, ']');
		*TmpPtr= 0;

		NetPDLElement->OffsetStartAt= atoi(OffsetStartAtPtr);
		NetPDLElement->OffsetSize= atoi(OffsetSizePtr);
	}

	NetPDLElement->VariableName= (char *) malloc(strlen(VariableName) + 1);
	if (NetPDLElement->VariableName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
		return NULL;
	}

	memcpy(NetPDLElement->VariableName, VariableName, strlen(VariableName) + 1);

	// Detecting if the variable is a number or a buffer
	for (i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
	struct _nbNetPDLElementBase *NetPDLTempElement;

		NetPDLTempElement= NetPDLDatabase->GlobalElementsList[i];

		if (NetPDLTempElement->Type == nbNETPDL_IDEL_VARIABLE)
		{
			NetPDLVariableDef= (struct _nbNetPDLElementVariable *) NetPDLTempElement;

			if (strcmp(NetPDLVariableDef->Name, NetPDLElement->VariableName) == 0)
			{
				NetPDLElement->VariableDataType= NetPDLVariableDef->VariableDataType;
				break;
			}
		}
	}

	// Detecting if the variable is a number or a buffer
	if (NetPDLElement->VariableDataType == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Found variable '%s' in expression which has not been defined in the NetPDL variable list.", NetPDLElement->VariableName);
		return NULL;
	}


	// Let's retrieve the expression
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_VALUE);

	if ((NetPDLElement->VariableDataType == nbNETPDL_VARIABLE_TYPE_NUMBER)
				|| (NetPDLElement->VariableDataType == nbNETPDL_VARIABLE_TYPE_PROTOCOL))
	{
		RetVal= CreateExpression(Attribute, CREATE_EXPR_ALLOW_NUMBER_ONLY, &(NetPDLElement->ExprString), &(NetPDLElement->ExprTree), ErrBuf, ErrBufSize);
	}
	else
	{
		RetVal= CreateExpression(Attribute, CREATE_EXPR_ALLOW_BUFFER_ONLY, &(NetPDLElement->ExprString), &(NetPDLElement->ExprTree), ErrBuf, ErrBufSize);
	}

	if (RetVal == nbFAILURE)
		return NULL;

	if ( ((NetPDLElement->VariableDataType == nbNETPDL_VARIABLE_TYPE_NUMBER) ||
		  (NetPDLElement->VariableDataType == nbNETPDL_VARIABLE_TYPE_PROTOCOL))
				&& (NetPDLElement->OffsetSize != 0))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
						"Only buffer/refbuffer variables can have offsets ('%s').", NetPDLElement->VariableName);
		return NULL;
	}

#ifdef _DEBUG
	// Initializing to a fake number, just for debug.
	NetPDLElement->CustomData = (void*) 99999999;
#endif

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementAlias(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementAlias *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementAlias *) malloc(sizeof(struct _nbNetPDLElementAlias));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementAlias));

	// Save the alias name
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_NAME);

	NetPDLElement->Name= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
	if (NetPDLElement->Name == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	strcpy(NetPDLElement->Name, Attribute);

	// Save the alias value
	Attribute= GetXMLAttribute(Attributes, NETPDL_ALIAS_ATTR_REPLACEWITH);

	NetPDLElement->ReplaceWith= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
	if (NetPDLElement->ReplaceWith == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	strcpy(NetPDLElement->ReplaceWith, Attribute);

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementAssignLookupTable(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementAssignLookupTable *NetPDLElement;
char* Attribute;
int RetVal;
char *TableName, *FieldName, *OffsetStartAtPtr, *OffsetSizePtr, *TmpPtr;

	NetPDLElement= (struct _nbNetPDLElementAssignLookupTable *) malloc(sizeof(struct _nbNetPDLElementAssignLookupTable));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementAssignLookupTable));


	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_NAME);


	// Let's split the variable in its components
	TableName= Attribute;

	TmpPtr= strchr(TableName, '.');

	// Split between table name and field name
	*TmpPtr= 0;
	FieldName= TmpPtr + 1;

	// Let's get if the variable has some offsets
	TmpPtr= strchr(FieldName, '[');
	if (TmpPtr)
	{
		*TmpPtr= 0;
		OffsetStartAtPtr= TmpPtr + 1;

		TmpPtr= strchr(OffsetStartAtPtr, ':');
		*TmpPtr= 0;
		OffsetSizePtr= TmpPtr + 1;

		TmpPtr= strchr(OffsetSizePtr, ']');
		*TmpPtr= 0;

		NetPDLElement->OffsetStartAt= atoi(OffsetStartAtPtr);
		NetPDLElement->OffsetSize= atoi(OffsetSizePtr);
	}


	NetPDLElement->TableName= (char *) malloc(strlen(TableName) + 1);
	if (NetPDLElement->TableName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
		return NULL;
	}

	memcpy(NetPDLElement->TableName, TableName, strlen(TableName) + 1);

	NetPDLElement->FieldName= (char *) malloc(strlen(FieldName) + 1);
	if (NetPDLElement->FieldName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
		return NULL;
	}

	memcpy(NetPDLElement->FieldName, FieldName, strlen(FieldName) + 1);


	// Detecting if the variable is a number or a buffer
	for (unsigned int i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
	struct _nbNetPDLElementBase* NetPDLTempElement;
	struct _nbNetPDLElementLookupTable* NetPDLLookupTableDef;

		NetPDLTempElement= NetPDLDatabase->GlobalElementsList[i];

		if (NetPDLTempElement->Type == nbNETPDL_IDEL_LOOKUPTABLE)
		{
			NetPDLLookupTableDef= (struct _nbNetPDLElementLookupTable *) NetPDLTempElement;

			if (strcmp(NetPDLLookupTableDef->Name, NetPDLElement->TableName) == 0)
			{
			struct _nbNetPDLElementKeyData* TempKeyData;

				TempKeyData= NetPDLLookupTableDef->FirstKey;

				while (TempKeyData)
				{
					if (strcmp(TempKeyData->Name, NetPDLElement->FieldName) == 0)
					{
						NetPDLElement->FieldDataType= TempKeyData->KeyDataType;
						break;
					}

					TempKeyData= TempKeyData->NextKeyData;
				}


				TempKeyData= NetPDLLookupTableDef->FirstData;

				while (TempKeyData)
				{
					if (strcmp(TempKeyData->Name, NetPDLElement->FieldName) == 0)
					{
						NetPDLElement->FieldDataType= TempKeyData->KeyDataType;
						break;
					}

					TempKeyData= TempKeyData->NextKeyData;
				}

				break;
			}
		}
	}

	if (NetPDLElement->FieldDataType == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Either the lookup table ('%s') or the field member ('%s') cannot be found in the lookup table definition.",
			NetPDLElement->TableName, NetPDLElement->FieldName);
		return NULL;
	}

	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_VALUE);

	if ((NetPDLElement->FieldDataType == nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER)
				|| (NetPDLElement->FieldDataType == nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_PROTOCOL))
	{
		RetVal= CreateExpression(Attribute, CREATE_EXPR_ALLOW_NUMBER_ONLY, &(NetPDLElement->ExprString), &(NetPDLElement->ExprTree), ErrBuf, ErrBufSize);
	}
	else
	{
		RetVal= CreateExpression(Attribute, CREATE_EXPR_ALLOW_BUFFER_ONLY, &(NetPDLElement->ExprString), &(NetPDLElement->ExprTree), ErrBuf, ErrBufSize);
	}

	if (RetVal == nbFAILURE)
		return NULL;

	if ( (NetPDLElement->FieldDataType != nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER)
				&& (NetPDLElement->OffsetSize != 0))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
						"Only buffer variables can have offsets ('%s.%s').", NetPDLElement->TableName, NetPDLElement->FieldName);
		return NULL;
	}

#ifdef _DEBUG
	// Initializing to a fake number, just for debug.
	NetPDLElement->TableCustomData= (void*) 99999999;
	NetPDLElement->FieldCustomData= (void*) 99999999;
#endif

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}




struct _nbNetPDLElementBase *CreateElementUpdateLookupTable(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementUpdateLookupTable *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementUpdateLookupTable *) malloc(sizeof(struct _nbNetPDLElementUpdateLookupTable));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementUpdateLookupTable));


	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_NAME);

	NetPDLElement->TableName= (char *) malloc(strlen(Attribute) + 1);
	if (NetPDLElement->TableName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
		return NULL;
	}
	strcpy(NetPDLElement->TableName, Attribute);


	Attribute= GetXMLAttribute(Attributes, NETPDL_UPDATELOOKUPTABLE_ATTR_ACTION);

	if (strcmp(Attribute, NETPDL_UPDATELOOKUPTABLE_ATTR_ACTION_ADD) == 0)
		NetPDLElement->Action= nbNETPDL_UPDATELOOKUPTABLE_ACTION_ADD;
	if (strcmp(Attribute, NETPDL_UPDATELOOKUPTABLE_ATTR_ACTION_PURGE) == 0)
		NetPDLElement->Action= nbNETPDL_UPDATELOOKUPTABLE_ACTION_PURGE;
	if (strcmp(Attribute, NETPDL_UPDATELOOKUPTABLE_ATTR_ACTION_OBSOLETE) == 0)
		NetPDLElement->Action= nbNETPDL_UPDATELOOKUPTABLE_ACTION_OBSOLETE;

	// Let's check if we're an ADD: in this case, we have to parse something more
	if (NetPDLElement->Action == nbNETPDL_UPDATELOOKUPTABLE_ACTION_ADD)
	{
		Attribute= GetXMLAttribute(Attributes, NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_VALIDITY);

		if (Attribute == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
				"An update-lookuptable element does not specify the 'validity' attribute.");
			return NULL;
		}

		if (strcmp(Attribute, NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_VALIDITY_KEEPFOREVER) == 0)
			NetPDLElement->Validity= nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPFOREVER;
		if (strcmp(Attribute, NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_VALIDITY_KEEPMAXTIME) == 0)
			NetPDLElement->Validity= nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPMAXTIME;
		if (strcmp(Attribute, NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_VALIDITY_UPDATEONHIT) == 0)
			NetPDLElement->Validity= nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_UPDATEONHIT;
		if (strcmp(Attribute, NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_VALIDITY_REPLACEONHIT) == 0)
			NetPDLElement->Validity= nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_REPLACEONHIT;
		if (strcmp(Attribute, NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_VALIDITY_ADDONHIT) == 0)
			NetPDLElement->Validity= nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_ADDONHIT;

		if (NetPDLElement->Validity != nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPFOREVER)
		{
			Attribute= GetXMLAttribute(Attributes, NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_KEEPTIME);
			if (Attribute)
			{
				NetPDLElement->KeepTime= atoi(Attribute);
			}
			else
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "An update-lookuptable element does not specify the 'keeptime' attribute.");
				return NULL;
			}
		}

		if ((NetPDLElement->Validity == nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_UPDATEONHIT) ||
			(NetPDLElement->Validity == nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_ADDONHIT))
		{
			Attribute= GetXMLAttribute(Attributes, NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_HITTIME);
			if (Attribute)
			{
				NetPDLElement->HitTime= atoi(Attribute);
			}
			else
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "An update-lookuptable element does not specify the 'hittime' attribute.");
				return NULL;
			}
		}

		if ((NetPDLElement->Validity == nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_REPLACEONHIT) ||
			(NetPDLElement->Validity == nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_ADDONHIT))
		{
			Attribute= GetXMLAttribute(Attributes, NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_NEWHITTIME);
			if (Attribute)
			{
				NetPDLElement->NewHitTime= atoi(Attribute);
			}
			else
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "An update-lookuptable element does not specify the 'newhittime' attribute.");
				return NULL;
			}
		}
	}

#ifdef _DEBUG
	// Initializing to a fake number, just for debug.
	NetPDLElement->TableCustomData= (void*) 99999999;
#endif

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementLookupKeyData(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementLookupKeyData *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementLookupKeyData *) malloc(sizeof(struct _nbNetPDLElementLookupKeyData));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementLookupKeyData));

	// Let's check if we have some masks
	if (CurrentElementType == nbNETPDL_IDEL_LOOKUPKEY)
	{
		Attribute= GetXMLAttribute(Attributes, NETPDL_LOOKUPTABLE_KEYFIELD_ATTR_MASK);

		if (Attribute)
		{
			// Check if the format is correct.
			if ((Attribute[0] == '0') && (Attribute[1] == 'x'))
			{
			int MaskLen;
			int RetVal;

				MaskLen= (int) strlen(&Attribute[2]) / 2;

				NetPDLElement->Mask= (unsigned char *) malloc(MaskLen);

				if (NetPDLElement->Mask == NULL)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
					return NULL;
				}

				RetVal= ConvertHexDumpAsciiToHexDumpBin(&Attribute[2], NetPDLElement->Mask, MaskLen);

				if (RetVal == nbFAILURE)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
									"Mask '%s' may contain some error.", Attribute);
					return NULL;
				}

			}
			else 
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Lookuptable mask '%s' has an incorrect format.", Attribute);
				return NULL;
			}
		}
	}

	// Let's retrieve the expression
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_VALUE);

	if (CreateExpression(Attribute, CREATE_EXPR_ALLOW_ALL, &(NetPDLElement->ExprString), &(NetPDLElement->ExprTree), ErrBuf, ErrBufSize) == nbSUCCESS)
		return (struct _nbNetPDLElementBase *) NetPDLElement;
	else
		return NULL;
}


struct _nbNetPDLElementBase *CreateElementShowTemplate(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementShowTemplate *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementShowTemplate *) malloc(sizeof(struct _nbNetPDLElementShowTemplate));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementShowTemplate));

	// Let's check the template name
	Attribute= GetXMLAttribute(Attributes, NETPDL_SHOWTEMPLATE_ATTR_NAME);

	NetPDLElement->Name= (char *) malloc(sizeof(char) * (strlen(Attribute) + 1));
	if (NetPDLElement->Name == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	strcpy(NetPDLElement->Name, Attribute);

	// In case we have the custom plugin, we are not forced to define the other attributes
	Attribute= GetXMLAttribute(Attributes, NETPDL_DISPLAY_ATTR_CUSTOMPLUGIN);
	if (Attribute)
	{
		NetPDLElement->PluginName= (char *) malloc(sizeof(char) * (strlen(Attribute) + 1));
		if (NetPDLElement->PluginName == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
		strcpy(NetPDLElement->PluginName, Attribute);
	}

	// Let's check for the native visualization function
	Attribute= GetXMLAttribute(Attributes, NETPDL_DISPLAY_ATTR_NATIVEFUNCTION);
	if (Attribute)
	{
		if (strcmp(Attribute, NETPDL_DISPLAY_ATTR_NATIVEFUNCTION_IPV4) == 0)
			NetPDLElement->ShowNativeFunction= nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_IPV4;

		if (strcmp(Attribute, NETPDL_DISPLAY_ATTR_NATIVEFUNCTION_ASCII) == 0)
			NetPDLElement->ShowNativeFunction= nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_ASCII;

		if (strcmp(Attribute, NETPDL_DISPLAY_ATTR_NATIVEFUNCTION_ASCIILINE) == 0)
			NetPDLElement->ShowNativeFunction= nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_ASCIILINE;

		if (strcmp(Attribute, NETPDL_DISPLAY_ATTR_NATIVEFUNCTION_HTTPCONTENT) == 0)
			NetPDLElement->ShowNativeFunction= nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_HTTPCONTENT;
	}

	// Let's check for the visualization type
	Attribute= GetXMLAttribute(Attributes, NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT);
	if (Attribute == NULL)
	{
		if ((NetPDLElement->PluginName == NULL) && (NetPDLElement->ShowNativeFunction == 0))
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Missing the showtype attribute.");
			return NULL;
		}
	}
	else
	{
		if (strcmp(Attribute, NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT_HEX) == 0)
			NetPDLElement->ShowMode= nbNETPDL_ID_TEMPLATE_DIGIT_HEX;

		if (strcmp(Attribute, NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT_HEXNOX) == 0)
			NetPDLElement->ShowMode= nbNETPDL_ID_TEMPLATE_DIGIT_HEXNOX;

		if (strcmp(Attribute, NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT_DEC) == 0)
			NetPDLElement->ShowMode= nbNETPDL_ID_TEMPLATE_DIGIT_DEC;

		if (strcmp(Attribute, NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT_BIN) == 0)
			NetPDLElement->ShowMode= nbNETPDL_ID_TEMPLATE_DIGIT_BIN;

		if (strcmp(Attribute, NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT_FLOAT) == 0)
			NetPDLElement->ShowMode= nbNETPDL_ID_TEMPLATE_DIGIT_FLOAT;

		if (strcmp(Attribute, NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT_DOUBLE) == 0)
			NetPDLElement->ShowMode= nbNETPDL_ID_TEMPLATE_DIGIT_DOUBLE;

		if (strcmp(Attribute, NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT_ASC) == 0)
			NetPDLElement->ShowMode= nbNETPDL_ID_TEMPLATE_DIGIT_ASCII;
	}

	// Let's check for the digit size
	Attribute= GetXMLAttribute(Attributes, NETPDL_DISPLAY_ATTR_DISPLAYGROUP);
	if (Attribute)
		NetPDLElement->DigitSize= atoi(Attribute);

	// Let's check for the separator (if it exists)
	Attribute= GetXMLAttribute(Attributes, NETPDL_DISPLAY_ATTR_DISPLAYSEPARATOR);
	if (Attribute)
	{
		NetPDLElement->Separator= (char *) malloc(sizeof(char) * (strlen(Attribute) + 1));
		if (NetPDLElement->Separator == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
		strcpy(NetPDLElement->Separator, Attribute);
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}




struct _nbNetPDLElementBase *CreateElementShowSumTemplate(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementShowSumTemplate *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementShowSumTemplate *) malloc(sizeof(struct _nbNetPDLElementShowSumTemplate));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementShowSumTemplate));

	// Let's check the template name
	Attribute= GetXMLAttribute(Attributes, NETPDL_SHOWSUMTEMPLATE_ATTR_NAME);

	NetPDLElement->Name= (char *) malloc(sizeof(char) * (strlen(Attribute) + 1));
	if (NetPDLElement->Name == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	strcpy(NetPDLElement->Name, Attribute);

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementSumSection(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementShowSumStructure *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementShowSumStructure *) malloc(sizeof(struct _nbNetPDLElementShowSumStructure));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementShowSumStructure));

	// Save the section name
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_NAME);

	NetPDLElement->Name= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
	if (NetPDLElement->Name == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	strcpy(NetPDLElement->Name, Attribute);

	// Save the section longname
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_LONGNAME);
	if (Attribute)
	{
		NetPDLElement->LongName= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
		if (NetPDLElement->LongName == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
		strcpy(NetPDLElement->LongName, Attribute);
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementIf(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementIf *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementIf *) malloc(sizeof(struct _nbNetPDLElementIf));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementIf));

	// Let's retrieve the expression
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_EXPR);

	if (CreateExpression(Attribute, CREATE_EXPR_ALLOW_NUMBER_ONLY, &(NetPDLElement->ExprString), &(NetPDLElement->ExprTree), ErrBuf, ErrBufSize) == nbSUCCESS)
		return (struct _nbNetPDLElementBase *) NetPDLElement;
	else
		return NULL;
}


struct _nbNetPDLElementBase *CreateElementCase(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementCase *NetPDLElement;
	char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementCase *) malloc(sizeof(struct _nbNetPDLElementCase));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementCase));

	Attribute= GetXMLAttribute(Attributes, NETPDL_CASE_ATTR_VALUE);

	// Let's check if we have to save it as a number or as a string
	if (Attribute[0] == '\'')
	{
		// It's a string; the AllocateAsciiString() already terminates the string
		NetPDLElement->ValueString= (unsigned char *) AllocateAsciiString(Attribute, &(NetPDLElement->ValueStringSize), 1, 0);
		if (NetPDLElement->ValueString == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
	}
	else
	{
		// It's a number
		NetPDLElement->ValueNumber= GetNumber(Attribute, (int) strlen(Attribute));

		Attribute= GetXMLAttribute(Attributes, NETPDL_CASE_ATTR_MAXVALUE);

		if (Attribute)
			NetPDLElement->ValueMaxNumber= GetNumber(Attribute, (int) strlen(Attribute));
	}


	// Let's check the 'show' attribute (if it exists)
	Attribute= GetXMLAttribute(Attributes, NETPDL_CASE_ATTR_SHOW);
	if (Attribute)
	{
		NetPDLElement->ShowString= (char *) malloc(sizeof(char) * (strlen(Attribute) + 1));

		if (NetPDLElement->ShowString == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
		strcpy(NetPDLElement->ShowString, Attribute);
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementDefault(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementCase *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementCase *) malloc(sizeof(struct _nbNetPDLElementCase));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementCase));

	// Let's check the 'show' attribute (if it exists)
	Attribute= GetXMLAttribute(Attributes, NETPDL_CASE_ATTR_SHOW);
	if (Attribute)
	{
		NetPDLElement->ShowString= (char *) malloc(sizeof(char) * (strlen(Attribute) + 1));

		if (NetPDLElement->ShowString == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
		strcpy(NetPDLElement->ShowString, Attribute);
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}



struct _nbNetPDLElementBase *CreateElementSwitch(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
	struct _nbNetPDLElementSwitch *NetPDLElement;
	char* Attribute;
	int RetVal;

	NetPDLElement= (struct _nbNetPDLElementSwitch *) malloc(sizeof(struct _nbNetPDLElementSwitch));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementSwitch));

	// Let's check if it is case sensitive
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_SWITCH_ATTR_CASESENSITIVE);

	if ((Attribute) && (strcmp(Attribute, NETPDL_FIELD_SWITCH_ATTR_CASESENSITIVE_NO) == 0))
		NetPDLElement->CaseSensitive= 0;
	else
		NetPDLElement->CaseSensitive= 1;

	// Let's retrieve the expression
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_EXPR);
	
	if(Attribute != NULL)
	{
		RetVal= CreateExpression(Attribute, CREATE_EXPR_ALLOW_ALL, &(NetPDLElement->ExprString), &(NetPDLElement->ExprTree), ErrBuf, ErrBufSize);
		if (RetVal == nbFAILURE)
			return NULL;

		if ((NetPDLElement->CaseSensitive == 0) && (NetPDLElement->ExprTree->ReturnType != nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER))
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Error: a case-sensitive switch can operate only with string expressions.");
			return NULL;
		}		
		return (struct _nbNetPDLElementBase *) NetPDLElement;
	}
	
	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
	"Found a switch without any expression to evaluate.");

	return NULL;
}


struct _nbNetPDLElementBase *CreateElementLoop(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementLoop *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementLoop *) malloc(sizeof(struct _nbNetPDLElementLoop));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementLoop));

	// Let's check the loop mode
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_LOOP_ATTR_LOOPTYPE);

	if (strcmp(Attribute, NETPDL_FIELD_LOOP_LPT_SIZE) == 0)
		NetPDLElement->LoopType= nbNETPDL_ID_LOOP_SIZE;
	if (strcmp(Attribute, NETPDL_FIELD_LOOP_LTP_T2R) == 0)
		NetPDLElement->LoopType= nbNETPDL_ID_LOOP_T2R;
	if (strcmp(Attribute, NETPDL_FIELD_LOOP_LTP_WHILE) == 0)
		NetPDLElement->LoopType= nbNETPDL_ID_LOOP_WHILE;
	if (strcmp(Attribute, NETPDL_FIELD_LOOP_LTP_DOWHILE) == 0)
		NetPDLElement->LoopType= nbNETPDL_ID_LOOP_DOWHILE;


	// Let's retrieve the expression
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_EXPR);

	if (CreateExpression(Attribute, CREATE_EXPR_ALLOW_NUMBER_ONLY, &(NetPDLElement->ExprString), &(NetPDLElement->ExprTree), ErrBuf, ErrBufSize) == nbSUCCESS)
		return (struct _nbNetPDLElementBase *) NetPDLElement;
	else
		return NULL;
}


struct _nbNetPDLElementBase *CreateElementLoopCtrl(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementLoopCtrl *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementLoopCtrl *) malloc(sizeof(struct _nbNetPDLElementLoopCtrl));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementLoopCtrl));

	// Let's check the loopctrl type
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_LOOPCTRL_ATTR_TYPE);

	if (strcmp(Attribute, NETPDL_FIELD_LOOPCTRL_ATTR_TYPE_BREAK) == 0 )
		NetPDLElement->LoopCtrlType= nbNETPDL_ID_LOOPCTRL_BREAK;
	else
		NetPDLElement->LoopCtrlType= nbNETPDL_ID_LOOPCTRL_CONTINUE;

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementIncludeBlk(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementIncludeBlk *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementIncludeBlk *) malloc(sizeof(struct _nbNetPDLElementIncludeBlk));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementIncludeBlk));

	// Let's check the name of the block that has to be included
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_INCLUDEBLK_NAME);

	NetPDLElement->IncludedBlockName= (char *) malloc(strlen(Attribute) + 1);
	if (NetPDLElement->IncludedBlockName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
		return NULL;
	}
	strcpy(NetPDLElement->IncludedBlockName, Attribute);

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}

struct _nbNetPDLElementBase *CreateElementBlock(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementBlock *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementBlock *) malloc(sizeof(struct _nbNetPDLElementBlock));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementBlock));

	// Let's check the node name
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_NAME);

	NetPDLElement->Name= (char *) malloc(sizeof(char) * (strlen(Attribute) + 1));
	if (NetPDLElement->Name == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
		return NULL;
	}

	strcpy(NetPDLElement->Name, Attribute);


	// Let's check the node longname
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_LONGNAME);
	if (Attribute)
	{
		NetPDLElement->LongName= (char *) malloc(sizeof(char) * (strlen(Attribute) + 1));
		if (NetPDLElement->LongName == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
			return NULL;
		}
		strcpy(NetPDLElement->LongName, Attribute);
	}


	// Let's check the 'showsumtemplate'
	Attribute= GetXMLAttribute(Attributes, NETPDL_PROTO_ATTR_SHOWSUMTEMPLATE);

	if (Attribute)
	{
		NetPDLElement->ShowSumTemplateName= (char *) malloc(strlen(Attribute) + 1);
		if (NetPDLElement->ShowSumTemplateName == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
			return NULL;
		}
		strcpy(NetPDLElement->ShowSumTemplateName, Attribute);
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


int GetFieldBaseInfo(struct _nbNetPDLElementFieldBase *NetPDLElement, uint8_t MandatoryNameAttr, const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
char* Attribute;

	// Let's check the node name
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_NAME);

	if (Attribute)
	{
		NetPDLElement->Name= (char *) malloc(sizeof(char) * (strlen(Attribute) + 1));
		if (NetPDLElement->Name == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
			return nbFAILURE;
		}
		strcpy(NetPDLElement->Name, Attribute);
	}
	else
		if (MandatoryNameAttr)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Mandatory '%s' attribute missing for a <%s> element.", NETPDL_COMMON_ATTR_NAME, NetPDLElement->ElementName);
			return nbFAILURE;
		}

	// Let's check the node longname
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_LONGNAME);
	if (Attribute)
	{
		NetPDLElement->LongName= (char *) malloc(sizeof(char) * (strlen(Attribute) + 1));
		if (NetPDLElement->LongName == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
			return nbFAILURE;
		}
		strcpy(NetPDLElement->LongName, Attribute);
	}

	// Let's check the network byte order
	NetPDLElement->IsInNetworkByteOrder= true;

	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_BIGENDIAN);
	if (Attribute)
	{
		if (strcmp(Attribute, NETPDL_FIELD_ATTR_BIGENDIAN_YES) == 0)
			NetPDLElement->IsInNetworkByteOrder= false;
	}

	// Let's check the 'showtemplate'
	if (NetPDLDatabase->Flags & nbPROTODB_FULL)
	{
		Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_SHOWTEMPLATE);
		if (Attribute)
		{
			NetPDLElement->ShowTemplateName= (char *) malloc(strlen(Attribute) + 1);
			if (NetPDLElement->ShowTemplateName == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
				return nbFAILURE;
			}
			strcpy(NetPDLElement->ShowTemplateName, Attribute);
		}
		else
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
						"Error in NetPDL field '%s' (longname '%s'): it does not have any visualization primitive associated to it.",
						NetPDLElement->Name, NetPDLElement->LongName);
					return nbFAILURE;
				}
	}

	// Let's check the 'adt' reference
	Attribute= GetXMLAttribute(Attributes, NETPDL_ADT_ATTR_BASEADT);
	if (Attribute)
	{
		NetPDLElement->ADTRef= (char *) malloc(strlen(Attribute) + 1);
		if (NetPDLElement->ADTRef == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
			return nbFAILURE;
		}
		strcpy(NetPDLElement->ADTRef, Attribute);
	}

	return nbSUCCESS;
}


struct _nbNetPDLElementBase *CreateElementField(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
	char* Attribute;
	struct _nbNetPDLElementFieldBase *NetPDLElement;

	NetPDLElement= NULL;

	// Let's check the field type
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_TYPE);
	
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_FIXED) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementFieldFixed(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_BIT) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementFieldBit(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_VARIABLE) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementFieldVariable(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_TOKENWRAPPED) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementFieldTokenWrapped(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_TOKENENDED) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementFieldTokenEnded(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_LINE) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementFieldLine(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_PATTERN) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementFieldPattern(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_EATALL) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementFieldEatall(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_PADDING) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementFieldPadding(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_PLUGIN) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementFieldPlugin(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}

	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_TLV) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldTLV(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_DELIMITED) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldDelimited(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_ELINE) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldLine(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_HDRLINE) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldHdrline(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_DYNAMIC) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldDynamic(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_ASN1) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldASN1(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_XML) == 0)
	{
		NetPDLElement= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldXML(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementField_EndTypeCheck;
	}

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Field type %s unknown.", Attribute);
		return NULL;
	}

CreateElementField_EndTypeCheck:

	// If the field has not been created, let's return error
	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Creation of field type %s failed.", Attribute);
		return NULL;
	}

	if (GetFieldBaseInfo(NetPDLElement, true, Attributes, ErrBuf, ErrBufSize) == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Creation of a <field> element fails during checks for common attributes.");
		return NULL;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementFieldFixed(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
	struct _nbNetPDLElementFieldFixed *NetPDLElement;
	char *Attribute;

	NetPDLElement= (struct _nbNetPDLElementFieldFixed *) malloc(sizeof(struct _nbNetPDLElementFieldFixed));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementFieldFixed));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELD_FIXED;

	// Let's check the size
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_SIZE);

	if (Attribute == NULL)
		return NULL;

	NetPDLElement->Size= atoi(Attribute);

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementFieldVariable(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementFieldVariable *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementFieldVariable *) malloc(sizeof(struct _nbNetPDLElementFieldVariable));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementFieldVariable));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELD_VARIABLE;


	// Let's retrieve the expression related to the size
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_EXPR);
	if (Attribute == NULL)
		return NULL;

	if (CreateExpression(Attribute, CREATE_EXPR_ALLOW_ALL, &(NetPDLElement->ExprString), &(NetPDLElement->ExprTree), ErrBuf, ErrBufSize) == nbFAILURE)
		return NULL;

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementFieldTokenEnded(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementFieldTokenEnded *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementFieldTokenEnded *) malloc(sizeof(struct _nbNetPDLElementFieldTokenEnded));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementFieldTokenEnded));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELD_TOKENENDED;


	// Let's check token-ended specific tags
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_ENDTOKEN);
	if (Attribute)
	{
		NetPDLElement->EndTokenString= (unsigned char *) AllocateAsciiString(Attribute, &(NetPDLElement->EndTokenStringSize),/*0*/ 1, 1);

		if (NetPDLElement->EndTokenString == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
	}
	else
	{
	int PCREFlags= 0;
	int PCREErrorOffset;
	const char *PCREErrorPtr;

		Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_ENDREGEX);

		NetPDLElement->EndRegularExpression= AllocateAsciiString(Attribute, NULL, 1, 1);

		NetPDLElement->EndPCRECompiledRegExp= (void *) pcre_compile(NetPDLElement->EndRegularExpression, PCREFlags, &PCREErrorPtr, &PCREErrorOffset, NULL);

		if (NetPDLElement->EndPCRECompiledRegExp == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
				"Regular expression compilation for string '%s' has failed at offset %d with error '%s'",
				NetPDLElement->EndRegularExpression, PCREErrorOffset, PCREErrorPtr);

			return NULL;
		}
	}

	// Let's check if we have an expression for the ending offset
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_ENDOFFSET);
	if (Attribute)
	{
		if (CreateExpression(Attribute, CREATE_EXPR_ALLOW_NUMBER_ONLY, &(NetPDLElement->EndOffsetExprString), &(NetPDLElement->EndOffsetExprTree), ErrBuf, ErrBufSize) == nbFAILURE)
			return NULL;
	}

	// Let's check if we have an expression for the ending offset
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_ENDDISCARD);
	if (Attribute)
	{
		if (CreateExpression(Attribute, CREATE_EXPR_ALLOW_NUMBER_ONLY, &(NetPDLElement->EndDiscardExprString), &(NetPDLElement->EndDiscardExprTree), ErrBuf, ErrBufSize) == nbFAILURE)
			return NULL;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementFieldTokenWrapped(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementFieldTokenWrapped *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementFieldTokenWrapped *) malloc(sizeof(struct _nbNetPDLElementFieldTokenWrapped));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementFieldTokenWrapped));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELD_TOKENWRAPPED;


	// Let's check token-wrapped specific tags
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_BEGINTOKEN);
	if (Attribute)
	{
		NetPDLElement->BeginTokenString= (unsigned char *) AllocateAsciiString(Attribute, &(NetPDLElement->BeginTokenStringSize), 0, 1);

		if (NetPDLElement->BeginTokenString == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
	}
	else
	{
	int PCREFlags= 0;
	int PCREErrorOffset;
	const char *PCREErrorPtr;

		Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_BEGINREGEX);

		NetPDLElement->BeginRegularExpression= AllocateAsciiString(Attribute, NULL, 1, 1);

		NetPDLElement->BeginPCRECompiledRegExp= (void *) pcre_compile(NetPDLElement->BeginRegularExpression, PCREFlags, &PCREErrorPtr, &PCREErrorOffset, NULL);

		if (NetPDLElement->BeginPCRECompiledRegExp == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
				"Regular expression compilation for string '%s' has failed at offset %d with error '%s'",
				NetPDLElement->BeginRegularExpression, PCREErrorOffset, PCREErrorPtr);

			return NULL;
		}
	}


	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_ENDTOKEN);
	if (Attribute)
	{
		NetPDLElement->EndTokenString= (unsigned char *) AllocateAsciiString(Attribute, &(NetPDLElement->EndTokenStringSize), 0, 1);

		if (NetPDLElement->EndTokenString == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
	}
	else
	{
	int PCREFlags= 0;
	int PCREErrorOffset;
	const char *PCREErrorPtr;

		Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_ENDREGEX);

		NetPDLElement->EndRegularExpression= AllocateAsciiString(Attribute, NULL, 1, 1);

		NetPDLElement->EndPCRECompiledRegExp= (void *) pcre_compile(NetPDLElement->EndRegularExpression, PCREFlags, &PCREErrorPtr, &PCREErrorOffset, NULL);

		if (NetPDLElement->EndPCRECompiledRegExp == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
				"Regular expression compilation for string '%s' has failed at offset %d with error '%s'",
				NetPDLElement->EndRegularExpression, PCREErrorOffset, PCREErrorPtr);

			return NULL;
		}
	}

	// Let's check if we have an expression for beginning and ending offsets
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_BEGINOFFSET);
	if (Attribute)
	{
		if (CreateExpression(Attribute, CREATE_EXPR_ALLOW_NUMBER_ONLY, &(NetPDLElement->BeginOffsetExprString), &(NetPDLElement->BeginOffsetExprTree), ErrBuf, ErrBufSize) == nbFAILURE)
			return NULL;
	}

	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_ENDOFFSET);
	if (Attribute)
	{
		if (CreateExpression(Attribute, CREATE_EXPR_ALLOW_NUMBER_ONLY, &(NetPDLElement->EndOffsetExprString), &(NetPDLElement->EndOffsetExprTree), ErrBuf, ErrBufSize) == nbFAILURE)
			return NULL;
	}

	// Let's check if we have an expression for the ending offset
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_ENDDISCARD);
	if (Attribute)
	{
		if (CreateExpression(Attribute, CREATE_EXPR_ALLOW_NUMBER_ONLY, &(NetPDLElement->EndDiscardExprString), &(NetPDLElement->EndDiscardExprTree), ErrBuf, ErrBufSize) == nbFAILURE)
			return NULL;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementFieldLine(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementFieldLine *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLElementFieldLine *) malloc(sizeof(struct _nbNetPDLElementFieldLine));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementFieldLine));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELD_LINE;

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementFieldPattern(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementFieldPattern *NetPDLElement;
char* Attribute;
int PCREFlags= 0;
int PCREErrorOffset;
const char *PCREErrorPtr;

	NetPDLElement= (struct _nbNetPDLElementFieldPattern *) malloc(sizeof(struct _nbNetPDLElementFieldPattern));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementFieldPattern));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELD_PATTERN;

	// Let's check pattern regular expression
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELDS_COMMON_ATTR_PATTERN);
	if (Attribute == NULL)
		return NULL;

	NetPDLElement->PatternRegularExpression= AllocateAsciiString(Attribute, NULL, 1, 1);

	NetPDLElement->PatternPCRECompiledRegExp= (void *) pcre_compile(NetPDLElement->PatternRegularExpression, PCREFlags, &PCREErrorPtr, &PCREErrorOffset, NULL);

	if (NetPDLElement->PatternPCRECompiledRegExp == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
			"Regular expression compilation for string '%s' has failed at offset %d with error '%s'",
			NetPDLElement->PatternRegularExpression, PCREErrorOffset, PCREErrorPtr);

		return NULL;
	}

	// Let's check the decoding behaviour in case of missing ending delimiter
	NetPDLElement->OnPartialDecodingContinue= true;

	Attribute= GetXMLAttribute(Attributes, NETPDL_CFIELD_ATTR_ONPARTIALMATCH);
	if (Attribute)
	{
		if (strcmp(Attribute, NETPDL_CFIELD_ATTR_ONEVENT_SKIPFIELD) == 0)
			NetPDLElement->OnPartialDecodingContinue= false;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementFieldEatall(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementFieldEatall *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLElementFieldEatall *) malloc(sizeof(struct _nbNetPDLElementFieldEatall));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementFieldEatall));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELD_EATALL;

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementFieldPadding(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementFieldPadding *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementFieldPadding *) malloc(sizeof(struct _nbNetPDLElementFieldPadding));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementFieldPadding));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELD_PADDING;


	// Let's check the align size
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_ALIGN);
	if (Attribute == NULL)
		return NULL;

	NetPDLElement->Align= atoi(Attribute);

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementFieldPlugin(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
	struct _nbNetPDLElementFieldPlugin *NetPDLElement;
	char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementFieldPlugin *) malloc(sizeof(struct _nbNetPDLElementFieldPlugin));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementFieldPlugin));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELD_PLUGIN;

	// Let's check the plugin name
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_PLUGINNAME);
	if (Attribute == NULL)
		return NULL;

	NetPDLElement->PluginName= (char *) malloc(strlen(Attribute) + 1);
	if (NetPDLElement->PluginName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
		return NULL;
	}
	strcpy(NetPDLElement->PluginName, Attribute);

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}

struct _nbNetPDLElementBase *CreateElementFieldBit(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
char* Attribute;
char TmpBuf[NETPDL_MAX_STRING + 1];
struct _nbNetPDLElementFieldBit *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLElementFieldBit *) malloc(sizeof(struct _nbNetPDLElementFieldBit));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementFieldBit));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELD_BIT;


	// Let's check the mask
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_MASK);

	// Store the bitmask as number
	int MaskLength= (int) strlen(Attribute);

	NetPDLElement->BitMask= GetNumber(Attribute, MaskLength);

	// Convert the mask in hex and store in the appropriate struct member
	ssnprintf(TmpBuf, sizeof(TmpBuf), "%0*x", MaskLength - 2, NetPDLElement->BitMask);

	NetPDLElement->BitMaskString= (char *) malloc(sizeof(char) * (strlen(TmpBuf) + 1));
	if (NetPDLElement->BitMaskString == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
		return NULL;
	}

	strcpy(NetPDLElement->BitMaskString, TmpBuf);

	// Let's check the size
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_SIZE);
	NetPDLElement->Size= atoi(Attribute);

	// The 'IsLastBitField' member is filled at the 'organize' step.

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}

struct _nbNetPDLElementBase *CreateElementCfieldTLV(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementCfieldTLV *NetPDLElement;
char* Attribute;
int RetVal;

	NetPDLElement= (struct _nbNetPDLElementCfieldTLV *) malloc(sizeof(struct _nbNetPDLElementCfieldTLV));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementCfieldTLV));

	NetPDLElement->FieldType= nbNETPDL_ID_CFIELD_TLV;

	// Let's check the size of the 'Type' portion
	Attribute= GetXMLAttribute(Attributes, NETPDL_CFIELD_ATTR_TSIZE);
	if (Attribute == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Attribute '%s' misses within <%s> element.", NETPDL_CFIELD_ATTR_TSIZE, NetPDLElement->ElementName);
		return NULL;
	}
	NetPDLElement->TypeSize= atoi(Attribute);
	if (NetPDLElement->TypeSize <= 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Attribute '%s' within <%s> element must be positive.", NETPDL_CFIELD_ATTR_TSIZE, NetPDLElement->ElementName);
		return NULL;
	}

	// Let's check the size of 'Length' portion
	Attribute= GetXMLAttribute(Attributes, NETPDL_CFIELD_ATTR_LSIZE);
	if (Attribute == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Attribute '%s' misses within <%s> element.", NETPDL_CFIELD_ATTR_LSIZE, NetPDLElement->ElementName);
		return NULL;
	}
	NetPDLElement->LengthSize= atoi(Attribute);
	if (NetPDLElement->LengthSize <= 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Attribute '%s' within <%s> element must be positive.", NETPDL_CFIELD_ATTR_LSIZE, NetPDLElement->ElementName);
		return NULL;
	}

	// Let's check if expression for 'Value' portion exists
	Attribute= GetXMLAttribute(Attributes, NETPDL_CFIELD_ATTR_VEXPR);
	if (Attribute)
	{
		RetVal= CreateExpression(Attribute, CREATE_EXPR_ALLOW_ALL, &(NetPDLElement->ValueExprString), &(NetPDLElement->ValueExprTree), ErrBuf, ErrBufSize);
		if (RetVal == nbFAILURE)
			return NULL;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementCfieldDelimited(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementCfieldDelimited *NetPDLElement;
char* Attribute;
int PCREFlags= 0;
int PCREErrorOffset;
const char *PCREErrorPtr;

	NetPDLElement= (struct _nbNetPDLElementCfieldDelimited *) malloc(sizeof(struct _nbNetPDLElementCfieldDelimited));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementCfieldDelimited));

	NetPDLElement->FieldType= nbNETPDL_ID_CFIELD_DELIMITED;

	// Let's check starting preamble regular expression
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELDS_COMMON_ATTR_BEGINREGEX);

	if (Attribute)
	{
		NetPDLElement->BeginRegularExpression= AllocateAsciiString(Attribute, NULL, 1, 1);

		NetPDLElement->BeginPCRECompiledRegExp= (void *) pcre_compile(NetPDLElement->BeginRegularExpression, PCREFlags, &PCREErrorPtr, &PCREErrorOffset, NULL);

		if (NetPDLElement->BeginPCRECompiledRegExp == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
				"Regular expression compilation for string '%s' has failed at offset %d with error '%s'",
				NetPDLElement->BeginRegularExpression, PCREErrorOffset, PCREErrorPtr);

			return NULL;
		}
	}

	// Let's check ending delimiter regular expression
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELDS_COMMON_ATTR_ENDREGEX);
	if (Attribute == NULL)
		return NULL;

	NetPDLElement->EndRegularExpression= AllocateAsciiString(Attribute, NULL, 1, 1);

	NetPDLElement->EndPCRECompiledRegExp= (void *) pcre_compile(NetPDLElement->EndRegularExpression, PCREFlags, &PCREErrorPtr, &PCREErrorOffset, NULL);

	if (NetPDLElement->EndPCRECompiledRegExp == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
			"Regular expression compilation for string '%s' has failed at offset %d with error '%s'",
			NetPDLElement->EndRegularExpression, PCREErrorOffset, PCREErrorPtr);

		return NULL;
	}

	// Let's check the decoding behaviour in case of missing starting preamble
	NetPDLElement->OnMissingBeginContinue= true;

	Attribute= GetXMLAttribute(Attributes, NETPDL_CFIELD_ATTR_ONMISSINGBEGIN);
	if (Attribute)
	{
		if (strcmp(Attribute, NETPDL_CFIELD_ATTR_ONEVENT_SKIPFIELD) == 0)
			NetPDLElement->OnMissingBeginContinue= false;
	}

	// Let's check the decoding behaviour in case of missing ending delimiter
	NetPDLElement->OnMissingEndContinue= true;

	Attribute= GetXMLAttribute(Attributes, NETPDL_CFIELD_ATTR_ONMISSINGEND);
	if (Attribute)
	{
		if (strcmp(Attribute, NETPDL_CFIELD_ATTR_ONEVENT_SKIPFIELD) == 0)
			NetPDLElement->OnMissingEndContinue= false;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementCfieldLine(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementCfieldLine *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementCfieldLine *) malloc(sizeof(struct _nbNetPDLElementCfieldLine));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementCfieldLine));

	NetPDLElement->FieldType= nbNETPDL_ID_CFIELD_LINE;

	// Let's check the encoding
	NetPDLElement->Encoding= nbNETPDL_ID_ENCODING_ASCII;
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELDS_COMMON_ATTR_ENCODING);
	if (Attribute != NULL)
	{
		if (strcmp(Attribute, NETPDL_FIELDS_COMMON_ATTR_ENCODING_ASCII) == 0)
			{ goto CreateElementCfieldLine_EncOk; }
		if (strcmp(Attribute, NETPDL_FIELDS_COMMON_ATTR_ENCODING_UTF8) == 0)
			{ NetPDLElement->Encoding= nbNETPDL_ID_ENCODING_UTF8; goto CreateElementCfieldLine_EncOk; }
		if (strcmp(Attribute, NETPDL_FIELDS_COMMON_ATTR_ENCODING_UNICODE) == 0)
			{ NetPDLElement->Encoding= nbNETPDL_ID_ENCODING_UNICODE; goto CreateElementCfieldLine_EncOk; }

		return NULL;

CreateElementCfieldLine_EncOk:
		;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementCfieldHdrline(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementCfieldHdrline *NetPDLElement;
char* Attribute;
int PCREFlags= 0;
int PCREErrorOffset;
const char *PCREErrorPtr;

	NetPDLElement= (struct _nbNetPDLElementCfieldHdrline *) malloc(sizeof(struct _nbNetPDLElementCfieldHdrline));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementCfieldHdrline));

	NetPDLElement->FieldType= nbNETPDL_ID_CFIELD_HDRLINE;

	// Let's check separator regular expression
	Attribute= GetXMLAttribute(Attributes, NETPDL_CFIELD_ATTR_SEPREGEX);
	if (Attribute == NULL)
		return NULL;
	
	NetPDLElement->SeparatorRegularExpression= AllocateAsciiString(Attribute, NULL, 1, 1);

	NetPDLElement->SeparatorPCRECompiledRegExp= (void *) pcre_compile(NetPDLElement->SeparatorRegularExpression, PCREFlags, &PCREErrorPtr, &PCREErrorOffset, NULL);

	if (NetPDLElement->SeparatorPCRECompiledRegExp == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
			"Regular expression compilation for string '%s' has failed at offset %d with error '%s'",
			NetPDLElement->SeparatorRegularExpression, PCREErrorOffset, PCREErrorPtr);

		return NULL;
	}

	// Let's check the encoding
	NetPDLElement->Encoding= nbNETPDL_ID_ENCODING_ASCII;
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELDS_COMMON_ATTR_ENCODING);
	if (Attribute != NULL)
	{
		if (strcmp(Attribute, NETPDL_FIELDS_COMMON_ATTR_ENCODING_ASCII) == 0)
			{ goto CreateElementCfieldHdrline_EncOk; }
		if (strcmp(Attribute, NETPDL_FIELDS_COMMON_ATTR_ENCODING_UTF8) == 0)
			{ NetPDLElement->Encoding= nbNETPDL_ID_ENCODING_UTF8; goto CreateElementCfieldHdrline_EncOk; }
		if (strcmp(Attribute, NETPDL_FIELDS_COMMON_ATTR_ENCODING_UNICODE) == 0)
			{ NetPDLElement->Encoding= nbNETPDL_ID_ENCODING_UNICODE; goto CreateElementCfieldHdrline_EncOk; }

		return NULL;

CreateElementCfieldHdrline_EncOk:
		;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementCfieldDynamic(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementCfieldDynamic *NetPDLElement;
char* Attribute;
int PCREFlags= 0;
int PCREErrorOffset;
const char *PCREErrorPtr;
int PCRENameEntrySize;
char *PCRENameTable;

	NetPDLElement= (struct _nbNetPDLElementCfieldDynamic *) malloc(sizeof(struct _nbNetPDLElementCfieldDynamic));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementCfieldDynamic));

	NetPDLElement->FieldType= nbNETPDL_ID_CFIELD_DYNAMIC;

	// Let's check pattern regular expression
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELDS_COMMON_ATTR_PATTERN);
	if (Attribute == NULL)
		return NULL;

	NetPDLElement->PatternRegularExpression= AllocateAsciiString(Attribute, NULL, 1, 1);

	NetPDLElement->PatternPCRECompiledRegExp= (void *) pcre_compile(NetPDLElement->PatternRegularExpression, PCREFlags, &PCREErrorPtr, &PCREErrorOffset, NULL);

	if (NetPDLElement->PatternPCRECompiledRegExp == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
			"Regular expression compilation for string '%s' has failed at offset %d with error '%s'",
			NetPDLElement->PatternRegularExpression, PCREErrorOffset, PCREErrorPtr);

		return NULL;
	}

	if (pcre_fullinfo((pcre *) NetPDLElement->PatternPCRECompiledRegExp, NULL, PCRE_INFO_NAMECOUNT, (void *) &NetPDLElement->NamesListNItems) != 0
		||
		pcre_fullinfo((pcre *) NetPDLElement->PatternPCRECompiledRegExp, NULL, PCRE_INFO_NAMEENTRYSIZE, (void *) &PCRENameEntrySize) != 0
		||
		pcre_fullinfo((pcre *) NetPDLElement->PatternPCRECompiledRegExp, NULL, PCRE_INFO_NAMETABLE, (void *) &PCRENameTable) != 0)
		return NULL;

	NetPDLElement->NamesList= (char **) malloc(sizeof(char *) * NetPDLElement->NamesListNItems);

	if (NetPDLElement->NamesList == NULL)
		return NULL;

	memset(NetPDLElement->NamesList, 0, NetPDLElement->NamesListNItems);

	unsigned int IndexNameTable= 0;
	int IndexNameEntry;

	while (IndexNameTable < NetPDLElement->NamesListNItems)
	{
		IndexNameEntry= PCRENameTable[IndexNameTable*PCRENameEntrySize] * 256 + 
						PCRENameTable[IndexNameTable*PCRENameEntrySize+1];

		NetPDLElement->NamesList[IndexNameEntry-1]= &PCRENameTable[IndexNameTable*PCRENameEntrySize+2];

		IndexNameTable++;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementCfieldASN1(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementCfieldASN1 *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementCfieldASN1 *) malloc(sizeof(struct _nbNetPDLElementCfieldASN1));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementCfieldASN1));

	NetPDLElement->FieldType= nbNETPDL_ID_CFIELD_ASN1;

	// Let's check the encoding
	NetPDLElement->Encoding= nbNETPDL_ID_ENCODING_BER;
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELDS_COMMON_ATTR_ENCODING);
	if (Attribute != NULL)
	{
		if (strcmp(Attribute, NETPDL_FIELDS_COMMON_ATTR_ENCODING_BER) == 0)
			{ goto CreateElementCfieldASN1_EncOk; }
		if (strcmp(Attribute, NETPDL_FIELDS_COMMON_ATTR_ENCODING_DER) == 0)
			{ NetPDLElement->Encoding= nbNETPDL_ID_ENCODING_DER; goto CreateElementCfieldASN1_EncOk; }
		if (strcmp(Attribute, NETPDL_FIELDS_COMMON_ATTR_ENCODING_CER) == 0)
			{ NetPDLElement->Encoding= nbNETPDL_ID_ENCODING_CER; goto CreateElementCfieldASN1_EncOk; }

		return NULL;

CreateElementCfieldASN1_EncOk:
		;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementCfieldXML(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementCfieldXML *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementCfieldXML *) malloc(sizeof(struct _nbNetPDLElementCfieldXML));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementCfieldXML));

	NetPDLElement->FieldType= nbNETPDL_ID_CFIELD_XML;


	// Let's retrieve the expression related to the size
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_SIZE);

	if (Attribute && CreateExpression(Attribute, CREATE_EXPR_ALLOW_ALL, &(NetPDLElement->SizeExprString), &(NetPDLElement->SizeExprTree), ErrBuf, ErrBufSize) == nbFAILURE)
		return NULL;

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


int GetSubfieldBaseInfo(struct _nbNetPDLElementSubfield *NetPDLElement, const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
char* Attribute;

	// Let's check the portion name
	Attribute= GetXMLAttribute(Attributes, NETPDL_SUBFIELD_ATTR_PORTION);
	if (Attribute == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return nbFAILURE;
	}

	if (strcmp(Attribute, NETPDL_SUBFIELD_ATTR_PORTION_TLV_TYPE) == 0)
	{
		NetPDLElement->Portion= nbNETPDL_ID_SUBFIELD_PORTION_TLV_TYPE;
		NetPDLElement->PortionName= NETPDL_SUBFIELD_NAME_TLV_TYPE;
		goto CreateSubfieldBase_EndPortionCheck;
	}
	if (strcmp(Attribute, NETPDL_SUBFIELD_ATTR_PORTION_TLV_LENGTH) == 0)
	{
		NetPDLElement->Portion= nbNETPDL_ID_SUBFIELD_PORTION_TLV_LENGTH;
		NetPDLElement->PortionName= NETPDL_SUBFIELD_NAME_TLV_LENGTH;
		goto CreateSubfieldBase_EndPortionCheck;
	}
	if (strcmp(Attribute, NETPDL_SUBFIELD_ATTR_PORTION_TLV_VALUE) == 0)
	{
		NetPDLElement->Portion= nbNETPDL_ID_SUBFIELD_PORTION_TLV_VALUE;
		NetPDLElement->PortionName= NETPDL_SUBFIELD_NAME_TLV_VALUE;
		goto CreateSubfieldBase_EndPortionCheck;
	}
	if (strcmp(Attribute, NETPDL_SUBFIELD_ATTR_PORTION_HDRLINE_HNAME) == 0)
	{
		NetPDLElement->Portion= nbNETPDL_ID_SUBFIELD_PORTION_HDRLINE_HNAME;
		NetPDLElement->PortionName= NETPDL_SUBFIELD_NAME_HDRLINE_HNAME;
		goto CreateSubfieldBase_EndPortionCheck;
	}
	if (strcmp(Attribute, NETPDL_SUBFIELD_ATTR_PORTION_HDRLINE_HVALUE) == 0)
	{
		NetPDLElement->Portion= nbNETPDL_ID_SUBFIELD_PORTION_HDRLINE_HVALUE;
		NetPDLElement->PortionName= NETPDL_SUBFIELD_NAME_HDRLINE_HVALUE;
		goto CreateSubfieldBase_EndPortionCheck;
	}
	if (strncmp(Attribute, NETPDL_SUBFIELD_ATTR_PORTION_DYNAMIC_, strlen(NETPDL_SUBFIELD_ATTR_PORTION_DYNAMIC_)) == 0)
	{
		NetPDLElement->Portion= nbNETPDL_ID_SUBFIELD_PORTION_DYNAMIC;

		NetPDLElement->PortionName= AllocateAsciiString(&Attribute[strlen(NETPDL_SUBFIELD_ATTR_PORTION_DYNAMIC_)], NULL, 1, 1);

		if (NetPDLElement->PortionName == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return nbFAILURE;
		}

		goto CreateSubfieldBase_EndPortionCheck;
	}

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Subfield %s unknown.", Attribute);
	return nbFAILURE;

CreateSubfieldBase_EndPortionCheck:

	return GetFieldBaseInfo((struct _nbNetPDLElementFieldBase *) NetPDLElement, true, Attributes, ErrBuf, ErrBufSize);
}


struct _nbNetPDLElementBase *CreateElementSubfield(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementSubfield *NetPDLElement;
char *Attribute;

	NetPDLElement= (struct _nbNetPDLElementSubfield *) malloc(sizeof(struct _nbNetPDLElementSubfield));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementSubfield));

	// Let's initialize current subfield as simple
	NetPDLElement->FieldType= nbNETPDL_ID_SUBFIELD_SIMPLE;

	// Let's check if current subfield is a complex type
	Attribute= GetXMLAttribute(Attributes, NETPDL_CSUBFIELD_ATTR_SUBTYPE);
	if (Attribute)
	{
		NetPDLElement->FieldType= nbNETPDL_ID_SUBFIELD_COMPLEX;

		if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_TLV) == 0)
		{
			NetPDLElement->ComplexSubfieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldTLV(Attributes, ErrBuf, ErrBufSize);
			goto CreateElementSubfield_EndTypeCheck;
		}
		if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_DELIMITED) == 0)
		{
			NetPDLElement->ComplexSubfieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldDelimited(Attributes, ErrBuf, ErrBufSize);
			goto CreateElementSubfield_EndTypeCheck;
		}
		if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_ELINE) == 0)
		{
			NetPDLElement->ComplexSubfieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldLine(Attributes, ErrBuf, ErrBufSize);
			goto CreateElementSubfield_EndTypeCheck;
		}
		if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_HDRLINE) == 0)
		{
			NetPDLElement->ComplexSubfieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldHdrline(Attributes, ErrBuf, ErrBufSize);
			goto CreateElementSubfield_EndTypeCheck;
		}
		if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_DYNAMIC) == 0)
		{
			NetPDLElement->ComplexSubfieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldDynamic(Attributes, ErrBuf, ErrBufSize);
			goto CreateElementSubfield_EndTypeCheck;
		}
		if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_ASN1) == 0)
		{
			NetPDLElement->ComplexSubfieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldASN1(Attributes, ErrBuf, ErrBufSize);
			goto CreateElementSubfield_EndTypeCheck;
		}
		if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_XML) == 0)
		{
			NetPDLElement->ComplexSubfieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldXML(Attributes, ErrBuf, ErrBufSize);
			goto CreateElementSubfield_EndTypeCheck;
		}

		// If the complex subfield type has not been recognized, let's return error
		if (NetPDLElement->ComplexSubfieldInfo == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Complex subfield type '%s' not allowed.", Attribute);
			return NULL;
		}

CreateElementSubfield_EndTypeCheck:

		// If data structure related to complex subfield has not been created, let's return error
		if (NetPDLElement->ComplexSubfieldInfo == NULL)
			return NULL;
	}

	// Let's retrieve basic information about subfield
	if (GetSubfieldBaseInfo(NetPDLElement, Attributes, ErrBuf, ErrBufSize) == nbFAILURE)
		return NULL;

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementMap(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
char* Attribute;
struct _nbNetPDLElementMapBase *NetPDLElement;

	NetPDLElement= NULL;

	// Let's check the map field type
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELDS_COMMON_ATTR_TYPE);

	if (strcmp(Attribute, NETPDL_MAP_ATTR_TYPE_XML_PI) == 0)
		NetPDLElement= (struct _nbNetPDLElementMapBase *) CreateElementMapXMLPI(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_MAP_ATTR_TYPE_XML_DOCTYPE) == 0)
		NetPDLElement= (struct _nbNetPDLElementMapBase *) CreateElementMapXMLDoctype(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_MAP_ATTR_TYPE_XML_ELEMENT) == 0)
		NetPDLElement= (struct _nbNetPDLElementMapBase *) CreateElementMapXMLElement(Attributes, ErrBuf, ErrBufSize);

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Map type %s unknown.", Attribute);
		return NULL;
	}

	// Let's check the name reference attribute
	Attribute= GetXMLAttribute(Attributes, NETPDL_MAP_ATTR_REFNAME);
	if (Attribute == NULL)
		return NULL;

	NetPDLElement->RefName= AllocateAsciiString(Attribute, NULL, 1, 1);

	if (NetPDLElement->RefName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	// Let's check the node name
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_NAME);

	if (Attribute)
	{
		NetPDLElement->Name= (char *) malloc(sizeof(char) * (strlen(Attribute) + 1));
		if (NetPDLElement->Name == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
			return NULL;
		}

		strcpy(NetPDLElement->Name, Attribute);

		// Let's check the node longname
		Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_LONGNAME);
		if (Attribute)
		{
			NetPDLElement->LongName= (char *) malloc(sizeof(char) * (strlen(Attribute) + 1));
			if (NetPDLElement->LongName == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
				return NULL;
			}
			strcpy(NetPDLElement->LongName, Attribute);
		}

		// Let's check the network byte order
		NetPDLElement->IsInNetworkByteOrder= true;

		Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_BIGENDIAN);
		if (Attribute)
		{
			if (strcmp(Attribute, NETPDL_FIELD_ATTR_BIGENDIAN_YES) == 0)
				NetPDLElement->IsInNetworkByteOrder= false;
		}

		// Let's check the 'showtemplate'
		Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_SHOWTEMPLATE);
		if (Attribute)
		{
			NetPDLElement->ShowTemplateName= (char *) malloc(strlen(Attribute) + 1);
			if (NetPDLElement->ShowTemplateName == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
				return NULL;
			}
			strcpy(NetPDLElement->ShowTemplateName, Attribute);
		}
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementMapXMLPI(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementMapXMLPI *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementMapXMLPI *) malloc(sizeof(struct _nbNetPDLElementMapXMLPI));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementMapXMLPI));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELD_MAP_XML_PI;

	// Let's check if attributes must be shown
	Attribute= GetXMLAttribute(Attributes, NETPDL_MAP_ATTR_ATTSVIEW);
	if (strcmp(Attribute, NETPDL_MAP_ATTR_ATTSVIEW_YES) == 0)
		NetPDLElement->ShowAttributes= true;

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementMapXMLDoctype(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementMapXMLDoctype *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLElementMapXMLDoctype *) malloc(sizeof(struct _nbNetPDLElementMapXMLDoctype));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementMapXMLDoctype));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELD_MAP_XML_DOCTYPE;

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementMapXMLElement(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementMapXMLElement *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementMapXMLElement *) malloc(sizeof(struct _nbNetPDLElementMapXMLElement));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementMapXMLElement));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELD_MAP_XML_ELEMENT;

	// Let's check if attributes must be shown
	Attribute= GetXMLAttribute(Attributes, NETPDL_MAP_ATTR_ATTSVIEW);
	if (strcmp(Attribute, NETPDL_MAP_ATTR_ATTSVIEW_YES) == 0)
		NetPDLElement->ShowAttributes= true;

	// Let's get XML element namespace
	Attribute= GetXMLAttribute(Attributes, NETPDL_MAP_ATTR_NAMESPACE);
	if (Attribute)
	{
		NetPDLElement->NamespaceString= AllocateAsciiString(Attribute, NULL, 1, 1);

		if (NetPDLElement->NamespaceString == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
	}

	// Let's get XML element hierarcy
	Attribute= GetXMLAttribute(Attributes, NETPDL_MAP_ATTR_HIERARCY);
	if (Attribute)
	{
		NetPDLElement->HierarcyString= AllocateAsciiString(Attribute, NULL, 1, 1);

		if (NetPDLElement->HierarcyString == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementFieldmatch(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementFieldmatch *NetPDLElement;
char* Attribute;
int RetVal;

	NetPDLElement= (struct _nbNetPDLElementFieldmatch *) malloc(sizeof(struct _nbNetPDLElementFieldmatch));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementFieldmatch));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELDMATCH;

	// Let's retrieve the expression to match this field
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELDS_COMMON_ATTR_MATCH);
	RetVal= CreateExpression(Attribute, CREATE_EXPR_ALLOW_NUMBER_ONLY, &(NetPDLElement->MatchExprString), &(NetPDLElement->MatchExprTree), ErrBuf, ErrBufSize);
	if (RetVal == nbFAILURE)
		return NULL;

	// Let's check the occurrency of the current match
	NetPDLElement->Recurring= false;

	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELDS_COMMON_ATTR_RECURRING);
	if (Attribute)
	{
		if (strcmp(Attribute, NETPDL_FIELDS_COMMON_ATTR_RECURRING_YES) == 0)
			NetPDLElement->Recurring= true;
	}

	if (GetFieldBaseInfo((struct _nbNetPDLElementFieldBase *) NetPDLElement, true, Attributes, ErrBuf, ErrBufSize) == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Creation of a <fieldmatch> element fails during checks for common attributes.");
		return NULL;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementAdtfield(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementAdtfield *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementAdtfield *) malloc(sizeof(struct _nbNetPDLElementAdtfield));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementAdtfield));

	NetPDLElement->FieldType= nbNETPDL_ID_ADTFIELD;

	// Let's check the ADT type
	Attribute= GetXMLAttribute(Attributes, NETPDL_ADT_ATTR_ADTTYPE);
	if (Attribute == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "<adtfield> element requires '%s' attribute.", NETPDL_ADT_ATTR_ADTTYPE);
		return NULL;
	}

	NetPDLElement->CalledADTName= (char *) malloc(strlen(Attribute) + 1);
	if (NetPDLElement->CalledADTName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
		return NULL;
	}
	strcpy(NetPDLElement->CalledADTName, Attribute);

	// Let's chesk basic field-related attributes
	if (GetFieldBaseInfo((struct _nbNetPDLElementFieldBase *) NetPDLElement, false, Attributes, ErrBuf, ErrBufSize) == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Creation of a <adtfield> element with ADT type %s fails during checks for common attributes.", NetPDLElement->CalledADTName);
		return NULL;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}

struct _nbNetPDLElementBase *CreateElementReplace(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementReplace *NetPDLElement;
char* Attribute;
////unsigned int FieldToRenameIndex;

	NetPDLElement= (struct _nbNetPDLElementReplace *) malloc(sizeof(struct _nbNetPDLElementReplace));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementReplace));

	NetPDLElement->FieldType= nbNETPDL_ID_FIELD_REPLACE;

	// Let's check the ADT's field to replace
	Attribute= GetXMLAttribute(Attributes, NETPDL_REPLACE_ATTR_NAMEREF);
	if (Attribute == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "<replace> element requires '%s' attribute.", NETPDL_REPLACE_ATTR_NAMEREF);
		return NULL;
	}

	NetPDLElement->FieldToRename= (char *) malloc(strlen(Attribute) + 1);
	if (NetPDLElement->FieldToRename == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
		return NULL;
	}
	strcpy(NetPDLElement->FieldToRename, Attribute);

	//* TEMP FABIO: This is the code to use the namespace for replacing within ADT */
	//// Let's try to locate the name of the field to rename and its own ADT
	//Attribute= GetXMLAttribute(Attributes, NETPDL_REPLACE_ATTR_ADTREF);
	//if (Attribute == NULL)
	//	return NULL;

	//FieldToRenameIndex= 1; // at most one byte has to belong to ADT name, let's start from 2nd byte
	//while ( (FieldToRenameIndex + strlen(NETPDL_COMMON_SYNTAX_SEP_ADTREF)) < strlen(Attribute) )
	//{
	//	if ( strncmp(&Attribute[FieldToRenameIndex], NETPDL_COMMON_SYNTAX_SEP_ADTREF, strlen(NETPDL_COMMON_SYNTAX_SEP_ADTREF)) == 0 )
	//	{
	//		NetPDLElement->ADTName= (char *) malloc(sizeof(char) * (FieldToRenameIndex + 1));
	//		if (NetPDLElement->ADTName == NULL)
	//		{
	//			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
	//			return NULL;
	//		}
	//		strncpy(NetPDLElement->ADTName, Attribute, FieldToRenameIndex);
	//		NetPDLElement->ADTName[FieldToRenameIndex]= '\0'; // Let's close current string

	//		FieldToRenameIndex+= strlen(NETPDL_COMMON_SYNTAX_SEP_ADTREF);

	//		NetPDLElement->FieldToRename= (char *) malloc(sizeof(char) * (strlen(&Attribute[FieldToRenameIndex]) + 1));
	//		if (NetPDLElement->FieldToRename == NULL)
	//		{
	//			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
	//			return NULL;
	//		}
	//		strncpy(NetPDLElement->FieldToRename, &Attribute[FieldToRenameIndex], strlen(&Attribute[FieldToRenameIndex]));
	//		NetPDLElement->FieldToRename[strlen(&Attribute[FieldToRenameIndex])]= '\0'; // Let's close current string

	//		break;
	//	}
	//	
	//	FieldToRenameIndex++;
	//}

	//if ((FieldToRenameIndex + strlen(NETPDL_COMMON_SYNTAX_SEP_ADTREF)) >= strlen(Attribute))
	//{
	//	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
	//		"The '%s' ADT reference has a bad syntax, relevant renaming can not be performed.", Attribute);
	//	return NULL;
	//}

	// Let's check the node name
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_NAME);
	if (Attribute)
	{
		NetPDLElement->Name= (char *) malloc(sizeof(char) * (strlen(Attribute) + 1));
		if (NetPDLElement->Name == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
			return NULL;
		}
		strcpy(NetPDLElement->Name, Attribute);
	}

	// Let's check the node longname
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_LONGNAME);
	if (Attribute)
	{
		NetPDLElement->LongName= (char *) malloc(sizeof(char) * (strlen(Attribute) + 1));
		if (NetPDLElement->LongName == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
			return NULL;
		}
		strcpy(NetPDLElement->LongName, Attribute);
	}

	// Let's check the 'showtemplate'
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_SHOWTEMPLATE);
	if (Attribute)
	{
		NetPDLElement->ShowTemplateName= (char *) malloc(strlen(Attribute) + 1);
		if (NetPDLElement->ShowTemplateName == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for creating internal structures.");
			return NULL;
		}
		strcpy(NetPDLElement->ShowTemplateName, Attribute);
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementSet(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
char* Attribute;
struct _nbNetPDLElementSet *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLElementSet *) malloc(sizeof(struct _nbNetPDLElementSet));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementSet));

	// Let's check the set-related field type
	Attribute= GetXMLAttribute(Attributes, NETPDL_CFIELD_ATTR_TYPE);

	NetPDLElement->FieldToDecode= NULL;

	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_FIXED) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldFixed(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_BIT) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldBit(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_LINE) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldLine(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_VARIABLE) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldVariable(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_TOKENENDED) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldTokenEnded(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_TOKENWRAPPED) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldTokenWrapped(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_PATTERN) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldPattern(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_EATALL) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldEatall(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_PADDING) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldPadding(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_PLUGIN) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldPlugin(Attributes, ErrBuf, ErrBufSize);

	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_TLV) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldTLV(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_DELIMITED) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldDelimited(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_ELINE) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldLine(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_HDRLINE) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldHdrline(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_DYNAMIC) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldDynamic(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_ASN1) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldASN1(Attributes, ErrBuf, ErrBufSize);

	if (NetPDLElement->FieldToDecode == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "set type '%s' not allowed.", Attribute);
		return NULL;
	}

	// Let's check the network byte order of the set-related field type
	NetPDLElement->FieldToDecode->IsInNetworkByteOrder= true;

	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_BIGENDIAN);
	if (Attribute)
	{
		if (strcmp(Attribute, NETPDL_FIELD_ATTR_BIGENDIAN_YES) == 0)
			NetPDLElement->FieldToDecode->IsInNetworkByteOrder= false;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementExitWhen(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
char* Attribute;
struct _nbNetPDLElementExitWhen *NetPDLElement;
int RetVal;

	NetPDLElement= (struct _nbNetPDLElementExitWhen *) malloc(sizeof(struct _nbNetPDLElementExitWhen));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementExitWhen));

	// Let's retrieve the expression to evaluate for the set-related exit condition
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_EXPR);

	RetVal= CreateExpression(Attribute, CREATE_EXPR_ALLOW_NUMBER_ONLY, &(NetPDLElement->ExitExprString), &(NetPDLElement->ExitExprTree), ErrBuf, ErrBufSize);
	if (RetVal == nbFAILURE)
		return NULL;

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementDefaultItem(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementFieldmatch *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLElementFieldmatch *) malloc(sizeof(struct _nbNetPDLElementFieldmatch));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementFieldmatch));

	NetPDLElement->FieldType= nbNETPDL_ID_DEFAULT_MATCH;

	if (GetFieldBaseInfo((struct _nbNetPDLElementFieldBase *) NetPDLElement, true, Attributes, ErrBuf, ErrBufSize) == nbFAILURE)
		return NULL;

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementChoice(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
char* Attribute;
struct _nbNetPDLElementChoice *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLElementChoice *) malloc(sizeof(struct _nbNetPDLElementChoice));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementChoice));

	// Let's check the choice-related field type
	Attribute= GetXMLAttribute(Attributes, NETPDL_CFIELD_ATTR_TYPE);

	NetPDLElement->FieldToDecode= NULL;

	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_FIXED) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldFixed(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_BIT) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldBit(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_LINE) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldLine(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_VARIABLE) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldVariable(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_TOKENENDED) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldTokenEnded(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_TOKENWRAPPED) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldTokenWrapped(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_PATTERN) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldPattern(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_EATALL) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldEatall(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_PADDING) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldPadding(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_PLUGIN) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementFieldPlugin(Attributes, ErrBuf, ErrBufSize);

	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_TLV) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldTLV(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_DELIMITED) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldDelimited(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_ELINE) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldLine(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_HDRLINE) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldHdrline(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_DYNAMIC) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldDynamic(Attributes, ErrBuf, ErrBufSize);
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_ASN1) == 0)
		NetPDLElement->FieldToDecode= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldASN1(Attributes, ErrBuf, ErrBufSize);

	if (NetPDLElement->FieldToDecode == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "choice type '%s' not allowed.", Attribute);
		return NULL;
	}

	// Let's check the network byte order of the choice-related field type
	NetPDLElement->FieldToDecode->IsInNetworkByteOrder= true;

	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_BIGENDIAN);
	if (Attribute)
	{
		if (strcmp(Attribute, NETPDL_FIELD_ATTR_BIGENDIAN_YES) == 0)
			NetPDLElement->FieldToDecode->IsInNetworkByteOrder= false;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementAdt(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementAdt *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementAdt *) malloc(sizeof(struct _nbNetPDLElementAdt));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementAdt));


	// Let's retrieve the name of the ADT
	Attribute= GetXMLAttribute(Attributes, NETPDL_ADT_ATTR_ADTNAME);

	NetPDLElement->ADTName= AllocateAsciiString(Attribute, NULL, 1, 1);

	if (NetPDLElement->ADTName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	// Let's check the field type
	Attribute= GetXMLAttribute(Attributes, NETPDL_FIELD_ATTR_TYPE);

	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_FIXED) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementFieldFixed(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_BIT) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementFieldBit(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_VARIABLE) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementFieldVariable(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_TOKENWRAPPED) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementFieldTokenWrapped(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_TOKENENDED) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementFieldTokenEnded(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_LINE) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementFieldLine(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_PATTERN) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementFieldPattern(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_EATALL) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementFieldEatall(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_PADDING) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementFieldPadding(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_FIELD_ATTR_TYPE_PLUGIN) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementFieldPlugin(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}

	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_TLV) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldTLV(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_DELIMITED) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldDelimited(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_ELINE) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldLine(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_HDRLINE) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldHdrline(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_DYNAMIC) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldDynamic(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_ASN1) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldASN1(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}
	if (strcmp(Attribute, NETPDL_CFIELD_ATTR_TYPE_XML) == 0)
	{
		NetPDLElement->ADTFieldInfo= (struct _nbNetPDLElementFieldBase *) CreateElementCfieldXML(Attributes, ErrBuf, ErrBufSize);
		goto CreateElementAdt_EndTypeCheck;
	}

	// If the field type has not been recognized, let's return error
	if (NetPDLElement->ADTFieldInfo == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "ADT type %s unknown.", Attribute);
		return NULL;
	}

CreateElementAdt_EndTypeCheck:

	// If the field has not been created, let's return error
	if (NetPDLElement->ADTFieldInfo == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Creation of ADT type %s failed.", Attribute);
		return NULL;
	}

	if (GetFieldBaseInfo(NetPDLElement->ADTFieldInfo, true, Attributes, ErrBuf, ErrBufSize) == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Creation of a <adt> element fails during checks for common attributes.");
		return NULL;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementNextProto(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
	struct _nbNetPDLElementNextProto *NetPDLElement;
	char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementNextProto *) malloc(sizeof(struct _nbNetPDLElementNextProto));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementNextProto));

	// Let's retrieve the expression that defines which is the next protocol
	Attribute= GetXMLAttribute(Attributes, NETPDL_NEXTPROTO_ATTR_PROTO);

	if (CreateExpression(Attribute, CREATE_EXPR_ALLOW_NUMBER_ONLY, &(NetPDLElement->ExprString), &(NetPDLElement->ExprTree), ErrBuf, ErrBufSize) == nbFAILURE)
		return NULL;
		
	Attribute = GetXMLAttribute(Attributes, NETPDL_NEXTPROTO_ATTR_PREFERRED); //[icerrato]
	if(Attribute!=NULL)
	{
		if (CreateExpression(Attribute, CREATE_EXPR_ALLOW_BOOL_ONLY, &(NetPDLElement->ExprPreferred), &(NetPDLElement->ExprPreferredTree), ErrBuf, ErrBufSize) == nbFAILURE)//[icerrrato]
			return NULL;
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementShowCodeProtoField(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
	struct _nbNetPDLElementProtoField *NetPDLElement;
	char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementProtoField *) malloc(sizeof(struct _nbNetPDLElementProtoField));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementProtoField));


	Attribute= GetXMLAttribute(Attributes, NETPDL_SHOW_ATTR_SHOWDATA);

	if (strcmp(Attribute, PDML_FIELD_ATTR_VALUE) == 0)
		NetPDLElement->FieldShowDataType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_VALUE;

	if (strcmp(Attribute, PDML_FIELD_ATTR_NAME) == 0)
		NetPDLElement->FieldShowDataType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_NAME;

	if (strcmp(Attribute, PDML_FIELD_ATTR_LONGNAME) == 0)
		NetPDLElement->FieldShowDataType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_LONGNAME;

	if (strcmp(Attribute, PDML_FIELD_ATTR_SHOWMAP) == 0)
		NetPDLElement->FieldShowDataType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SHOWMAP;

	if (strcmp(Attribute, PDML_FIELD_ATTR_SHOWDTL) == 0)
		NetPDLElement->FieldShowDataType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SHOWDTL;

	if (strcmp(Attribute, PDML_FIELD_ATTR_SHOW) == 0)
		NetPDLElement->FieldShowDataType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SHOW;

	if (strcmp(Attribute, PDML_FIELD_ATTR_MASK) == 0)
		NetPDLElement->FieldShowDataType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_MASK;

	if (strcmp(Attribute, PDML_FIELD_ATTR_POSITION) == 0)
		NetPDLElement->FieldShowDataType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_POSITION;

	if (strcmp(Attribute, PDML_FIELD_ATTR_SIZE) == 0)
		NetPDLElement->FieldShowDataType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SIZE;

	// This attribute must be present in the summary visualization; however, it is not needed in the detailed view.
	Attribute= GetXMLAttribute(Attributes, NETPDL_SHOW_ATTR_NAME);

	if (Attribute)
	{
		NetPDLElement->FieldName= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
		if (NetPDLElement->FieldName == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
		strcpy(NetPDLElement->FieldName, Attribute);
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementShowCodeProtoHdr(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementProtoHdr *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementProtoHdr *) malloc(sizeof(struct _nbNetPDLElementProtoHdr));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementProtoHdr));


	Attribute= GetXMLAttribute(Attributes, NETPDL_SHOW_ATTR_SHOWDATA);

	if (strcmp(Attribute, PDML_FIELD_ATTR_VALUE) == 0)
		NetPDLElement->ProtoAttribType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_VALUE;

	if (strcmp(Attribute, PDML_FIELD_ATTR_NAME) == 0)
		NetPDLElement->ProtoAttribType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_NAME;

	if (strcmp(Attribute, PDML_FIELD_ATTR_LONGNAME) == 0)
		NetPDLElement->ProtoAttribType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_LONGNAME;

	if (strcmp(Attribute, PDML_FIELD_ATTR_SHOWMAP) == 0)
		NetPDLElement->ProtoAttribType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SHOWMAP;

	if (strcmp(Attribute, PDML_FIELD_ATTR_SHOWDTL) == 0)
		NetPDLElement->ProtoAttribType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SHOWDTL;

	if (strcmp(Attribute, PDML_FIELD_ATTR_SHOW) == 0)
		NetPDLElement->ProtoAttribType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SHOW;

	if (strcmp(Attribute, PDML_FIELD_ATTR_MASK) == 0)
		NetPDLElement->ProtoAttribType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_MASK;

	if (strcmp(Attribute, PDML_FIELD_ATTR_POSITION) == 0)
		NetPDLElement->ProtoAttribType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_POSITION;

	if (strcmp(Attribute, PDML_FIELD_ATTR_SIZE) == 0)
		NetPDLElement->ProtoAttribType= nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SIZE;

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementShowCodePacketHdr(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementPacketHdr *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementPacketHdr *) malloc(sizeof(struct _nbNetPDLElementPacketHdr));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementPacketHdr));


	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_VALUE);

	if (strcmp(Attribute, PDML_FIELD_ATTR_NAME_TIMESTAMP) == 0)
		NetPDLElement->Value= nbNETPDL_ID_NODE_SHOWCODE_PKTHDR_ATTRIB_TIMESTAMP;

	if (strcmp(Attribute, PDML_FIELD_ATTR_NAME_NUM) == 0)
		NetPDLElement->Value= nbNETPDL_ID_NODE_SHOWCODE_PKTHDR_ATTRIB_NUM;

	if (strcmp(Attribute, PDML_FIELD_ATTR_NAME_LENGTH) == 0)
		NetPDLElement->Value= nbNETPDL_ID_NODE_SHOWCODE_PKTHDR_ATTRIB_LEN;

	if (strcmp(Attribute, PDML_FIELD_ATTR_NAME_CAPLENGTH) == 0)
		NetPDLElement->Value= nbNETPDL_ID_NODE_SHOWCODE_PKTHDR_ATTRIB_CAPLEN;

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementShowCodeText(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementText *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementText *) malloc(sizeof(struct _nbNetPDLElementText));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementText));


	Attribute= GetXMLAttribute(Attributes, NETPDL_SHOW_TEXT_ATTR_VALUE);
	if (Attribute)
	{
		NetPDLElement->Value= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
		if (NetPDLElement->Value == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
		strcpy(NetPDLElement->Value, Attribute);
	}

	// Let's retrieve the expression related to the size
	Attribute= GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_EXPR);
	if (Attribute)
	{
		if (CreateExpression(Attribute, CREATE_EXPR_ALLOW_ALL, &(NetPDLElement->ExprString), &(NetPDLElement->ExprTree), ErrBuf, ErrBufSize) == nbFAILURE)
			return NULL;
	}

	// Let's retrieve the expression related to the size
	//Attribute= GetXMLAttribute(Attributes, NETPDL_SHOW_TEXT_ATTR_WHEN);

	//if (Attribute)
	//{
	//	if (strcmp(Attribute, NETPDL_SHOW_TEXT_ATTR_WHEN_ONLYEMPTY) == 0)
	//		NetPDLElement->When= nbNETPDL_TEXT_WHEN_ONLYEMPTY;

	//	if (strcmp(Attribute, NETPDL_SHOW_TEXT_ATTR_WHEN_ONLYSECTIONHASTEXT) == 0)
	//		NetPDLElement->When= nbNETPDL_TEXT_WHEN_ONLYSECTIONHASTEXT;

	//	if (strcmp(Attribute, NETPDL_SHOW_TEXT_ATTR_WHEN_ALWAYS) == 0)
	//		NetPDLElement->When= nbNETPDL_TEXT_WHEN_ALWAYS;
	//}
	//else
	//	NetPDLElement->When= nbNETPDL_TEXT_WHEN_ALWAYS;

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


struct _nbNetPDLElementBase *CreateElementShowCodeSection(const Attributes& Attributes, char *ErrBuf, int ErrBufSize)
{
struct _nbNetPDLElementSection *NetPDLElement;
char* Attribute;

	NetPDLElement= (struct _nbNetPDLElementSection *) malloc(sizeof(struct _nbNetPDLElementSection));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLElementSection));


	Attribute= GetXMLAttribute(Attributes, NETPDL_SHOW_ATTR_NAME);

	if (strcmp(Attribute, NETPDL_SHOW_ATTR_NAME_NEXT) == 0)
	{
		NetPDLElement->IsNext= true;
	}
	else
	{
		NetPDLElement->SectionName= (char *) malloc(sizeof(char) * strlen(Attribute) + 1);
		if (NetPDLElement->SectionName == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
			return NULL;
		}
		strcpy(NetPDLElement->SectionName, Attribute);
	}

	return (struct _nbNetPDLElementBase *) NetPDLElement;
}


/*!
	\brief Allocates a string that contains the call handler (if it has been specified).

	\param CallHandleAttribute: value of the attributes, as contained in the NetPDL file.

	\param CallHandlerInfo: pointer to the structure that will contain the information required by
	the call handler. This structure will be allocated inside the function, hence the caller has
	to call this function passing a (struct _nbCallHandlerInfo*) by reference.

	\param ErrBuf: user-allocated buffer that will contain the error message (if any).

	\param ErrBufSize: size of the previous buffer.

	\return nbSUCCESS if everything is fine, nbFAILURE in case of error.
*/
int AllocateCallHandle(char* CallHandleAttribute, struct _nbCallHandlerInfo **CallHandlerInfo, char* ErrBuf, int ErrBufSize)
{
int i, CallHandleAttributeLength;
int Separator;
char* Event = NULL;
int EventType;

	// If the user does not specify any call handler, let's return right now
	if (CallHandleAttribute == NULL)
		return nbSUCCESS;

	// Let's check the validity of the handle string
	CallHandleAttributeLength= (int) strlen(CallHandleAttribute);
	Separator= 0;

	for (i= 0; i < CallHandleAttributeLength; i++)
	{
		if (CallHandleAttribute[i] == ':')
		{
			CallHandleAttribute[i]= 0;
			Separator++;
			Event= &(CallHandleAttribute[i+1]);
		}
	}

	if (Separator != 2)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "The call handle must be in the form 'namespace:function:on_event'.");
		return nbFAILURE;
	}

	EventType= 0;

	// Let's check the event
	if (strcmp(Event, NETPDL_CALLHANDLE_EVENT_BEFORE) == 0)
		EventType= nbNETPDL_CALLHANDLE_EVENT_BEFORE;
	if (strcmp(Event, NETPDL_CALLHANDLE_EVENT_AFTER) == 0)
		EventType= nbNETPDL_CALLHANDLE_EVENT_AFTER;

	if (EventType == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "The event '%s' has not been recognized.", Event);
		return nbFAILURE;
	}

	// ... and, if everything is fine, let's allocate the struct
	*CallHandlerInfo= (struct _nbCallHandlerInfo *) malloc(sizeof(struct _nbCallHandlerInfo));
	if (*CallHandlerInfo == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return nbFAILURE;
	}
	memset(*CallHandlerInfo, 0, sizeof(struct _nbCallHandlerInfo));

	(*CallHandlerInfo)->FunctionName= (char *) malloc(sizeof(char) * (Event - CallHandleAttribute));
	if ((*CallHandlerInfo)->FunctionName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory for building the protocol database.");
		return nbFAILURE;
	}

	memcpy((*CallHandlerInfo)->FunctionName, CallHandleAttribute, (Event - CallHandleAttribute));

	(*CallHandlerInfo)->FireOnEvent= EventType;

	return nbSUCCESS;
}


/*!
	\brief Convert a buffer containing a number (e.g. '0x0800') into a number.

	This function takes into account the radix of the number.

	\param NumberBuffer: buffer containing the number
	\param BufferLength: length of the previous buffer
	\return The value contained the buffer.
*/
int GetNumber(char *NumberBuffer, int BufferLength)
{
int Result;

	if ((NumberBuffer[0] == '0') && (NumberBuffer[1] == 'x'))	// Hex number
		Result= NetPDLAsciiStringToLong(NumberBuffer+2, BufferLength - 2, 16);
	else
		if ((NumberBuffer[0] == '0') && (NumberBuffer[1] == 'b'))	// Bin number
			Result= NetPDLAsciiStringToLong(NumberBuffer+2, BufferLength - 2, 2);
		else											// Dec number
			Result= NetPDLAsciiStringToLong(NumberBuffer, BufferLength, 10);

	return Result;
}


char* GetXMLAttribute(const Attributes& Attributes, const char* AttributeName)
{
static char TmpBufAscii[2048];
static XMLCh TmpBufXML[2048];
const XMLCh* Attribute;

	XMLString::transcode(AttributeName, TmpBufXML, sizeof(TmpBufXML) / sizeof(XMLCh) - 1);

	Attribute= Attributes.getValue(TmpBufXML);

	if (Attribute)
	{
		XMLString::transcode(Attribute, TmpBufAscii, sizeof(TmpBufAscii) - 1);
		return (TmpBufAscii);
	}
	else
		return NULL;
}

