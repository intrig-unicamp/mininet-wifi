/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <string.h>

#include <xercesc/framework/MemBufInputSource.hpp>

#include <nbprotodb.h>
#include <nbprotodb_defs.h>

#include "netpdldecoder.h"
#include "psmlreader.h"
#include "../utils/netpdlutils.h"
#include "../globals/debug.h"
#include "../globals/utils.h"
#include "../globals/globals.h"



extern struct _nbNetPDLDatabase *NetPDLDatabase;



//! Default constructor.
CPSMLReader::CPSMLReader()
{
	// Initialize main variables
	m_summarySection= NULL;
}


//! Default destructor
CPSMLReader::~CPSMLReader()
{
	if (m_summarySection)
		delete m_summarySection;
}


/*!
	\brief Initializes the variables contained into this class.

	\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPSMLReader::Initialize()
{
	// Allocate various ascii buffers
	if (m_asciiBuffer.Initialize() == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "%s", m_asciiBuffer.GetLastError() );
		return nbFAILURE;
	}

	return CPxMLReader::Initialize();
}


/*!
	\brief It inizializes the source file when the user wants to read a PSML file on disk.

	Upon returning from this method, this class will have the entire PSML file 'parsed',
	i.e. an array will contain the starting offset of any PSML fragment as it is on the file on disk.
	This is not a real parsing sice we still have each PSML fragment in ascii, instead of
	being properly formatted with the proper internal structures.
	Howewever, this will allow the user to ask for a specific packet; at that time we will parse
	the specific PSML fragment (with DOM) and we return it properly formatted the user.

	\param FileName: string containing the PSML file to open.
	
	\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPSMLReader::InitializeSource(char *FileName)
{
char StartTag[NETPDL_MAX_STRING];
int BufferPtr;
int StartTagLength;
unsigned long CurrentOffset;
char *AsciiBuffer;

	if (m_NetPDLDecodingEngine)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
				"The nbPSMLReader has already been configured to manage packets returned by "
				"the nbPacketDecoder. If you want to read a file, you have to use another nbPSMLReader instance.");
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
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Unable to open the PSML file.");
		return nbFAILURE;
	}

	// Format a string with the starting tag we have to look for (<packet>)
	StartTagLength= ssnprintf(StartTag, sizeof(StartTag), "<%s>", PSML_PACKET);

	BufferPtr= 0;
	CurrentOffset= 0;
	AsciiBuffer= m_asciiBuffer.GetStartBufferPtr();

	while( !feof( m_sourceOnDiskFileHandle ) )
	{
	unsigned int ReadBytes;
	char *TagPtr;

		// Read data from file; we have a buffer with size m_asciiBuffer.GetBufferTotSize()
		ReadBytes= fread(&AsciiBuffer[BufferPtr], sizeof (char), m_asciiBuffer.GetBufferTotSize() - 1 - BufferPtr, m_sourceOnDiskFileHandle);
		AsciiBuffer[BufferPtr + ReadBytes]= 0;
		
		// If we're still at the beginnning of the file, let's locate the summary
		if (CurrentOffset == 0)
		{
		char StructureTag[NETPDL_MAX_STRING];
		char *StartStructure, *EndStructure;

			// Locate the starting point of the <structure>
			ssnprintf(StructureTag, sizeof(StructureTag), "<%s>", PSML_INDEXSTRUCTURE);
			StartStructure= strstr(AsciiBuffer, StructureTag);

			// Locate the ending point of the <structure>
			ssnprintf(StructureTag, sizeof(StructureTag), "<%s>", PSML_PACKET);
			EndStructure= strstr(AsciiBuffer, StructureTag);

			// Save the structure within an ascii buffer
			m_summarySection= new char [EndStructure - StartStructure];
			if (m_summarySection == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to contain the summary session");
				return nbFAILURE;
			}

			m_summarySection[0]= 0;
			sstrncat(m_summarySection, StartStructure, (int) (EndStructure - StartStructure - 1));
		}

		// Let's locate all the <packet> contained in this buffer
		TagPtr= m_asciiBuffer.GetStartBufferPtr();
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
		char *AsciiBufferPtr;

			AsciiBufferPtr= m_asciiBuffer.GetStartBufferPtr();

			// Copy some of the bytes at the end of the buffer to the beginning 
			// of the new one, so that we can speed up the process (strstr() can scan a contiguous buffer)
			strncpy(AsciiBufferPtr, &AsciiBufferPtr[m_asciiBuffer.GetBufferTotSize() - StartTagLength], StartTagLength - 1);

			BufferPtr= StartTagLength - 1;
			CurrentOffset= CurrentOffset + (m_asciiBuffer.GetBufferTotSize() - StartTagLength);
		}
	}

	if (m_currNumPackets == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), 
			"Initializing a PSML file with no packets in it. Are you sure the input data is correct?.");
		return nbFAILURE;
	}

	// If we have N packet, we have (N+1) meaningful cells within this vector
	m_packetList[m_currNumPackets]= ftell(m_sourceOnDiskFileHandle);

	return nbSUCCESS;
}


// Documented in the base class
int CPSMLReader::GetSummary(char **SummaryPtr)
{
char **SummaryData;

	if (m_NetPDLDecodingEngine)
	{
		// If our source is the Decoding Engine, we can access to some internal structures of the PSMLMaker
		SummaryData= ((CNetPDLDecoder *) m_NetPDLDecodingEngine)->m_PSMLMaker->m_summaryStructData;

		// Convert PSMLMaker internal data to a '\0' delimited ascii buffer
		return FormatItem(SummaryData, SummaryPtr);
	}
	else
	{
		// If our source is a file, we have to list the '<section>' contained in the summary node

		// Convert XML data to a '\0' delimited ascii buffer
		return FormatItem(m_summarySection, (int) strlen(m_summarySection), SummaryPtr);
	}
}


// Documented in the base class
int CPSMLReader::GetSummaryXML(char* &SummaryPtr, unsigned int &SummaryLength)
{
	if (m_NetPDLDecodingEngine)
	{
		m_asciiBuffer.ClearBuffer(true /* resizing permitted */);

		SummaryPtr= m_asciiBuffer.GetStartBufferPtr();
		((CNetPDLDecoder *) m_NetPDLDecodingEngine)->m_PSMLMaker->GetSummaryAscii(SummaryPtr, m_asciiBuffer.GetBufferTotSize() );
		
		SummaryLength= strlen(SummaryPtr);

		return nbSUCCESS;
	}
	else
	{
		// If our source is a file, we have to list the '<section>' contained in the summary node
		SummaryPtr= m_summarySection;

		SummaryLength= strlen(m_summarySection);

		return nbSUCCESS;
	}
}


