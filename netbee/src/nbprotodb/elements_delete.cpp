/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <pcre.h>
#include "elements_delete.h"
#include "protodb_globals.h"
#include "expressions.h"
#include "../nbee/globals/globals.h"
#include "../nbee/globals/debug.h"



void DeleteElementGeneric(struct _nbNetPDLElementBase *NetPDLElementBase)
{
	free(NetPDLElementBase);
}


void DeleteElementProto(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementProto *NetPDLElement= (struct _nbNetPDLElementProto *) NetPDLElementBase;

	FREE_PTR(NetPDLElement->Name);
	FREE_PTR(NetPDLElement->LongName);
	FREE_PTR(NetPDLElement->ShowSumTemplateName);

	free(NetPDLElement);
}


void DeleteElementExecuteX(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementExecuteX *NetPDLElement= (struct _nbNetPDLElementExecuteX *) NetPDLElementBase;

	if (NetPDLElement->WhenExprTree)
	{
		FreeExpression(NetPDLElement->WhenExprTree);
		DELETE_PTR(NetPDLElement->WhenExprString);
	}

	free(NetPDLElement);
}


void DeleteElementKeyData(struct _nbNetPDLElementBase *NetPDLElementBase)
{
	struct _nbNetPDLElementKeyData *NetPDLElement= (struct _nbNetPDLElementKeyData *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->Name);

	free(NetPDLElement);
}


void DeleteElementVariable(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementVariable *NetPDLElement= (struct _nbNetPDLElementVariable *) NetPDLElementBase;

	FREE_PTR(NetPDLElement->Name);
	DELETE_PTR(NetPDLElement->InitValueString);

	free(NetPDLElement);
}


void DeleteElementLookupTable(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementLookupTable *NetPDLElement= (struct _nbNetPDLElementLookupTable *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->Name);

	free(NetPDLElement);
}


void DeleteElementAssignVariable(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementAssignVariable *NetPDLElement= (struct _nbNetPDLElementAssignVariable *) NetPDLElementBase;

	FREE_PTR(NetPDLElement->VariableName);

	FreeExpression(NetPDLElement->ExprTree);
	FREE_PTR(NetPDLElement->ExprString);

	free(NetPDLElement);
}


void DeleteElementAssignLookupTable(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementAssignLookupTable *NetPDLElement= (struct _nbNetPDLElementAssignLookupTable *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->TableName);
	DELETE_PTR(NetPDLElement->FieldName);

	FreeExpression(NetPDLElement->ExprTree);
	DELETE_PTR(NetPDLElement->ExprString);

	free(NetPDLElement);
}


void DeleteElementUpdateLookupTable(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementUpdateLookupTable *NetPDLElement= (struct _nbNetPDLElementUpdateLookupTable *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->TableName);

	free(NetPDLElement);
}


void DeleteElementLookupKeyData(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementLookupKeyData *NetPDLElement= (struct _nbNetPDLElementLookupKeyData *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->Mask);

	FreeExpression(NetPDLElement->ExprTree);
	DELETE_PTR(NetPDLElement->ExprString);

	free(NetPDLElement);
}


void DeleteElementAlias(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementAlias *NetPDLElement= (struct _nbNetPDLElementAlias *) NetPDLElementBase;

	FREE_PTR(NetPDLElement->Name);
	FREE_PTR(NetPDLElement->ReplaceWith);

	free(NetPDLElement);
}


void DeleteElementShowTemplate(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementShowTemplate *NetPDLElement= (struct _nbNetPDLElementShowTemplate *) NetPDLElementBase;

	FREE_PTR(NetPDLElement->Name);
	FREE_PTR(NetPDLElement->PluginName);
 	FREE_PTR(NetPDLElement->Separator);

	free(NetPDLElement);
}



void DeleteElementShowSumTemplate(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementShowSumTemplate *NetPDLElement= (struct _nbNetPDLElementShowSumTemplate *) NetPDLElementBase;

	FREE_PTR(NetPDLElement->Name);

	free(NetPDLElement);
}


void DeleteElementSumSection(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementShowSumStructure *NetPDLElement= (struct _nbNetPDLElementShowSumStructure *) NetPDLElementBase;

	FREE_PTR(NetPDLElement->Name);
	FREE_PTR(NetPDLElement->LongName);

	free(NetPDLElement);
}


void DeleteElementAdt(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementAdt *NetPDLElement= (struct _nbNetPDLElementAdt *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->ADTName);
	DeleteElementField((struct _nbNetPDLElementBase *) NetPDLElement->ADTFieldInfo);

	free(NetPDLElement);
}


