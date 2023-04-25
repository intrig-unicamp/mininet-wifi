/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <string.h>
#include <nbprotodb_defs.h>
#include "pxmlreader.h"
#include "../utils/asciibuffer.h"
#include "../globals/utils.h"
#include "../globals/globals.h"
#include "../globals/debug.h"



// Please note that utf-8 should be better, however it does not support latin chars such as 'à', 'è', ...
// With UTF-8, these chars shoudl must be translated into &egrave; etc, which is really annoing.
// I'm pretty sure that this will insert other problems with other charsets (e.g. oriental charsets), but, for now, 
// this is enough.
#define XML_FILE_HEADER "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n" 


extern struct _nbNetPDLDatabase *NetPDLDatabase;


//! Default constructor.
CPxMLReader::CPxMLReader()
{
	// Initialzie main variables
	m_DOMParser= NULL;
	m_NetPDLDecodingEngine= NULL;

	m_sourceOnDiskFileHandle= NULL;
	m_tempDumpFileName= NULL;
	m_tempDumpFileHandle= NULL;

	m_packetList= NULL;

	m_currNumPackets=0;
	m_maxNumPackets= PXML_MINIMUM_LIST_SIZE;

	m_rootXMLTag[0]= 0;
	m_initText[0]= 0;

	memset(m_errbuf, 0, sizeof(m_errbuf));
}


//! Default destructor
CPxMLReader::~CPxMLReader()
{
	if (m_DOMParser)
		delete m_DOMParser;
	if (m_sourceOnDiskFileHandle)
		fclose(m_sourceOnDiskFileHandle);

	if (m_packetList)
		delete[] m_packetList;

	// Close the temp file
	if (m_tempDumpFileHandle)
	{
		fclose(m_tempDumpFileHandle);
		// Now, let's remove the temp file from disk
		remove(m_tempDumpFileName);
	}

	if (m_tempDumpFileName)
		free(m_tempDumpFileName);
}


