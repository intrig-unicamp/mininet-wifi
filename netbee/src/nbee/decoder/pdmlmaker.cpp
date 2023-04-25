/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <time.h>
#include <xercesc/util/PlatformUtils.hpp>

#include <nbprotodb.h>
#include <nbprotodb_defs.h>

#include "pdmlmaker.h"
#include "netpdldecoderutils.h"
#include "showplugin/show_plugin.h"
#include "showplugin/native_showfunctions.h"

#include "../utils/netpdlutils.h"
#include "../globals/utils.h"
#include "../globals/debug.h"
#include "../globals/globals.h"

#ifndef WIN32
#include "../globals/crossplatform.h"
#endif


// This definition enables printing every decoded field on screen as soon as it gets decoded
// This is extremely useful in case of debugging, when the library hungs. In this way,
// we can know which is the last field before the one that creates the problem.
//#define DEBUG_LOUD


// Global variables
extern struct _nbNetPDLDatabase *NetPDLDatabase;
extern struct _ShowPluginList ShowPluginList[];




/*!
	\brief Standard constructor.

	It initializes the class that manages the PDML file creation.

	\param ExprHandler: pointer to a CNetPDLExpression (this class is shared among all the
	NetPDL classes in order to save memory).

	\param PDMLReader: Pointer to a PDMLReader; needed to manage PDML files (storing data and such).

	\param NetPDLFlags: flags that indicates if the user wants to create the
	tags related to the NetPDL visualization extention into the PDML file;
	if so, we have to decide if all the packets are kept in memory or only the last one is kept.

	\param ErrBuf: pointer to a user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer belongs to the class that creates this one.
	Data stored in this buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.
*/
CPDMLMaker::CPDMLMaker(CNetPDLExpression *ExprHandler, CPDMLReader *PDMLReader, int NetPDLFlags, char *ErrBuf, int ErrBufSize)
{
	m_protoList= NULL;
	m_fieldsList= NULL;

	m_maxNumProto= 20;
	m_maxNumFields= 400;

	m_isVisExtRequired= NetPDLFlags & nbDECODER_GENERATEPDML_COMPLETE;
	m_generateRawDump= NetPDLFlags & nbDECODER_GENERATEPDML_RAWDUMP;
	m_keepAllPackets= NetPDLFlags & nbDECODER_KEEPALLPDML;

	m_PDMLReader= PDMLReader;
	m_exprHandler= ExprHandler;

	// Store internally the pointer to the error buffer. This buffer belongs to the class that creates this one.
	m_errbuf= ErrBuf;
	m_errbufSize= ErrBufSize;
}


/*!
	\brief Initializes the variables contained into this class.

	\param RtVars: pointer to the class that contains run-time variables for the current instance
	of the NetPDL engine.

	\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPDMLMaker::Initialize(CNetPDLVariables *RtVars)
{
unsigned int i;

	m_netPDLVariables= RtVars;

	// Initialize the parameters needed to dump everything to file (if needed)
	if (m_keepAllPackets)
	{
		if (m_PDMLReader->InitializeParsForDump(PDML_CAPTURE, NULL) == nbFAILURE)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "%s", m_PDMLReader->GetLastError() );
			return nbFAILURE;
		}
	}

	if (m_tempFieldData.Initialize() == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "%s", m_tempFieldData.GetLastError() );
		return nbFAILURE;
	}

	if (m_packetAsciiBuffer.Initialize() == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "%s", m_packetAsciiBuffer.GetLastError() );
		return nbFAILURE;
	}

	// Allocates a new vector for containing the pointers to the protocol list
	m_protoList=  new _nbPDMLProto* [m_maxNumProto];
	if (m_protoList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough memory to allocate the protolist buffer.");
		return nbFAILURE;
	}

	// Allocate items in the protocol list
	for (i= 0; i< m_maxNumProto; i++)
	{
		m_protoList[i]= new _nbPDMLProto;
		if (m_protoList[i] == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough memory to allocate a protolist item.");
			return nbFAILURE;
		}
	}

	// Allocates a new vector for containing the pointers to the Fields list
	m_fieldsList= new _nbPDMLField* [m_maxNumFields];
	if (m_fieldsList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough memory to allocate the fieldlist buffer.");
		return nbFAILURE;
	}
	// Allocate items in the field list
	for (i= 0; i< m_maxNumFields; i++)
	{
		m_fieldsList[i]= new _nbPDMLField;
		if (m_fieldsList[i] == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough memory to allocate a fieldlist item.");
			return nbFAILURE;
		}
	}

	return nbSUCCESS;
}


//! Standard destructor
CPDMLMaker::~CPDMLMaker()
{
unsigned int i;

	if (m_protoList)
	{
		for (i= 0; i< m_maxNumProto; i++)
			delete m_protoList[i];

		delete[] m_protoList;
	}
	if (m_fieldsList)
	{
		for (i= 0; i< m_maxNumFields; i++)
			delete m_fieldsList[i];

		delete[] m_fieldsList;
	}
}



/*!
	\brief Tell this class that a new packet is going to be created.

	This method initializes class structures in order to accomodate a new packet.
	Basically, a DOMDocument for the PDML file is created when the class get instantiated;
	then, each time a new packet has to be created, this method purges all the structures
	that are referred to the previous packet (i.e. all the DOMNodes that are children of the 
	PMDLPacketElement).
*/
void CPDMLMaker::PacketInitialize()
{
	m_tempFieldData.ClearBuffer(false /* resizing not permitted */);
	m_currNumFields= 0;
	m_currNumProto= 0;
	m_previousProto= NULL;
}



/*!
	\brief It initializes the internal structures according to the new header element that has to be created.

	A PDML file is made up of packes, which are made up of protocols, which contain fields. This method
	initializes a protocol according to the given parameters.
	This method is not able to fill in all the fields related to a protocol. For instance, this method
	is usually invoked prior the header has been decoded, so we cannot know the header size. Therefore
	it sets only the node type, the attributes "name" and "longname". Other attributes are set by the
	HeaderElementUpdate() method.

	\param Offset: it keeps the offset of the current header against the beginning of the packet.

	\param NetPDLProtoItem: the index (in NetPDLDatabase) of the protocol which 
	is currently generating our PDML description.
	This is needed in order to satisfy some 'showdtl' attributes, because we have to locate the template
	used to print the current attribute.

	\return A pointer related to the newly created protocol, or NULL in case of errors.
*/
_nbPDMLProto *CPDMLMaker::HeaderElementInitialize(int Offset, int NetPDLProtoItem)
{
	// Initialize current item
	memset(m_protoList[m_currNumProto], 0, sizeof (_nbPDMLProto));

	if (CPDMLReader::AppendItemString(NetPDLDatabase->ProtoList[NetPDLProtoItem]->Name,
		&(m_protoList[m_currNumProto]->Name), &m_tempFieldData, m_errbuf, m_errbufSize) == nbFAILURE)
		return NULL;

	// If the user does not want to create the visualization extension primitives, avoid the following code
	if (m_isVisExtRequired)
	{
		if (CPDMLReader::AppendItemString(NetPDLDatabase->ProtoList[NetPDLProtoItem]->LongName,
			&(m_protoList[m_currNumProto]->LongName), &m_tempFieldData, m_errbuf, m_errbufSize) == nbFAILURE)
			return NULL;
	}

	m_protoList[m_currNumProto]->Position= Offset;

	// Save a reference to the current NetPDLHeader;
	m_currentNetPDLProto= NetPDLProtoItem;

	// Set previous field to NULL
	m_previousField= NULL;

	// initialize to a first valid field
	m_protoList[m_currNumProto]->FirstField= m_fieldsList[m_currNumFields];

	// Initialize packet summary
	m_protoList[m_currNumProto]->PacketSummary= &m_packetSummary;

#ifdef DEBUG_LOUD
	// This piece of code prints every decoded field on screen as soon as it gets decoded
	// This is extremely useful in case of debugging, when the library hungs. In this way,
	// we can know which is the last field before the one that creates the problem.
	CAsciiBuffer TempAsciiBuffer;

	TempAsciiBuffer.Initialize();

	if (TempAsciiBuffer.AppendFormatted(
		"<" PDML_PROTO " " PDML_FIELD_ATTR_NAME "=\"%s\" " 
		PDML_FIELD_ATTR_LONGNAME "=\"%s\" "
		PDML_FIELD_ATTR_POSITION "=\"%d\" "
		PDML_FIELD_ATTR_SIZE "=\"%d\">\n",
		m_protoList[m_currNumProto]->Name, m_protoList[m_currNumProto]->LongName,
		m_protoList[m_currNumProto]->Position, m_protoList[m_currNumProto]->Size) == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "%s", TempAsciiBuffer.GetLastError() );
		return NULL;
	}

	fprintf(stderr, TempAsciiBuffer.GetStartBufferPtr());
#endif

	return m_protoList[m_currNumProto];
}



/*!
	\brief Updates the header node which has previously been created by the InitializeHeaderElement().

	This method sets some remaining attributes (like the header size) and it purges all the 'temp'
	nodes which have been created only to make the decoding process simpler.

	\param HLen: length of the so created header (to be inserted into the PDML header).

	\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
*/
int CPDMLMaker::HeaderElementUpdate(int HLen)
{
	// Update the pointer to m_previousProto
	// This variable is NULL when we're decoding the first protocol
	if (m_previousProto)
	{
		m_protoList[m_currNumProto]->PreviousProto= m_previousProto;
		m_previousProto->NextProto= m_protoList[m_currNumProto];
	}
	else
	{
		// This is the first protocol. Let's update the pointer in the packet summary
		m_packetSummary.FirstProto= m_protoList[m_currNumProto];
	}

	m_previousProto= m_protoList[m_currNumProto];

	// Update the size of the current proto
	m_protoList[m_currNumProto]->Size= HLen;
	// Let's move to the next proto
	m_currNumProto++;

	if (m_currNumProto >= m_maxNumProto)
	{
		if (CPDMLReader::UpdateProtoList(&m_maxNumProto, &m_protoList, m_errbuf, m_errbufSize) == nbFAILURE)
			return nbFAILURE;
	}
	return nbSUCCESS;
}



