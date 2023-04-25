/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <xercesc/util/PlatformUtils.hpp>
#include <time.h>

#include <nbprotodb.h>
#include <nbprotodb_defs.h>

#include "psmlmaker.h"
#include "pdmlmaker.h"
#include "../utils/netpdlutils.h"
#include "../globals/globals.h"
#include "../globals/debug.h"

#if defined(_WIN32) || defined(_WIN64)
  #define snprintf _snprintf
#endif

// Global variable
extern struct _nbNetPDLDatabase *NetPDLDatabase;



/*!
	\brief Standard constructor; it includes a set of initialization operations.

	This function parses the NetPDL document looking for the definition of the capture index
	(i.e. the number of sections, their name, etc). Then, it creates the first item into the
	PSML document, which contains the index structure.

	Moreover, it initializes some other internal structure (for example an expression handler).

	\param ExprHandler: pointer to a CNetPDLExpression (this class is shared among all the
	NetPDL classes in order to save memory).

	\param PSMLReader: Pointer to a PSMLReader; needed to manage PSML files (storing data and such).

	\param NetPDLFlags: flags that indicates if the user wants to create the PSML file;
	if so, we have to decide if all the packets are kept in memory or only the last one is kept.

	\param ErrBuf: pointer to a user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer belongs to the class that creates this one.
	Data stored in this buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.
*/
CPSMLMaker::CPSMLMaker(CNetPDLExpression *ExprHandler, CPSMLReader *PSMLReader, int NetPDLFlags, char *ErrBuf, int ErrBufSize)
{
	m_isVisExtRequired= NetPDLFlags & nbDECODER_GENERATEPSML;
	m_keepAllPackets= NetPDLFlags & nbDECODER_KEEPALLPSML;

	m_PSMLReader= PSMLReader;
	m_exprHandler= ExprHandler;

	// Store internally the pointer to the error buffer. This buffer belongs to the class that creates this one.
	m_errbuf= ErrBuf;
	m_errbufSize= ErrBufSize;
}


