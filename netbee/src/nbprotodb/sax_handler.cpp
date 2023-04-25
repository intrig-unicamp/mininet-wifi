/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include <xercesc/sax/AttributeList.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/sax/SAXException.hpp>

#include <nbprotodb_defs.h>
#include "sax_handler.h"
#include "protodb_globals.h"
#include "../nbee/globals/globals.h"
#include "../nbee/globals/debug.h"
#include "../nbee/globals/utils.h"


//! Global variable; used when we have avoid several handlers for similar tags.
//! With this variable we can do some sligthly different processing wuthin an unique handler.
int CurrentElementType;



/*!
	\param ErrBuf: pointer to a user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer belongs to the class that creates this one.
	Data stored in this buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.
*/
CNetPDLSAXHandler::CNetPDLSAXHandler(char *ErrBuf, int ErrBufSize)
{
	m_errorOccurred= false;

	m_currentLevel= 0;
	m_currNetPDLElementNestingLevel= 0;

	// Store internally the pointer to the error buffer. This buffer belongs to the class that creates this one.
	m_errbuf= ErrBuf;
	m_errbufSize= ErrBufSize;

	memset(NetPDLNestingLevels, 0, sizeof(uint32_t) * NETPDL_MAX_NESTING_LEVELS);
}

CNetPDLSAXHandler::~CNetPDLSAXHandler()
{
}



void CNetPDLSAXHandler::startElement(const XMLCh* const URI, const XMLCh* const LocalName, const XMLCh* const Qname, const Attributes& Attributes)
{
	struct _nbNetPDLElementBase **GlobalElementsList;
	struct _nbNetPDLElementBase *NetPDLElement;
	unsigned int i= 0;
	char ElementName[NETPDL_MAX_STRING + 1];

	GlobalElementsList= NetPDLDatabase->GlobalElementsList;

	XMLString::transcode(LocalName, ElementName, NETPDL_MAX_STRING);
	while (ElementList[i].Type != nbNETPDL_IDEL_INVALID)
	{
		if (XMLString::compareString(ElementName, ElementList[i].Name) == 0)
			break;

		i++;
	}

	// No suitable elements have been found
	if (ElementList[i].Type == nbNETPDL_IDEL_INVALID)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "NetPDL file contains an element that has not been recognized: <%s>.", ElementName);
		m_errorOccurred= true;
		return;
	}

	// Allocates proper element
	CurrentElementType= ElementList[i].Type;

	NetPDLElement= ElementList[i].NetPDLElementCreateHandler(Attributes, m_errbuf, m_errbufSize);
	if (NetPDLElement == NULL)
	{
		m_errorOccurred= true;
		return;
	}

	// Set basic members of this structure
	NetPDLElement->Type= ElementList[i].Type;
	NetPDLElement->Parent= nbNETPDL_INVALID_ELEMENT;
	NetPDLElement->FirstChild= nbNETPDL_INVALID_ELEMENT;
	NetPDLElement->PreviousSibling= nbNETPDL_INVALID_ELEMENT;
	NetPDLElement->NextSibling= nbNETPDL_INVALID_ELEMENT;

	// Let's check if we have a 'callhandle'
	if (AllocateCallHandle(GetXMLAttribute(Attributes, NETPDL_COMMON_ATTR_CALLHANDLE), &(NetPDLElement->CallHandlerInfo), m_errbuf, m_errbufSize) == nbFAILURE)
	{
		m_errorOccurred= true;
		return;
	}


#ifdef _DEBUG
	XMLString::transcode(LocalName, NetPDLElement->ElementName, sizeof (NetPDLElement->ElementName) - 1);
	NetPDLElement->NestingLevel= m_currentLevel;
	NetPDLElement->ElementNumber= NetPDLDatabase->GlobalElementsListNItems;