/*!
	\brief This function must be called when the packet decoding is ended.

	This method dumps the current element on file (if the user wants to keep all the elements).

	\return nbSUCCESS if the function is successful, nbFAILURE if something goes wrong.
	The error message is stored in the 'm_errbuf' internal variable.
*/
int CPDMLMaker::PacketDecodingEnded()
{
	// Purge all the nodes that have previously been created
	if (m_keepAllPackets)
	{
		m_packetAsciiBuffer.ClearBuffer(true /* resizing permitted */);

		if (DumpPDMLPacket(&m_packetSummary, m_isVisExtRequired, m_generateRawDump, &m_packetAsciiBuffer, m_errbuf, m_errbufSize) == nbFAILURE)
			return nbFAILURE;

		if (m_PDMLReader->StorePacket(m_packetAsciiBuffer.GetStartBufferPtr(), m_packetAsciiBuffer.GetBufferCurrentSize()) == nbFAILURE)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "%s", m_PDMLReader->GetLastError() );
			return nbFAILURE;
		}
	}

	return nbSUCCESS;
}



#define STARTING_POS _TEXT("0")	/* It keeps the first byte that belongs to the packet (obviously, the first one) */


/*!
	\brief It creates the PDML fragment that keeps the general information about the packet.

	This function puts the general information related to the current packet into the PDML file.
	It prints the ordinal number of the packet into the dump, the packet length, the captured length,
	the timestamp.

	The so created PDML fragment can be retrieved by means of the GetPDMLPacketInfo() method.

	\param PcapHeader: a pcap_pkthdr structure that contains the packet header.
	\param PcapPktData: pointer to the raw packet data, formatted according to the standard libpcap/WinPcap syntax.
	\param PacketCounter: the ordinal number of this packet.
*/
void CPDMLMaker::PacketGenInfoInitialize(const struct pcap_pkthdr *PcapHeader, const unsigned char * PcapPktData, int PacketCounter)
{
	m_packetSummary.CapturedLength= PcapHeader->caplen;
	m_packetSummary.Length= PcapHeader->len;
	m_packetSummary.Number= PacketCounter;
	m_packetSummary.TimestampSec= PcapHeader->ts.tv_sec;
	m_packetSummary.TimestampUSec= PcapHeader->ts.tv_usec;
	m_packetSummary.FirstProto= m_protoList[0];
	m_packetSummary.PacketDump= PcapPktData;

	// Save CapLen, since it is required in some following functions
	m_capLen= PcapHeader->caplen;
	// Save also the packet buffer pointer, just in case we need to dump the packet

#ifdef DEBUG_LOUD
	// This piece of code prints every decoded field on screen as soon as it gets decoded
	// This is extremely useful in case of debugging, when the library hungs. In this way,
	// we can know which is the last field before the one that creates the problem.
	CAsciiBuffer TempAsciiBuffer;

	TempAsciiBuffer.Initialize();

	// FirstProto must be initialzied to NULL, in this 'debug' function
	m_packetSummary.FirstProto= NULL;

	DumpPDMLPacket(&m_packetSummary, m_isVisExtRequired, m_generateRawDump, &TempAsciiBuffer, m_errbuf, m_errbufSize);
	fprintf(stderr, "\n");
	fprintf(stderr, TempAsciiBuffer.GetStartBufferPtr());

	// Restore previous value of FirstProto
	m_packetSummary.FirstProto= m_protoList[0];
#endif

}


/*!
	\brief It returns a _nbPDMLPacket pointer, which contains the summary information (caplen, len, timestamp, and more).

	\return A pointer to a _nbPDMLPacket structure that keeps the packet information.
*/
struct _nbPDMLPacket *CPDMLMaker::GetPDMLPacketInfo()
{
	return &m_packetSummary;
}


/*!
	\brief It creates a new PDMLElement by appending a new item to the given parent element.

	This method is needed to create a new item in the PDML tree. However, only the
	CNetPDLProtoDecoder knows exactly where this item has to be created. Therefore, the best
	solution is to let that class tell us in which position the new element has to be created.
	This is done by specifying the PDMLParent as parameter.

	\param PDMLParent: the PDML element on which the new element has to be appended. For instance,
	if the field is a bit-masked field, the PDMLParent will be the 'root' field of all the masked 
	bits.

	\return The PDML element that has been created.
*/
_nbPDMLField *CPDMLMaker::PDMLElementInitialize(_nbPDMLField *PDMLParent)
{
	// Initialize current element
	memset(m_fieldsList[m_currNumFields], 0, sizeof (_nbPDMLField));

	// Update links to the parent protocol and such
	m_fieldsList[m_currNumFields]->ParentProto= m_protoList[m_currNumProto];

	// Update links to the parent node
	if (PDMLParent)
	{
		m_fieldsList[m_currNumFields]->ParentField= PDMLParent;

		// Updates the first child if the parent node only if it was not set (yet)
		if (PDMLParent->FirstChild == NULL)
		{
			PDMLParent->FirstChild= m_fieldsList[m_currNumFields];

			// If this is the first child of the parent, let's put to NULL the m_previousField
			m_previousField= NULL;
		}
	}

	// Update link to previous/next fields
	if (PDMLParent == NULL)
	{
	_nbPDMLField *TempField;
		
		// If we do not have any 'parent field', we're going to create a field which is at
		// the first level within the current protocol
		// So, let's scan the current protocol in order to locate the last field

		TempField= (m_fieldsList[m_currNumFields])->ParentProto->FirstField;

		// Let's move to the last field of this protocol
		if (TempField != m_fieldsList[m_currNumFields])
		{
			while (TempField->NextField)
				TempField= TempField->NextField;

				m_fieldsList[m_currNumFields]->PreviousField= TempField;
				TempField->NextField= m_fieldsList[m_currNumFields];
		}
		else
			m_fieldsList[m_currNumFields]->PreviousField= NULL;
	}
	else
	{
	_nbPDMLField *TempField;
		
		// If we do have a 'parent field', we have to scan the child of the current
		// parent looking for the last one. The resulting field is the one that precedes
		// the field that we are currently updating

		TempField= PDMLParent->FirstChild;

		// Check that a first child exists and it is not the current field
		if ( (TempField) && (TempField != m_fieldsList[m_currNumFields]) )
		{
			// Let's move to the last child of the parent field
			while (TempField->NextField)
				TempField= TempField->NextField;

			TempField->NextField= m_fieldsList[m_currNumFields];
			m_fieldsList[m_currNumFields]->PreviousField= TempField;
		}
		else
			m_fieldsList[m_currNumFields]->PreviousField= NULL;

	}

	m_previousField= m_fieldsList[m_currNumFields];

	// Update the number of currently used fields
	m_currNumFields++;
	if (m_currNumFields >= m_maxNumFields)
	{
		if (CPDMLReader::UpdateFieldsList(&m_maxNumFields, &m_fieldsList, m_errbuf, m_errbufSize) == nbFAILURE)
			return NULL;
	}

	return m_fieldsList[m_currNumFields - 1];
}


/*!
	\brief Discards a previously created PDML element.

	This method is needed because there are several cases in which a new PDML element is created
	but, later on, the CNetPDLProtoDecoder detect that this action was wrong.
	This is the case of an Block, in which a new PDMLElement has to be created in order to decode
	the fields of that block. If fact, fields can be appended to the PDML tree only of their parent
	has been created.
	
	However, a 'presentif' element can tell you that the block is not the correct one. In that case,
	all the PDML elements have to be deleted; this is why this method is needed.

	\param PDMLElementToDelete: the PDML node that has to be deleted.
*/
void CPDMLMaker::PDMLElementDiscard(_nbPDMLField *PDMLElementToDelete)
{
	// This field has to be discarded. So, we can reuse it
	m_currNumFields--;

	// If this is the first field, let's update the pointer in the protocol list
	if (PDMLElementToDelete == m_protoList[m_currNumProto]->FirstField)
		m_protoList[m_currNumProto]->FirstField= m_fieldsList[m_currNumFields];

	// Let's update also the 'previous field' variable, because this field has to be discarded
	m_previousField= PDMLElementToDelete->PreviousField;
	if (m_previousField)
		m_previousField->NextField= NULL;

	// We do not have to update the previous sibling, because links are created
	// by the PDMLElementUpdate, which is called only when this function is not required.
}


#define MIN(a,b) ((a < b) ? a : b)

