/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>

#include <nbee.h>
#include <nbprotodb.h>
#include <nbsockutils.h>
#include "../globals/globals.h"
#include "../globals/utils.h"
#include "../globals/debug.h"

// For Decoding engine
#include "../decoder/showplugin/show_plugin.h"
#include "../decoder/decoderplugin/decoder_plugin.h"
#include "../decoder/netpdlvariables.h"
#include "../decoder/netpdlexpression.h"
#include "../utils/netpdlutils.h"


// Locally defined functions
int InitProtoCode(struct _nbNetPDLElementBase *ExecuteElement, CNetPDLExpression *ExprHandler, CNetPDLVariables *NetPDLVariables, char *ErrBuf, int ErrBufSize);
int InitVariablesInExpression(struct _nbNetPDLExprBase* Expression, CNetPDLVariables *NetPDLVariables, char *ErrBuf, int ErrBufSize);


// For visualization and processing plugins
extern struct _ShowPluginList ShowPluginList[];
extern struct _PluginList PluginList[];
extern struct _nbNetPDLDatabase *NetPDLDatabase;




int InitializeNetPDLPlugins(char *ErrBuf, int ErrBufSize)
{
unsigned int i, j;

	// Set-up data for the visualization plug-ins
	for (i= 0; i < NetPDLDatabase->ShowTemplateNItems; i++)
	{
	struct _nbNetPDLElementShowTemplate *TemplateInfo;

		TemplateInfo= NetPDLDatabase->ShowTemplateList[i];
	
		if (TemplateInfo->PluginName)
		{
			for ( j= 0; ShowPluginList[j].PluginName != NULL; j++)
			{
				if (strcmp(TemplateInfo->PluginName, ShowPluginList[j].PluginName) == 0)
				{
					TemplateInfo->PluginIndex= j;
					break;
				}
			}

			if (ShowPluginList[j].PluginName == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
					"Custom visualization plugin '%s' (in template '%s') cannot be found in the current implementation.",
					TemplateInfo->PluginName, TemplateInfo->Name);

				nbProtoDBXMLCleanup();
				NetPDLDatabase= NULL;
				return nbFAILURE;
			}
		}
	}

	// Set-up data for the decoding plug-ins
	for (i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
	struct _nbNetPDLElementFieldPlugin *NetPDLElement;

		if (NetPDLDatabase->GlobalElementsList[i]->Type != nbNETPDL_IDEL_FIELD)
			continue;

		// We're examining a field
		// Then, let's check we're not going to process a plugin
		NetPDLElement= (struct _nbNetPDLElementFieldPlugin *) NetPDLDatabase->GlobalElementsList[i];

		if (NetPDLElement->FieldType != nbNETPDL_ID_FIELD_PLUGIN)
			continue;


		// Store the index in the current plugin list
		for (j= 0; PluginList[j].PluginName != NULL; j++)
		{
			if (strcmp(NetPDLElement->PluginName, PluginList[j].PluginName) == 0)
			{
				NetPDLElement->PluginIndex= j;
				break;
			}
		}

		if (PluginList[j].PluginName == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
				"Custom decoding plugin '%s' (in field '%s') cannot be found in the current implementation.",
				NetPDLElement->PluginName, NetPDLElement->Name);

			nbProtoDBXMLCleanup();
			NetPDLDatabase= NULL;
			return nbFAILURE;
		}
	}

	return nbSUCCESS;
}