// Documented in the base class
int CPSMLReader::GetCurrentPacket(char **PacketPtr)
{
char **ItemData;

	if (m_NetPDLDecodingEngine)
	{
		// If our source is the Decoding Engine, we can access to some internal structures of the PSMLMaker
		ItemData= ((CNetPDLDecoder *) m_NetPDLDecodingEngine)->m_PSMLMaker->m_summaryItemsData;

		// Convert PSMLMaker internal data to a '\0' delimited ascii buffer
		return FormatItem(ItemData, PacketPtr);
	}

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The current PSML item cannot be returned when the PSML source is a file.");
	return nbFAILURE;
}


// Documented in the base class
int CPSMLReader::GetCurrentPacketXML(char* &PacketPtr, unsigned int &PacketLength)
{
	if (m_NetPDLDecodingEngine)
	{
		m_asciiBuffer.ClearBuffer(true /* resizing permitted */);

		PacketPtr= m_asciiBuffer.GetStartBufferPtr();
		((CNetPDLDecoder *) m_NetPDLDecodingEngine)->m_PSMLMaker->GetCurrentPacketAscii(PacketPtr, m_asciiBuffer.GetBufferTotSize() );
		PacketLength= m_asciiBuffer.GetBufferCurrentSize();

		return nbSUCCESS;
	}

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The current PSML item cannot be returned when the PSML source is a file.");
	return nbFAILURE;
}


// Documented in the base class
int CPSMLReader::GetPacket(unsigned long PacketNumber, char **PacketPtr)
{
unsigned int AsciiBufferLen;
int RetVal;

	RetVal= GetXMLPacketFromDump(PacketNumber, m_asciiBuffer.GetStartBufferPtr(), m_asciiBuffer.GetBufferTotSize(), AsciiBufferLen);
	if ((RetVal == nbWARNING) || (RetVal == nbFAILURE))
		return RetVal;

	// Convert XML data to a '\0' delimited ascii buffer
	return FormatItem(m_asciiBuffer.GetStartBufferPtr(), AsciiBufferLen, PacketPtr);
}


// Documented in the base class
int CPSMLReader::GetPacketXML(unsigned long PacketNumber, char* &PacketPtr, unsigned int &PacketLength)
{
int RetVal;

	RetVal= GetXMLPacketFromDump(PacketNumber, m_asciiBuffer.GetStartBufferPtr(), m_asciiBuffer.GetBufferTotSize(), PacketLength);
	PacketPtr= m_asciiBuffer.GetStartBufferPtr();

	// Return nbSUCCESS in any case
	return RetVal;
}