/*!
	\brief Update the common fields of the PDMLElement which corresponds to the current NetPDL element.

	This method is, by far, the most important of the PDMLMaker class.

	This function set all the required attributes into a PDML element. Some attributes (like the 
	"print" one) are created based on the specifications contained into the NetPDL element; others
	(like the "longname") are copied from the NetPDL element.

	This method does not address the problems related to the "printing" of the field; it calls the
	PDMLElementUpdateShowExtension() for that.

	This method has to be invoked for all fields, except for the Block and Header elements into the 
	NetPDL description.

	\param PDMLElement: pointer to the PDML element that has to be created.

	\param NetPDLField: the original element in the NetPDL file that contains this description.
	In order words, the NetPDLElement keeps the specification of a given field, while the PDML 
	element keeps the result of the decoding process of the selected field.

	\param Len: keeps the length (in bytes) of the decoded field.

	\param Offset: keeps the relative offset (from the beginning of the packet) of this field.
	In other words, the first byte of an IP packet contained into an Ethernet frame has Offset equal to 14.

	\param PacketFieldPtr: a pointer to the packet buffer that contains the current data (in the pcap format).
	This is needed in order to get the value of the current field and put it into the prope attributes
	of the PDML element.
	This pointer refers to the beginning of the current field.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPDMLMaker::PDMLElementUpdate(struct _nbPDMLField *PDMLElement, struct _nbNetPDLElementFieldBase *NetPDLField, int Len, int Offset, const unsigned char *PacketFieldPtr)
{
char FieldValueString[NETPDL_MAX_PACKET * 6 + 1];	// We have to take into account that extended ascii are printed as &#xxx; ==> one bytes uses ascii 6 bytes
char MaskedValueString[NETPDL_MAX_STRING * 2 + 1];
int Size;
int RetVal;

	// Extract info from description and append to PDML tree with offset, length and data
	if ( (Offset + Len) <= m_capLen)
		Size = Len;
	else
		Size = m_capLen - Offset;

	PDMLElement->Position = Offset;
	PDMLElement->Size = Size;
	PDMLElement->isField = true;

	// Extracts common attribute
	if (CPDMLReader::AppendItemString(NetPDLField->Name, &PDMLElement->Name, &m_tempFieldData, m_errbuf, m_errbufSize) == nbFAILURE)
		return nbFAILURE;

	// If we're the main block element, please return (we do not have to print any other attributes)
	if (NetPDLField->Type == nbNETPDL_IDEL_BLOCK)
	{
		// If this is a block, set the associated flag
		PDMLElement->isField = false;

		// If the user does not want to create the visualization extension primitives, avoid the following code
		if (m_isVisExtRequired)
		{
			if (CPDMLReader::AppendItemString(NetPDLField->LongName, &PDMLElement->LongName, &m_tempFieldData, m_errbuf, m_errbufSize) == nbFAILURE)
				return nbFAILURE;
		}

		return nbSUCCESS;
	}

	if (NetPDLField->FieldType == nbNETPDL_ID_FIELD_BIT)
	{
	struct _nbNetPDLElementFieldBit *NetPDLBitField;

		NetPDLBitField= (struct _nbNetPDLElementFieldBit *) NetPDLField;

		if (CPDMLReader::AppendItemString(NetPDLBitField->BitMaskString, &PDMLElement->Mask, &m_tempFieldData, m_errbuf, m_errbufSize) == nbFAILURE)
			return nbFAILURE;

		// This code handles the case in which we have masked fields. For them, we have to insert the
		// 'unmasked' value of the field, so that the PDML parsing is much simpler (we do not have
		// to deal with the masked value, because we have, in clear, the exact value of the entire 
		// field without masks)
		nbNetPDLUtils::HexDumpBinToHexDumpAscii( (char *) PacketFieldPtr, Size, 
					NetPDLBitField->IsInNetworkByteOrder, MaskedValueString, sizeof(MaskedValueString));

		if (CPDMLReader::AppendItemString(MaskedValueString, &PDMLElement->Value, &m_tempFieldData, m_errbuf, m_errbufSize) == nbFAILURE)
			return nbFAILURE;
	}
	else
	{
		// Copy the given field into 'HexValueAscii'. This is an hex number, since there are fields which cannot
		// be translated into a decimal number (like IPv6 addresses)
		// Note: variable 'ShowValueString' is recycled here
		nbNetPDLUtils::HexDumpBinToHexDumpAscii( (char *) PacketFieldPtr, Size, 
					NetPDLField->IsInNetworkByteOrder, FieldValueString, sizeof(FieldValueString));

		if (CPDMLReader::AppendItemString(FieldValueString, &PDMLElement->Value, &m_tempFieldData, m_errbuf, m_errbufSize) == nbFAILURE)
			return nbFAILURE;
	}

	// If the visualization extensions are not required, we can return right now.
	if (!m_isVisExtRequired)
		return nbSUCCESS;

	if (CPDMLReader::AppendItemString(NetPDLField->LongName, &PDMLElement->LongName, &m_tempFieldData, m_errbuf, m_errbufSize) == nbFAILURE)
		return nbFAILURE;

	RetVal= PDMLElementUpdateShowExtension(PDMLElement, NetPDLField, Len, Size, Offset, PacketFieldPtr);

	if (RetVal != nbSUCCESS)
		return RetVal;

#ifdef DEBUG_LOUD
	// This piece of code prints every decoded field on screen as soon as it gets decoded
	// This is extremely useful in case of debugging, when the library hungs. In this way,
	// we can know which is the last field before the one that creates the problem.
	CAsciiBuffer TempAsciiBuffer;

	TempAsciiBuffer.Initialize();

	DumpPDMLFields(PDMLElement, &TempAsciiBuffer, m_errbuf, m_errbufSize);
	fprintf(stderr, "\t");
	fprintf(stderr, TempAsciiBuffer.GetStartBufferPtr());
#endif

	return nbSUCCESS;
}



/*!
	\brief Update the common fields of the PDMLElement with respect to the visualization primitives.

	This method is called by PDMLElementUpdate() and it updates all the members of a PDMLElement
	with respect to the visualization primitives. This method is not called in case the NetPDL element
	does not contain any visualization primitive (or, the user is not interested in formatting properly
	the decoded element).

	This method addresses all the problem related to the "printing" of the field: printing in
	hex/ascii/..., masked fields, fields which require a custom printing ("showdtl"), fields which 
	have to be mapped with a string ("showmap"), and so on.

	\param PDMLElement: pointer to the PDML element that is being created.

	\param NetPDLField: the original element in the NetPDL file that contains this description.
	In order words, the NetPDLElement keeps the specification of a given field, while the PDML 
	element keeps the result of the decoding process of the selected field.

	\param Len: keeps the length (in bytes) of the decoded field.

	\param Size: usually equal to 'Len'; it may be shorter in case the field is truncated; in this case,
	it contains tha amount of valid data (e.g. fields of 10 bytes, truncated at byte 6: 'Len' = 10,
	'Size' = 6)..

	\param Offset: keeps the relative offset (from the beginning of the packet) of this field.
	In other words, the first byte of an IP packet contained into an Ethernet frame has Offset equal to 14.

	\param PacketFieldPtr: a pointer to the packet buffer that contains the current data (in the pcap format).
	This is needed in order to get the value of the current field and put it into proper attributes
	of the PDML element.
	This pointer refers to the beginning of the current field.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPDMLMaker::PDMLElementUpdateShowExtension(struct _nbPDMLField *PDMLElement, struct _nbNetPDLElementFieldBase *NetPDLField, int Len, int Size, int Offset, const unsigned char *PacketFieldPtr)
{
char ShowValueString[NETPDL_MAX_PACKET * 6 + 1];	// We have to take into account that extended ascii are printed as &#xxx; ==> one bytes uses ascii 6 bytes
char MaskedValueString[NETPDL_MAX_STRING * 2 + 1];
char BinValue[4 * 8 + 1];					// TAKE CARE: we support only masked values which are max 32 bits wide
int displaygroup;
int RetVal;
int ShowValueIndex= 0;
int PacketIndex = 0;


	// In case the field is truncated, let's break processing here by placing a conventional string in the field formatted value
	if ( (Offset + Len) > m_capLen)
	{
		strcpy(ShowValueString, "(Truncated field)");

		if (CPDMLReader::AppendItemString(ShowValueString, &PDMLElement->ShowValue, &m_tempFieldData, m_errbuf, m_errbufSize) == nbFAILURE)
			return nbFAILURE;

		return nbSUCCESS;
	}

	// If the field is associated to a native printing function, let's call it and exit
	if (NetPDLField->ShowTemplateInfo->ShowNativeFunction)
		return PrintNativeFunction(PDMLElement, Size, PacketFieldPtr, NetPDLField->ShowTemplateInfo->ShowNativeFunction);

	RetVal= nbWARNING;
	// If this field does have a visualization external template attribute, call the specific function
	if (NetPDLField->ShowTemplateInfo->PluginName)
	{
	struct _ShowPluginParams ShowPluginParams;

		ShowPluginParams.AsciiBuffer= &m_tempFieldData;
		ShowPluginParams.CapturedLength= m_capLen;
		ShowPluginParams.ErrBuf= m_errbuf;
		ShowPluginParams.ErrBufSize= m_errbufSize;
		ShowPluginParams.Packet= (PacketFieldPtr - Offset);
		ShowPluginParams.PDMLElement= PDMLElement;
		ShowPluginParams.NetPDLVariables= m_netPDLVariables;

		RetVal= (ShowPluginList[NetPDLField->ShowTemplateInfo->PluginIndex].ShowPluginHandler) (&ShowPluginParams);

		// Return code of previous call may also be nbWARNING; this code must be handled internally
		// and not returned to the caller
		if (RetVal == nbFAILURE)
			return nbFAILURE;
	}

	// It means that either
	// - we do not have a visualization plugin
	// - we have it, but it was not able to work properly
	//
	// So, we have to rely on the standard attributes to display the value of this field
	if (RetVal == nbWARNING)
	{
		// Check the displaygroup
		if (NetPDLField->ShowTemplateInfo->DigitSize == 0)
			displaygroup= Size;
		else
			displaygroup= NetPDLField->ShowTemplateInfo->DigitSize;

		switch (NetPDLField->ShowTemplateInfo->ShowMode)
		{
			case nbNETPDL_ID_TEMPLATE_DIGIT_HEX:
			{
				// Prepend the appropriate header in case of hex and binary display
				strcpy(ShowValueString, "0x");
				ShowValueIndex= 2;
			}; break;

			case nbNETPDL_ID_TEMPLATE_DIGIT_BIN:
			{
				// Prepend the appropriate header in case of hex and binary display
				strcpy(ShowValueString, "0b");
				ShowValueIndex= 2;
			}; break;

			default:
				break;
		}

		// Initialize the string that will keep the shown value
		ShowValueString[ShowValueIndex]= 0;

		while (PacketIndex < Size)
		{
		unsigned long ivalue;
		char HexDigit[NETPDL_MAX_STRING + 1];
		char *HexDigitPtr;

			// Check if the number must be printed within a single group
			// If so, we can avoid to call the HexToString another time
			if (displaygroup != Size)
			{
				if (NetPDLField->FieldType == nbNETPDL_ID_FIELD_BIT)
				{
				struct _nbNetPDLElementFieldBit *NetPDLBitField;

					NetPDLBitField= (struct _nbNetPDLElementFieldBit *) NetPDLField;

					HexDumpBinToHexDumpAsciiWithMask(&PacketFieldPtr[PacketIndex], MIN(displaygroup, Size - PacketIndex),
							NetPDLField->IsInNetworkByteOrder, NetPDLBitField->BitMask, HexDigit, sizeof(HexDigit), BinValue);
				}
				else
				{
					nbNetPDLUtils::HexDumpBinToHexDumpAscii((char *) &PacketFieldPtr[PacketIndex], 
															MIN(displaygroup, Size - PacketIndex),
															NetPDLField->IsInNetworkByteOrder, HexDigit, sizeof(HexDigit));
				}

				HexDigitPtr= HexDigit;
			}
			else
			{
				if (NetPDLField->FieldType == nbNETPDL_ID_FIELD_BIT)
				{
				struct _nbNetPDLElementFieldBit *NetPDLBitField;

					NetPDLBitField= (struct _nbNetPDLElementFieldBit *) NetPDLField;

					// Copy the given field into 'HexValue'. This is an hex number, since there are fields which cannot
					// be translated into a decimal number (like IPv6 addresses)
					// Note: variable 'ShowValueString' is recycled here
					HexDumpBinToHexDumpAsciiWithMask(PacketFieldPtr, Size, NetPDLBitField->IsInNetworkByteOrder,
								NetPDLBitField->BitMask, MaskedValueString, sizeof(MaskedValueString), BinValue);

					HexDigitPtr= MaskedValueString;
				}
				else
				{
					HexDigitPtr= PDMLElement->Value;
				}
			}

			// FULVIO 30/12/2007 we he can optimize a bit using buffer 'm_tempFieldData' to store the printed value
			// Instead, in this code we store the content into a static buffer, then we copy this value to that class
			// at the end of this block. The first problem is that we are not able to check for buffer overruns in the
			// static buffer, the second is that we copy data 2 times.
			switch (NetPDLField->ShowTemplateInfo->ShowMode)
			{
				case nbNETPDL_ID_TEMPLATE_DIGIT_BIN:
				{
					// In case Mask does not exist, the HexDumpBinToHexDumpAscii() function does not create 
					// the binary string, so we have to create it here
					if (NetPDLField->FieldType != nbNETPDL_ID_FIELD_BIT)
					{
						if (displaygroup > 4)
						{
							errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize,
								"Possible NetPDL error: field '%s.%s' is greather than 4 bytes, and it cannot be formatted with the current visualization template.",
								PDMLElement->ParentProto->Name, PDMLElement->Name);

							return nbFAILURE;
						}

						ivalue= NetPDLHexDumpToLong(&PacketFieldPtr[PacketIndex], displaygroup, NetPDLField->IsInNetworkByteOrder);

						// ltoa can create strings up to 33 bytes; so, let's check for overruns
						if ((sizeof(ShowValueString) - ShowValueIndex) < 33)
						{
							errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Packet too big for the NetPDL engine");
							return nbFAILURE;
						}

						ultoa(ivalue, &ShowValueString[ShowValueIndex], 2);
					}
					else
						ShowValueIndex+= sstrncpy(&ShowValueString[ShowValueIndex], BinValue, sizeof(ShowValueString) - ShowValueIndex);
					break;
				};

				case nbNETPDL_ID_TEMPLATE_DIGIT_DEC:
				{
					if (NetPDLField->FieldType != nbNETPDL_ID_FIELD_BIT)
					{
						if (displaygroup > 4)
						{
							errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize,
								"Possible NetPDL error: field '%s.%s' is greather than 4 bytes, and it cannot be formatted with the current visualization template.",
								PDMLElement->ParentProto->Name, PDMLElement->Name);

							return nbFAILURE;
						}

						ivalue= NetPDLHexDumpToLong(&PacketFieldPtr[PacketIndex], displaygroup, NetPDLField->IsInNetworkByteOrder);

						// ltoa can create strings up to 33 bytes; so, let's check for overruns
						if ((sizeof(ShowValueString) - ShowValueIndex) < 33)
						{
							errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Packet too big for the NetPDL engine");
							return nbFAILURE;
						}

						ultoa(ivalue, &ShowValueString[ShowValueIndex], 10);
					}
					else
					{
						// We're converting from ASCII strings, which are two times bigger than HEX strings
						ivalue= NetPDLAsciiStringToLong(HexDigitPtr, displaygroup * 2, 16);
						ultoa(ivalue, &ShowValueString[ShowValueIndex], 10);
					}
					break;
				}

				case nbNETPDL_ID_TEMPLATE_DIGIT_HEX:
				case nbNETPDL_ID_TEMPLATE_DIGIT_HEXNOX:
				{
					ShowValueIndex+= sstrncpy(&ShowValueString[ShowValueIndex], HexDigitPtr, sizeof(ShowValueString) - ShowValueIndex);
					break;
				}

				case nbNETPDL_ID_TEMPLATE_DIGIT_FLOAT:
				{
				float* FloatPtr;

#ifdef _DEBUG
					if (displaygroup != sizeof(float))
					{
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize,
							"Runtime error: wrong size for field '%s.%s': floats can be only %d bytes.", 
							PDMLElement->ParentProto->Name, PDMLElement->Name, sizeof(float));
						return nbFAILURE;
					}
#endif
					FloatPtr= (float* ) &PacketFieldPtr[PacketIndex];

					ShowValueIndex+= ssnprintf(&ShowValueString[ShowValueIndex], sizeof(ShowValueString) - ShowValueIndex, "%g", *FloatPtr);
					break;
				}

				case nbNETPDL_ID_TEMPLATE_DIGIT_DOUBLE:
				{
				double* DoublePtr;

#ifdef _DEBUG
					if (displaygroup != sizeof(double))
					{
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize,
							"Runtime error: wrong size for field '%s.%s': doubles can be only %d bytes.",
							PDMLElement->ParentProto->Name, PDMLElement->Name, sizeof(double));
						return nbFAILURE;
					}
#endif
					DoublePtr= (double* ) &PacketFieldPtr[PacketIndex];

					ShowValueIndex+= ssnprintf(&ShowValueString[ShowValueIndex], sizeof(ShowValueString) - ShowValueIndex, "%g", *DoublePtr);
					break;
				}

				case nbNETPDL_ID_TEMPLATE_DIGIT_ASCII:
				{
					ivalue= NetPDLAsciiStringToLong(HexDigitPtr, (int) strlen(HexDigitPtr), 16);

					if (ivalue < 0x20)
					{
						// If it is not an ascii value, we print a '.' instead
						// Take care; the char can still be a carriage return or such these things
						if ( (ivalue != 0x0d) && (ivalue != 0x0a) && (ivalue != 0) )
							ShowValueIndex+= sstrncpy(&ShowValueString[ShowValueIndex], ".", sizeof(ShowValueString) - ShowValueIndex);

						break;
					}

					if (ivalue > 0x7F)
					{
						ShowValueIndex+= ssnprintf(&ShowValueString[ShowValueIndex], sizeof(ShowValueString) - ShowValueIndex, "&#%d;", ivalue);
						break;
					}

					// Check the character, and print only the ones that matters
					switch (ivalue)
					{
						// There are some fields which cannot be printed, so we have to translate them into
						// XML-compatible fields
						case '<':
						{
							ShowValueIndex+= sstrncpy(&ShowValueString[ShowValueIndex], "&lt;", sizeof(ShowValueString) - ShowValueIndex);
							break;
						}
						case '>':
						{
							ShowValueIndex+= sstrncpy(&ShowValueString[ShowValueIndex], "&gt;", sizeof(ShowValueString) - ShowValueIndex);
							break;
						}
						case '"':
						{
							ShowValueIndex+= sstrncpy(&ShowValueString[ShowValueIndex], "&quot;", sizeof(ShowValueString) - ShowValueIndex);
							break;
						}
						case '&':
						{
							ShowValueIndex+= sstrncpy(&ShowValueString[ShowValueIndex], "&amp;", sizeof(ShowValueString) - ShowValueIndex);
							break;
						}

						default:
						{
							ShowValueIndex+= sstrncpy(&ShowValueString[ShowValueIndex], (char *) &ivalue, sizeof(ShowValueString) - ShowValueIndex);
							break;
						}
					}

					break;
				}
			}

			// Let's check for overruns
			// FULVIO 30/12/2007: this code is useless, because previous functions return "-1" in case of errors, so
			// it's very unlikely that this control succeedes
//			if (sizeof(ShowValueString) == ShowValueIndex)
//			{
//				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Packet too big for the NetPDL engine");
//				return nbFAILURE;
//			}

			PacketIndex+= displaygroup;

			if ((PacketIndex < Size) && (NetPDLField->ShowTemplateInfo->Separator))
				ShowValueIndex+= sstrncat(&ShowValueString[ShowValueIndex], NetPDLField->ShowTemplateInfo->Separator, sizeof(ShowValueString) - ShowValueIndex) ;
		}

		if (CPDMLReader::AppendItemString(ShowValueString, &PDMLElement->ShowValue, &m_tempFieldData, m_errbuf, m_errbufSize) == nbFAILURE)
			return nbFAILURE;
	}

	// If this field does have a table map attribute, call the specific function
	if (NetPDLField->ShowTemplateInfo->MappingTableInfo)
	{
		if (PrintMapTable(PDMLElement, NetPDLField->ShowTemplateInfo->MappingTableInfo) == nbFAILURE)
			return nbFAILURE;
	}

	// If this field has a custom template attribute, call the specific function
	if (NetPDLField->ShowTemplateInfo->CustomTemplateFirstField)
	{
		if (PrintFieldDetails(PDMLElement,
					NetPDLField->ShowTemplateInfo->CustomTemplateFirstField,
					&(PDMLElement->ShowDetails)) == nbFAILURE)
			return nbFAILURE;
	}

	return nbSUCCESS;
}



/*!
	\brief Update the common fields of the PDMLElement which corresponds to the current NetPDL block.

	\param PDMLElement: pointer to the PDML element that has to be created.

	\param NetPDLBlock: the original element in the NetPDL file that contains this description.
	In order words, the NetPDLElement keeps the specification of a given field, while the PDML 
	element keeps the result of the decoding process of the selected field.

	\param Len: keeps the length (in bytes) of the decoded field.

	\param Offset: keeps the relative offset (from the beginning of the packet) of this field.
	In other words, the first byte of an IP packet contained into an Ethernet frame has Offset equal to 14.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPDMLMaker::PDMLBlockElementUpdate(_nbPDMLField *PDMLElement, struct _nbNetPDLElementBlock *NetPDLBlock, int Len, int Offset)
{
int Size;

	// Extract info from description and append to PDML tree with offset, length and data
	if ( (Offset + Len) <= m_capLen)
		Size= Len;
	else
		Size= m_capLen - Offset;

	PDMLElement->Position= Offset;
	PDMLElement->Size= Size;

	// Extracts common attribute
	if (CPDMLReader::AppendItemString(NetPDLBlock->Name, &PDMLElement->Name, &m_tempFieldData, m_errbuf, m_errbufSize) == nbFAILURE)
		return nbFAILURE;

	// If the user does not want to create the visualization extension primitives, avoid the following code
	if (m_isVisExtRequired)
	{
		if (CPDMLReader::AppendItemString(NetPDLBlock->LongName, &PDMLElement->LongName, &m_tempFieldData, m_errbuf, m_errbufSize) == nbFAILURE)
			return nbFAILURE;
	}

	return nbSUCCESS;
}



/*!
	\brief It looks into a NetPDL map table and it returns the string that matches the current value.

	This method is used in order to print a literal description according to the value of a given
	field. For example, the "opcode" into a ARP packet can be "1" (ARP Request), or "2" (ARP Reply).
	This function allows to map a given value to its entry in the appropriate map table.

	\param PDMLElement: pointer to the PDMLElement that has to be matched with the appropriate value.
	This parameter is needed in order to make things simpler. For instance, usually this function
	uses the "value" attribute of the PDML document, unless an "expression" is specified into the
	print map table. So, to keeps things simpler, the PDMLElement is passed, and this function checks
	if it needs to know the value of the field or if this field has to be elaborated by an expression.

	\param MappingTableInfo: pointer to the structure associated to a mapping table (which is the same
	structure associated to a 'switch' element).

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.

	\warning This function has to check if the matching value has to be managed as "string" or as 
	"number".
*/
int CPDMLMaker::PrintMapTable(_nbPDMLField *PDMLElement, struct _nbNetPDLElementSwitch *MappingTableInfo)
{
struct _nbNetPDLElementCase *CaseNodeInfo;
int RetVal;

	// Let's look for the proper matching item
	RetVal= m_exprHandler->EvaluateSwitch(MappingTableInfo, PDMLElement, &CaseNodeInfo);

	if (RetVal != nbSUCCESS)
		return RetVal;

	if (CaseNodeInfo)
	{
		if (CPDMLReader::AppendItemString(CaseNodeInfo->ShowString, &PDMLElement->ShowMap, &m_tempFieldData, m_errbuf, m_errbufSize) == nbFAILURE)
			return nbFAILURE;

		return nbSUCCESS;
	}
	else
	{
	CAsciiBuffer AsciiBuffer;

		if (AsciiBuffer.Initialize() == nbFAILURE)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "%s", AsciiBuffer.GetLastError() );
			return nbFAILURE;
		}

		if (DumpPDMLFields(PDMLElement, &AsciiBuffer, m_errbuf, m_errbufSize) == nbFAILURE)
			return nbFAILURE;

		// We found the table, but the match has not been found
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
			"The mapping specified in the following element (protocol '%s'):\n '%s'\n\n has not been found.",
			PDMLElement->ParentProto->Name, AsciiBuffer.GetStartBufferPtr());

		return nbFAILURE;
	}
}



