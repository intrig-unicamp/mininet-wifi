/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



// Allow including this file only once
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/*!
	\brief Prototype for functions that deletes a NetPDL element in memory.

	This function accepts the following parameters:
	- pointer to the element that has to be deleted

	This function deletes the element and it does not report any error.
*/
typedef void (*nbNetPDLElementDeleteHandler) (struct _nbNetPDLElementBase *NetPDLElementBase);


void DeleteElementGeneric(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementProto(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementProtoCheck(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementExecuteX(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementVariable(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementLookupTable(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementKeyData(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementAlias(struct _nbNetPDLElementBase *NetPDLElementBase);

void DeleteElementAssignVariable(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementAssignLookupTable(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementUpdateLookupTable(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementLookupKeyData(struct _nbNetPDLElementBase *NetPDLElementBase);

void DeleteElementShowTemplate(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementShowSumTemplate(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementSumSection(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementAdt(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementIf(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementCase(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementDefault(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementSwitch(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementLoop(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementLoopCtrl(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementIncludeBlk(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementBlock(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementSet(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementExitWhen(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementChoice(struct _nbNetPDLElementBase *NetPDLElementBase);

void DeleteElementNextProto(struct _nbNetPDLElementBase *NetPDLElementBase);

void DeleteElementShowCodeProtoField(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementShowCodeProtoHdr(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementShowCodePacketHdr(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementShowCodeText(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementShowCodeSection(struct _nbNetPDLElementBase *NetPDLElementBase);

void DeleteElementField(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementFieldFixed(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementFieldBit(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementFieldVariable(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementFieldTokenEnded(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementFieldTokenWrapped(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementFieldLine(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementFieldPattern(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementFieldEatall(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementFieldPadding(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementFieldPlugin(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementCfieldTLV(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementCfieldDelimited(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementCfieldLine(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementCfieldHdrline(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementCfieldDynamic(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementCfieldASN1(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementCfieldXML(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementSubfield(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementFieldmatch(struct _nbNetPDLElementBase *NetPDLElementBase);

void DeleteElementMap(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementMapXMLPI(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementMapXMLDoctype(struct _nbNetPDLElementBase *NetPDLElementBase);
void DeleteElementMapXMLElement(struct _nbNetPDLElementBase *NetPDLElementBase);



#ifdef __cplusplus
}
#endif