/*!
	\brief Initializes the variables contained into this class.

	\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPSMLMaker::Initialize()
{
unsigned int i;

	// If the user does not want to create the packet summary, exit right now.
	if (!m_isVisExtRequired)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize,
			"The decoder has been configured not to generate PSML files. PSMLMaker initialization failed.");

		return nbFAILURE;
	}

	// Initialize the variables in which we store the index string before putting them into an XML element
	m_summaryStructData= new char * [NetPDLDatabase->ShowSumStructureNItems];
	m_summaryItemsData= new char * [NetPDLDatabase->ShowSumStructureNItems + 1];

	if ((m_summaryStructData == NULL) || (m_summaryItemsData == NULL))
		goto outofmem;

	for (i= 0; i < NetPDLDatabase->ShowSumStructureNItems; i++)
	{
		m_summaryStructData[i]= new char [NETPDL_MAX_STRING + 1];
		if (m_summaryStructData[i] == NULL)
			goto outofmem;
		m_summaryStructData[i] [NETPDL_MAX_STRING]= 0;

		m_summaryItemsData[i]= new char [NETPDL_MAX_STRING + 1];
		if (m_summaryItemsData[i] == NULL)
			goto outofmem;
		m_summaryItemsData[i] [NETPDL_MAX_STRING]= 0;
	}

	// We have to initialize one more data item
	m_summaryItemsData[i]= new char [NETPDL_MAX_STRING + 1];
	if (m_summaryItemsData[i] == NULL)
		goto outofmem;

	m_summaryItemsData[i] [NETPDL_MAX_STRING]= 0;

	// Initialize summary structure
	for (i= 0; i < NetPDLDatabase->ShowSumStructureNItems; i++)
		sstrncpy(m_summaryStructData[i], NetPDLDatabase->ShowSumStructureList[i]->LongName, NETPDL_MAX_STRING);

	// initialize the parameters needed to dump everything to file (if needed)
	if (m_keepAllPackets)
	{
//	unsigned int BytesToWrite;
	char Buffer[NETPDL_MAX_STRING];

//		BytesToWrite= GetSummaryAscii(Buffer, sizeof(Buffer) );
		GetSummaryAscii(Buffer, sizeof(Buffer) );

		if (m_PSMLReader->InitializeParsForDump(PSML_ROOT, Buffer) == nbFAILURE)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "%s", m_PSMLReader->GetLastError() );
			return nbFAILURE;
		}
	}

	if (m_tempAsciiBuffer.Initialize() == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "%s", m_tempAsciiBuffer.GetLastError() );
		return nbFAILURE;
	}

	return nbSUCCESS;

outofmem:
	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize,
		"Not enough memory to inizialize PSMLMaker internal structures.");

	return nbFAILURE;
}




//! Standard destructor
CPSMLMaker::~CPSMLMaker()
{
	if (m_isVisExtRequired)
	{
		unsigned int i;
		for (i= 0; i < NetPDLDatabase->ShowSumStructureNItems; i++)
		{
			delete[] m_summaryStructData[i];
			delete[] m_summaryItemsData[i];
		}

		// We have to delete one more Summary Items Data
		delete[] m_summaryItemsData[i];

		delete[] m_summaryStructData;
		delete[] m_summaryItemsData;
	}
}



/*!
	\brief It analyzes a decoded header and it prints the related index view.

	This function is called by the NetPDL decoding engine each time a new header
	has been decoded (i.e. each time a new protocol has been decoded).
	This function locates the index definition and it applies these commands.

	\param PDMLProtoItem: pointer to the _nbPDMLProto item that keeps the protocol currently been decoded.

	\param NetPDLProtoItem: the index (in NetPDLDatabase) of the 
	protocol which is currently generating our PSML description.

	\param CurrentNetPDLBlock: pointer to the current block in the NetPDL file. This pointer is 
	valid only if the field currently being decoded is within a block. This information is needed
	in order to allow searching for '&lt;showsum&gt;' within the block.

	\param CurrentOffset: the current offset into the decoded packet (i.e. the pointer to the next byte
	that has to be examined, which will belong to the next protocol).

	\param SnapLen: portion of the packet that has been captured.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPSMLMaker::AddHeaderFragment(_nbPDMLProto *PDMLProtoItem, int NetPDLProtoItem, struct _nbNetPDLElementFieldBase *CurrentNetPDLBlock, 
								   int CurrentOffset, int SnapLen)
{
struct _nbNetPDLElementBase *NetPDLIndexItem;
int OriginalCurrentSection = 0;	// var that has to be used only if we're adding a summary from within a block

	// If the user does not want to create the packet summary, exit right now.
	if (!m_isVisExtRequired)
		return nbSUCCESS;

	// We're going to generate the summary visualization for a protocol
	if (CurrentNetPDLBlock == NULL)
	{
		// Initialize a temp variable needed to scan the summary description
		// This is done only if if we're creating the summary of a protocol
		//
		// Obviously, this is done only if the protocol has the summary template
		if (NetPDLDatabase->ProtoList[NetPDLProtoItem]->ShowSumTemplateInfo)
			NetPDLIndexItem= (struct _nbNetPDLElementBase *) nbNETPDL_GET_ELEMENT(NetPDLDatabase, NetPDLDatabase->ProtoList[NetPDLProtoItem]->ShowSumTemplateInfo->FirstChild);
		else
			return nbSUCCESS;
	}
	else
	{
	struct _nbNetPDLElementBlock *BlockInfo;

		BlockInfo= (struct _nbNetPDLElementBlock *) CurrentNetPDLBlock;

		if (BlockInfo->ShowSumTemplateInfo)
			NetPDLIndexItem= (struct _nbNetPDLElementBase *) nbNETPDL_GET_ELEMENT(NetPDLDatabase, BlockInfo->ShowSumTemplateInfo->FirstChild);
		else
			return nbSUCCESS;

		// Let's save the current section (whether or not the block contains a <showsum> element)
		OriginalCurrentSection= m_currentSection;

		// Let's move to the 'fake' section that aims to keep the summary text inserted by the block
		m_currentSection= NetPDLDatabase->ShowSumStructureNItems;
	}

	if (PrintProtoBlockDetails(PDMLProtoItem, CurrentOffset, SnapLen, NetPDLIndexItem) == nbFAILURE)
		return nbFAILURE;

	// Let's check if the string that keeps block text contains anything valid
	// Obviously, we don't have to be within a block
	if ( (CurrentNetPDLBlock == NULL) && (m_summaryItemsData[NetPDLDatabase->ShowSumStructureNItems][0]) )
	{
		// If so, let's append the content to the current string
		sstrncat(m_summaryItemsData[m_currentSection], m_summaryItemsData[NetPDLDatabase->ShowSumStructureNItems], NETPDL_MAX_STRING);

		// Initialize the string (for the next protocol that has blocks)
		m_summaryItemsData[NetPDLDatabase->ShowSumStructureNItems][0]= 0;
	}

	// Let'restore the current section in case we're working from within a block
	if (CurrentNetPDLBlock != NULL) 
		m_currentSection= OriginalCurrentSection;

	return nbSUCCESS;
}


/*!
	\brief It analyzes a decoded header/block and it prints the related summary view.

	This function does basically the dirty job for AddHeaderFragment(), but it supports
	recursive calls (which is important in case of nested &lt;if&gt; elements).

	A tricky point of this method is to avoid error messages in case we captured only
	a portion of the packet (snaplen) and the header sumary view requires to print the value
	of a field which is not present into the decoded portion of the packet. See comments in the
	code about how this is managed.

	\param PDMLProtoItem: pointer to the _nbPDMLProto item that keeps the protocol currently been decoded.

	\param CurrentOffset: the current offset into the decoded packet (i.e. the pointer to the next byte
	that has to be examined, which will belong to the next protocol).

	\param SnapLen: portion of the packet that has been captured.

	\param ShowSummaryFirstItem: pointer to the first child element of the template used for
	the summary visualization that has to be used to create the summary view.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPSMLMaker::PrintProtoBlockDetails(_nbPDMLProto *PDMLProtoItem, int CurrentOffset, int SnapLen, struct _nbNetPDLElementBase *ShowSummaryFirstItem)
{
struct _nbNetPDLElementBase *NetPDLIndexItem;

	NetPDLIndexItem= ShowSummaryFirstItem;

	while (NetPDLIndexItem != NULL)
	{
		switch (NetPDLIndexItem->Type)
		{
			case nbNETPDL_IDEL_PROTOFIELD:
			{
			char *Attrib;
			int RetVal;
			struct _nbNetPDLElementProtoField*IndexItemInfo;

				if (PDMLProtoItem == NULL)
					break;

				IndexItemInfo= (struct _nbNetPDLElementProtoField *) NetPDLIndexItem;

				RetVal= CPDMLMaker::ScanForFieldRefAttrib(PDMLProtoItem->FirstField, NULL, 
					IndexItemInfo->FieldName, IndexItemInfo->FieldShowDataType, &Attrib, m_errbuf, m_errbufSize);

				// Handles the case in which the field has not been found
				if (RetVal != nbSUCCESS)
				{
					if (RetVal == nbWARNING)
					{
						// Check if the desired fieldref is missing because the packet has been truncated
						// This is the case, let's say, of a protocol whose length is 10, but only the first 4 bytes
						// have been captured (maybe due to the snaplen). So, let's check that:
						// - we're not the last protocol (and)
						// - the current offset into the protocol is not pointing the the end of the packet (so the proto can be truncated)
						//
						// We do not check if the packet has a different value for packet length and snaplen 
						// (i.e. it is a snapshot), because there are cases in which this is not true.
						// Let's suppose an ICMP error that contains IP and TCP inside: the TCP packet can be truncated, due to
						// the limitation of length of the ICMP payload, but packetlen == snaplen.

						if (CurrentOffset < SnapLen)
						{
							errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
								"The NetPDL protocol database contains a reference to a field that cannot be located in the PDML fragment: Protocol '%s', Field '%s'",
								PDMLProtoItem->LongName, IndexItemInfo->FieldName);
						}
						break;
					}

					return RetVal;
				}
				else
				{
					sstrncat(m_summaryItemsData[m_currentSection], Attrib, NETPDL_MAX_STRING + 1);
				}

				break;
			}

			case nbNETPDL_IDEL_TEXT:
			{
			struct _nbNetPDLElementText *IndexItemInfo;

				if (PDMLProtoItem == NULL)
					break;

				IndexItemInfo= (struct _nbNetPDLElementText *) NetPDLIndexItem;

				// Deleted 16/06/2007
				////// Let's check is we really have to print something or not
				////if ((IndexItemInfo->When == nbNETPDL_TEXT_WHEN_ONLYEMPTY) && *(m_summaryItemsData[m_currentSection]))
				////	break;
				////if ((IndexItemInfo->When == nbNETPDL_TEXT_WHEN_ONLYSECTIONHASTEXT) && (*(m_summaryItemsData[m_currentSection]) == 0))
				////	break;

				if (IndexItemInfo->Value)
					sstrncat(m_summaryItemsData[m_currentSection], IndexItemInfo->Value, NETPDL_MAX_STRING + 1);

				if (IndexItemInfo->ExprTree)
				{
					if (IndexItemInfo->ExprTree->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER)
					{
					unsigned char *TempBufferAscii;
					unsigned int TempBufferAsciiSize;
					int RetVal;

						RetVal= m_exprHandler->EvaluateExprString(IndexItemInfo->ExprTree, 
										PDMLProtoItem->FirstField, (unsigned char**) &TempBufferAscii, &TempBufferAsciiSize);

						if (RetVal != nbSUCCESS)
							return RetVal;



						TempBufferAscii[TempBufferAsciiSize]= 0;
						sstrncat(m_summaryItemsData[m_currentSection], (char *)TempBufferAscii, TempBufferAsciiSize);
					}
					else
					{
					char TmpBufferAscii[NETPDL_MAX_STRING + 1];
					unsigned int Result;
					int RetVal;

						RetVal= m_exprHandler->EvaluateExprNumber(IndexItemInfo->ExprTree, PDMLProtoItem->FirstField, &Result);

						if (RetVal != nbSUCCESS)
							return RetVal;

						ssnprintf(TmpBufferAscii, NETPDL_MAX_STRING, "%d", Result);
						TmpBufferAscii[NETPDL_MAX_STRING]= 0;

						sstrncat(m_summaryItemsData[m_currentSection], TmpBufferAscii, NETPDL_MAX_STRING + 1);
					}
				}

				break;
			}

			case nbNETPDL_IDEL_SECTION:
			{
			struct _nbNetPDLElementSection *IndexItemInfo;

				IndexItemInfo= (struct _nbNetPDLElementSection *) NetPDLIndexItem;

				if (IndexItemInfo->IsNext)
				{
					if (m_currentSection < (NetPDLDatabase->ShowSumStructureNItems -1))
						m_currentSection++;
					else
					{
						if (m_summaryItemsData[m_currentSection][0])
							// We're already at the last section; let's add a 'space' to the current content, since
							// we presume we're going to add some more text to the one that is already there
							sstrncat(m_summaryItemsData[m_currentSection], " - ", NETPDL_MAX_STRING + 1);
					}
				}
				else
				{
					if ( (int) m_currentSection == IndexItemInfo->SectionNumber)
					{
						if (m_summaryItemsData[m_currentSection][0])
						// We're already at the last section; let's add a 'space' to the current content, since
						// we presume we're going to add some more text to the one that is already there
						sstrncat(m_summaryItemsData[m_currentSection], " - ", NETPDL_MAX_STRING + 1);
					}
					else
						m_currentSection= IndexItemInfo->SectionNumber;
				}

				break;
			}

			case nbNETPDL_IDEL_PROTOHDR:
			{
			char *Attrib;
			struct _nbNetPDLElementProtoHdr *IndexItemInfo;

				if (PDMLProtoItem == NULL)
					break;

				IndexItemInfo= (struct _nbNetPDLElementProtoHdr *) NetPDLIndexItem;

				Attrib= CPDMLMaker::GetPDMLProtoAttribute(IndexItemInfo->ProtoAttribType, PDMLProtoItem);

				if (Attrib)
					sstrncat(m_summaryItemsData[m_currentSection], Attrib, NETPDL_MAX_STRING + 1);
				break;
			}

			case nbNETPDL_IDEL_PKTHDR:
			{
			struct _nbNetPDLElementPacketHdr *IndexItemInfo;
			char BufferString[NETPDL_MAX_STRING];

				IndexItemInfo= (struct _nbNetPDLElementPacketHdr *) NetPDLIndexItem;

				switch (IndexItemInfo->Value)
				{
					case nbNETPDL_ID_NODE_SHOWCODE_PKTHDR_ATTRIB_TIMESTAMP:
					{
					// We have to use this timesec variable in order to avoid a problem with VS2005, in which
					// localtime requires a 64bit value
					time_t timesec;
					struct tm *Time;

						timesec= (long) m_PDMLPacket->TimestampSec;
						Time= localtime( &timesec );

						strftime(BufferString, sizeof(BufferString), "%H:%M:%S", Time);
						sstrncat(m_summaryItemsData[m_currentSection], BufferString, NETPDL_MAX_STRING + 1);
						break;
					}
					case nbNETPDL_ID_NODE_SHOWCODE_PKTHDR_ATTRIB_NUM:
					{
						ssnprintf(BufferString, sizeof(BufferString), "%d", m_PDMLPacket->Number);
						sstrncat(m_summaryItemsData[m_currentSection], BufferString, NETPDL_MAX_STRING + 1);
						break;
					}
					case nbNETPDL_ID_NODE_SHOWCODE_PKTHDR_ATTRIB_LEN:
					{
						ssnprintf(BufferString, sizeof(BufferString), "%d", m_PDMLPacket->Length);
						sstrncat(m_summaryItemsData[m_currentSection], BufferString, NETPDL_MAX_STRING + 1);
						break;
					}
					case nbNETPDL_ID_NODE_SHOWCODE_PKTHDR_ATTRIB_CAPLEN:
					{
						ssnprintf(BufferString, sizeof(BufferString), "%d", m_PDMLPacket->CapturedLength);
						sstrncat(m_summaryItemsData[m_currentSection], BufferString, NETPDL_MAX_STRING + 1);
						break;
					}
				}

				break;
			};

			case nbNETPDL_IDEL_IF:
			{
//			int RetVal;
			unsigned int Result;
			struct _nbNetPDLElementIf *IndexItemInfo;

				if (PDMLProtoItem == NULL)
					break;

				IndexItemInfo= (struct _nbNetPDLElementIf *) NetPDLIndexItem;

//				RetVal= m_exprHandler->EvaluateExprNumber(IndexItemInfo->ExprTree, PDMLProtoItem->FirstField, &Result);
				m_exprHandler->EvaluateExprNumber(IndexItemInfo->ExprTree, PDMLProtoItem->FirstField, &Result);

				// If the 'if' is verified, Result contains a non-zero number
				if (Result)
				{
					if (PrintProtoBlockDetails(PDMLProtoItem, CurrentOffset, SnapLen, IndexItemInfo->FirstValidChildIfTrue) == nbFAILURE)
						return nbFAILURE;
				}
				else
				{
					if (IndexItemInfo->FirstValidChildIfFalse)
					{
						if (PrintProtoBlockDetails(PDMLProtoItem, CurrentOffset, SnapLen, IndexItemInfo->FirstValidChildIfFalse) == nbFAILURE)
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
			break;
		}

		// Move to the next element
		NetPDLIndexItem= nbNETPDL_GET_ELEMENT(NetPDLDatabase, NetPDLIndexItem->NextSibling);
	}

	return nbSUCCESS;
}



/*!
	\brief This function must be called when the packet is ended
	
	This function tells the PSML engine that the content of the SummaryItemsData variable must 
	be copied into a proper PSML fragment.

	\return nbSUCCESS if the function is successful, nbFAILURE if something goes wrong.
	The error message is returned in the m_errbuf class variable, which will always be NULL-terminated.
*/
int CPSMLMaker::PacketDecodingEnded()
{
	// If the user does not want to create the packet summary, exit right now.
	if (!m_isVisExtRequired)
		return nbSUCCESS;

	if (m_keepAllPackets)
	{
	unsigned int BytesToWrite;
	char *BufferPtr;

		BufferPtr= m_tempAsciiBuffer.GetStartBufferPtr();
		BytesToWrite= GetCurrentPacketAscii(BufferPtr, m_tempAsciiBuffer.GetBufferTotSize() );

		if (m_PSMLReader->StorePacket(BufferPtr, BytesToWrite) == nbFAILURE)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "%s", m_PSMLReader->GetLastError() );
			return nbFAILURE;
		}
	}

	return nbSUCCESS;
}