/*!
	\brief Converts an hex dump (i.e. a network packet) into an hex string, taking into account of its mask.

	This method is more or less the same as nbNetPDLUtils::HexDumpBinToHexDumpAscii(); however, it
	works with masks.

	This method is not very simple because it has to manage standard fields and masked fields. Moreover, 
	the printed string must contain all the characters needed to fill in a N-bytes field. In other words,
	if the result of the conversion is '0' and the field has a 4bit length, the resulting string will
	be '0000'.

	\param PacketData: a pointer to the point of the packet that contains the field we want to print.

	\param HexDumpIsInNetworkByteOrder: 'true' if the field is stored in network byte order,
	'false' otherwise.

	\param Mask: the mask contained into the NetPDL description of this field (if one, "" otherwise).

	\param DataLen: the length of the field we have to print.

	\param HexResult: a pointer to the string that will contain the result in hex characters.
	This string must be allocated by the user.

	\param BinResult: a pointer to the string that will contain the result in bin characters.
	The result will have '0' or '1' digits when the mask is valid, and '.' when the mask is not valid.
	This string must be allocated by the user.

	\param HexResultStringSize: the size of the buffer that contains the result.
	The size of the BinResult string MUST be 4 times the size of the Hex result (since binary digits needs
	4 times the space compared to a Hex digit).

	\return nbSUCCESS if the conversion was possible, nbFAILURE if some error occurred.
*/
int CPDMLMaker::HexDumpBinToHexDumpAsciiWithMask(const unsigned char *PacketData, int DataLen, int HexDumpIsInNetworkByteOrder, int Mask, char *HexResult, int HexResultStringSize, char *BinResult)
{
unsigned long UnmaskedValue;
unsigned long SingleBit;
unsigned long Result;
int i;
char FormatSpecifier[NETPDL_MAX_STRING];

	if (DataLen > 4)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Masked fields cannot span more than 32 bits.");
		return nbFAILURE;
	}

	// ltoa can create strings up to 33 bytes; so, let's check for overruns
	if (HexResultStringSize < 33)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough buffer to contain the ascii dump of a field.");
		return nbFAILURE;
	}

	UnmaskedValue= NetPDLHexDumpToLong(PacketData, DataLen, HexDumpIsInNetworkByteOrder);

	SingleBit= 1 << (DataLen * 8 - 1);
	Result= 0;

	for (i= 0; i < (DataLen * 8); i++)
	{
		// If current bit is set in the mask, let's update Hex and Bin results
		if (SingleBit & Mask)
		{
			BinResult[i]= ((SingleBit & Mask) & UnmaskedValue) ? '1' : '0';
			Result+= ((SingleBit & Mask) & UnmaskedValue);
		}
		else
		{
			BinResult[i]= '.';
			Result = Result >> 1;
		}

		SingleBit= SingleBit >> 1;
	}
	BinResult[i]= 0;

	// obtain the "%0[numberofdigit]x" string
	ssnprintf(FormatSpecifier, sizeof(FormatSpecifier), "%%0%dX", DataLen * 2);

	// Print the number in hexadecimal format
	ssnprintf(HexResult, HexResultStringSize, FormatSpecifier, Result);

	return nbSUCCESS;
}