/*!
	\brief Initializes the variables contained into this class.

	\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPxMLReader::Initialize()
{
	// Instantiate the DOM parser.
    m_DOMParser = new XercesDOMParser;

	if (m_DOMParser == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Allocation of the Xerces DOM parser internal object failed.");

		return nbFAILURE;
	}

    m_DOMParser->setValidationScheme(XercesDOMParser::Val_Never);
    m_DOMParser->setDoNamespaces(false);
    m_DOMParser->setDoSchema(false);
    m_DOMParser->setValidationSchemaFullChecking(false);

	// Allocates a new vector for containing the packet list
	m_packetList= new unsigned long [m_maxNumPackets];
	if (m_packetList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__,  m_errbuf, sizeof(m_errbuf),
			"Not enough memory to allocate the packetlist buffer.");

		return nbFAILURE;
	}

	// Initialize first item
	m_packetList[0]= 0;

	return nbSUCCESS;
}



/*!
	\brief It tells the PxMLReader that the source of PSML items is a nbPacketDecoded class which is currently
	decoding network packets.

	This function is called internally from the PacketDecoder class; it is not exported to the user.

	\param NetBeePacketDecoder: pointer to the Packet Decoder class which is currently decoding network packets.

	\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPxMLReader::InitializeDecoder(nbPacketDecoder *NetBeePacketDecoder)
{
	if (NetBeePacketDecoder == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Invalid pointer to the Packet Decoder class.");

		return nbFAILURE;
	}

	// Save a reference to the decoder for future use
	m_NetPDLDecodingEngine= NetBeePacketDecoder;
	return nbSUCCESS;
}


/*!
	\brief Checks the number of packets in 'm_packetList'; it enlarges this vector if needed.

	\return nbSUCCESS if everything is fine, nbFAILURE if something goes wrong.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPxMLReader::CheckPacketListSize()
{
	// The vector is not enough; Please allocate a new one, and let's hope it is enough
	if (m_currNumPackets == (m_maxNumPackets - 1))
	{
		long newsize= m_maxNumPackets * 10;

		unsigned long *newVector= new unsigned long [newsize];
		if (newVector == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
				"Not enough memory to allocate the new packetlist buffer.");

			return nbFAILURE;
		}

		for (unsigned long i=0; i < m_maxNumPackets; i++)
			newVector[i]= m_packetList[i];

		delete m_packetList;

		m_maxNumPackets= newsize;
		m_packetList= newVector;
	}

	return nbSUCCESS;
}


/*!
	\brief Initialize the parameters needed for dumping packets to file.
	
	Basically, this is needed to open a temp file in case we want to store PDML/PSML packets to disk.
    This function is called only when a CPSMLMaker/CPDMLMaker is using this class to dump its content on disk
	(and the user wants want to keep all packets).

	\param RootXMLTag: string that keeps the root tag (e.g. 'pdml') in the resulting file.

	\param InitText: string that keeps any text that must be placed at the beginning of the file
	(such as the summary index for the PSML file).

	\return nbSUCCESS function is successful, nbFAILURE if something goes wrong.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPxMLReader::InitializeParsForDump(const char *RootXMLTag, const char *InitText)
{
	strncpy(m_rootXMLTag, RootXMLTag, sizeof(m_rootXMLTag) );
	if (InitText)
		strncpy(m_initText, InitText, sizeof(m_initText) );

	// Get temp file name
	m_tempDumpFileName= tempnam(NULL, NULL);
	if (m_tempDumpFileName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Fatal error: cannot open a temp file to store PDML/PSML data.");

		return nbFAILURE;
	}

// b: we have to use binary files in order to avoid problems with \n and such (fseek() does not work well with them)
#if defined(WIN32) || defined(WIN64)
	m_tempDumpFileHandle= fopen(m_tempDumpFileName, "w+bT");
#else
	m_tempDumpFileHandle= fopen(m_tempDumpFileName, "w+b");
#endif

	if (m_tempDumpFileHandle == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Fatal error: cannot open a temp file to store PDML/PSML data.");

		return nbFAILURE;
	}

	if (InitializeDumpFile(m_tempDumpFileHandle) == nbFAILURE)
		return nbFAILURE;

	// Initialize first item (we have to skip the '<pdml>' tag and such)
	m_packetList[0]= ftell(m_tempDumpFileHandle);

	return nbSUCCESS;
}


/*!
	\brief Initializes the dump file by placing the proper tags at its beginning.

	\param DumpFileHandle: pointer to the (open) file in which we have to write.

	\return nbSUCCESS if everything is fine, nbFAILURE in case of errors.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPxMLReader::InitializeDumpFile(FILE *DumpFileHandle)
{
char TempString[NETPDL_MAX_STRING];
unsigned int BytesToWrite;

	BytesToWrite= ssnprintf(TempString, sizeof(TempString), XML_FILE_HEADER  "<%s %s %s date=\"%s\">\n", m_rootXMLTag, 
		PxML_VERSION, PxML_CREATOR, NetPDLDatabase->CreationDate);

	// Initialize the temp file
	if ( fwrite(TempString, sizeof (char), BytesToWrite, DumpFileHandle) != BytesToWrite)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__,  m_errbuf, sizeof(m_errbuf),
			"Error writing data in the temp file");

		return nbFAILURE;
	}

	// Dump the summary (if present)
	if (m_initText[0])
	{
		BytesToWrite= (int) strlen(m_initText);

		// initialize the temp file
		if ( fwrite(m_initText, sizeof (char), BytesToWrite, DumpFileHandle) != BytesToWrite)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__,  m_errbuf, sizeof(m_errbuf),
				"Error writing data in the temp file");

			return nbFAILURE;
		}
	}

	return nbSUCCESS;
}

/*!
	\brief It adds an new packet to the list of packets.

	This function is called only from within the derived classes in case we want to keep all packets.

	\param Buffer: pointer to a char buffer that keeps the entire packet (in ascii) that we want
	to dump on file.

	\param BytesToWrite: the number of bytes (in 'Buffer') that needs to be written on file.

	\return nbSUCCESS if the function is successful, nbFAILURE if something goes wrong.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPxMLReader::StorePacket(char *Buffer, unsigned int BytesToWrite)
{
	if ( fwrite(Buffer, sizeof (char), BytesToWrite, m_tempDumpFileHandle) != BytesToWrite)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Error writing data in the temp file");

		return nbFAILURE;
	}

	m_currNumPackets++;
	m_packetList[m_currNumPackets]= ftell(m_tempDumpFileHandle);

	// Check if the vector is enough; if not, let's allocate a new one, and let's hope it is enough
	if (CheckPacketListSize() == nbFAILURE)
		return nbFAILURE;
	
	return nbSUCCESS;
}



//!Documented in the base class
int CPxMLReader::RemovePacket(unsigned long PacketNumber)
{
	if (PacketNumber > m_currNumPackets)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"The requested element is out of range.");

		return nbFAILURE;
	}

	for (unsigned long i= (PacketNumber-1); i < m_currNumPackets; i++)
		m_packetList[i] = m_packetList[i+1];

	m_currNumPackets--;

	return nbSUCCESS;
}



/*!
	\brief It returns the XML ascii dump of the requested element.

	This method is provided in order to allow random access to the list of decoded packets.

	\param PacketNumber: the ordinal number of the packet (starting from '1') that has to
	be returned.

	\param Buffer: pointer to a char buffer that will keep the packet (in ascii) that we 
	want to retrieve. The packet will be returned as a string of characters in XML 
	format (i.e. PDML/PSML).

	\param BufferSize: the size of the 'Buffer'.

	\param ValidData: upon return, it contains the number of valid data within 'Buffer',
	not counting the string terminator at the end.

	\return nbSUCCESS if the function is successful, nbFAILURE if some error occurred,
	nbWARNING if the user asked for a packet that is out of range.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPxMLReader::GetXMLPacketFromDump(unsigned long PacketNumber, char *Buffer, unsigned int BufferSize, unsigned int &ValidData)
{
unsigned long BytesToRead, BytesRead;
FILE *SourceFileHandle;

	if (m_NetPDLDecodingEngine)
		SourceFileHandle= m_tempDumpFileHandle;
	else
		SourceFileHandle= m_sourceOnDiskFileHandle;

	// Check that the requested packet is not out of range
	if (PacketNumber > m_currNumPackets)
	{
		// Please beware that this is not a 'real' error; it must be considered a warning instead
		// This is the reason why we do not use the errorsnprintf here
		return nbWARNING;
	}

	fseek(SourceFileHandle, m_packetList[PacketNumber-1], SEEK_SET);

	BytesToRead= m_packetList[PacketNumber] - m_packetList[PacketNumber-1];

	if (BufferSize < BytesToRead)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__,  m_errbuf, sizeof(m_errbuf),
			"The temporary buffer is too small for a decoded packet");

		return nbFAILURE;
	}

	BytesRead= (int) fread(Buffer, sizeof(char), BytesToRead, SourceFileHandle);

	if (BytesRead != BytesToRead)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__,  m_errbuf, sizeof(m_errbuf),
			"Error reading the PxML file: got a number of bytes smaller than the expected.");

		return nbFAILURE;
	}

	// Put a '0' to terminate the string
	Buffer[BytesRead - 1]= 0;
	ValidData= BytesRead - 1;

	// Move the pointer of the file at the end
	fseek(SourceFileHandle, 0, SEEK_END);

	return nbSUCCESS;
}



// Documented in the base class
int CPxMLReader::SaveDocumentAs(const char *Filename)
{
FILE *DestFileHandle;
FILE *SourceFileHandle;
unsigned int BytesToWrite;
CAsciiBuffer AsciiBuffer;
char *BufferPtr;
unsigned int BufferSize;

	// Initialize the ascii buffer
	if (AsciiBuffer.Initialize() == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"%s", AsciiBuffer.GetLastError() );

		return nbFAILURE;
	}

	BufferPtr= AsciiBuffer.GetStartBufferPtr();
	BufferSize= AsciiBuffer.GetBufferTotSize();

	if (m_NetPDLDecodingEngine)
		SourceFileHandle= m_tempDumpFileHandle;
	else
		SourceFileHandle= m_sourceOnDiskFileHandle;

	// In case the PacketDecoder has not been configured to keep all data, this pointer will be NULL
	if (SourceFileHandle == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Cannot dump data on file when the Packet Decoder has not been configured to keep all packets.");

		return nbFAILURE;
	}

	DestFileHandle= fopen(Filename, "w");

	if (!DestFileHandle)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Cannot create destination file.");

		return nbFAILURE;
	}

	// Move pointer to the beginning of the file
	fseek(SourceFileHandle, 0, SEEK_SET);

	// Initialize the destination file
	if (InitializeDumpFile(DestFileHandle) == nbFAILURE)
		return nbFAILURE;

	for (unsigned i= 0; i< m_currNumPackets; i++)
	{
		// move pointer to the beginning of the file
		fseek(SourceFileHandle, m_packetList[i], SEEK_SET);

		BytesToWrite= (int) fread(BufferPtr, sizeof(char), m_packetList[i+1] - m_packetList[i], SourceFileHandle);

		if (BytesToWrite > BufferSize)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__,  m_errbuf, sizeof(m_errbuf),
				"The amount of memory needed dump the file on disk is not enough");

			return nbFAILURE;
		}

		if ( fwrite(BufferPtr, sizeof (char), BytesToWrite, DestFileHandle) != BytesToWrite)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
				"Error writing data in the destination file");

			return nbFAILURE;
		}
	}

	// move pointer to the end of the file
	fseek(SourceFileHandle, 0, SEEK_END);

	// Terminate the destination file
	BytesToWrite= ssnprintf(BufferPtr, BufferSize, "%s%s%s", "</", m_rootXMLTag, ">\n");

	if (fwrite(BufferPtr, sizeof (char), BytesToWrite, DestFileHandle) != BytesToWrite)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Error writing data in the destination file");

		return nbFAILURE;
	}

	fclose(DestFileHandle);

	return nbSUCCESS;
}