int InitializeNetPDLInternalStructures(CNetPDLExpression *ExprHandler, CNetPDLVariables *NetPDLVariables, char *ErrBuf, int ErrBufSize)
{
unsigned int ProtoIndex;

	// Scan every member of the lookup table in order to update the size of their fields,
	// which is needed before creating variables.
	// Scan for every 'assign' element and update the custom pointer in them 
	//  so that they can use the VariableID instead of the variable name
	for (unsigned int i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
		if ((NetPDLDatabase->GlobalElementsList[i]->Type == nbNETPDL_IDEL_KEY) ||
			(NetPDLDatabase->GlobalElementsList[i]->Type == nbNETPDL_IDEL_DATA))
		{
		struct _nbNetPDLElementKeyData *KeyDataElement;

			KeyDataElement= (struct _nbNetPDLElementKeyData *) NetPDLDatabase->GlobalElementsList[i];

			switch (KeyDataElement->KeyDataType)
			{
				case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER:
					KeyDataElement->Size= sizeof(int);
					break;

				case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER:
					break;

				case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_PROTOCOL:
					KeyDataElement->Size= sizeof(int);
					break;

				default:
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
						"Internal error: code not allowed.");
					return nbFAILURE;
				}
			}
		}
	}


	// Scan protocol and execute 'init' code (if present)
	for (ProtoIndex= 0; ProtoIndex < NetPDLDatabase->ProtoListNItems; ProtoIndex++)
	{
	struct _nbNetPDLElementExecuteX *InitElement;

		InitElement= NetPDLDatabase->ProtoList[ProtoIndex]->FirstExecuteInit;

		while(InitElement)
		{
		int RetVal;
		unsigned int Result= 1;

			if (InitElement->WhenExprTree)
			{
				RetVal= ExprHandler->EvaluateExprNumber(InitElement->WhenExprTree, NULL, &Result);

				if (RetVal != nbSUCCESS)
					return RetVal;
			}

			if (Result)
			{
				RetVal= InitProtoCode(nbNETPDL_GET_ELEMENT(NetPDLDatabase, InitElement->FirstChild), ExprHandler, NetPDLVariables, ErrBuf, ErrBufSize);

				if (RetVal != nbSUCCESS)
					return RetVal;
			}

			// We may even have to execute several sections (if more 'when' attributes matches)
			InitElement= InitElement->NextExecuteElement;
		}
	}

	// Update the variableID for default variables
	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_LINKTYPE,			&(NetPDLVariables->m_defaultVarList.LinkType));
	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_FRAMELENGTH,			&(NetPDLVariables->m_defaultVarList.FrameLength));
	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_PACKETLENGTH,		&(NetPDLVariables->m_defaultVarList.PacketLength));
	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_CURRENTOFFSET,		&(NetPDLVariables->m_defaultVarList.CurrentOffset));
	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_CURRENTPROTOOFFSET,	&(NetPDLVariables->m_defaultVarList.CurrentProtoOffset));
	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_TIMESTAMP_SEC,		&(NetPDLVariables->m_defaultVarList.TimestampSec));
	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_TIMESTAMP_USEC,		&(NetPDLVariables->m_defaultVarList.TimestampUSec));
	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_PACKETBUFFER,		&(NetPDLVariables->m_defaultVarList.PacketBuffer));
	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_PREVPROTO,			&(NetPDLVariables->m_defaultVarList.PrevProto));
	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_NEXTPROTO,			&(NetPDLVariables->m_defaultVarList.NextProto));

	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_PROTOVERIFYRESULT,	&(NetPDLVariables->m_defaultVarList.ProtoVerifyResult));
	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_SHOWNETWORKNAMES,	&(NetPDLVariables->m_defaultVarList.ShowNetworkNames));

	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_TOKEN_BEGINTOKENLEN,	&(NetPDLVariables->m_defaultVarList.TokenBeginTokenLen));
	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_TOKEN_FIELDLEN,		&(NetPDLVariables->m_defaultVarList.TokenFieldLen));
	NetPDLVariables->GetVariableID(NETPDL_VARIABLE_TOKEN_ENDTOKENLEN,	&(NetPDLVariables->m_defaultVarList.TokenEndTokenLen));

	struct _nbNetPDLExprBase* Expression;

	// Update all the variables in the expression list so that they can use the VariableID instead of the variable name
	Expression= NetPDLDatabase->ExpressionList;

	while (Expression)
	{
		if (InitVariablesInExpression(Expression, NetPDLVariables, ErrBuf, ErrBufSize) == nbFAILURE)
			return nbFAILURE;

		Expression= Expression->NextExpression;
	}


	// Scan for every 'assign' and 'lookuptable' element and update the custom pointer in them 
	//  so that they can use the VariableID instead of the variable name
	for (unsigned int i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
		switch(NetPDLDatabase->GlobalElementsList[i]->Type)
		{
			case nbNETPDL_IDEL_ASSIGNVARIABLE:
			{
			struct _nbNetPDLElementAssignVariable *AssignVariableElement;
			int RetVal;
			int VariableID;

				AssignVariableElement= (struct _nbNetPDLElementAssignVariable*) NetPDLDatabase->GlobalElementsList[i];

				RetVal= NetPDLVariables->GetVariableID(AssignVariableElement->VariableName, &VariableID);

				if (RetVal == nbFAILURE)
					return nbFAILURE;

				// Here code is really dirty, since we're assigning an integer to a void*
				AssignVariableElement->CustomData= (void*) VariableID;
			};
			break;

			case nbNETPDL_IDEL_ASSIGNLOOKUPTABLE:
			{
			struct _nbNetPDLElementAssignLookupTable *LookupTableElement;
			int TableID, DataID;
			int RetVal;

				LookupTableElement= (struct _nbNetPDLElementAssignLookupTable*) NetPDLDatabase->GlobalElementsList[i];

				// We're not interested in the field name
				RetVal= NetPDLVariables->GetTableID(LookupTableElement->TableName, LookupTableElement->FieldName, &TableID, &DataID);

				if (RetVal == nbFAILURE)
					return nbFAILURE;

				// Here code is really dirty, since we're assigning an integer to a void*
				LookupTableElement->TableCustomData= (void*) TableID;
				LookupTableElement->FieldCustomData= (void*) DataID;
			};
			break;

			case nbNETPDL_IDEL_UPDATELOOKUPTABLE:
			{
			struct _nbNetPDLElementUpdateLookupTable *LookupTableElement;
			int TableID;
			int RetVal;

				LookupTableElement= (struct _nbNetPDLElementUpdateLookupTable*) NetPDLDatabase->GlobalElementsList[i];

				// We're not interested in the field name
				RetVal= NetPDLVariables->GetTableID(LookupTableElement->TableName, NULL, &TableID, NULL);

				if (RetVal == nbFAILURE)
					return nbFAILURE;

				// Here code is really dirty, since we're assigning an integer to a void*
				LookupTableElement->TableCustomData= (void*) TableID;
			};
			break;
		}
	}

	return nbSUCCESS;
}


