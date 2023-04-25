/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

#include <pcap.h>

#include "netpdlvariables.h"
#include "netpdlexpression.h"
#include "showplugin/show_plugin.h"
#include "pdmlreader.h"
#include "../utils/asciibuffer.h"


//! Maximum number of elements allowed within the temp list.
#define PDML_MAX_TEMP_ELEMENTS 100



/*!
	\brief It provides the interface to create a PDML file.

	This class includes all the methods that are needed in order to create a PDML file, i.e.
	a file that keeps a detailed description of the captured packets.

	\warning The PDML document will be deallocated:
	- each time the PacketInitialize() is being called (this function will clean the previously
	existing document)
	- when this class is being deallocated, because the last element is still allocated since
	the PacketInitialize() is called only in case of a new packet to be decoded.
*/
class CPDMLMaker
{
	friend class CNetPDLDecoderUtils;
	friend class CPDMLReader;

public:
	CPDMLMaker(CNetPDLExpression *ExprHandler, CPDMLReader *PDMLReader, int NetPDLFlags, char *ErrBuf, int ErrBufSize);
	virtual ~CPDMLMaker();

	// Initialization functions
	int Initialize(CNetPDLVariables *RtVars);
	void PacketInitialize();

	// PDMLElement functions
	int PDMLElementUpdate(struct _nbPDMLField *PDMLElement, struct _nbNetPDLElementFieldBase *NetPDLField, int Len, int Offset, const unsigned char *PacketFieldPtr);
	int PDMLElementUpdateShowExtension(struct _nbPDMLField *PDMLElement, struct _nbNetPDLElementFieldBase *NetPDLField, int Len, int Size, int Offset, const unsigned char *PacketFieldPtr);
	int PDMLBlockElementUpdate(struct _nbPDMLField *PDMLElement, struct _nbNetPDLElementBlock *NetPDLBlock, int Len, int Offset);
	struct _nbPDMLField *PDMLElementInitialize(struct _nbPDMLField *PDMLParent);
	void PDMLElementDiscard(struct _nbPDMLField *PDMLElementToDelete);

	// PDMLHeader functions
	int HeaderElementUpdate(int HLen);
	struct _nbPDMLProto *HeaderElementInitialize(int Offset, int NetPDLProtoItem);


	// Utilities
	struct _nbPDMLPacket *GetPDMLPacketInfo();
	void PacketGenInfoInitialize(const struct pcap_pkthdr *PcapHeader, const unsigned char * PcapPktData, int PacketCounter);
	int GetNetPDLShowDefaultType(DOMElement *NetPDLElement);
	int PacketDecodingEnded();

	static int ScanForFieldRefAttrib(struct _nbPDMLField *PMDLField, char *ProtoName, 
		char *FieldName, int AttribCode, char **AttribValue, char *ErrBuf, int ErrBufSize);

	static int ScanForFieldRefValue(struct _nbPDMLField *PDMLField, char *ProtoName, char *FieldName, 
		unsigned int *FieldOffset, unsigned int *FieldSize, char **FieldMask, char *ErrBuf, int ErrBufSize);

	static int ScanForFieldRef(struct _nbPDMLField *PDMLField, char *ProtoName, 
		char *FieldName, struct _nbPDMLField **PDMLLocatedField, char *ErrBuf, int ErrBufSize);

	// The error message is not needed here, because it is manages by the calling function
	static char *GetPDMLFieldAttribute(int AttribCode, _nbPDMLField *PDMLField);
	static char *GetPDMLProtoAttribute(int AttribCode, _nbPDMLProto *PDMLProto);

	static int DumpPDMLPacket(struct _nbPDMLPacket *PDMLPacket, int IsVisExtRequired, int GenerateRawDump, CAsciiBuffer *AsciiBuffer, char *ErrBuf, int ErrBufSize);
	static int DumpPDMLFields(struct _nbPDMLField *PDMLField, CAsciiBuffer *AsciiBuffer, char *ErrBuf, int ErrBufSize);

	//! Structure that keeps the most important info of the packet
	struct _nbPDMLPacket m_packetSummary;

private:

	// Printing functions
	int PrintFieldDetails(struct _nbPDMLField *PDMLElement, struct _nbNetPDLElementBase *ShowDetailsElement, char **ResultString);
	int PrintMapTable(struct _nbPDMLField *PDMLElement, struct _nbNetPDLElementSwitch *MappingTableInfo);
	int PrintNativeFunction(struct _nbPDMLField *PDMLElement, int Size, const unsigned char *PacketFieldPtr, int NativeFunctionID);

	// Private utilities
	int HexDumpBinToHexDumpAsciiWithMask(const unsigned char *PacketData, int DataLen, int HexDumpIsInNetworkByteOrder, 
		int Mask, char *HexResult, int HexResultStringSize, char *BinResult);

	// These functions do not manage errors (in case the field cannot be found, they return NULL)
	static struct _nbPDMLField* FindLastField(_nbPDMLField *PDMLStartField);
	static struct _nbPDMLField* FindLastChild(_nbPDMLField *PDMLStartField);

	//! Pointer to the run-time variables managed by the NetPDL engine
	CNetPDLVariables *m_netPDLVariables;

	//! Length of the portion of packet captured (i.e. the number of bytes we have to decode)
	int m_capLen;

	//! Pointer to the previously created PDML protocol item
	_nbPDMLProto *m_previousProto;

	//! Pointer to the previously created PDML field item
	_nbPDMLField *m_previousField;

	//! Keeps trace of the current NetPDL protocol (i.e. an index into the GlobalData->ProtoInfo
	//! vector) which is generating our PDML description.
	int m_currentNetPDLProto;


	//! Pointer to the array that will keep the protocol structures
	_nbPDMLProto **m_protoList;
	//! Current size of the array will keep the protocol structures
	unsigned long m_maxNumProto;
	//! Keeps how many elements we have currently in the protocol structures
	unsigned long m_currNumProto;

	//! Pointer to the array that will keep the field structures
	_nbPDMLField **m_fieldsList;
	//! Current size of the array will keep the field structures
	unsigned long m_maxNumFields;
	//! Keeps how many elements we have currently in the field structures
	unsigned long m_currNumFields;

	//! Protocol and fields must be formatted through several char string; this variable is useful to avoid
	//! to allocate a char * for each variable; we have a "shared memory pool" (this buffer), and who needs
	//! it, can get space.
	CAsciiBuffer m_tempFieldData;

	//! Buffer that will contain the ascii dump of the packet; needed only if we want to store packets on file
	CAsciiBuffer m_packetAsciiBuffer;

	//! Pointer to the same expression handler we have into the NetPDL decoder
	CNetPDLExpression *m_exprHandler;

	//! Value that indicates if the user wants also visualization primitives (e.g. 'showvalue' attribute in the PDML fiel and the packet summary, i.e. the PSML description).
	int m_isVisExtRequired;

	//! Value that indicates if the user wants also to dump the raw packet dump in the generated PDML fragment.
	int m_generateRawDump;

	//! Value that indicates if the user wants to keep all the packets stored or just the last one.
	int m_keepAllPackets;

	//! Pointer to a PDMLReader; needed to manage PDML files (storing data and such)
	CPDMLReader *m_PDMLReader;

	//! Pointer to the buffer that will keep the error message (if any); this buffer belongs to the class that creates this one.
	char *m_errbuf;

	//! Size of the buffer that will keep the error message (if any); this buffer belongs to the class that creates this one.
	int m_errbufSize;
};