void DeleteElementIf(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementIf *NetPDLElement= (struct _nbNetPDLElementIf *) NetPDLElementBase;

	FreeExpression(NetPDLElement->ExprTree);
	FREE_PTR(NetPDLElement->ExprString);

	free(NetPDLElement);
}


void DeleteElementCase(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementCase *NetPDLElement= (struct _nbNetPDLElementCase *) NetPDLElementBase;

	FREE_PTR(NetPDLElement->ValueString);
	FREE_PTR(NetPDLElement->ShowString);

	free(NetPDLElement);
}


void DeleteElementDefault(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementCase *NetPDLElement= (struct _nbNetPDLElementCase *) NetPDLElementBase;

	FREE_PTR(NetPDLElement->ShowString);

	free(NetPDLElement);
}



void DeleteElementSwitch(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementSwitch *NetPDLElement= (struct _nbNetPDLElementSwitch *) NetPDLElementBase;

	FreeExpression(NetPDLElement->ExprTree);
	FREE_PTR(NetPDLElement->ExprString);

	free(NetPDLElement);
}


void DeleteElementLoop(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementLoop *NetPDLElement= (struct _nbNetPDLElementLoop *) NetPDLElementBase;

	FreeExpression(NetPDLElement->ExprTree);
	DELETE_PTR(NetPDLElement->ExprString);

	free(NetPDLElement);
}


void DeleteElementLoopCtrl(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementLoopCtrl *NetPDLElement= (struct _nbNetPDLElementLoopCtrl *) NetPDLElementBase;

	free(NetPDLElement);
}


void DeleteElementIncludeBlk(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementIncludeBlk *NetPDLElement= (struct _nbNetPDLElementIncludeBlk *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->IncludedBlockName);

	free(NetPDLElement);
}

void DeleteElementBlock(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementBlock *NetPDLElement= (struct _nbNetPDLElementBlock *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->Name);
	DELETE_PTR(NetPDLElement->LongName);
	DELETE_PTR(NetPDLElement->ShowSumTemplateName);

	free(NetPDLElement);
}


void DeleteElementSet(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementSet *NetPDLElement= (struct _nbNetPDLElementSet *) NetPDLElementBase;

	DeleteElementField((struct _nbNetPDLElementBase *)NetPDLElement->FieldToDecode);

	free(NetPDLElement);
}


void DeleteElementExitWhen(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementExitWhen *NetPDLElement= (struct _nbNetPDLElementExitWhen *) NetPDLElementBase;

	FreeExpression(NetPDLElement->ExitExprTree);
	DELETE_PTR(NetPDLElement->ExitExprString);

	free(NetPDLElement);
}


void DeleteElementChoice(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementChoice *NetPDLElement= (struct _nbNetPDLElementChoice *) NetPDLElementBase;
//struct _nbNetPDLElementFieldBase *NetPDLFieldElement= NetPDLElement->FieldToDecode;

	DeleteElementField((struct _nbNetPDLElementBase *)NetPDLElement->FieldToDecode);

	free(NetPDLElement);
}