/*!
	\brief This method is executed in case a NetPDL decoded field requires a custom view.

	This means that the field has a "showdtl" tag in its description.
	This code is almost equal to the code present in the CPSMLMaker::AddHeaderFragment().
	In case there is some error, the resulting string will contain the error description.

	\param PDMLElement: a pointer (in the PDML fragment) of the node that has to be printed 
	with more details.

	\param ShowDetailsFirstItem: pointer to the first child element of the custom template 
	("&lt;showdtl&gt;") that has to be used to create the detailed view.

	\param ResultString: pointer to a buffer that will contain the result of this processing.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.

	The value contained in parameter 'ResultString' is not meaningful (and it should be NULL) 
	in case the procedure returns an error.
*/
int CPDMLMaker::PrintFieldDetails(_nbPDMLField *PDMLElement, struct _nbNetPDLElementBase *ShowDetailsFirstItem, char **ResultString)
{
struct _nbNetPDLElementBase *NetPDLDtlItem;
char *ReturnString;

	NetPDLDtlItem= ShowDetailsFirstItem;
	*ResultString= NULL;
	ReturnString= &(m_tempFieldData.GetStartBufferPtr()) [m_tempFieldData.GetBufferCurrentSize()];

	while (NetPDLDtlItem != NULL)
	{
		switch (NetPDLDtlItem->Type)
		{
			case nbNETPDL_IDEL_PROTOFIELD:
			{
			struct _nbNetPDLElementProtoField *DtlItemInfo;
			char *Attrib;
			int RetVal;

				DtlItemInfo= (struct _nbNetPDLElementProtoField *) NetPDLDtlItem;

				RetVal= CPDMLMaker::ScanForFieldRefAttrib(PDMLElement, NULL, PDMLElement->Name, DtlItemInfo->FieldShowDataType, 
														&Attrib, m_errbuf, m_errbufSize);

				if (RetVal != nbSUCCESS)
				{
					if (RetVal == nbWARNING)
					{
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
							"The NetPDL protocol database contains a reference to a field that cannot be located in the PDML fragment: Protocol '%s', Field '%s'",
							PDMLElement->ParentProto->LongName , PDMLElement->Name);
					}
					return RetVal;
				}

				if (m_tempFieldData.Append(Attrib) == nbFAILURE)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "%s", m_tempFieldData.GetLastError() );
					return nbFAILURE;
				}

				break;
			}

			case nbNETPDL_IDEL_TEXT:
			{
			struct _nbNetPDLElementText *DtlItemInfo;

				DtlItemInfo= (struct _nbNetPDLElementText *) NetPDLDtlItem;

				if (DtlItemInfo->Value)
				{
					if (m_tempFieldData.Append(DtlItemInfo->Value) == nbFAILURE)
					{
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "%s", m_tempFieldData.GetLastError() );
						return nbFAILURE;
					}
				}

				if (DtlItemInfo->ExprTree)
				{
					if (DtlItemInfo->ExprTree->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER)
					{
					unsigned char *TempBufferAscii;
//					void *TempBufferAscii_cast;
					unsigned int TempBufferAsciiSize;
					int RetVal;

						RetVal= m_exprHandler->EvaluateExprString(DtlItemInfo->ExprTree, PDMLElement,
																&TempBufferAscii, &TempBufferAsciiSize);

						if (RetVal != nbSUCCESS)
							return RetVal;

//						TempBufferAscii_cast = TempBufferAscii;

						if (m_tempFieldData.Append((char *) TempBufferAscii /*TempBufferAscii_cast*/, TempBufferAsciiSize) == nbFAILURE)
						{
							errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "%s", m_tempFieldData.GetLastError() );
							return nbFAILURE;
						}
					}
					else
					{
					unsigned int Result;
					int RetVal;

						RetVal= m_exprHandler->EvaluateExprNumber(DtlItemInfo->ExprTree, PDMLElement, &Result);

						if (RetVal != nbSUCCESS)
							return RetVal;

						if (m_tempFieldData.AppendFormatted("%u", Result) == nbFAILURE)
						{
							errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "%s", m_tempFieldData.GetLastError() );
							return nbFAILURE;
						}
					}
				}

				break;
			}

			case nbNETPDL_IDEL_IF:
			{
			struct _nbNetPDLElementIf *DtlItemInfo;
			int RetVal;
			unsigned int Result;

				DtlItemInfo= (struct _nbNetPDLElementIf *) NetPDLDtlItem;
			
				RetVal= m_exprHandler->EvaluateExprNumber(DtlItemInfo->ExprTree, PDMLElement, &Result);

				if (RetVal != nbSUCCESS)
					return RetVal;

				// If the 'if' is verified, Result is a non-zero number
				if (Result)
				{
					if (PrintFieldDetails(PDMLElement, DtlItemInfo->FirstValidChildIfTrue, ResultString) == nbFAILURE)
						return nbFAILURE;
				}
				else
				{
					if (DtlItemInfo->FirstValidChildIfFalse)
					{
						if (PrintFieldDetails(PDMLElement, DtlItemInfo->FirstValidChildIfFalse, ResultString) == nbFAILURE)
							return nbFAILURE;
					}
				}

				break;
			}

			default:
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Encountered a node which has not been recognized.");
				return nbFAILURE;
			}
		}

		// Move to the next element
		NetPDLDtlItem= nbNETPDL_GET_ELEMENT(NetPDLDatabase, NetPDLDtlItem->NextSibling);
	}

	*ResultString= ReturnString;
	return nbSUCCESS;
}