#endif

	// Update child / parent links
	if (m_currentLevel > 0)
	{
	uint32_t ParentElementIndex;

		ParentElementIndex= NetPDLNestingLevels[m_currentLevel - 1];

		// Update parent
		NetPDLElement->Parent= ParentElementIndex;

		// If needed, update the child ptr in the parent node as well
		if (GlobalElementsList[ParentElementIndex]->FirstChild == 0)
			GlobalElementsList[ParentElementIndex]->FirstChild= NetPDLDatabase->GlobalElementsListNItems;
	}

	uint32_t PrevElementAtSameLevelIndex;

	// Update previous / next links (only for nodes following the first one)
	PrevElementAtSameLevelIndex= NetPDLNestingLevels[m_currentLevel];
	if ((PrevElementAtSameLevelIndex != 0) &&
		(GlobalElementsList[PrevElementAtSameLevelIndex]->Parent == NetPDLElement->Parent))
	{
	uint32_t PreviousElementIndex;

		PreviousElementIndex= NetPDLNestingLevels[m_currentLevel];

		// Update previous
		NetPDLElement->PreviousSibling= PreviousElementIndex;

		// Update next ptr in the previous node as well
		GlobalElementsList[PreviousElementIndex]->NextSibling= NetPDLDatabase->GlobalElementsListNItems;
	}

	NetPDLNestingLevels[m_currentLevel]= NetPDLDatabase->GlobalElementsListNItems;
	GlobalElementsList[NetPDLDatabase->GlobalElementsListNItems]= NetPDLElement;


	m_currNetPDLElementNestingLevel= m_currentLevel;

	NetPDLDatabase->GlobalElementsListNItems++;
	m_currentLevel++;

	// Update global variable

	if (m_currentLevel == NETPDL_MAX_NESTING_LEVELS)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: reached the nesting level limit.");
	    m_errorOccurred= true;
	}

	if (NetPDLDatabase->GlobalElementsListNItems == NETPDL_MAX_NELEMENTS)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: reached the maximum number of NetPDL elements.");
	    m_errorOccurred= true;
	}
}




void CNetPDLSAXHandler::endElement(const XMLCh* const URI, const XMLCh* const LocalName, const XMLCh* const Qname)
{
	m_currentLevel--;
}


//---------------------------------------------------------------------------
// Overrides of the SAX2 ErrorHandler interface
//---------------------------------------------------------------------------
void CNetPDLSAXHandler::error(const SAXParseException& Exception)
{
//char Message[NETPDL_MAX_STRING + 1];
//char FileName[NETPDL_MAX_STRING + 1];
//uint32 line_no;
//uint32 column_no;	

    m_errorOccurred= true;

	// This code is not needed here; we intercept and print the error in the sas_parser
	//XMLString::transcode(Exception.getMessage(), Message, NETPDL_MAX_STRING);
	//XMLString::transcode(Exception.getSystemId(), FileName, NETPDL_MAX_STRING);
	//line_no = Exception.getLineNumber();
	//column_no = Exception.getColumnNumber();
	//
	//errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
	//	"Error at file %s, line %d, column %d: %s" , 
	//	FileName, line_no, column_no, Message);

	throw Exception;
}


void CNetPDLSAXHandler::fatalError(const SAXParseException& Exception)
{
//char Message[NETPDL_MAX_STRING + 1];
//char FileName[NETPDL_MAX_STRING + 1];
//uint32 line_no;
//uint32 column_no;

    m_errorOccurred= true;

	// This code is not needed here; we intercept and print the error in the sas_parser
	//XMLString::transcode(Exception.getMessage(), Message, NETPDL_MAX_STRING);
	//XMLString::transcode(Exception.getSystemId(), FileName, NETPDL_MAX_STRING);
	//line_no = Exception.getLineNumber();
	//column_no = Exception.getColumnNumber();	

	//errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
	//	"Fatal error at file %s, line %d, char %d: %s" , 
	//	FileName, line_no, column_no, Message);

	throw Exception;
}


void CNetPDLSAXHandler::warning(const SAXParseException& Exception)
{
char Message[NETPDL_MAX_STRING + 1];
char FileName[NETPDL_MAX_STRING + 1];
uint32_t LineNumber;
uint32_t ColumnNumber;

	XMLString::transcode(Exception.getMessage(), Message, NETPDL_MAX_STRING);
	XMLString::transcode(Exception.getSystemId(), FileName, NETPDL_MAX_STRING);
	LineNumber= (uint32_t) Exception.getLineNumber();
	ColumnNumber= (uint32_t) Exception.getColumnNumber();		

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
		"Warning at file %s, line %d, char %d: %s" , 
		FileName, LineNumber, ColumnNumber, Message);
}