void DeleteElementField(struct _nbNetPDLElementBase *NetPDLElementBase)
{
	struct _nbNetPDLElementFieldBase *NetPDLElement= (struct _nbNetPDLElementFieldBase *) NetPDLElementBase;

	// Let's check the field type
	switch(NetPDLElement->FieldType)
	{
	case nbNETPDL_ID_FIELD_FIXED:
		DeleteElementFieldFixed(NetPDLElementBase); break;
	case nbNETPDL_ID_FIELD_BIT:
		DeleteElementFieldBit(NetPDLElementBase); break;
	case nbNETPDL_ID_FIELD_VARIABLE:
		DeleteElementFieldVariable(NetPDLElementBase); break;
	case nbNETPDL_ID_FIELD_TOKENENDED:
		DeleteElementFieldTokenEnded(NetPDLElementBase); break;
	case nbNETPDL_ID_FIELD_TOKENWRAPPED:
		DeleteElementFieldTokenWrapped(NetPDLElementBase); break;
	case nbNETPDL_ID_FIELD_LINE:
		DeleteElementFieldLine(NetPDLElementBase); break;
	case nbNETPDL_ID_FIELD_PATTERN:
		DeleteElementFieldPattern(NetPDLElementBase); break;
	case nbNETPDL_ID_FIELD_EATALL:
		DeleteElementFieldEatall(NetPDLElementBase); break;
	case nbNETPDL_ID_FIELD_PADDING:
		DeleteElementFieldPadding(NetPDLElementBase); break;
	case nbNETPDL_ID_FIELD_PLUGIN:
		DeleteElementFieldPlugin(NetPDLElementBase); break;

	case nbNETPDL_ID_CFIELD_TLV:
		DeleteElementCfieldTLV(NetPDLElementBase); break;
	case nbNETPDL_ID_CFIELD_DELIMITED:
		DeleteElementCfieldDelimited(NetPDLElementBase); break;
	case nbNETPDL_ID_CFIELD_LINE:
		DeleteElementCfieldLine(NetPDLElementBase); break;
	case nbNETPDL_ID_CFIELD_HDRLINE:
		DeleteElementCfieldHdrline(NetPDLElementBase); break;
	case nbNETPDL_ID_CFIELD_DYNAMIC:
		DeleteElementCfieldDynamic(NetPDLElementBase); break;
	case nbNETPDL_ID_CFIELD_ASN1:
		DeleteElementCfieldASN1(NetPDLElementBase); break;
	case nbNETPDL_ID_CFIELD_XML:
		DeleteElementCfieldXML(NetPDLElementBase); break;

	case nbNETPDL_ID_SUBFIELD_SIMPLE:
	case nbNETPDL_ID_SUBFIELD_COMPLEX:
		DeleteElementSubfield(NetPDLElementBase); break;

	case nbNETPDL_ID_FIELDMATCH:
		DeleteElementFieldmatch(NetPDLElementBase); break;

	default:
		// Here we miss several types of elements which are not cleared at all
		break;
	}

	FREE_PTR(NetPDLElement->Name);
	FREE_PTR(NetPDLElement->LongName);
	FREE_PTR(NetPDLElement->ShowTemplateName);

	free(NetPDLElement);
}


void DeleteElementFieldFixed(struct _nbNetPDLElementBase *NetPDLElementBase)
{
// Nothing to delete, here, since all data of a fixed element is statically allocated and
// hence will be cleared up automatically when we delete the general structure
//struct _nbNetPDLElementFieldFixed *NetPDLElement= (struct _nbNetPDLElementFieldFixed *) NetPDLElementBase;
}


void DeleteElementFieldVariable(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementFieldVariable *NetPDLElement= (struct _nbNetPDLElementFieldVariable *) NetPDLElementBase;

	FreeExpression(NetPDLElement->ExprTree);
	FREE_PTR(NetPDLElement->ExprString);
}


void DeleteElementFieldTokenEnded(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementFieldTokenEnded *NetPDLElement= (struct _nbNetPDLElementFieldTokenEnded *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->EndTokenString);

	DELETE_PTR(NetPDLElement->EndRegularExpression);

	if (NetPDLElement->EndPCRECompiledRegExp)
		pcre_free(NetPDLElement->EndPCRECompiledRegExp);

	DELETE_PTR(NetPDLElement->EndOffsetExprString);
	if (NetPDLElement->EndOffsetExprTree)
		FreeExpression(NetPDLElement->EndOffsetExprTree);

	DELETE_PTR(NetPDLElement->EndDiscardExprString);
	if (NetPDLElement->EndDiscardExprTree)
		FreeExpression(NetPDLElement->EndDiscardExprTree);
}


void DeleteElementFieldTokenWrapped(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementFieldTokenWrapped *NetPDLElement= (struct _nbNetPDLElementFieldTokenWrapped *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->BeginTokenString);
	DELETE_PTR(NetPDLElement->EndTokenString);

 	DELETE_PTR(NetPDLElement->BeginRegularExpression);
	if (NetPDLElement->BeginPCRECompiledRegExp)
		pcre_free(NetPDLElement->BeginPCRECompiledRegExp);

 	DELETE_PTR(NetPDLElement->EndRegularExpression);
	if (NetPDLElement->EndPCRECompiledRegExp)
		pcre_free(NetPDLElement->EndPCRECompiledRegExp);

	DELETE_PTR(NetPDLElement->BeginOffsetExprString);
	if (NetPDLElement->BeginOffsetExprTree)
		FreeExpression(NetPDLElement->BeginOffsetExprTree);

	DELETE_PTR(NetPDLElement->EndOffsetExprString);
	if (NetPDLElement->EndOffsetExprTree)
		FreeExpression(NetPDLElement->EndOffsetExprTree);

	DELETE_PTR(NetPDLElement->EndDiscardExprString);
	if (NetPDLElement->EndDiscardExprTree)
		FreeExpression(NetPDLElement->EndDiscardExprTree);
}


void DeleteElementFieldLine(struct _nbNetPDLElementBase *NetPDLElementBase)
{
}