/*!
	\brief Print the field according to a native function.

	In case the field is associated to a native visualization function, it calls it and returns.
	The code is not very elegant, but speed matters here.

	\param PDMLElement: pointer to the PDML element that is being created.

	\param Size: usually equal to length of the field, but it may be shorter in case the field is
	truncated; in this case, it contains tha amount of valid data (e.g. fields of 10 bytes,
	truncated at byte 6: 'Len' = 10, 'Size' = 6)..

	\param PacketFieldPtr: a pointer to the packet buffer that contains the current data (in the pcap format).
	This is needed in order to get the value of the current field and put it into the proper members
	of the PDML element.
	This pointer refers to the beginning of the current field.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPDMLMaker::PrintNativeFunction(struct _nbPDMLField *PDMLElement, int Size, const unsigned char *PacketFieldPtr, int NativeFunctionID)
{
char *FormattedField= m_tempFieldData.GetStartFreeBufferPtr();
int FormattedFieldSize= m_tempFieldData.GetBufferTotSize() - m_tempFieldData.GetBufferCurrentSize();
unsigned long FormattedFieldLength;
int RetVal;

	if (FormattedFieldSize == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Internal error: the buffer required to print field value is too small.");
		return nbFAILURE;
	}

	switch (NativeFunctionID)
	{
		case nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_IPV4:
		{
			RetVal= NativePrintIPv4Address(PacketFieldPtr, FormattedField, FormattedFieldSize, &FormattedFieldLength, m_errbuf, m_errbufSize);

			if (RetVal == nbSUCCESS)
			{
				m_tempFieldData.SetBufferCurrentSize( m_tempFieldData.GetBufferCurrentSize() + FormattedFieldLength);
				PDMLElement->ShowValue= FormattedField;

				return nbSUCCESS;
			}
			else
				return nbFAILURE;
		};

		case nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_ASCII:
		{
			RetVal= NativePrintAsciiField(PacketFieldPtr, Size, FormattedField, FormattedFieldSize, &FormattedFieldLength, m_errbuf, m_errbufSize);

			if (RetVal == nbSUCCESS)
			{
				m_tempFieldData.SetBufferCurrentSize( m_tempFieldData.GetBufferCurrentSize() + FormattedFieldLength);
				PDMLElement->ShowValue= FormattedField;

				return nbSUCCESS;
			}
			else
				return nbFAILURE;
		};

		case nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_ASCIILINE:
		{
			RetVal= NativePrintAsciiLineField(PacketFieldPtr, Size, FormattedField, FormattedFieldSize, &FormattedFieldLength, m_errbuf, m_errbufSize);

			if (RetVal == nbSUCCESS)
			{
				m_tempFieldData.SetBufferCurrentSize( m_tempFieldData.GetBufferCurrentSize() + FormattedFieldLength);
				PDMLElement->ShowValue= FormattedField;

				return nbSUCCESS;
			}
			else
				return nbFAILURE;
		};
		case nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_HTTPCONTENT:
		{
			RetVal= NativePrintHTTPContentField(PacketFieldPtr, Size, FormattedField, FormattedFieldSize, &FormattedFieldLength, m_errbuf, m_errbufSize);

			if (RetVal == nbSUCCESS)
			{
				m_tempFieldData.SetBufferCurrentSize( m_tempFieldData.GetBufferCurrentSize() + FormattedFieldLength);
				PDMLElement->ShowValue= FormattedField;

				return nbSUCCESS;
			}
			else
				return nbFAILURE;
		};

		// No native printing function, so let's continue with the rest of the code
		default:
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: a native printing function is called, but it does not appear to be in the code.");
			return nbFAILURE;
		}
	}
}



/*!
	\brief This function parses the given PDML fragment looking for the field named 'Name'.

	Given a portion of the PDML file (containing the decoded fields), it looks for
	the attribute 'Attrib' of the field whose name is specified in 'Name'.
	The user can specify also the protocol in which this field has to be located.

	This function is recursive, i.e. it looks inside each PDML element in order to see
	if there are any childs. If so, it looks inside the children as well.
	This function adopts a 'backward scan' method (from the latest field to the first one);
	this is used to report always the most recent field in case there are several fields
	with the same name.

	\param PDMLField: pointer to the element belonging to the PDML fragment we are looking into.
	This function will walk through the PDML tree in order to locate the proper field
	according to the other parameters.

	\param ProtoName: ascii string that contains the name of the protocol (in the PDML file) 
	in which the field we are looking for is contained. This can be NULL in case we want to
	look into the current protocol.

	\param FieldName: ascii string that contains the name of the field (in the PDML file) 
	we are looking for.

	\param AttribCode: the internal code of the attribute we are looking for.

	\param AttribValue: the value of the requested attribute. It is filled in by the function.
	In case of errors, this value will be NULL when the function returns.

	\param ErrBuf: a user-applicated buffer that will contain the error message (if any).

	\param ErrBufSize: length of ErrBuf buffer.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
	The value of the requested attribute is returned into the AttribValue parameter.<br>
	In case the field we are looking for has not been found, it returns nbWARNING.
	This is due because in other piece of code we need to know if there has been an error
	when locating the field or the field simply has not been found.

	\note This function has been declared as 'static' because it is called also from other contexts.
*/
int CPDMLMaker::ScanForFieldRefAttrib(struct _nbPDMLField *PDMLField, char *ProtoName, char *FieldName, int AttribCode,
								  char **AttribValue, char *ErrBuf, int ErrBufSize)
{
struct _nbPDMLField *Result;
char *Attribute;
int RetVal;

	RetVal= ScanForFieldRef(PDMLField, ProtoName, FieldName, &Result, ErrBuf, ErrBufSize);

	// The field has not been found, however no errors occurred during the scan
	// (it may be a truncated packet, so that the wanted field it is not here)
	if (RetVal != nbSUCCESS)
	{
		*AttribValue= NULL;
		return RetVal;
	}

	Attribute= GetPDMLFieldAttribute(AttribCode, Result);

	if ( (Attribute != NULL) && (*Attribute != 0) )
	{
		*AttribValue= Attribute;
		return nbSUCCESS;
	}
	else
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
			"The NetPDL protocol database contains a reference to an attribute that cannot be located in the PDML fragment: Protocol '%s', Field '%s'",
			(ProtoName == NULL) ? PDMLField->ParentProto->LongName : ProtoName, FieldName);

		*AttribValue= NULL;
		return nbFAILURE;
	}
}


