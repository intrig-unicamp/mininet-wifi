/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


// Indicate using Xerces-C++ namespace in general
// This is required starting from Xerces 2.2.0
XERCES_CPP_NAMESPACE_USE


#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <nbee_pxmlreader.h>
#include "pxmlreader.h"
#include "../utils/asciibuffer.h"



//! This class implements the nbReader abstract class.
class CPSMLReader : public nbPSMLReader, public CPxMLReader
{
public:
	CPSMLReader();
	virtual ~CPSMLReader();

	int Initialize();
	int InitializeSource(char *FileName);

	int GetSummary(char **SummaryPtr);
	int GetCurrentPacket(char **PacketPtr);
	int GetPacket(unsigned long PacketNumber, char **PacketPtr);

	int GetSummaryXML(char* &SummaryPtr, unsigned int &SummaryLength);
	int GetCurrentPacketXML(char* &PacketPtr, unsigned int &PacketLength);
	int GetPacketXML(unsigned long PacketNumber, char* &PacketPtr, unsigned int &PacketLength);

	// Documented in the base class
	int RemovePacket(unsigned long PacketNumber) { return CPxMLReader::RemovePacket(PacketNumber); }

	// Documented in the base class
	int SaveDocumentAs(const char *Filename) { return CPxMLReader::SaveDocumentAs(Filename); }

	// Documented in the base class
	char *GetLastError() { return m_errbuf; };

private:
	int FormatItem(char **ItemData, char **PacketPtr);
	int FormatItem(char *Buffer, int BufferLen, char **PacketPtr);
	DOMNodeList *ParseMemBuf(char *Buffer, int BytesToParse);

	//! Temp buffer needed to store some temporary data in it (we need a big buffer in several places, so we allocate it once at the beginning)
	CAsciiBuffer m_asciiBuffer;

	//! Pointer to the element that keeps PSML structure; it is used when we want to load a PSML file
	char *m_summarySection;
};

