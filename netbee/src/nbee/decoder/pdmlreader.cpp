/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <string.h>

#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>

#include <nbprotodb.h>
#include <nbprotodb_defs.h>

#include "netpdldecoder.h"
#include "pdmlreader.h"
#include "../utils/netpdlutils.h"
#include "../globals/utils.h"
#include "../globals/debug.h"
#include "../globals/globals.h"


extern struct _nbNetPDLDatabase *NetPDLDatabase;


//! Default constructor.
CPDMLReader::CPDMLReader()
{
	// Initialize main variables
	m_protoList= NULL;
	m_fieldsList= NULL;

	m_maxNumProto= 20;
	m_maxNumFields= 400;
}


//! Default destructor
CPDMLReader::~CPDMLReader()
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
	\brief Initializes the variables contained into this class.

	\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
	The error message will be returned in the ErrBuf parameter.
*/
int CPDMLReader::Initialize()
{
unsigned int i;

	// Allocate various ascii buffers
	if (m_asciiBuffer.Initialize() == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "%s", m_asciiBuffer.GetLastError() );
		return nbFAILURE;
	}

	// Allocates a new vector for containing the pointers to the protocol list
	m_protoList= new _nbPDMLProto* [m_maxNumProto];
	if (m_protoList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to allocate the protolist buffer.");
		return nbFAILURE;
	}
	// Allocate items in the protocol list
	for (i= 0; i< m_maxNumProto; i++)
	{
		m_protoList[i]= new _nbPDMLProto;
		if (m_protoList[i] == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to allocate a protolist item.");
			return nbFAILURE;
		}
	}

	// Allocates a new vector for containing the pointers to the Fields list
	m_fieldsList= new _nbPDMLField* [m_maxNumFields];
	if (m_fieldsList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to allocate the fieldlist buffer.");
		return nbFAILURE;
	}
	// Allocate items in the field list
	for (i= 0; i< m_maxNumFields; i++)
	{
		m_fieldsList[i]= new _nbPDMLField;
		if (m_fieldsList[i] == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to allocate a fieldlist item.");
			return nbFAILURE;
		}
	}

	return CPxMLReader::Initialize();
}


/*!
	\brief It inizializes the source file when the user wants to read a PDML file on disk.

	Upon returning from this method, this class will have the entire PDML file 'parsed',
	i.e. an array will contain the starting offset of any PDML fragment as it is on the file on disk.
	This is not a real parsing sice we still have each PDML fragment in ascii, instead of
	being properly formatted with the proper internal structures.
	Howewever, this will allow the user to ask for a specific packet; at that time we will parse
	the specific PDML fragment (with DOM) and we return it properly formatted the user.

	\param FileName: string containing the PDML file to open.
	
	\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPDMLReader::InitializeSource(char *FileName)
{
char StartTag[NETPDL_MAX_STRING];
int BufferPtr;
int StartTagLength;
unsigned long CurrentOffset;
char *AsciiBuffer;

	if (m_NetPDLDecodingEngine)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
				"The nbPDMLReader has already been configured to manage packets returned by "
				"the nbPacketDecoder. If you want to read a file, you have to use another nbPDMLReader instance.");
		return nbFAILURE;
	}

	/*
		Here we have two choices in parsing the document:
		- parse the whole file at once, with Xerces native code
		- parse each packet within a different instance.

		The first method is very fast (test showed 1/4 of the time compared to the second method), but
		the memory consumption is very very high.
		So, in order to make everything scalable, this implementation parses each packet within 
		different DOM documents, and we keep in memory only what we need for parsing the current packet.
	*/

	m_sourceOnDiskFileHandle= fopen(FileName, "r+b");
	if (m_sourceOnDiskFileHandle == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Unable to open the PDML file.");
		return nbFAILURE;
	}

	// Format a string with the starting tag we have to look for (<packet>)
	StartTagLength= ssnprintf(StartTag, sizeof(StartTag), "<%s ", PDML_PACKET);

	BufferPtr= 0;
	CurrentOffset= 0;
	AsciiBuffer= m_asciiBuffer.GetStartBufferPtr();

	while( !feof( m_sourceOnDiskFileHandle ) )
	{
	unsigned int ReadBytes;
	char *TagPtr;

		// Read data from file; we have a buffer with size m_asciiBuffer.GetBufferTotSize()
		ReadBytes= (int) fread(&AsciiBuffer[BufferPtr], sizeof(char), m_asciiBuffer.GetBufferTotSize() - 1 - BufferPtr, m_sourceOnDiskFileHandle);
		AsciiBuffer[BufferPtr + ReadBytes]= 0;
		
		// Let's locate all the '<packet ' contained in this buffer
		TagPtr= AsciiBuffer;
		while ((TagPtr= strstr(TagPtr, StartTag) ))
		{
			m_packetList[m_currNumPackets]= (int) (CurrentOffset + TagPtr - AsciiBuffer);
			m_currNumPackets++;

			// Check if the vector is enough; if not, let's allocate a new one, and let's hope it is enough
			if (CheckPacketListSize() == nbFAILURE)
				return nbFAILURE;

			TagPtr++;
		}

		// Check if presumably we have still something to read from file
		if (ReadBytes == (m_asciiBuffer.GetBufferTotSize() - 1 - BufferPtr))
		{
			// Copy some of the bytes at the end of the buffer to the beginning 
			// of the new one, so that we can speed up the process (strstr() can scan a contiguous buffer)
			strncpy(AsciiBuffer, &AsciiBuffer[m_asciiBuffer.GetBufferTotSize() - StartTagLength], StartTagLength - 1);

			BufferPtr= StartTagLength - 1;
			CurrentOffset= CurrentOffset + (m_asciiBuffer.GetBufferTotSize() - StartTagLength);
		}
	}

	if (m_currNumPackets == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), 
			"Initializing a PDML file with no packets in it. Are you sure the input data is correct?.");
		return nbFAILURE;
	}

	// If we have N packet, we have (N+1) meaningful cells within this array
	m_packetList[m_currNumPackets]= ftell(m_sourceOnDiskFileHandle) ;

	return nbSUCCESS;
}