/*!
	\brief This function parses the given PDML fragment looking for the field named 'Name'.

	Given a portion of the PDML file (containing the decoded fields), it return the offset
	(within the packet buffer) of the wanted field, and the mask (if any).

	This function is pretty similar to the ScanForFieldRefAttrib().

	\param PDMLField: pointer to the element belonging to the PDML fragment we are looking into.
	This function will walk through the PDML tree in order to locate the proper field
	according to the other parameters.

	\param ProtoName: ascii string that contains the name of the protocol (in the PDML file) 
	in which the field we are looking for is contained. This can be NULL in case we want to
	look into the current protocol.

	\param FieldName: ascii string that contains the name of the field (in the PDML file) 
	we are looking for.

	\param FieldOffset: this parameter is used to return the offset (in the packet dump)
	of the current field.

	\param FieldSize: this parameter is used to return the size in bytes (in the packet dump)
	of the current field.

	\param FieldMask: this parameter is used to return the mask of the current field.
	In case the field does not have masks (its length is an integer multiple of bytes),
	the returned value is NULL. The mask is returned as ascii buffer.

	\param ErrBuf: a user-applicated buffer that will contain the error message (if any).

	\param ErrBufSize: length of ErrBuf buffer.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
	The value of the requested attribute is returned into the AttribValue parameter.<br>
	In case the field we are looking for has not been found, it returns nbWARNING.
	This is due because in other piece of code we need to know if there has been an error
	when locating the field or the field simply has not been found.

	\note This function has been declared as 'static' because it is called also from other contexts.
*/
int CPDMLMaker::ScanForFieldRefValue(struct _nbPDMLField *PDMLField, char *ProtoName, char *FieldName, unsigned int *FieldOffset,
								  unsigned int *FieldSize, char **FieldMask, char *ErrBuf, int ErrBufSize)
{
struct _nbPDMLField *Result;
int RetVal;

	RetVal= ScanForFieldRef(PDMLField, ProtoName, FieldName, &Result, ErrBuf, ErrBufSize);

	// The field has not been found, however no errors occurred during the scan
	// (it may be a truncated packet, so that the wanted field it is not here)
	if (RetVal != nbSUCCESS)
	{
		*FieldOffset= 0;
		*FieldSize= 0;
		*FieldMask= NULL;
		return RetVal;
	}

	*FieldOffset= Result->Position;
	*FieldSize= Result->Size;
	*FieldMask= Result->Mask;

	return nbSUCCESS;
}


/*!
	\brief This function parses the given PDML fragment looking for a field named 'Name'.

	Given a portion of the PDML file (containing the decoded fields), it looks for
	the field whose name is specified in 'Name'.
	The user can specify also the protocol in which this field has to be located.

	This function is recursive, i.e. it looks inside each PDML element in order to see
	if there are any childs. If so, it looks inside the children as well.
	This function adopts a 'backward scan' method (from the latest field to the first one);
	this is used to report always the most recent field in case there are several fields
	with the same name.

	\param PDMLField: pointer to the element belonging to the PDML fragment we are looking into.
	This function will walk through the PDML tree in order to locate the proper field
	according to the other parameters.

	\param ProtoName: ascii string that contains the name of the protocol (in the PDML file) 
	in which the field we are looking for is contained. This can be NULL in case we want to
	look into the current protocol.

	\param FieldName: ascii string that contains the name of the field (in the PDML file) 
	we are looking for.

	\param PDMLLocatedField: this parameter is used to return a pointer to the element 
	that has been found and that matches our request.

	\param ErrBuf: a user-applicated buffer that will contain the error message (if any).

	\param ErrBufSize: length of ErrBuf buffer.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
	The value of the requested attribute is returned into the AttribValue parameter.<br>
	In case the field we are looking for has not been found, it returns nbWARNING.
	This is due because in other piece of code we need to know if there has been an error
	when locating the field or the field simply has not been found.

	\note This function has been declared as 'static' because it is called also from other contexts.
*/
int CPDMLMaker::ScanForFieldRef(struct _nbPDMLField *PDMLField, char *ProtoName, char *FieldName, _nbPDMLField **PDMLLocatedField, char *ErrBuf, int ErrBufSize)
{
// struct _nbPDMLField *Result;
struct _nbPDMLField *PDMLLastField;

// Result= NULL;

	// The caller didn't specify the name of the field we are looking for
	// So, we're looking for the current field.
	// This happens when a "print details" template is called
	if (FieldName == NULL)
	{
		*PDMLLocatedField= PDMLField;
		return nbSUCCESS;
	}

	if ( (ProtoName != NULL) && (*ProtoName != 0) )
	{
	struct _nbPDMLProto *PDMLProto;

		// We have to locate the correct protocol
		PDMLProto= PDMLField->ParentProto;

		while (PDMLProto)
		{
			if (strcmp(PDMLProto->Name, ProtoName) == 0)
				break;

			PDMLProto= PDMLProto->PreviousProto;
		}

		if (PDMLProto == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "We are not able to locate the requested protocol within the PDML fragment.");
			*PDMLLocatedField= NULL;
			return nbFAILURE;
		}

		// Locate the last child in the selected protocol
		PDMLLastField= FindLastField(PDMLProto->FirstField);
	}
	else
	{
		// Let's locate the last child within the current protocol
		PDMLLastField= FindLastField(PDMLField);
	}

	while (PDMLLastField)
	{
		// Now, I have the latest child of the tree

		// Check if this field is the one we are looking for
		// In case the field has not been completely decoded, PDMLLastField->Name can
		// point to a NULL address; so we have to prevent this case
		if ( (PDMLLastField->Name) && (strcmp(PDMLLastField->Name, FieldName) == 0) )
		{
			*PDMLLocatedField= PDMLLastField;
			return nbSUCCESS;
		}

		// Check if we our previous sibling is valid
		// If so, let's move to that field and restart the loop
		if (PDMLLastField->PreviousField)
		{
			PDMLLastField= FindLastChild(PDMLLastField->PreviousField);
			continue;
		}

		// We do not have any previous sibling.
		// So, let's move to the parent field (if it exists)
		PDMLLastField= PDMLLastField->ParentField;
	}

	*PDMLLocatedField= NULL;
	return nbWARNING;
}



/*!
	\brief Scan the field list (starting from a given point) to locate the last item

	This function scans also the sibling of 'PDMLStartField'; vice versa, the  
	FindLastChild() scans only the children of the 'PDMLStartField'.

	\param PDMLStartField: pointer to the field that is used to start the search.

	\return A pointer to the element that is the last item of the tree, or NULL if the element
	cannot be found.
*/
struct _nbPDMLField *CPDMLMaker::FindLastField(struct _nbPDMLField *PDMLStartField)
{
struct _nbPDMLField *PDMLLastField;

	PDMLLastField= PDMLStartField;

repeat:
	// Locate the last sibling of the current field
	while ( (PDMLLastField) && (PDMLLastField->NextField) )
		PDMLLastField= PDMLLastField->NextField;

	// Check if the last sibling has still some children
	if ( (PDMLLastField) && (PDMLLastField->FirstChild) )
	{
		PDMLLastField= PDMLLastField->FirstChild;
		goto repeat;
	}

	return PDMLLastField;
}


/*!
	\brief Scan the field list (starting from a given point) to locate the last child

	This function does not scan the sibling of 'PDMLStartField'; vice versa, the  
	FindLastField() scans also the children of the 'PDMLStartField'.

	\param PDMLStartField: pointer to the field that is used to start the search.

	\return A pointer to the element that is the last item of the tree, or NULL if the element
	cannot be found.
*/
struct _nbPDMLField *CPDMLMaker::FindLastChild(struct _nbPDMLField *PDMLStartField)
{
struct _nbPDMLField *PDMLLastField;

		PDMLLastField= PDMLStartField;

repeat:
		// Check if the last sibling has still some children
		if ( (PDMLLastField) && (PDMLLastField->FirstChild) )
		{
			PDMLLastField= PDMLLastField->FirstChild;

			while ( (PDMLLastField) && (PDMLLastField->NextField) )
				PDMLLastField= PDMLLastField->NextField;

			goto repeat;
		}

		return PDMLLastField;
}


/*!
	\brief Checks the wanted attribute within a field node.

	\param AttribCode: internal code referred to the attribute that has to be checked.
	\param PDMLField: pointer to the field in which we want to locate the attribute.

	\return It returns the value of the wanted attribute, or NULL if the
	attribute has not been found.

	\warning This function is not thread-safe, although there should not be any problem because
	this refers only to the cases in which we want to know the size and the position of a given field.
*/
char *CPDMLMaker::GetPDMLFieldAttribute(int AttribCode, struct _nbPDMLField *PDMLField)
{
static char DataBuffer[NETPDL_MAX_STRING];

	switch (AttribCode)
	{
		case nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_VALUE: return PDMLField->Value;
		case nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_NAME: return PDMLField->Name;
		case nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_LONGNAME: return PDMLField->LongName;
		case nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SHOWMAP: return PDMLField->ShowMap;
		case nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SHOWDTL: return PDMLField->ShowDetails;
		case nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SHOW: return PDMLField->ShowValue;
		case nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_MASK: return PDMLField->Mask;
		case nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_POSITION: 
		{
			ssnprintf(DataBuffer, sizeof(DataBuffer), "%d", PDMLField->Position);
			return DataBuffer;
		}
		case nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SIZE: 
		{
			ssnprintf(DataBuffer, sizeof(DataBuffer), "%d", PDMLField->Size);
			return DataBuffer;
		}
	}

	return NULL;
}


