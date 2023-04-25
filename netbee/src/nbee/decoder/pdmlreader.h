/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include "pxmlreader.h"
#include "../utils/asciibuffer.h"
#include "../globals/utils.h"


// Indicate using Xerces-C++ namespace in general
// This is required starting from Xerces 2.2.0
XERCES_CPP_NAMESPACE_USE



//! This class implements the nbReader abstract class.
class CPDMLReader : public nbPDMLReader, public CPxMLReader
{
public:
	CPDMLReader();
	virtual ~CPDMLReader();

	int Initialize();
	int InitializeSource(char *FileName);

	int GetCurrentPacket(struct _nbPDMLPacket **PDMLPacket);
	int GetPacket(unsigned long PacketNumber, struct _nbPDMLPacket **PDMLPacket);

	int GetCurrentPacketXML(char* &PacketPtr, unsigned int &PacketLength);
	int GetPacketXML(unsigned long PacketNumber, char* &PacketPtr, unsigned int &PacketLength);

	int GetPDMLField(char *ProtoName, char *FieldName, struct _nbPDMLField *FirstField, struct _nbPDMLField **ExtractedField);
	int GetPDMLField(unsigned long PacketNumber, char *ProtoName, char *FieldName, struct _nbPDMLField *FirstField, struct _nbPDMLField **ExtractedField);

	// Documented in the base class
	int RemovePacket(unsigned long PacketNumber) { return CPxMLReader::RemovePacket(PacketNumber); }

	// Documented in the base class
	int SaveDocumentAs(const char *Filename) { return CPxMLReader::SaveDocumentAs(Filename); }

	// Documented in the base class
	char *GetLastError() { return m_errbuf; };


	// Static members, used from several places within the code
	static int UpdateProtoList(unsigned long *CurrentNumProto, _nbPDMLProto ***CurrentProtoList, char *ErrBuf, int ErrBufSize);
	static int UpdateFieldsList(unsigned long *CurrentNumFields, _nbPDMLField ***CurrentProtoList, char *ErrBuf, int ErrBufSize);

	static int AppendItemString(DOMNode *SourceNode, const char *TagToLookFor, char **AppendAt, CAsciiBuffer *TmpBuffer, char *ErrBuf, int ErrBufSize);
	static int AppendItemString(const char *SourceString, char **AppendAt, CAsciiBuffer *TmpBuffer, char *ErrBuf, int ErrBufSize);
	static int AppendItemLong(DOMNode *SourceNode, const char *TagToLookFor, unsigned long *AppendAt, char *ErrBuf, int ErrBufSize);

private:

	int FormatItem(DOMNodeList *ItemList, char *Buffer, int BufSize);
	int FormatProtoItem(DOMDocument *PDMLDocument);
	int FormatFieldNodes(DOMNode *CurrentField, struct _nbPDMLProto *ParentProto, struct _nbPDMLField *ParentField);
	int FormatPacketSummary(DOMDocument *PDMLDocument);
	int FormatDumpItem(DOMDocument *PDMLDocument, CAsciiBuffer *TmpBuffer, char *ErrBuf, int ErrBufSize);

	DOMDocument* ParseMemBuf(char *Buffer, int BytesToParse);

	int GetPDMLFieldInternal(_nbPDMLPacket *PDMLPacket, char *ProtoName, char *FieldName, struct _nbPDMLField *FirstField, struct _nbPDMLField **ExtractedField);
	struct _nbPDMLField *ScanForFields(char *FieldName, struct _nbPDMLField *FirstField, bool AllowToGoUp);

	//! Protocol and fields must be formatted through several char string; this variable is useful to avoid
	//! to allocate a char * for each variable; we have a "shared memory pool" (this buffer), and who needs
	//! it, can get space.
	CAsciiBuffer m_asciiBuffer;

	//! Pointer to the array that will keep the protocol structures
	struct _nbPDMLProto **m_protoList;
	//! Current size of the array will keep the protocol structures
	unsigned long m_maxNumProto;

	//! Pointer to the array that will keep the field structures
	struct _nbPDMLField **m_fieldsList;
	//! Current size of the array will keep the field structures
	unsigned long m_maxNumFields;
	//! Keeps how many elements we have currently in the field structures
	unsigned long m_currNumFields;

	//! It keeps the packet summary according to the definition of structure _nbPDMLPacket
	struct _nbPDMLPacket m_packetSummary;
};