/*!
	\brief This function converts the packet summary from the PSMLMaker internal format to the PSMLReader one.

	\param ItemData: pointer to the variable (inside the PSMLReader) that keeps the PSML data.
	\param PacketPtr: pointer to a 'char *' in which PSMLReader will put the will put the formatted data.

	\return The number of '&lt;sections&gt;' that have been copied in the returned buffer,
	or nbFAILURE if some errors occurred.
	In case of error, the error message can be retrieved by the GetLastError() method.

	\note This function is used only when the PSML fragment has to be generated from the current decoding engine
	(i.e. the source is a nbDecodingEngine).
*/
int CPSMLReader::FormatItem(char **ItemData, char **PacketPtr)
{
unsigned int ItemNumber;

	m_asciiBuffer.ClearBuffer(true /* resizing permitted */);

	// Loop through the <section> and put them into the same ascii buffer
	for (ItemNumber= 0; ItemNumber < NetPDLDatabase->ShowSumStructureNItems; ItemNumber++)
	{
		if (m_asciiBuffer.Store(ItemData[ItemNumber]) == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "%s", m_asciiBuffer.GetLastError() );
			(*PacketPtr)= m_asciiBuffer.GetStartBufferPtr();
			return nbFAILURE;
		}
	}

	(*PacketPtr)= m_asciiBuffer.GetStartBufferPtr();
	return (int) ItemNumber;
}



/*!
	\brief This function converts the packet summary data from the PSMLMaker internal format to the PSMLReader one.

	\param Buffer: pointer to an ascii buffer that contains the list of &lt;section&gt; nodes that belong to the current packet.
	\param BufferLen: number of chars in the previous buffer.
	\param PacketPtr: pointer to a 'char *' in which PSMLReader will put the will put the formatted data.

	\return The number of '&lt;sections&gt;' that have been copied in the returned buffer,
	or nbFAILURE if some error occurred.
	In case of error, the error message can be retrieved by the GetLastError() method.

	\note This function is used only when the PSML fragment has to be read from file. This happens either
	when our source is a nbDecodingEngine but we want to return previously decoded packets, or when our source
	is a file.
*/
int CPSMLReader::FormatItem(char *Buffer, int BufferLen, char **PacketPtr)
{
unsigned int ItemNumber;

	// Parse the XML string and return a pointer to the list of '<section>' contained into it
	DOMNodeList *ItemList= ParseMemBuf(Buffer, BufferLen);

	if (ItemList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The file does not appear to be a valid PSML structure.");
		return nbFAILURE;
	}

	m_asciiBuffer.ClearBuffer(true /* resizing permitted */);

	for (ItemNumber= 0; ItemNumber < ItemList->getLength(); ItemNumber++)
	{
		DOMText *TextNode= (DOMText *) (ItemList->item(ItemNumber))->getFirstChild();

		// In case we have <section></section>, the TextNode is NULL, so we have to avoid following code.
		if (TextNode)
		{
			if (m_asciiBuffer.TranscodeAndStore( (wchar_t *)(XMLCh *) TextNode->getNodeValue()) == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "%s", m_asciiBuffer.GetLastError() );
				(*PacketPtr)= m_asciiBuffer.GetStartBufferPtr();
				return nbFAILURE;
			}
		}
		else
		{
			if (m_asciiBuffer.Store("") == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "%s", m_asciiBuffer.GetLastError() );
				(*PacketPtr)= m_asciiBuffer.GetStartBufferPtr();
				return nbFAILURE;
			}
		}
	}

	(*PacketPtr)= m_asciiBuffer.GetStartBufferPtr();
	return (int) ItemNumber;
}

/*
	\brief It accepts a XML (ascii) buffer and it parses it.

	This function is used when we have a char buffer containing an XML fragment, and we have to parse it.
	This function returns the list of PSML items (&lt;section&gt;) contained in the XML buffer.

	\param Buffer: pointer to a buffer that contains the XML fragment (in ascii).
	\param BufSize: number of valid bytes in the previous buffer; not NULL terminated buffers are supported as well.

	\return A pointer to the list of &lt;section&gt; nodes contained into the XML fragment if everything
	is fine, NULL otherwise.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
DOMNodeList *CPSMLReader::ParseMemBuf(char *Buffer, int BytesToParse)
{
DOMDocument *Document;
XMLCh TempBuffer[NETPDL_MAX_STRING + 1];
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
				"Fatal error: cannot parse PSML file. Possible errors: "
				"wrong file location, or not well-formed XML file.");
			return NULL;
		}
	}
	catch (...)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Unexpected exception during PSML buffer parsing.");
		return NULL;
	}

	if (Document == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Unexpected exception during PSML buffer parsing.");
		return NULL;
	}

	XMLString::transcode(PSML_SECTION, TempBuffer, NETPDL_MAX_STRING);

	// After parsing the '<packet>' fragment, let's list the <section> contained into it
	return Document->getElementsByTagName(TempBuffer);
}