/*!
	\brief Checks the wanted attribute within a protocol node.

	\param AttribCode: internal code of the attribute that has to be checked.
	\param PDMLProto: pointer to the protocol in which we want to locate the attribute.

	\return It returns the value of the wanted attribute, or NULL if the
	attribute has not been found.

	\warning This function is not thread-safe, although there should not be any problem because
	this refers only to the cases in which we want to know the size and the position of a given field.
*/
char *CPDMLMaker::GetPDMLProtoAttribute(int AttribCode, struct _nbPDMLProto *PDMLProto)
{
static char DataBuffer[NETPDL_MAX_STRING];

	switch (AttribCode)
	{
		case nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_NAME: return PDMLProto->Name;
		case nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_LONGNAME: return PDMLProto->LongName;
		case nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_POSITION: 
		{
			ssnprintf(DataBuffer, sizeof(DataBuffer), "%d", PDMLProto->Position);
			return DataBuffer;
		}
		case nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SIZE: 
		{
			ssnprintf(DataBuffer, sizeof(DataBuffer), "%d", PDMLProto->Size);
			return DataBuffer;
		}
	}

	return NULL;
}



/*!
	\brief It dumps the entire content of a _nbPDMLPacket into an ascii buffer.

	\param PDMLPacket: pointer to the packet that has to be dumped.

	\param AsciiBuffer: pointer to the CAsciiBuffer class, which will contain the dumped packet.

	\param IsVisExtRequired: flag that defines if we want to dump only 'raw' data
	(i.e. basically field value/length) or we want also visualization primitives.

	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.

	\return nbSUCCESS if everthing is fine, nbFAILURE in case of erors.
	The error message will be returned in the ErrBuf parameter.

	\note This function has been declared as 'static' because it is called also from other contexts.
*/
int CPDMLMaker::DumpPDMLPacket(struct _nbPDMLPacket *PDMLPacket, int IsVisExtRequired, int GenerateRawDump, CAsciiBuffer *AsciiBuffer, char *ErrBuf, int ErrBufSize)
{
struct _nbPDMLProto *ProtoItem;
struct tm *Time;
char TimeString[1024];

	// Format timestamp
	// We have to use this timesec variable in order to avoid a problem with VS2005, in which
	// localtime requires a 64bit value
	time_t timesec;
	timesec= (long) PDMLPacket->TimestampSec;
	Time= localtime( &timesec );

	strftime(TimeString, sizeof(TimeString), "%H:%M:%S", Time);

	if (AsciiBuffer->AppendFormatted("<" PDML_PACKET " " PDML_FIELD_ATTR_NAME_NUM "=\"%d\" "
			PDML_FIELD_ATTR_NAME_LENGTH "=\"%d\" " PDML_FIELD_ATTR_NAME_CAPLENGTH "=\"%d\" "
			PDML_FIELD_ATTR_NAME_TIMESTAMP "=\"%d.%.6d\" >\n",
			PDMLPacket->Number, PDMLPacket->Length, PDMLPacket->CapturedLength,
			PDMLPacket->TimestampSec, PDMLPacket->TimestampUSec) == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
		return nbFAILURE;
	}


	// Let's dump the content of 'standard' protocols
	ProtoItem= PDMLPacket->FirstProto;

	while(ProtoItem)
	{
		// 'proto'
		if (AsciiBuffer->AppendFormatted(
			"<" PDML_PROTO " " PDML_FIELD_ATTR_NAME "=\"%s\" ", ProtoItem->Name) == nbFAILURE)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
			return nbFAILURE;
		}

		if ((IsVisExtRequired) && (ProtoItem->LongName))
		{
			if (AsciiBuffer->AppendFormatted(
				PDML_FIELD_ATTR_LONGNAME "=\"%s\" ",ProtoItem->LongName) == nbFAILURE)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
				return nbFAILURE;
			}
		}

		if (AsciiBuffer->AppendFormatted(
			PDML_FIELD_ATTR_POSITION "=\"%d\" "
			PDML_FIELD_ATTR_SIZE "=\"%d\">\n",
			ProtoItem->Position, ProtoItem->Size) == nbFAILURE)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
			return nbFAILURE;
		}

		if (ProtoItem->FirstField)
		{
			if (DumpPDMLFields(ProtoItem->FirstField, AsciiBuffer, ErrBuf, ErrBufSize) == nbFAILURE)
				return nbFAILURE;
		}

		// Put 'proto' ending tag
		if (AsciiBuffer->AppendFormatted("</" PDML_PROTO ">\n") == nbFAILURE)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
			return nbFAILURE;
		}

		ProtoItem= ProtoItem->NextProto;
	}

	if (GenerateRawDump)
	{
	char HexDumpAscii[NETPDL_MAX_PACKET * 2 + 1];

		// Put 'dump' starting tag
		if (AsciiBuffer->AppendFormatted("<" PDML_DUMP ">") == nbFAILURE)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
			return nbFAILURE;
		}

		// Convert the hex dump into a binary string and dump it in our buffer
		if (ConvertHexDumpBinToHexDumpAscii((char *) PDMLPacket->PacketDump, PDMLPacket->CapturedLength,
					1 /* network byte order*/, HexDumpAscii, sizeof(HexDumpAscii)) == nbFAILURE)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Internal error: buffer tool small.");
			return nbFAILURE;
		}	

		if (AsciiBuffer->AppendFormatted("%s", HexDumpAscii) == nbFAILURE)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
			return nbFAILURE;
		}	

		// Put 'dump' ending tag
		if (AsciiBuffer->AppendFormatted("</" PDML_DUMP ">\n") == nbFAILURE)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
			return nbFAILURE;
		}
	}

	if (AsciiBuffer->AppendFormatted("</" PDML_PACKET ">\n") == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
		return nbFAILURE;
	}

	return nbSUCCESS;
}


/*!
	\brief It dumps the list of fields from this one to the last one of the current tree into an ascii buffer.

	\param PDMLField: first field that has to be dumped.

	\param AsciiBuffer: pointer to the CAsciiBuffer class, which will contain the dumped packet.

	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.

	\return nbSUCCESS if everthing is fine, nbFAILURE in case of errors.
	The error message will be returned in the ErrBuf parameter.

	\note This function has been declared as 'static' because it is called also from other contexts.
*/
int CPDMLMaker::DumpPDMLFields(_nbPDMLField *PDMLField, CAsciiBuffer *AsciiBuffer, char *ErrBuf, int ErrBufSize)
{
	while(PDMLField)
	{
		// Check if this is a field or a block
		if  (PDMLField->isField)
		{
			if (AsciiBuffer->AppendFormatted("<" PDML_FIELD " " PDML_FIELD_ATTR_NAME "=\"%s\" ", PDMLField->Name) == nbFAILURE)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
				return nbFAILURE;
			}
		} else {
			if (AsciiBuffer->AppendFormatted("<" PDML_BLOCK " " PDML_FIELD_ATTR_NAME "=\"%s\" ", PDMLField->Name) == nbFAILURE)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
				return nbFAILURE;
			}
		}

		if (PDMLField->LongName)
		{
			if (AsciiBuffer->AppendFormatted(PDML_FIELD_ATTR_LONGNAME "=\"%s\" ", PDMLField->LongName) == nbFAILURE)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
				return nbFAILURE;
			}
		}

		if (AsciiBuffer->AppendFormatted(
								PDML_FIELD_ATTR_SIZE "=\"%d\" "
								PDML_FIELD_ATTR_POSITION "=\"%d\" ",
								PDMLField->Size, PDMLField->Position) == nbFAILURE)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
			return nbFAILURE;
		}

		if (PDMLField->Value)
		{
			if (AsciiBuffer->AppendFormatted(PDML_FIELD_ATTR_VALUE "=\"%s\" ", PDMLField->Value) == nbFAILURE)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
				return nbFAILURE;
			}
		}

		if (PDMLField->Mask)
		{
			if (AsciiBuffer->AppendFormatted(PDML_FIELD_ATTR_MASK "=\"%s\" ", PDMLField->Mask) == nbFAILURE)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
				return nbFAILURE;
			}
		}

		if (PDMLField->ShowValue)
		{
			if (AsciiBuffer->AppendFormatted(PDML_FIELD_ATTR_SHOW "=\"%s\" ", PDMLField->ShowValue) == nbFAILURE)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
				return nbFAILURE;
			}
		}
		if (PDMLField->ShowDetails)
		{
			if (AsciiBuffer->AppendFormatted(PDML_FIELD_ATTR_SHOWDTL "=\"%s\" ", PDMLField->ShowDetails) == nbFAILURE)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
				return nbFAILURE;
			}
		}
		if (PDMLField->ShowMap)
		{
			if (AsciiBuffer->AppendFormatted(PDML_FIELD_ATTR_SHOWMAP "=\"%s\" ", PDMLField->ShowMap) == nbFAILURE)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
				return nbFAILURE;
			}
		}

		if (PDMLField->FirstChild)
		{
			if (AsciiBuffer->AppendFormatted(">\n") == nbFAILURE)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
				return nbFAILURE;
			}

			if (DumpPDMLFields(PDMLField->FirstChild, AsciiBuffer, ErrBuf, ErrBufSize) == nbFAILURE)
				return nbFAILURE;

			// Put 'field' ending tag
			if (PDMLField->isField)
			{
				if (AsciiBuffer->AppendFormatted("</" PDML_FIELD ">\n") == nbFAILURE)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
					return nbFAILURE;
				}
			} else {
				if (AsciiBuffer->AppendFormatted("</" PDML_BLOCK ">\n") == nbFAILURE)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
					return nbFAILURE;
				}
			}
		}
		else
		{
			// Put 'field' ending tag
			if (AsciiBuffer->AppendFormatted("/>\n") == nbFAILURE)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", AsciiBuffer->GetLastError() );
				return nbFAILURE;
			}
		}

		PDMLField= PDMLField->NextField;
	}

	return nbSUCCESS;
}

