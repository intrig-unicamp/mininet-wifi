/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



// Allow including this file only once
#pragma once


#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>


// Indicate using Xerces-C++ namespace in general
// This is required starting from Xerces 2.2.0
XERCES_CPP_NAMESPACE_USE


#ifdef __cplusplus
extern "C" {
#endif


/*!
	\brief Prototype for functions that allocates a NetPDL element.

	This function accepts the following parameters:
	- list of attributes related to a given element
	- a buffer in which it can print an error string (if something bad occurs)
	- the size of the previous buffer

	This function allocates the element, and it and returns 
	a pointer to it (casted to a _nbNetPDLElementBase * for compatibility).
	In case of failure, it returns NULL and it sets the proper error.
*/
typedef struct _nbNetPDLElementBase * (*nbNetPDLElementCreateHandler) (const Attributes& Attributes, char *ErrBuf, int ErrBufSize);


struct _nbNetPDLElementBase *CreateElementGeneric(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementNetPDL(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementProto(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementProtoCheck(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementExecuteX(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementVariable(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementLookupTable(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementKeyData(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementAlias(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);

struct _nbNetPDLElementBase *CreateElementAssignVariable(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementAssignLookupTable(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementUpdateLookupTable(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementLookupKeyData(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);

struct _nbNetPDLElementBase *CreateElementShowTemplate(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementShowSumTemplate(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementSumSection(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementIf(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementCase(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementDefault(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementSwitch(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementLoop(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementLoopCtrl(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementIncludeBlk(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementBlock(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);

struct _nbNetPDLElementBase *CreateElementAdt(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);

struct _nbNetPDLElementBase *CreateElementNextProto(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);

struct _nbNetPDLElementBase *CreateElementShowCodeProtoField(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementShowCodeProtoHdr(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementShowCodeText(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementShowCodeSection(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementShowCodePacketHdr(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);

struct _nbNetPDLElementBase *CreateElementField(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementFieldFixed(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementFieldBit(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementFieldVariable(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementFieldTokenEnded(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementFieldTokenWrapped(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementFieldLine(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementFieldPattern(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementFieldEatall(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementFieldPadding(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementFieldPlugin(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementCfieldTLV(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementCfieldDelimited(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementCfieldHdrline(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementCfieldLine(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementCfieldDynamic(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementCfieldASN1(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementCfieldXML(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);

struct _nbNetPDLElementBase *CreateElementSubfield(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);

struct _nbNetPDLElementBase *CreateElementMap(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementMapXMLPI(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementMapXMLDoctype(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementMapXMLElement(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);

struct _nbNetPDLElementBase *CreateElementFieldmatch(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);

struct _nbNetPDLElementBase *CreateElementAdtfield(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementReplace(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);

struct _nbNetPDLElementBase *CreateElementSet(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementExitWhen(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementDefaultItem(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementBase *CreateElementChoice(const Attributes& Attributes, char *ErrBuf, int ErrBufSize);

int GetNumber(char *NumberBuffer, int BufferLength);

char* GetXMLAttribute(const Attributes& Attributes, const char* AttributeName);
int AllocateCallHandle(char* CallHandleAttribute, struct _nbCallHandlerInfo **CallHandlerInfo, char* ErrBuf, int ErrBufSize);



#ifdef __cplusplus
}
#endif