/*!
	\brief This function initializes the summary view related to a new packet.

	This function is required in order put to zero the vector that keeps the summary strings,
	and to put the m_currentSection variable to the first one.

	This function is called:
	- when the PSML engine is instantiated
	- when the decoding of a packet is completed, and it's time to create the PSML fragment
*/
void CPSMLMaker::PacketInitialize(_nbPDMLPacket *PDMLPacket)
{
	// Initializes the variables that keeps the text that has to be inserted in each section
	for (unsigned int i= 0; i <= NetPDLDatabase->ShowSumStructureNItems; i++)
		m_summaryItemsData[i][0] = 0;

	m_currentSection= 0;
	m_PDMLPacket= PDMLPacket;
}



/*!
	\brief It returns the structure of the summary of the PSML document.

	This function returns an ascii buffer that looks like an XML fragment (&ltstructure&gt;, and more).

	\param Buffer: a user-allocated buffer that will keep the resulting PSML fragment.

	\param BufferSize: size of 'Buffer'.

	\return The number of bytes written into the buffer.
*/
int CPSMLMaker::GetSummaryAscii(char *Buffer, int BufferSize)
{
int BufIndex;

	BufIndex= 0;

	// Just in case
	if (BufferSize > 0)
		*Buffer= 0;

	// Dump the summary
	BufIndex= ssnprintf(&Buffer[BufIndex], BufferSize - BufIndex, "<%s>\n", PSML_INDEXSTRUCTURE);

	for (unsigned int i= 0; i < NetPDLDatabase->ShowSumStructureNItems; i++)
	{
		BufIndex+= ssnprintf(&Buffer[BufIndex], BufferSize - BufIndex, "<%s>%s</%s>\n", PSML_SECTION, m_summaryStructData[i], PSML_SECTION);
	}

	BufIndex+= ssnprintf(&Buffer[BufIndex], BufferSize - BufIndex, "</%s>\n", PSML_INDEXSTRUCTURE);

	return BufIndex;
}



/*!
	\brief It returns the current item in the PSML document.

	This function returns an ascii buffer that looks like an XML fragment (&ltpacket&gt;, and more).

	\param Buffer: a user-allocated buffer that will keep the resulting PSML fragment.

	\param BufferSize: size of 'Buffer'.

	\return The number of bytes written into the buffer.
*/
int CPSMLMaker::GetCurrentPacketAscii(char *Buffer, int BufferSize)
{
int BufIndex;

	BufIndex= 0;

	// Dump the summary
	BufIndex= snprintf(&Buffer[BufIndex], BufferSize - BufIndex, "<%s>\n", PSML_PACKET);

	for (unsigned int i= 0; i < NetPDLDatabase->ShowSumStructureNItems; i++)
	{
		BufIndex+= ssnprintf(&Buffer[BufIndex], BufferSize - BufIndex, "<%s>%s</%s>\n", PSML_SECTION, m_summaryItemsData[i], PSML_SECTION);
	}

	BufIndex+= ssnprintf(&Buffer[BufIndex], BufferSize - BufIndex, "</%s>\n", PSML_PACKET);

	return BufIndex;
}
