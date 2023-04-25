/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include <stdio.h>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>


// Indicate using Xerces-C++ namespace in general
// This is required starting from Xerces 2.2.0
XERCES_CPP_NAMESPACE_USE

#include <nbee_packetdecoder.h>
#include "../globals/globals.h"


//! By default, we are able to keep track of 10000 packet stored on disk, then we have to enlarge the buffer
#define PXML_MINIMUM_LIST_SIZE 10000



/*!
	\brief This class implements some basic features that are required by CPSMLReader and CPDMLReader classes.

	Basically, this class helps in some functions (like dumping the XML document on file)
	that are common throughout both classes.
*/
class CPxMLReader
{
public:
	CPxMLReader();
	virtual ~CPxMLReader();

	// Function used only when we're using the decoder as source
	int InitializeDecoder(nbPacketDecoder *NetBeePacketDecoder);

	// Functions used by CPDMLMaker and CPSMLMaker
	int InitializeParsForDump(const char *RootXMLTag, const char *InitText);
	int StorePacket(char *Buffer, unsigned int BytesToWrite);

	//! Name of the file that is used to store temporary data on file when we're getting packets from the decoder
	char* m_tempDumpFileName;
	//! Handle to the file that is used to store temporary data on file when we're getting packets from the decoder
	FILE* m_tempDumpFileHandle;

protected:
	int Initialize();
	int CheckPacketListSize();

	int SaveDocumentAs(const char *Filename);
	int RemovePacket(unsigned long PacketNumber);
	int GetXMLPacketFromDump(unsigned long PacketNumber, char *Buffer, unsigned int BufferSize, unsigned int &ValidData);

protected:

	//! Current size of the array that keeps the offsets of the packet on the file
	unsigned long m_maxNumPackets;

	//! Keeps how many elements we have currently in the array
	unsigned long m_currNumPackets;

	//! Pointer to the array that will keep the data
	unsigned long *m_packetList;

	//! Keeps a Xerces DOM parser object; it used to parse packets when we want to open a PxML file from disk
	XercesDOMParser *m_DOMParser;

	//! Pointer to a Packet Decoder class; it is used to parse packet when we want to get data from the current decoder
	nbPacketDecoder *m_NetPDLDecodingEngine;

	//! Handle to the PxML file that we have to open; used only when we want to open a PxML file from disk
	FILE *m_sourceOnDiskFileHandle;

	//! Buffer that keeps the error message (if any)
	char m_errbuf[2048];

private:
	int InitializeDumpFile(FILE *DumpFileHandle);

	//! Keeps the tag which will be used as 'root' XML tag in the resulting document
	char m_rootXMLTag[NETPDL_MAX_STRING];

	//! Keeps the text that must be placed at the beginning of the file (such as the summary index for the PSML file)
	char m_initText[NETPDL_MAX_STRING];
};