// Documented in the base class
int CPDMLReader::GetCurrentPacket(_nbPDMLPacket **PDMLPacket)
{
	if (m_NetPDLDecodingEngine)
	{
		// If our source is the Decoding Engine, we can access to some internal structures of the PDMLMaker
		*PDMLPacket= &( ((CNetPDLDecoder *)m_NetPDLDecodingEngine)->m_PDMLMaker->m_packetSummary);

		return nbSUCCESS;
	}

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
		"The current PDML item cannot be returned when the PDML source is a file.");

	return nbFAILURE;
}


// Documented in the base class
int CPDMLReader::GetCurrentPacketXML(char* &PacketPtr, unsigned int &PacketLength)
{
	if (m_NetPDLDecodingEngine)
	{
		m_asciiBuffer.ClearBuffer(true /* resizing permitted */);

		// If our source is the Decoding Engine, we can access to some internal structures of the PDMLMaker
		if (CPDMLMaker::DumpPDMLPacket( &( ((CNetPDLDecoder *)m_NetPDLDecodingEngine)->m_PDMLMaker->m_packetSummary),
			((CNetPDLDecoder *)m_NetPDLDecodingEngine)->m_PDMLMaker->m_isVisExtRequired,
			((CNetPDLDecoder *)m_NetPDLDecodingEngine)->m_PDMLMaker->m_generateRawDump,
				&m_asciiBuffer, m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;

		PacketPtr= m_asciiBuffer.GetStartBufferPtr();
		PacketLength= m_asciiBuffer.GetBufferCurrentSize();

		return nbSUCCESS;
	}

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
		"The current PDML item cannot be returned when the PDML source is a file.");

	return nbFAILURE;
}


// Documented in the base class
int CPDMLReader::GetPacket(unsigned long PacketNumber, struct _nbPDMLPacket **PDMLPacket)
{
int NumProtos;
unsigned int AsciiBufferLen;
DOMDocument *PDMLDOMFragment;
int RetVal;

	// Initialize a variable whose scope is limited to the current packet
	m_currNumFields= 0;

	// Let's initialize it to NULL, just in case
	*PDMLPacket= NULL;

	// If our source is the Decoding Engine, we can retrieve PDML packets from within the PDMLMaker
	// (which is currently storing PDML fragments to disk)
	RetVal= GetXMLPacketFromDump(PacketNumber, m_asciiBuffer.GetStartBufferPtr(), m_asciiBuffer.GetBufferTotSize(), AsciiBufferLen);
	if ((RetVal == nbWARNING) || (RetVal == nbFAILURE))
		return RetVal;

	// Parse the XML string and return a pointer to the list of '<proto>' contained into it
	PDMLDOMFragment= ParseMemBuf(m_asciiBuffer.GetStartBufferPtr(), AsciiBufferLen);

	// Update packet summary
	if (FormatPacketSummary(PDMLDOMFragment) == nbFAILURE)
		return nbFAILURE;

	// Now, let's get the raw packet dump
	if (FormatDumpItem(PDMLDOMFragment, &m_asciiBuffer, m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
		return nbFAILURE;

	NumProtos= FormatProtoItem(PDMLDOMFragment);

	if (NumProtos!= nbFAILURE)
	{
		// Update the pointer to the caller
		*PDMLPacket= &m_packetSummary;
		return NumProtos;
	}

	// Now, let's parse the <proto> contained into it
	return FormatProtoItem(PDMLDOMFragment);
}


// Documented in the base class
int CPDMLReader::GetPacketXML(unsigned long PacketNumber, char* &PacketPtr, unsigned int &PacketLength)
{
int RetVal;

	// Initialize a variable whose scope is limited to the current packet
	m_currNumFields= 0;

	// If our source is the Decoding Engine, we can retrieve PDML packets from within the PDMLMaker
	// (which is currently storing PDML fragments to disk)
	RetVal= GetXMLPacketFromDump(PacketNumber, m_asciiBuffer.GetStartBufferPtr(), m_asciiBuffer.GetBufferTotSize(), PacketLength);
	PacketPtr= m_asciiBuffer.GetStartBufferPtr();

	// Return nbSUCCESS in any case
	return RetVal;
}


/*!
	\brief It gets a list of DOM nodes containing a protocol, and formats them into _nbPDMLProto structures.

	This function converts each protocol and all its child fields. This function will call the FormatFieldsItem()
	for this purpose.

	\param PDMLDocument The DOMDocument associated to the current PDML packet.

	\return The number of '&lt;proto&gt;' that have been copied in the returned buffer, 
	nbFAILURE if some error occurred.
	The error message is stored in the m_errbuf internal buffer.
*/
int CPDMLReader::FormatProtoItem(DOMDocument *PDMLDocument)
{
DOMNodeList *ProtoList;
XMLCh TempBuffer[NETPDL_MAX_STRING + 1];
unsigned int ProtoNumber;
unsigned int TotNumProto;

	XMLString::transcode(PDML_PROTO, TempBuffer, NETPDL_MAX_STRING);

	// After parsing the '<packet>' fragment, let's list the <proto> contained into it
	ProtoList= PDMLDocument->getElementsByTagName(TempBuffer);

	// Get the number of protocols contained in this packet
	TotNumProto= ProtoList->getLength();

	// Check if the protocol structures allocated previously are enough. If not, let's allocate new structures
	if (TotNumProto >= m_maxNumProto)
	{
		if (UpdateProtoList(&m_maxNumProto, &m_protoList, m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;
	}

	// Reset the buffer
	m_asciiBuffer.ClearBuffer(false /* resizing not permitted */);

	for (ProtoNumber= 0; ProtoNumber < TotNumProto; ProtoNumber++)
	{
	DOMElement *ProtoItem;

		// Set everything to zero
		memset(m_protoList[ProtoNumber], 0, sizeof(_nbPDMLProto));

		// Update pointer to the packet summary
		m_protoList[ProtoNumber]->PacketSummary= &m_packetSummary;

		// Updates the protocol chain
		if (ProtoNumber >= 1)
		{
			m_protoList[ProtoNumber - 1]->NextProto= m_protoList[ProtoNumber];
			m_protoList[ProtoNumber]->PreviousProto= m_protoList[ProtoNumber - 1];
		}

		ProtoItem= (DOMElement *) ProtoList->item(ProtoNumber);

		// Copies all the attributes of this proto in the new structure
 		if (AppendItemString(ProtoItem, PDML_FIELD_ATTR_NAME,
			&(m_protoList[ProtoNumber]->Name), &m_asciiBuffer, m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;
 		if (AppendItemString(ProtoItem, PDML_FIELD_ATTR_LONGNAME,
			&(m_protoList[ProtoNumber]->LongName), &m_asciiBuffer, m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;
		if (AppendItemLong(ProtoItem, PDML_FIELD_ATTR_SIZE,
			&(m_protoList[ProtoNumber]->Size), m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;
		if (AppendItemLong(ProtoItem, PDML_FIELD_ATTR_POSITION,
			&(m_protoList[ProtoNumber]->Position), m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;

#ifdef DOM_DEBUG
		// FULVIO This code is still present, because it should be needed in order to solve
		// a bug discovered on Dec 4, 2006
		// See my email in the mailbox
		// get a serializer, an instance of DOMWriter
		XMLCh tempStr[100];
		XMLString::transcode("LS", tempStr, 99);

		DOMImplementation *impl          = DOMImplementationRegistry::getDOMImplementation(tempStr);
		DOMWriter         *theSerializer = ((DOMImplementationLS*)impl)->createDOMWriter();

		MemBufFormatTarget myFormTarget;

		theSerializer->writeNode(&myFormTarget, *ProtoItem);

		const XMLByte *Result= myFormTarget.getRawBuffer();

		delete theSerializer;
#endif

		if (ProtoItem->hasChildNodes() )
		{
			if (FormatFieldNodes(ProtoItem->getFirstChild(), m_protoList[ProtoNumber], NULL) == nbFAILURE)
				return nbFAILURE;
		}
	}

	// Update the pointer to the first protocol
	if (TotNumProto > 0)
		m_packetSummary.FirstProto= m_protoList[0];
	else
		m_packetSummary.FirstProto= NULL;

	return (int) ProtoNumber;
}


/*!
	\brief It gets an DOMDocument containing the PDML fragment related to a single packet and formats the packet header.

	This function parses the DOMDocument that contains a PDML fragment related to a single packet
	and it fills the proper members of the m_packetSummary structure.

	\param PDMLDocument The DOMDocument associated to the current PDML packet.

	\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
	In case of errors, the error message is stored in the m_errbuf internal buffer.
*/
int CPDMLReader::FormatPacketSummary(DOMDocument *PDMLDocument)
{
char TempAsciiBuffer[NETPDL_MAX_STRING + 1];
XMLCh TempXMLBuffer[NETPDL_MAX_STRING + 1];
const XMLCh *TempPtr;
DOMNodeList *PacketList;
DOMNode *PacketNode;


	// Let's get the pointer to a 'packet' node
	XMLString::transcode(PDML_PACKET, TempXMLBuffer, NETPDL_MAX_STRING);
	PacketList= PDMLDocument->getElementsByTagName(TempXMLBuffer);
	PacketNode= PacketList->item(0);

	if (PacketNode == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"The PDML dump does not contain the '%s' element.", PDML_PACKET);

		return nbFAILURE;
	}

	// Ok, now we can extract attributes
	XMLString::transcode(PDML_FIELD_ATTR_NAME_NUM, TempXMLBuffer, NETPDL_MAX_STRING);

	TempPtr= ((DOMElement *) PacketNode)->getAttribute(TempXMLBuffer);

	// If the XML element contains that attribute, transform it in ascii and return it to the caller
	if (TempPtr)
	{
		XMLString::transcode(TempPtr, TempAsciiBuffer, NETPDL_MAX_STRING);
		m_packetSummary.Number= atoi(TempAsciiBuffer);
	}

	XMLString::transcode(PDML_FIELD_ATTR_NAME_LENGTH, TempXMLBuffer, NETPDL_MAX_STRING);
	TempPtr= ((DOMElement *) PacketNode)->getAttribute(TempXMLBuffer);
	if (TempPtr)
	{
		XMLString::transcode(TempPtr, TempAsciiBuffer, NETPDL_MAX_STRING);
		m_packetSummary.Length= atoi(TempAsciiBuffer);
	}

	XMLString::transcode(PDML_FIELD_ATTR_NAME_CAPLENGTH, TempXMLBuffer, NETPDL_MAX_STRING);
	TempPtr= ((DOMElement *) PacketNode)->getAttribute(TempXMLBuffer);
	if (TempPtr)
	{
		XMLString::transcode(TempPtr, TempAsciiBuffer, NETPDL_MAX_STRING);
		m_packetSummary.CapturedLength= atoi(TempAsciiBuffer);
	}

	XMLString::transcode(PDML_FIELD_ATTR_NAME_TIMESTAMP, TempXMLBuffer, NETPDL_MAX_STRING);
	TempPtr= ((DOMElement *) PacketNode)->getAttribute(TempXMLBuffer);
	if (TempPtr)
	{
		XMLString::transcode(TempPtr, TempAsciiBuffer, NETPDL_MAX_STRING);
		sscanf(TempAsciiBuffer, "%ld.%ld", &m_packetSummary.TimestampSec, &m_packetSummary.TimestampUSec);
	}

	return nbSUCCESS;
}


/*!
	\brief In case the protocol list is not enough, it allocates a new protocol list which is 10 times bigger.

	\param CurrentNumProto: pointer to a variable that keeps the current size of the CurrentProtoList vector.
	When the function returns, the value of this pointer will contain the new number of items in the new vector.

	\param CurrentProtoList: pointer to the current Protocol List vector.
	When the function returns, the value of this pointer will point to the new vector. The old one will be deallocated 
	by this function.

	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.

	\return nbSUCCESS if everything is fine, nbFAILURE if something goes wrong.
	The error message will be returned in the 'ErrBuf' buffer.
	
	\note This function has been declared as 'static' because it is called also from other contexts.
*/
int CPDMLReader::UpdateProtoList(unsigned long *CurrentNumProto, _nbPDMLProto ***CurrentProtoList, char *ErrBuf, int ErrBufSize)
{
unsigned long NewSize;

	NewSize= *CurrentNumProto * 10;

	_nbPDMLProto **newVector= new _nbPDMLProto* [NewSize];
	// Check if something goes wrong
	if (newVector == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory to allocate the new protolist buffer.");
		return nbFAILURE;
	}

	unsigned long i;
	for (i=0; i < *CurrentNumProto; i++)
		newVector[i]= (*CurrentProtoList)[i];

	// Delete old buffer
	delete (*CurrentProtoList);

	// Update the pointer to the protocol list
	(*CurrentProtoList)= newVector;

	// Allocate items in the protocol list
	for(i= *CurrentNumProto; i < NewSize; i++)
	{
		newVector[i]= new _nbPDMLProto;
		if (newVector[i] == NULL)
		{
			// Update the number of protocols (taking into account the right number of allocations)
			*CurrentNumProto= i - 1;

			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
				"Not enough memory to allocate a protolist item.");

			return nbFAILURE;
		}
	}

	// Update the number of protocols
	*CurrentNumProto= NewSize;

	return nbSUCCESS;
}


/*!
	\brief In case the fields list is not enough, it allocates a new fields list which is 10 times bigger.

	\param CurrentNumFields: pointer to a variable that keeps the current size of the CurrentFieldsList vector.
	When the function returns, the value of this pointer will contain the new number of items in the new vector.

	\param CurrentFieldsList: pointer to the current Fields List vector.
	When the function returns, the value of this pointer will point to the new vector. The old one will be deallocated 
	by this function.

	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.

	\return nbSUCCESS if everything is fine, nbFAILURE if something goes wrong.
	The error message will be returned in the 'ErrBuf' buffer.

	\note This function has been declared as 'static' because it is called also from other contexts.
*/
int CPDMLReader::UpdateFieldsList(unsigned long *CurrentNumFields, _nbPDMLField ***CurrentFieldsList, char *ErrBuf, int ErrBufSize)
{
unsigned long NewSize;

	NewSize= *CurrentNumFields * 10;

	_nbPDMLField **newVector= new _nbPDMLField* [NewSize];
	// Check if something goes wrong
	if (newVector == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Not enough memory to allocate the new fieldslist buffer.");
		return nbFAILURE;
	}

	unsigned long i;
	for (i=0; i < *CurrentNumFields; i++)
		newVector[i]= (*CurrentFieldsList)[i];

	// Delete old buffer
	delete (*CurrentFieldsList);

	// Update the pointer to the fields list
	(*CurrentFieldsList)= newVector;

	// Allocate items in the protocol list
	for(i= *CurrentNumFields; i < NewSize; i++)
	{
		newVector[i]= new _nbPDMLField;
		if (newVector[i] == NULL)
		{
			// Update the number of fields (taking into account the right number of allocations)
			*CurrentNumFields= i - 1;

			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
				"Not enough memory to allocate a fieldslist item.");

			return nbFAILURE;
		}
	}

	// Update the number of fields
	*CurrentNumFields= NewSize;

	return nbSUCCESS;
}



/*!
	\brief It gets a pointer to a DOMNode containing a field, and it formats it (and its children) into _nbPDMLFields structures.

	This function convert each field, all its sibling and all its children. It loops through the siblings and
	it calls itself recursively in case the field has children

	\param CurrentField: a pointer to the first element that contains a &lt;field&gt; element.
	\param ParentProto: pointer to the _nbPDMLProto that contains the given field.
	\param ParentField: pointer to the parent field, if any.

	\return nbSUCCESS if everything is fine, or nbFAILURE in case or errors.
	The error message is stored in the m_errbuf internal buffer.
*/
int CPDMLReader::FormatFieldNodes(DOMNode *CurrentField, struct _nbPDMLProto *ParentProto, struct _nbPDMLField *ParentField)
{
struct _nbPDMLField *PreviousField;

	while (	(CurrentField) && (CurrentField->getNodeType() != DOMNode::ELEMENT_NODE) )
		CurrentField= CurrentField->getNextSibling();

	PreviousField= NULL;

	while (CurrentField)
	{
		// Set everything to zero
		memset(m_fieldsList[m_currNumFields], 0, sizeof(_nbPDMLField));

		// Copies all the attributes of this proto in the new structure
 		if (AppendItemString(CurrentField, PDML_FIELD_ATTR_NAME,
			&(m_fieldsList[m_currNumFields]->Name), &m_asciiBuffer, m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;
 		if (AppendItemString(CurrentField, PDML_FIELD_ATTR_LONGNAME,
			&(m_fieldsList[m_currNumFields]->LongName), &m_asciiBuffer, m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;
 		if (AppendItemString(CurrentField, PDML_FIELD_ATTR_SHOW,
			&(m_fieldsList[m_currNumFields]->ShowValue), &m_asciiBuffer, m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;
 		if (AppendItemString(CurrentField, PDML_FIELD_ATTR_SHOWDTL,
			&(m_fieldsList[m_currNumFields]->ShowDetails), &m_asciiBuffer, m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;
 		if (AppendItemString(CurrentField, PDML_FIELD_ATTR_SHOWMAP,
			&(m_fieldsList[m_currNumFields]->ShowMap), &m_asciiBuffer, m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;
 		if (AppendItemString(CurrentField, PDML_FIELD_ATTR_VALUE,
			&(m_fieldsList[m_currNumFields]->Value), &m_asciiBuffer, m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;
		if (AppendItemString(CurrentField, PDML_FIELD_ATTR_MASK,
			&(m_fieldsList[m_currNumFields]->Mask), &m_asciiBuffer, m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;
		if (AppendItemLong(CurrentField, PDML_FIELD_ATTR_SIZE,
			&(m_fieldsList[m_currNumFields]->Size), m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;
		if (AppendItemLong(CurrentField, PDML_FIELD_ATTR_POSITION,
			&(m_fieldsList[m_currNumFields]->Position), m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
			return nbFAILURE;

		// Update the pointer to the parent protocol
		m_fieldsList[m_currNumFields]->ParentProto= ParentProto;

		// Update the pointer of the parent protocol
		// Check if we're the first field of the current protocol
		if (ParentProto->FirstField == NULL)
		{
			ParentProto->FirstField= m_fieldsList[m_currNumFields];
		}
		else
		{
			// Updates the protocol chain
			m_fieldsList[m_currNumFields]->PreviousField= PreviousField;

			if (PreviousField)
				PreviousField->NextField= m_fieldsList[m_currNumFields];
		}

		// Update the pointer to the parent child, if it exist
		if ((ParentField) && (ParentField->FirstChild == NULL))
			ParentField->FirstChild= m_fieldsList[m_currNumFields];

		m_fieldsList[m_currNumFields]->ParentField= ParentField;

		// Save a reference to this field for the future 
		PreviousField= m_fieldsList[m_currNumFields];

		// Update the number of fields used in the m_fieldList vector
		m_currNumFields++;

		// If the current field has children, let's call recursively this function in order to handle all of them
		if (CurrentField->hasChildNodes() )
		{
			if (FormatFieldNodes(CurrentField->getFirstChild(), ParentProto, m_fieldsList[m_currNumFields - 1]) == nbFAILURE)
				return nbFAILURE;
		}

		// Move to the next field
		do
		{
			CurrentField= CurrentField->getNextSibling();
		}
		while (	(CurrentField) && (CurrentField->getNodeType() != DOMNode::ELEMENT_NODE) );

		// Check if the field structures allocated previously are enough. If not, let's allocate new structures
		if (m_currNumFields >= m_maxNumFields)
		{
			if (UpdateFieldsList(&m_maxNumFields, &m_fieldsList, m_errbuf, sizeof(m_errbuf)) == nbFAILURE)
				return nbFAILURE;
		}
	}

	return nbSUCCESS;
}




/*!
	\brief It fills the member of the _nbPDMLPacket structure that is in charge of the packet dump.

	This function extracts the value of the given node, it transforms it into an ascii buffer (appropriately 
	located into the m_asciiBuffer buffer), and updates the appropriate pointer to it.

	This function updates also the pointer into the m_asciiBuffer buffer in order to point to the first
	free location in this buffer.

	\param PDMLDocument The DOMDocument associated to the current PDML packet.
	\param TmpBuffer: pointer to the CAsciiBuffer class, which manages an ascii temporary buffer in which 
	the content of the XML node has to be stored. The XML content will be stored in binary.
	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer will always be NULL terminated.
	\param ErrBufSize: size of the previous buffer.

	\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
	In case of errors, the  error message will be returned in the ErrBuf parameter.
*/
int CPDMLReader::FormatDumpItem(DOMDocument *PDMLDocument, CAsciiBuffer *TmpBuffer, char *ErrBuf, int ErrBufSize)
{
XMLCh TempBuffer[NETPDL_MAX_STRING + 1];
const XMLCh *TempPtr;
char* AsciiBufPtr;
DOMNodeList *DumpList;
DOMNode *DumpItem;
DOMNode *DumpValue;

	// Let's initialize this member, just in case
	m_packetSummary.PacketDump= NULL;

	XMLString::transcode(PDML_DUMP, TempBuffer, NETPDL_MAX_STRING);

	// Now, let's get the raw packet dump
	DumpList= PDMLDocument->getElementsByTagName(TempBuffer);
	if (DumpList == NULL)
		return nbSUCCESS;

	// Let's check if the PDML fragment contains also the packet dump. If not, let's return.
	DumpItem= DumpList->item(0);
	if (DumpItem == NULL)
		return nbSUCCESS;

	DumpValue= DumpItem->getFirstChild();
	if (DumpValue == NULL)
		return nbSUCCESS;

	TempPtr= DumpValue->getNodeValue();

	// If the XML element contains that attribute, transform it in ascii and return it to the caller
	if (TempPtr && (*TempPtr) )
	{
		AsciiBufPtr= TmpBuffer->TranscodeAndStore( (wchar_t *)(XMLCh *) TempPtr);

		if (AsciiBufPtr == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", TmpBuffer->GetLastError() );
			return nbFAILURE;
		}

		// Since we're tranforming an ascii buffer into a bin buffer (which is half the previous one)
		//  there's no problem of buffer overflow.
		ConvertHexDumpAsciiToHexDumpBin(AsciiBufPtr, (unsigned char *) AsciiBufPtr, strlen(AsciiBufPtr) + 1);

		m_packetSummary.PacketDump= (unsigned char *) AsciiBufPtr;
	}
	else	// otherwise, append 'NULL'
		m_packetSummary.PacketDump= NULL;

	return nbSUCCESS;
}


/*!
	\brief It extracts a given attribute from the current element and stores it in the appropriate ascii buffer.

	This function replaces a very annoying task. It extracts a given attribute from the current DOM element,
	transform it into an ascii buffer (appropriately located into the m_asciiBuffer buffer), and updates the
	appropriate pointer to it.

	This function updates also the pointer into the m_asciiBuffer buffer in order to point to the first free location
	in this buffer.

	\param SourceNode: pointer to the node containing the attribute that has to be extracted.
	\param TagToLookFor: name of the attribute that has to be extracted.
	\param AppendAt: pointer to the member of the _nbPDMLField/_nbPDMLProto that has to keep the attribute 
	extracted from the XML element.
	\param TmpBuffer: pointer to the CAsciiBuffer class, which manages an ascii temporary buffer in which 
	the content of the XML attribute has to be stored. The XML attribute will be stored in ASCII.
	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer will always be NULL terminated.
	\param ErrBufSize: size of the previous buffer.

	\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
	The error message will be returned in the ErrBuf parameter.

	\note This function has been declared as 'static' because it is called also from other contexts.
*/
int CPDMLReader::AppendItemString(DOMNode *SourceNode, const char *TagToLookFor, char **AppendAt, CAsciiBuffer *TmpBuffer, char *ErrBuf, int ErrBufSize)
{
const XMLCh *TempPtr;
XMLCh XMLTagToLookFor[NETPDL_MAX_STRING + 1];

	XMLString::transcode(TagToLookFor, XMLTagToLookFor, NETPDL_MAX_STRING);

	TempPtr= ((DOMElement *) SourceNode)->getAttribute(XMLTagToLookFor);

	// If the XML element contains that attribute, transform it in ascii and return it to the caller
	if (TempPtr && (*TempPtr) )
	{
		(*AppendAt)= TmpBuffer->TranscodeAndStore( (wchar_t *)(XMLCh *) TempPtr);

		if ((*AppendAt) == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", TmpBuffer->GetLastError() );
			return nbFAILURE;
		}
	}
	else	// otherwise, append 'NULL'
		(*AppendAt)= NULL;

	return nbSUCCESS;
}


/*!
	\brief It copies a given string into an ascii buffer and it modifies a pointer in order to point to it.

	This function is very similar to the other AppendItemString().

	This function updates also the pointer into the m_asciiBuffer buffer in order to point to the first free location
	in this buffer.

	\param SourceString: pointer to the ASCII string that has to be converted into a char *
	\param AppendAt: pointer to the member of the _nbPDMLField/_nbPDMLProto that has to keep the attribute 
	extracted from the XML element.
	\param TmpBuffer: pointer to the CAsciiBuffer class, which manages an ascii temporary buffer in which 
	the content of the XML attribute has to be stored. The XML attribute will be stored in ASCII.
	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer will always be NULL terminated.
	\param ErrBufSize: size of the previous buffer.

	\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
	The error message will be returned in the ErrBuf parameter.

	\note This function has been declared as 'static' because it is called also from other contexts.
*/
int CPDMLReader::AppendItemString(const char *SourceString, char **AppendAt, CAsciiBuffer *TmpBuffer, char *ErrBuf, int ErrBufSize)
{
	(*AppendAt)= TmpBuffer->Store(SourceString);

	if ((*AppendAt) == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "%s", TmpBuffer->GetLastError() );
		return nbFAILURE;
	}

	return nbSUCCESS;
}


/*!
	\brief It extracts a given attribute from the current element and fills the appropriate field in the _nbPDMLField/Proto structure.

	This function replace a very annoying task. It extracts a given attribute from the current DOM element,
	transform it into a number, and updates the appropriate member in the _nbPDMLField/_nbPDMLProto structure.

	\param SourceNode: pointer to the node containing the attribute that has to be extracted.
	\param TagToLookFor: name of the attribute that has to be extracted.
	\param AppendAt: pointer to the member of the _nbPDMLField/_nbPDMLProto that has to keep the attribute 
	extracted from the XML element.

	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.

	\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
	The error message will be returned in the ErrBuf parameter.

	\note This function has been declared as 'static' because it is called also from other contexts.
*/
int CPDMLReader::AppendItemLong(DOMNode *SourceNode, const char *TagToLookFor, unsigned long *AppendAt, char *ErrBuf, int ErrBufSize)
{
const XMLCh *TempPtr;
XMLCh XMLTagToLookFor[NETPDL_MAX_STRING + 1];

	XMLString::transcode(TagToLookFor, XMLTagToLookFor, NETPDL_MAX_STRING);

	TempPtr= ((DOMElement *) SourceNode)->getAttribute(XMLTagToLookFor);

	// If the XML element contains that attribute, transform it in ascii and return it to the caller
	if (TempPtr)
	{
	char TmpBufAscii[NETPDL_MAX_STRING + 1];

		XMLString::transcode(TempPtr, TmpBufAscii, NETPDL_MAX_STRING);

		sscanf(TmpBufAscii, "%lu", AppendAt);
		return nbSUCCESS;
	}

	(*AppendAt)= 0;
	
	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "This function was called with a wrong parameter");
	return nbFAILURE;
}


/*!
	\brief This function formats the summary data from the PDMLMaker internal format to the PDMLReader one.

	\param ItemList: pointer to the list of &lt;proto&gt; nodes that belong to the current packet.
	\param Buffer: pointer to a user-allocated buffer in which PDMLReader will put the returned ascii data.
	\param BufSize: size of previous buffer. There is no guarantee that the buffer will be NULL terminated.

	\return The number of '&lt;sections&gt;' that have been copied in the returned buffer, 
	or nbFAILURE if some error occurred.
	In case of error, the error message can be retrieved by the GetLastError() method.

	\note This function is used only when the PDML fragment has to be read from file. This happens either
	when our source is a nbDecodingEngine but we want to return previously decoded packets, or when our source
	is a file.
*/
int CPDMLReader::FormatItem(DOMNodeList *ItemList, char *Buffer, int BufSize)
{
int CopiedChars;
unsigned int ItemNumber;

	CopiedChars= 0;
	for (ItemNumber= 0; ItemNumber < ItemList->getLength(); ItemNumber++)
	{
	char TempBuffer[NETPDL_MAX_STRING + 1];
	int TempChars;

		DOMText *TextNode= (DOMText *) (ItemList->item(ItemNumber))->getFirstChild();

		// In case we have <section></section>, the TextNode is NULL, so we have to avoid following code.
		if (TextNode)
		{
			XMLString::transcode(TextNode->getNodeValue(), TempBuffer, NETPDL_MAX_STRING);
			TempChars= ssnprintf(&Buffer[CopiedChars], BufSize - CopiedChars, "%s", TempBuffer);
		}
		else
		{
			Buffer[CopiedChars]= 0;

			// Check to avoid a buffer overrun
			if (CopiedChars >= (BufSize - 1) )
				TempChars= -1;
			else
				TempChars= 0;
		}

		if (TempChars >= 0)
			CopiedChars+= (TempChars + 1); // ssnprintf does not count the terminating null character
		else
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The buffer is not big enough to contain summary items.");
			return nbFAILURE;
		}
	}

	return (int) ItemNumber;
}


/*
	\brief It accepts a XML (ascii) buffer and it parses it.

	This function is used when we have a char buffer containing an XML fragment, and we have to parse it.
	This function returns the list of PDML items (&lt;section&gt;) contained in the XML buffer.

	\param Buffer: pointer to a buffer that contains the XML fragment (in ascii).
	\param BytesToParse: number of valid bytes in the previous buffer; not NULL terminated buffers are supported as well.

	\return A pointer to the DOM document associated to the XML fragment if
	everything is fine, NULL otherwise.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
DOMDocument* CPDMLReader::ParseMemBuf(char *Buffer, int BytesToParse)
{
DOMDocument *Document;
MemBufInputSource MemBuf( (const XMLByte *) Buffer, BytesToParse, "", false);

	try
	{
		// Reset any other document previously created
		m_DOMParser->resetDocumentPool();

		// Load the description in memory and get document root
		m_DOMParser->parse(MemBuf);

		Document= m_DOMParser->getDocument();

		if ( (Document->getDocumentElement()) == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
				"Fatal error: cannot parse PDML file. Possible errors: "
				"wrong file location, or not well-formed XML file.");
			return NULL;
		}
	}
	catch (...)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Unexpected exception during PDML buffer parsing.");
		return NULL;
	}

	if (Document == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Unexpected exception during PDML buffer parsing.");
		return NULL;
	}

	return Document;
}


// Documented in the base class
int CPDMLReader::GetPDMLField(char *ProtoName, char *FieldName, struct _nbPDMLField *FirstField, struct _nbPDMLField **ExtractedField)
{
struct _nbPDMLPacket *PDMLPacket;

	if (!m_NetPDLDecodingEngine)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"This method cannot be used to return a field when the PDML source is a file.");

		return nbFAILURE;
	}

	// If our source is the Decoding Engine, we can access to some internal structures of the PDMLMaker
	PDMLPacket= &( ((CNetPDLDecoder *)m_NetPDLDecodingEngine)->m_PDMLMaker->m_packetSummary);

	if (PDMLPacket == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"No packets have been decoded so far. You must first decode a packet, then look for a specific field in it.");

		return nbFAILURE;
	}

	// Let's jump to the common code
	return GetPDMLFieldInternal(PDMLPacket, ProtoName, FieldName, FirstField, ExtractedField);
}


// Documented in the base class
int CPDMLReader::GetPDMLField(unsigned long PacketNumber, char *ProtoName, char *FieldName, struct _nbPDMLField *FirstField, struct _nbPDMLField **ExtractedField)
{
struct _nbPDMLPacket *PDMLPacket;
int Res;

	Res= GetPacket(PacketNumber, &PDMLPacket);

	if (Res == nbFAILURE)
		return nbFAILURE;

	if (Res == nbWARNING)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"The packet you are looking for is probably out of range.");
		return nbFAILURE;
	}

	// Let's jump to the common code
	return GetPDMLFieldInternal(PDMLPacket, ProtoName, FieldName, FirstField, ExtractedField);
}


/*!
	\return nbSUCCESS if everything is fine, nbFAILURE if the field cannot be found,
	nbWARNING in case there is more than one occurrence of the field.
	The error message is stored in the m_errbuf internal buffer.
*/
int CPDMLReader::GetPDMLFieldInternal(struct _nbPDMLPacket *PDMLPacket, char *ProtoName, char *FieldName, 
										struct _nbPDMLField *FirstField, struct _nbPDMLField **ExtractedField)
{
struct _nbPDMLField *Result;

	// In case the user tells us the first field to be used for beginning the scan, let's avoid this piece of code.
	// Vice versa, if the user does not know the field to begin with, let's locate the correct protocol
	// and let's point to the first protocol contained in it.
	if (FirstField == NULL)
	{
	_nbPDMLProto *CurrentPDMLProto;

		if ( (ProtoName== NULL) || (*ProtoName == 0) )
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
				"The protocol name is void. Please specify a valid protocol name.");

			return nbFAILURE;
		}

		CurrentPDMLProto= PDMLPacket->FirstProto;

		while (CurrentPDMLProto)
		{
			if (strcmp(CurrentPDMLProto->Name, ProtoName) == 0)
			{
				// The protocol we're looking for has been found; let's break this loop
				FirstField= CurrentPDMLProto->FirstField;
				break;
			}
			CurrentPDMLProto= CurrentPDMLProto->NextProto;
		}

		if (FirstField == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
				"Protocol '%s' cannot be found in current PDML fragment.", ProtoName);

			return nbFAILURE;
		}
	}
	else
	{
		// In case the user specifies the first field, let's move to the following one
		// (because the field set by the user already contains this name)
		if (FirstField->FirstChild)
			FirstField= FirstField->FirstChild;
		else
			FirstField= FirstField->NextField;
	}

	if (FirstField)
	{
		// Let's scan the fields (and allow to go up in the fields hierarchy in order to scan the whole tree)
		Result= ScanForFields(FieldName, FirstField, true);

		if (Result)
		{
			*ExtractedField= Result;
			return nbSUCCESS;
		}
		else
		{
		_nbPDMLProto *CurrentPDMLProto;

			// There are no other occurrencies of the current field in the current protocol
			// Let's see if there's another instance of the same protocol

			CurrentPDMLProto= (FirstField->ParentProto)->NextProto;

			while (CurrentPDMLProto)
			{
				if (strcmp(CurrentPDMLProto->Name, ProtoName) == 0)
				{
					// There is another instance of the same protocol in the packet; let's break this loop
					FirstField= CurrentPDMLProto->FirstField;
					break;
				}
				CurrentPDMLProto= CurrentPDMLProto->NextProto;
			}

			if (FirstField == NULL)
			{
				// We found no other instances of the same protocol. So, we can return safely
				return nbWARNING;
			}
			else
			{
				// Let's scan the fields (and allow to go up in the fields hierarchy in order to scan the whole tree)
				Result= ScanForFields(FieldName, FirstField, true);

				if (Result)
				{
					*ExtractedField= Result;
					return nbSUCCESS;
				}
				else
				{
					return nbWARNING;
				}
			}
		}
	}
	else
	{
		// Let's scan the fields without allowing to go down in the hierarchy, because we're already
		// managing fields starting from the first available field within the selected protocol, so
		// there's not need to go down in the hierarchy
		Result= ScanForFields(FieldName, FirstField, false);

		if (Result)
		{
			*ExtractedField= Result;
			return nbSUCCESS;
		}
		else
		{
			// The field we are looking for cannot be located within the selected protocol
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
				"Field '%s' (in protocol '%s') cannot be found in current PDML fragment.", FieldName, ProtoName);

			return nbFAILURE;
		}
	}
}


/*!
	\brief It scans a given PDML fragment looking for the given field.

	This function scans the PDML fields tree looking for the given field.
	This function has two running modes according to the 'AllowToGoUp' flag.

	\param FieldName: string that contains the name of the field (in the PDML file) 
	we are looking for.

	\param FirstField: this parameter indicates the first field that has to be examined.
	This function scans the given field and all the fields that are children or that follow the given one.

	\param AllowToGoUp: in case it is 'false', this function scans from the 'FirstField' onward.
	In case it is 'true', when the give subtree is ended, this function goes up in the PDML fields tree
	and scans also the fields that have still to be examined.

	The second mode is needed to locate fields that are present more than one time within the 
	current protocol.
*/
struct _nbPDMLField *CPDMLReader::ScanForFields(char *FieldName, struct _nbPDMLField *FirstField, bool AllowToGoUp)
{
struct _nbPDMLField *CurrentField= FirstField;

	while (CurrentField)
	{
		if (strcmp(CurrentField->Name, FieldName) == 0)
			return CurrentField;

		// Check if we have children
		// If so, let's scan all the children
		if (CurrentField->FirstChild)
		{
		struct _nbPDMLField *Result;

			Result= ScanForFields(FieldName, CurrentField->FirstChild, false);
			if (Result)
				return Result;
			// else let's scan the other sibling, looking for the wanted field
		}

		// There are some additional controls here
		// Since 'FirstField' can be in a arbitrary position inside the packet, we have to return
		// back in the hierarchy in order to scan all the remaining fields that are not children
		// of the given one
		if (AllowToGoUp)
		{
			// If the field has a sibling, let's move to it
			if (CurrentField->NextField)
				CurrentField= CurrentField->NextField;
			else
			{
				// if the field does not have a sibling, let's return to the parent
				// However, the parent has already been checked; so, let's move
				// to the first sibling of the parent.
				if (CurrentField->ParentField)
					CurrentField= (CurrentField->ParentField)->NextField;
				else
					CurrentField= NULL;
			}
		}
		else
		{
			// We are not allowed to go up. So, let's simply move to the next child
			CurrentField= CurrentField->NextField;
		}
	}

	return NULL;
}