/*!
	\brief Executes code that is required to initialize each protocol.

	\param ExecuteElement: first valid element within the initialization section.

	\param ExprHandler: pointer to the class that manages NetPDL expressions.
	\param NetPDLVariables: pointer to the class that handles NetPDL variables.
	\param ErrBuf: user-allocated buffer that will keep a description of the error (if any).
	\param ErrBufSize: size of the previous buffer.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int InitProtoCode(struct _nbNetPDLElementBase *ExecuteElement, CNetPDLExpression *ExprHandler, CNetPDLVariables *NetPDLVariables, char *ErrBuf, int ErrBufSize)
{
int RetVal;

	while (ExecuteElement)
	{
		switch (ExecuteElement->Type)
		{
			case nbNETPDL_IDEL_VARIABLE:
			{
			struct _nbNetPDLElementVariable *VariableElement;

				VariableElement= (struct _nbNetPDLElementVariable*) ExecuteElement;

				RetVal= NetPDLVariables->CreateVariable(VariableElement);

				if (RetVal == nbFAILURE)
					return nbFAILURE;

			}; break;

			case nbNETPDL_IDEL_LOOKUPTABLE:
			{
			struct _nbNetPDLElementLookupTable *LookupTableElement;

				LookupTableElement= (struct _nbNetPDLElementLookupTable*) ExecuteElement;

				RetVal= NetPDLVariables->CreateTable(LookupTableElement);

				if (RetVal == nbFAILURE)
					return nbFAILURE;

			}; break;

			case nbNETPDL_IDEL_ALIAS:
			{
			}; break;

			default:
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "The <init> section of NetPDL description contains an element which has not been recognized.");
				return nbFAILURE;

			}; break;
		}

		ExecuteElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, ExecuteElement->NextSibling);
	}

	return nbSUCCESS;
}


int InitVariablesInExpression(struct _nbNetPDLExprBase* Expression, CNetPDLVariables *NetPDLVariables, char *ErrBuf, int ErrBufSize)
{
int RetVal;

	if (Expression == NULL)
		return nbSUCCESS;

	switch (Expression->Type)
	{
		case nbNETPDL_ID_EXPR_OPERAND_EXPR:
		{
			if (InitVariablesInExpression(((struct _nbNetPDLExpression*) Expression)->Operand1, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;

			if (InitVariablesInExpression(((struct _nbNetPDLExpression*) Expression)->Operand2, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;

		}; break;

		case nbNETPDL_ID_EXPR_OPERAND_VARIABLE:
		{
		struct _nbNetPDLExprVariable* ExpressionVariable;
		int VariableID;

			ExpressionVariable= (struct _nbNetPDLExprVariable* ) Expression;

			RetVal= NetPDLVariables->GetVariableID(ExpressionVariable->Name, &VariableID);

			if (RetVal == nbFAILURE)
				return nbFAILURE;

			// Here code is really dirty, since we're assigning an integer to a void*
			ExpressionVariable->CustomData= (void*) VariableID;

			if (InitVariablesInExpression(ExpressionVariable->OffsetSize, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;

			if (InitVariablesInExpression(ExpressionVariable->OffsetStartAt, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;
		}; break;

		case nbNETPDL_ID_EXPR_OPERAND_LOOKUPTABLE:
		{
		struct _nbNetPDLExprLookupTable* ExpressionLookupTable;
		int TableID, DataID;

			ExpressionLookupTable= (struct _nbNetPDLExprLookupTable* ) Expression;

			RetVal= NetPDLVariables->GetTableID(ExpressionLookupTable->TableName, ExpressionLookupTable->FieldName, &TableID, &DataID);

			if (RetVal == nbFAILURE)
				return nbFAILURE;

			// Here code is really dirty, since we're assigning an integer to a void*
			ExpressionLookupTable->TableCustomData= (void*) TableID;
			ExpressionLookupTable->FieldCustomData= (void*) DataID;

			if (InitVariablesInExpression(ExpressionLookupTable->OffsetSize, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;

			if (InitVariablesInExpression(ExpressionLookupTable->OffsetStartAt, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;
		}; break;

		case nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD:
		case nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD_THIS:
		{
		struct _nbNetPDLExprFieldRef* ExpressionFieldRef;

			ExpressionFieldRef= (struct _nbNetPDLExprFieldRef* ) Expression;

			if (InitVariablesInExpression(ExpressionFieldRef->OffsetSize, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;

			if (InitVariablesInExpression(ExpressionFieldRef->OffsetStartAt, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;
		}; break;

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_ISASN1TYPE:
		{
		struct _nbNetPDLExprFunctionIsASN1Type* ExpressionIsASN1Type;

			ExpressionIsASN1Type= (struct _nbNetPDLExprFunctionIsASN1Type* ) Expression;

			if (InitVariablesInExpression(ExpressionIsASN1Type->StringExpression, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;

			if (InitVariablesInExpression(ExpressionIsASN1Type->ClassNumber, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;

			if (InitVariablesInExpression(ExpressionIsASN1Type->TagNumber, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;
		}; break;

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_HASSTRING:
		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_EXTRACTSTRING:
		{
		struct _nbNetPDLExprFunctionRegExp* ExpressionRegEx;

			ExpressionRegEx= (struct _nbNetPDLExprFunctionRegExp* ) Expression;

			if (InitVariablesInExpression(ExpressionRegEx->SearchBuffer, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;
		}; break;

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_BUF2INT:
		{
		struct _nbNetPDLExprFunctionBuf2Int* ExpressionBuf2Int;

			ExpressionBuf2Int= (struct _nbNetPDLExprFunctionBuf2Int* ) Expression;

			if (InitVariablesInExpression(ExpressionBuf2Int->StringExpression, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;
		}; break;

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_INT2BUF:
		{
		struct _nbNetPDLExprFunctionInt2Buf* ExpressionInt2Buf;

			ExpressionInt2Buf= (struct _nbNetPDLExprFunctionInt2Buf* ) Expression;

			if (InitVariablesInExpression(ExpressionInt2Buf->NumericExpression, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;
		}; break;

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_ASCII2INT:
		{
		struct _nbNetPDLExprFunctionAscii2Int* ExpressionAscii2Int;

			ExpressionAscii2Int= (struct _nbNetPDLExprFunctionAscii2Int* ) Expression;

			if (InitVariablesInExpression(ExpressionAscii2Int->AsciiStringExpression, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;
		}; break;

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHANGEBYTEORDER:
		{
		struct _nbNetPDLExprFunctionChangeByteOrder* ExpressionChangeByteOrder;

			ExpressionChangeByteOrder= (struct _nbNetPDLExprFunctionChangeByteOrder* ) Expression;

			if (InitVariablesInExpression(ExpressionChangeByteOrder->OriginalStringExpression, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
				return nbFAILURE;
		}; break;

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHECKLOOKUPTABLE:
		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_UPDATELOOKUPTABLE:
		{
		struct _nbNetPDLExprFunctionCheckUpdateLookupTable* ExpressionCheckLookupTable;
		struct _nbParamsLinkedList* ParameterItem;
		int TableID;

			ExpressionCheckLookupTable= (struct _nbNetPDLExprFunctionCheckUpdateLookupTable* ) Expression;

			RetVal= NetPDLVariables->GetTableID(ExpressionCheckLookupTable->TableName, NULL, &TableID, NULL);

			if (RetVal == nbFAILURE)
				return nbFAILURE;

			// Here code is really dirty, since we're assigning an integer to a void*
			ExpressionCheckLookupTable->LookupTableCustomData= (void*) TableID;

			ParameterItem= ExpressionCheckLookupTable->ParameterList;

			while (ParameterItem)
			{
				if (InitVariablesInExpression(ParameterItem->Expression, NetPDLVariables, ErrBuf, ErrBufSize) != nbSUCCESS)
					return nbFAILURE;

				ParameterItem= ParameterItem->NextParameter;
			}
		}; break;

		default:
			break;
	}

	return nbSUCCESS;
}