void DeleteElementFieldPattern(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementFieldPattern *NetPDLElement= (struct _nbNetPDLElementFieldPattern *) NetPDLElementBase;

 	DELETE_PTR(NetPDLElement->PatternRegularExpression);
	if (NetPDLElement->PatternPCRECompiledRegExp)
		pcre_free(NetPDLElement->PatternPCRECompiledRegExp);
}


void DeleteElementFieldEatall(struct _nbNetPDLElementBase *NetPDLElementBase)
{
}


void DeleteElementFieldPadding(struct _nbNetPDLElementBase *NetPDLElementBase)
{
}


void DeleteElementFieldPlugin(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementFieldPlugin *NetPDLElement= (struct _nbNetPDLElementFieldPlugin *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->PluginName);
}


void DeleteElementFieldBit(struct _nbNetPDLElementBase *NetPDLElementBase)
{
	struct _nbNetPDLElementFieldBit *NetPDLElement= (struct _nbNetPDLElementFieldBit *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->BitMaskString);
}


void DeleteElementCfieldTLV(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementCfieldTLV *NetPDLElement= (struct _nbNetPDLElementCfieldTLV *) NetPDLElementBase;

	FreeExpression(NetPDLElement->ValueExprTree);
	DELETE_PTR(NetPDLElement->ValueExprString);
}


void DeleteElementCfieldDelimited(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementCfieldDelimited *NetPDLElement= (struct _nbNetPDLElementCfieldDelimited *) NetPDLElementBase;

 	DELETE_PTR(NetPDLElement->BeginRegularExpression);
	if (NetPDLElement->BeginPCRECompiledRegExp)
		pcre_free(NetPDLElement->BeginPCRECompiledRegExp);

 	DELETE_PTR(NetPDLElement->EndRegularExpression);
	if (NetPDLElement->EndPCRECompiledRegExp)
		pcre_free(NetPDLElement->EndPCRECompiledRegExp);
}


void DeleteElementCfieldLine(struct _nbNetPDLElementBase *NetPDLElementBase)
{
}


void DeleteElementCfieldHdrline(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementCfieldHdrline *NetPDLElement= (struct _nbNetPDLElementCfieldHdrline *) NetPDLElementBase;

 	DELETE_PTR(NetPDLElement->SeparatorRegularExpression);
	if (NetPDLElement->SeparatorPCRECompiledRegExp)
		pcre_free(NetPDLElement->SeparatorPCRECompiledRegExp);
}


void DeleteElementCfieldDynamic(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementCfieldDynamic *NetPDLElement= (struct _nbNetPDLElementCfieldDynamic *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->NamesList);

	DELETE_PTR(NetPDLElement->PatternRegularExpression);
	if (NetPDLElement->PatternPCRECompiledRegExp)
		pcre_free(NetPDLElement->PatternPCRECompiledRegExp);
}


void DeleteElementCfieldASN1(struct _nbNetPDLElementBase *NetPDLElementBase)
{
}


void DeleteElementCfieldXML(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementCfieldXML *NetPDLElement= (struct _nbNetPDLElementCfieldXML *) NetPDLElementBase;

	FreeExpression(NetPDLElement->SizeExprTree);
	DELETE_PTR(NetPDLElement->SizeExprString);
}


void DeleteElementFieldmatch(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementFieldmatch *NetPDLElement= (struct _nbNetPDLElementFieldmatch *) NetPDLElementBase;

	FreeExpression(NetPDLElement->MatchExprTree);
	DELETE_PTR(NetPDLElement->MatchExprString);
}


void DeleteElementSubfield(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementSubfield *NetPDLElement= (struct _nbNetPDLElementSubfield *) NetPDLElementBase;

	if (NetPDLElement->ComplexSubfieldInfo)
	{
	struct _nbNetPDLElementFieldBase *NetPDLFieldElement= NetPDLElement->ComplexSubfieldInfo;

		switch(NetPDLElement->FieldType)
		{
		case nbNETPDL_ID_CFIELD_TLV:
			DeleteElementCfieldTLV((struct _nbNetPDLElementBase *) NetPDLFieldElement); break;
		case nbNETPDL_ID_CFIELD_DELIMITED:
			DeleteElementCfieldDelimited((struct _nbNetPDLElementBase *) NetPDLFieldElement); break;
		case nbNETPDL_ID_CFIELD_LINE:
			DeleteElementCfieldLine((struct _nbNetPDLElementBase *) NetPDLFieldElement); break;
		case nbNETPDL_ID_CFIELD_HDRLINE:
			DeleteElementCfieldHdrline((struct _nbNetPDLElementBase *) NetPDLFieldElement); break;
		case nbNETPDL_ID_CFIELD_DYNAMIC:
			DeleteElementCfieldDynamic((struct _nbNetPDLElementBase *) NetPDLFieldElement); break;
		case nbNETPDL_ID_CFIELD_ASN1:
			DeleteElementCfieldASN1((struct _nbNetPDLElementBase *) NetPDLFieldElement); break;
		case nbNETPDL_ID_CFIELD_XML:
			DeleteElementCfieldXML((struct _nbNetPDLElementBase *) NetPDLFieldElement); break;
		default:
			// Several nbNETPDL_ID_* missing; nothing to delete, here?
			break;
		}
	}
}


void DeleteElementMap(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementMapBase *NetPDLElement= (struct _nbNetPDLElementMapBase *) NetPDLElementBase;

	// Let's check the field type
	switch(NetPDLElement->FieldType)
	{
	case nbNETPDL_ID_FIELD_MAP_XML_PI:
		DeleteElementMapXMLPI(NetPDLElementBase); break;
	//case nbNETPDL_ID_FIELD_MAP_XML_PI_ATTR:
	case nbNETPDL_ID_FIELD_MAP_XML_DOCTYPE:
		DeleteElementMapXMLDoctype(NetPDLElementBase); break;
	case nbNETPDL_ID_FIELD_MAP_XML_ELEMENT:
		DeleteElementMapXMLElement(NetPDLElementBase); break;
	//case nbNETPDL_ID_FIELD_MAP_XML_ELEMENT_ATTR:
	default:
		// Several nbNETPDL_ID_* missing; nothing to delete, here?
		break;
	}

	DELETE_PTR(NetPDLElement->Name);
	DELETE_PTR(NetPDLElement->LongName);
	DELETE_PTR(NetPDLElement->ShowTemplateName);

	DELETE_PTR(NetPDLElement->RefName);

	free(NetPDLElement);
}


void DeleteElementMapXMLPI(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementMapXMLPI *NetPDLElement= (struct _nbNetPDLElementMapXMLPI *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->XMLPIRegularExpression);
	if (NetPDLElement->XMLPIPCRECompiledRegExp)
		pcre_free(NetPDLElement->XMLPIPCRECompiledRegExp);
}

void DeleteElementMapXMLDoctype(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementMapXMLDoctype *NetPDLElement= (struct _nbNetPDLElementMapXMLDoctype *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->XMLDoctypeRegularExpression);
	if (NetPDLElement->XMLDoctypePCRECompiledRegExp)
		pcre_free(NetPDLElement->XMLDoctypePCRECompiledRegExp);
}


void DeleteElementMapXMLElement(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementMapXMLElement *NetPDLElement= (struct _nbNetPDLElementMapXMLElement *) NetPDLElementBase;

	DELETE_PTR(NetPDLElement->XMLElementRegularExpression);
	if (NetPDLElement->XMLElementPCRECompiledRegExp)
		pcre_free(NetPDLElement->XMLElementPCRECompiledRegExp);

	DELETE_PTR(NetPDLElement->NamespaceString);
	DELETE_PTR(NetPDLElement->HierarcyString);
}


void DeleteElementNextProto(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementNextProto *NetPDLElement= (struct _nbNetPDLElementNextProto *) NetPDLElementBase;

	FreeExpression(NetPDLElement->ExprTree);
	FREE_PTR(NetPDLElement->ExprString);

	free(NetPDLElement);
}


void DeleteElementShowCodeProtoField(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementProtoField *NetPDLElement= (struct _nbNetPDLElementProtoField *) NetPDLElementBase;

	FREE_PTR(NetPDLElement->FieldName);
	free(NetPDLElement);
}


void DeleteElementShowCodeProtoHdr(struct _nbNetPDLElementBase *NetPDLElementBase)
{
	free(NetPDLElementBase);
}


void DeleteElementShowCodePacketHdr(struct _nbNetPDLElementBase *NetPDLElementBase)
{
	free(NetPDLElementBase);
}


void DeleteElementShowCodeText(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementText *NetPDLElement= (struct _nbNetPDLElementText *) NetPDLElementBase;

	FREE_PTR(NetPDLElement->Value);
	FREE_PTR(NetPDLElement->ExprString);
	if (NetPDLElement->ExprTree)
		FreeExpression(NetPDLElement->ExprTree);
	free(NetPDLElement);
}


void DeleteElementShowCodeSection(struct _nbNetPDLElementBase *NetPDLElementBase)
{
struct _nbNetPDLElementSection *NetPDLElement= (struct _nbNetPDLElementSection *) NetPDLElementBase;

	FREE_PTR(NetPDLElement->SectionName);
	free(NetPDLElement);
}

