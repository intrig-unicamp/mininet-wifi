/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <stdlib.h>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>

// Indicate using Xerces-C++ namespace in general
// This is required starting from Xerces 2.2.0
XERCES_CPP_NAMESPACE_USE


#include "sax_parser.h"
#include "sax_handler.h"

#include <nbprotodb_defs.h>
#include "protodb_globals.h"
#include "lists_organize.h"
#include "expressions.h"

#include "../nbee/globals/globals.h"
#include "../nbee/globals/debug.h"
#include "../nbee/globals/utils.h"


// Variables defined in the expression file; needed to create a linked list among the expressions.
extern /*"C"*/ struct _nbNetPDLExprBase* FirstExpression;


//! Default constructor
CNetPDLSAXParser::CNetPDLSAXParser()
{
	NetPDLDatabase= NULL;
	memset(m_errbuf, 0, sizeof(m_errbuf));
}


//! Default destructor
CNetPDLSAXParser::~CNetPDLSAXParser()
{
	// Call the Xerces termination method
    XMLPlatformUtils::Terminate();

#if defined(_DEBUG) && defined(_MSC_VER) && defined (_CRTDBG_MAP_ALLOC)
//#ifdef _CRTDBG_MAP_ALLOC
//	#pragma message( "Memory leaks checking turned on" )
	_CrtDumpMemoryLeaks();
// Uncomment this line for dumping memory leaks
#endif
}



/*!
	\brief Initialize the class by setting the NetPDL description to be parsed.

	\param FileName: NULL terminated string that keeps the name of the file containing
	the NetPDL description to be used in the parsing.

	\param Flags: Type of the NetPDL protocol database we want to load. It can assume the values defined in #nbProtoDBFlags.

	\return Pointer to the main structure that contains the NetPDL database, or NULL in case
	of errors. The error message can be retrieved by the GetLastError() method.
*/
struct _nbNetPDLDatabase *CNetPDLSAXParser::Initialize(const char* FileName, int Flags)
{
	struct _nbNetPDLExprBase* CurrentExpression;
	SAX2XMLReader *SAXParser;			// Keeps a Xerces SAX parser object
	unsigned int i;
	int StandardProtosArePresent;
	int Result;

	// Initialize the XML4C system
	try
	{
		XMLPlatformUtils::Initialize();
	}
	catch (...)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Fatal error during Xerces initialization.");
		return NULL;
	}

	SAXParser= XMLReaderFactory::createXMLReader();
	if (SAXParser == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Failed creating SAX Parser.");
		return NULL;
	}

	SAXParser->setFeature(XMLUni::fgSAX2CoreNameSpaces, true);
	SAXParser->setFeature(XMLUni::fgXercesSchema, true);
	SAXParser->setFeature(XMLUni::fgXercesSchemaFullChecking, true);
	SAXParser->setFeature(XMLUni::fgXercesIdentityConstraintChecking, true);
	SAXParser->setFeature(XMLUni::fgSAX2CoreNameSpacePrefixes, true);
	// Turns on validation (either DTD or Schema) on the XML file only if the structure is specified within the NetPDL file
	// Please remember to use an XML header like the following in order to turn validation on:
	//	<netpdl name="nbee.org NetPDL Database" version="0.9"
	//		creator="nbee.org" date="07-12-2010"
	//		xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	//		xsi:noNamespaceSchemaLocation="netpdl.xsd">
	// Please remember that validation occurs only if the XSD schema is embedded in the NetPDL file;
	// validation does not work if performed against an external file, not linked in the XML.
	if (Flags & nbPROTODB_VALIDATE)
		SAXParser->setFeature(XMLUni::fgSAX2CoreValidation, true);
	else
		SAXParser->setFeature(XMLUni::fgSAX2CoreValidation, false);

	SAXParser->setFeature(XMLUni::fgXercesDynamic, true);
	
	//  Create our SAX handler object and install it on the parser, as the
    //  document and error handler.
    CNetPDLSAXHandler NetPDLSAXHandler(m_errbuf, sizeof(m_errbuf));
    SAXParser->setContentHandler(&NetPDLSAXHandler);
    SAXParser->setErrorHandler(&NetPDLSAXHandler);
 

	// Create main structure in memory
	NetPDLDatabase= (struct _nbNetPDLDatabase *) malloc(sizeof(struct _nbNetPDLDatabase));
	if (NetPDLDatabase == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory for building the protocol database.");
		goto ClearAndExit;
	}
	memset(NetPDLDatabase, 0, sizeof(struct _nbNetPDLDatabase));

	NetPDLDatabase->Flags= Flags;

	NetPDLDatabase->GlobalElementsList= (struct _nbNetPDLElementBase **) malloc(sizeof(struct _nbNetPDLElementBase *) * NETPDL_MAX_NELEMENTS);

	if (NetPDLDatabase->GlobalElementsList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory for building the protocol database.");
			goto ClearAndExit;
	}

	// Initialize the first item to NULL; this is used to refer to an invalid element
	NetPDLDatabase->GlobalElementsList[nbNETPDL_INVALID_ELEMENT]= NULL;
	NetPDLDatabase->GlobalElementsListNItems=1;

	// Load the description in memory and get document root
	try
	{
		// Create a progressive scan token
		XMLPScanToken XMLScanToken;
		bool MoreData= true;

		if (!SAXParser->parseFirst(FileName, XMLScanToken))
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "NetPDL parsing failed");
			goto ClearAndExit;
		}

		// We started ok, so lets call scanNext()
		// until we find what we want or hit the end.
		while (MoreData && !NetPDLSAXHandler.ErrorOccurred())
			MoreData= SAXParser->parseNext(XMLScanToken);
	}
	// Catch only exceptions that are generated in XML; do not catch other exceptions
	//   (I prefer to have a clear indication of where the problem is)
	catch (const XMLException& Exception)
	{
		char Message[NETPDL_MAX_STRING + 1];

		XMLString::transcode(Exception.getMessage(), Message, NETPDL_MAX_STRING);
	
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
			"Exception during parsing: %s.", Message);

		goto ClearAndExit;
	}
	// Following exceptions are generated only when the XSD schema validation is turned on
	catch (const SAXParseException& Exception)
	{
		char Message[NETPDL_MAX_STRING + 1];
		char FileName[NETPDL_MAX_STRING + 1];
		uint32_t LineNumber;
		uint32_t ColumnNumber;	

		XMLString::transcode(Exception.getMessage(), Message, NETPDL_MAX_STRING);
		XMLString::transcode(Exception.getSystemId(), FileName, NETPDL_MAX_STRING);
		LineNumber= (uint32_t) Exception.getLineNumber();
		ColumnNumber= (uint32_t) Exception.getColumnNumber();
	
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), 
			"Error at file %s, line %d, column %d: %s." , 
			FileName, LineNumber, ColumnNumber, Message);

		goto ClearAndExit;
	}



// Uncomment this if you have some bugs in the parsing and you want to print on screen what has been recognized so far
//#define _DEBUG_LOUD
#ifdef _DEBUG_LOUD
	PrintElements();
#endif

	if (NetPDLSAXHandler.ErrorOccurred())
		goto ClearAndExit;
	
	// Now, basic parsing has been completed.
	// Let's create the links between elements.

	NetPDLDatabase->ProtoListNItems= CountSpecificElement(nbNETPDL_IDEL_PROTO);
	NetPDLDatabase->ProtoList= OrganizeListProto(NetPDLDatabase->ProtoListNItems, m_errbuf, sizeof(m_errbuf));

	if (NetPDLDatabase->ProtoList == NULL)
		goto ClearAndExit;

	NetPDLDatabase->ShowSumStructureNItems= CountSpecificElement(nbNETPDL_IDEL_SUMSECTION);
	NetPDLDatabase->ShowSumStructureList= OrganizeListShowSumStructure(NetPDLDatabase->ShowSumStructureNItems, &Result, m_errbuf, sizeof(m_errbuf));

	if (Result == nbFAILURE)
		goto ClearAndExit;

	NetPDLDatabase->ShowSumTemplateNItems= CountSpecificElement(nbNETPDL_IDEL_SHOWSUMTEMPLATE);
	NetPDLDatabase->ShowSumTemplateList= OrganizeListShowSumTemplate(NetPDLDatabase->ShowSumTemplateNItems, &Result, m_errbuf, sizeof(m_errbuf));

	if (Result == nbFAILURE)
		goto ClearAndExit;

	NetPDLDatabase->ShowTemplateNItems= CountSpecificElement(nbNETPDL_IDEL_SHOWTEMPLATE);
	NetPDLDatabase->ShowTemplateList= OrganizeListShowTemplate(NetPDLDatabase->ShowTemplateNItems, &Result, m_errbuf, sizeof(m_errbuf));

	if (Result == nbFAILURE)
		goto ClearAndExit;

	NetPDLDatabase->ADTNItems= CountSpecificADTElement(nbNETPDL_IDEL_ADT, nbNETPDL_IDEL_NETPDL);
	if (NetPDLDatabase->ADTNItems)
	{
		NetPDLDatabase->ADTList= OrganizeListGlobalAdt(NetPDLDatabase->ADTNItems, m_errbuf, sizeof(m_errbuf));

		if (NetPDLDatabase->ADTList == NULL)
			goto ClearAndExit;
	}

	NetPDLDatabase->LocalADTNItems= CountSpecificADTElement(nbNETPDL_IDEL_ADT, nbNETPDL_IDEL_PROTO);
	if (NetPDLDatabase->LocalADTNItems)
	{
		NetPDLDatabase->LocalADTList= OrganizeListLocalAdt(NetPDLDatabase->LocalADTNItems, m_errbuf, sizeof(m_errbuf));

		if (NetPDLDatabase->LocalADTList == NULL)
			goto ClearAndExit;
	}

	StandardProtosArePresent= 0;

	// Locate the first and the last protocol
	for (i= 0; i <	NetPDLDatabase->ProtoListNItems; i++)
	{
		if (strcmp(NetPDLDatabase->ProtoList[i]->Name, NETPDL_PROTOCOL_STARTPROTO) == 0)
		{
			NetPDLDatabase->StartProtoIndex= i;
			StandardProtosArePresent++;
		}
		if (strcmp(NetPDLDatabase->ProtoList[i]->Name, NETPDL_PROTOCOL_DEFAULTPROTO) == 0)
		{
			NetPDLDatabase->DefaultProtoIndex= i;
			StandardProtosArePresent++;
		}

		// Etherpadding is important (and it has to be managed with care), but it may be missing
		if (strcmp(NetPDLDatabase->ProtoList[i]->Name, NETPDL_PROTOCOL_ETHERPADDINGPROTO) == 0)
			NetPDLDatabase->EtherpaddingProtoIndex= i;
	}

	if (StandardProtosArePresent != 2)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), 
			"Error: protocols '%s' and '%s' must always be present in a valid NetPDL file.",
			NETPDL_PROTOCOL_STARTPROTO, NETPDL_PROTOCOL_DEFAULTPROTO);

		goto ClearAndExit;
	}

	// Scan all the elements in order to create cross links between them
	for (i= 1; i <	NetPDLDatabase->GlobalElementsListNItems; i++)
	{
	struct _nbNetPDLElementBase *NetPDLElement;

		NetPDLElement= NetPDLDatabase->GlobalElementsList[i];
		Result= ElementList[NetPDLElement->Type].NetPDLElementOrganizeHandler(NetPDLElement, m_errbuf, sizeof(m_errbuf));

		if (Result == nbFAILURE)
			goto ClearAndExit;
	}

	// Update the pointer related to the first expression
	NetPDLDatabase->ExpressionList= FirstExpression;

	// Update expressions (in case this is needed)
	CurrentExpression= FirstExpression;
	while (CurrentExpression)
	{
		if (UpdateProtoRefInExpression(CurrentExpression) == nbFAILURE)
			goto ClearAndExit;
		CurrentExpression= CurrentExpression->NextExpression;
	}

	// Initialize the index of NetPDL elements allocated for ADT replication
	NetPDLDatabase->GlobalElementsListNItemsPlusADTCopies= NetPDLDatabase->GlobalElementsListNItems;

	/* Scan all NetPDL elements in order to swap all adt calls performed by 
		'ADTFIELD' element with the actual fields 
	*/
	for (i= 1; i <	NetPDLDatabase->GlobalElementsListNItems; i++)
	{
		if (NetPDLDatabase->GlobalElementsList[i]->Type == nbNETPDL_IDEL_ADTFIELD)
		{
		struct _nbNetPDLElementAdtfield *NetPDLElementAdtfield;
		struct _nbNetPDLElementAdtfield NetPDLElementAdtfieldBuf;
		struct _nbNetPDLElementAdt *NetPDLElementADTWanted= NULL;
		size_t SizeToDuplicate;
		struct _nbNetPDLElementFieldBase *NetPDLElementField;

			NetPDLElementAdtfield= (struct _nbNetPDLElementAdtfield *) NetPDLDatabase->GlobalElementsList[i];
			const char *CurrProtoName= GetProtoName((struct _nbNetPDLElementBase *) NetPDLElementAdtfield);

			// Let's check the 'adt' -- Local ADTs first
			for (unsigned int j= 0; (NetPDLElementADTWanted == NULL) && (j < NetPDLDatabase->LocalADTNItems); j++)
			{
				if (strcmp(NetPDLDatabase->LocalADTList[j]->ProtoName, CurrProtoName) == 0)
					if (strcmp(NetPDLDatabase->LocalADTList[j]->ADTName, NetPDLElementAdtfield->CalledADTName) == 0)
						NetPDLElementADTWanted= NetPDLDatabase->LocalADTList[j];
			}

			// Let's check the 'adt' -- Global ADTs as last resort
			for (unsigned int j= 0; (NetPDLElementADTWanted == NULL) && (j < NetPDLDatabase->ADTNItems); j++)
			{
				if (strcmp(NetPDLDatabase->ADTList[j]->ADTName, NetPDLElementAdtfield->CalledADTName) == 0)
					NetPDLElementADTWanted= NetPDLDatabase->ADTList[j];
			}

			if (NetPDLElementADTWanted == NULL)
			{
				// If the ADT has not been found, let's print an error
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), 
					"Adtfield '%s' points to field-ADT '%s' which does not exist.",
					NetPDLElementAdtfield->Name, NetPDLElementAdtfield->CalledADTName);

				goto ClearAndExit;
			}

			// Save current 'ADTFIELD' element in order to keep information 
			// that the new replacement inherits
			memcpy(&NetPDLElementAdtfieldBuf, NetPDLElementAdtfield, sizeof(struct _nbNetPDLElementAdtfield));
			NetPDLElementAdtfield= NULL;

			// Deallocate data structure related to current 'ADTFIELD' element
			free(NetPDLDatabase->GlobalElementsList[i]);
			NetPDLDatabase->GlobalElementsList[i]= NULL;

			// Assign suited 'FIELD' element whose adt is based on
			switch(NetPDLElementADTWanted->ADTFieldInfo->FieldType)
			{
			case nbNETPDL_ID_FIELD_FIXED:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldFixed); break;
			case nbNETPDL_ID_FIELD_BIT:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldBit); break;
			case nbNETPDL_ID_FIELD_VARIABLE:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldVariable); break;
			case nbNETPDL_ID_FIELD_TOKENENDED:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldTokenEnded); break;
			case nbNETPDL_ID_FIELD_TOKENWRAPPED:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldTokenWrapped); break;
			case nbNETPDL_ID_FIELD_LINE:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldLine); break;
			case nbNETPDL_ID_FIELD_PATTERN:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldPattern); break;
			case nbNETPDL_ID_FIELD_EATALL:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldEatall); break;
			case nbNETPDL_ID_FIELD_PADDING:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldPadding); break;
			case nbNETPDL_ID_FIELD_PLUGIN:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldPlugin); break;
			case nbNETPDL_ID_CFIELD_TLV:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementCfieldTLV); break;
			case nbNETPDL_ID_CFIELD_DELIMITED:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementCfieldDelimited); break;
			case nbNETPDL_ID_CFIELD_LINE:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementCfieldLine); break;
			case nbNETPDL_ID_CFIELD_HDRLINE:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementCfieldHdrline); break;
			case nbNETPDL_ID_CFIELD_DYNAMIC:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementCfieldDynamic); break;
			case nbNETPDL_ID_CFIELD_ASN1:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementCfieldASN1); break;
			case nbNETPDL_ID_CFIELD_XML:
				SizeToDuplicate= sizeof(struct _nbNetPDLElementCfieldXML); break;
			default:
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
						"<adtfield> element cannot include invalid field element.");

					goto ClearAndExit;
				}
			}

			// Let's allocate new NetPDL field element to replace the 'ADTFIELD' just removed
			NetPDLDatabase->GlobalElementsList[i]= (struct _nbNetPDLElementBase *) malloc(SizeToDuplicate);

			if (NetPDLDatabase->GlobalElementsList[i] == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
					"Not enough memory for creating internal structures.");

				goto ClearAndExit;
			}

			// Let's copy all saved information to the new field element
			memcpy(NetPDLDatabase->GlobalElementsList[i], NetPDLElementADTWanted->ADTFieldInfo, SizeToDuplicate);

			// Let's restore links for the 'ADTFIELD' replacement
			NetPDLDatabase->GlobalElementsList[i]->Parent= NetPDLElementAdtfieldBuf.Parent;
			NetPDLDatabase->GlobalElementsList[i]->FirstChild= NetPDLElementAdtfieldBuf.FirstChild;
			NetPDLDatabase->GlobalElementsList[i]->PreviousSibling= NetPDLElementAdtfieldBuf.PreviousSibling;
			NetPDLDatabase->GlobalElementsList[i]->NextSibling= NetPDLElementAdtfieldBuf.NextSibling;

			// Let's overwrite name, longname, ..., if needed
			NetPDLElementField= (struct _nbNetPDLElementFieldBase *) NetPDLDatabase->GlobalElementsList[i];
			NetPDLElementField->Type= nbNETPDL_IDEL_FIELD;
			NetPDLElementField->Name= NetPDLElementAdtfieldBuf.Name ? NetPDLElementAdtfieldBuf.Name : strdup(NetPDLElementADTWanted->ADTFieldInfo->Name);
			NetPDLElementField->LongName= NetPDLElementAdtfieldBuf.LongName ? NetPDLElementAdtfieldBuf.LongName : strdup(NetPDLElementADTWanted->ADTFieldInfo->LongName);
			NetPDLElementField->ShowTemplateName= NetPDLElementAdtfieldBuf.ShowTemplateName ? NetPDLElementAdtfieldBuf.ShowTemplateName : strdup(NetPDLElementADTWanted->ADTFieldInfo->ShowTemplateName);
			NetPDLElementField->ShowTemplateInfo= NetPDLElementAdtfieldBuf.ShowTemplateInfo ? NetPDLElementAdtfieldBuf.ShowTemplateInfo : NetPDLElementADTWanted->ADTFieldInfo->ShowTemplateInfo;
			NetPDLElementField->ADTRef= NetPDLElementAdtfieldBuf.CalledADTName;

			if (!NetPDLElementField->Name || !NetPDLElementField->LongName || !NetPDLElementField->ShowTemplateName)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
					"Not enough memory for creating internal structures.");

				goto ClearAndExit;
			}
		}
	}

	/* Scan all NetPDL elements in order to get all adt calls (they are 
		performed via 'ADTFIELD' element or 'baseadt' attribute) and replace 
		them with ADT's elements */
	// Let's duplicate at first ADTs which are called within other ADTs
	for (i= 1; i <	NetPDLDatabase->GlobalElementsListNItems; i++)
	{
		if (NetPDLDatabase->GlobalElementsList[i]->Type == nbNETPDL_IDEL_ADT)
			DuplicateInnerADT(NETPDL_GET_ELEMENT(NetPDLDatabase->GlobalElementsList[i]->FirstChild));
	}
	// Let's duplicate remaining ADT calls
	for (i= 1; i <	NetPDLDatabase->GlobalElementsListNItems; i++)
	{
		if (NetPDLDatabase->GlobalElementsList[i]->Type == nbNETPDL_IDEL_FORMAT)
			DuplicateInnerADT(NETPDL_GET_ELEMENT(NetPDLDatabase->GlobalElementsList[i]->FirstChild));
	}

	return NetPDLDatabase;

ClearAndExit:
	if (SAXParser)
		delete SAXParser;
	if (NetPDLDatabase)
		free(NetPDLDatabase);
	return NULL;
}


/*!
	\brief Function that cleans all data related to the NetPDL database.

	This method cleans the NetPDL database, but it does not clean the internal
	data of this class, which is cleared when this class is deallocated.
*/
void CNetPDLSAXParser::Cleanup()
{
	// The expression list must not be deleted here; expressions have the Next member
	// in the definition of their base structure; therefore the linked list is automatically
	// cleared when deleting expressions.
	FirstExpression= NULL;

	// TEMP FABIO 20/05/2008: erase copies of ADT created at NetPDL Database creation time
	// Scan all ADT duplicated copies in order to delete them (they were been created by dynamic allocation)
	for (unsigned int i= NetPDLDatabase->GlobalElementsListNItems; i < NetPDLDatabase->GlobalElementsListNItemsPlusADTCopies; i++)
	{
		free(NetPDLDatabase->GlobalElementsList[i]);
	}

	// Scan all the elements in order to delete dynamic allocations in them
	for (unsigned int i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
	struct _nbNetPDLElementBase *NetPDLElement;

		NetPDLElement= NetPDLDatabase->GlobalElementsList[i];

		if (NetPDLElement->CallHandlerInfo)
		{
			DELETE_PTR(NetPDLElement->CallHandlerInfo->FunctionName);
			DELETE_PTR(NetPDLElement->CallHandlerInfo);
		}

		ElementList[NetPDLElement->Type].NetPDLElementDeleteHandler(NetPDLElement);
	}

	FREE_PTR(NetPDLDatabase->ProtoList);
	FREE_PTR(NetPDLDatabase->ShowSumStructureList);
	FREE_PTR(NetPDLDatabase->ShowSumTemplateList);
	FREE_PTR(NetPDLDatabase->ShowTemplateList);
	DELETE_PTR(NetPDLDatabase->ADTList);

	FREE_PTR(NetPDLDatabase->GlobalElementsList);

	FREE_PTR(NetPDLDatabase->Creator);
	FREE_PTR(NetPDLDatabase->CreationDate);

	FREE_PTR(NetPDLDatabase);
}


/*!
	\brief Updates the 'struct _nbNetPDLExprProtoRef' items.

	It updates these items by setting the protocol number in it.
	This function is recursive.

	\param Expression: expression that may contain something that require to be updated.

	\return nbSUCCESS if everything is fine, nbFAILURE in case of error.
	In the latter case, the error message is stored in the ErrBuf variable.
*/
int CNetPDLSAXParser::UpdateProtoRefInExpression(struct _nbNetPDLExprBase* Expression)
{
	if (Expression == NULL)
		return nbSUCCESS;

	switch (Expression->Type)
	{
		case nbNETPDL_ID_EXPR_OPERAND_EXPR:
		{
			if (UpdateProtoRefInExpression(((struct _nbNetPDLExpression*) Expression)->Operand1) != nbSUCCESS)
				return nbFAILURE;

			if (UpdateProtoRefInExpression(((struct _nbNetPDLExpression*) Expression)->Operand2) != nbSUCCESS)
				return nbFAILURE;

		}; break;

		case nbNETPDL_ID_EXPR_OPERAND_PROTOREF:
		{
		unsigned int i;
		struct _nbNetPDLExprProtoRef* ProtoRefItem;

			ProtoRefItem= (struct _nbNetPDLExprProtoRef*) Expression;

			for (i= 0; i < NetPDLDatabase->ProtoListNItems; i++)
			{
				if (strcmp(NetPDLDatabase->ProtoList[i]->Name, ProtoRefItem->ProtocolName) == 0)
				{
					ProtoRefItem->Value= i;
					return nbSUCCESS;
				}
			}

			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), 
				"Error: a reference to a protocol ('#%s') points to a non-existing protocol.", ProtoRefItem->ProtocolName);
			return nbFAILURE;

		}; break;

		default:
		{
			// Just to avoid a compiler warning on GCC
		}; break;

	}

	return nbSUCCESS;
}


/*!
	\brief Performs a complete scan of NetPDL elements from an initial given one in order to detect and process ADT calls.

	It recognizes ADT calls either through 'ADTFIELD' element or 'baseadt' attribute and
	it triggers relevant ADT duplication.
	This function is recursive.

	\param FirstElement: NetPDL element from which scanning starts (first child 
	                     of current ADT).

	\return nbSUCCESS if everything is fine, nbFAILURE in case of error.
*/
int CNetPDLSAXParser::DuplicateInnerADT(struct _nbNetPDLElementBase *FirstChildElement)
{
struct _nbNetPDLElementBase *NetPDLTempElement;

	NetPDLTempElement= FirstChildElement;

	while (NetPDLTempElement)
	{
		// Let's narrow searching for ADT calls to related NetPDL elements
		switch (NetPDLTempElement->Type)
		{
		case nbNETPDL_IDEL_FIELD:
		case nbNETPDL_IDEL_SUBFIELD:
		case nbNETPDL_IDEL_FIELDMATCH:
		case nbNETPDL_IDEL_DEFAULTITEM:
			{
			struct _nbNetPDLElementFieldBase *NetPDLFieldElement;

				NetPDLFieldElement= (struct _nbNetPDLElementFieldBase *) NetPDLTempElement;

				if (NetPDLFieldElement->ADTRef)
				{
				struct _nbNetPDLElementAdt *NetPDLElementADTWanted= NULL;
				const char *CurrProtoName= GetProtoName(NetPDLTempElement);

					// Let's check the 'adt' -- Local ADTs first
					for (unsigned int i= 0; (NetPDLElementADTWanted == NULL) && (i < NetPDLDatabase->LocalADTNItems); i++)
					{
						if (strcmp(NetPDLDatabase->LocalADTList[i]->ProtoName, CurrProtoName) == 0)
							if (strcmp(NetPDLDatabase->LocalADTList[i]->ADTName, NetPDLFieldElement->ADTRef) == 0)
								NetPDLElementADTWanted= NetPDLDatabase->LocalADTList[i];
					}

					// Let's check the 'adt' -- Global ADTs as last resort
					for (unsigned int i= 0; (NetPDLElementADTWanted == NULL) && (i < NetPDLDatabase->ADTNItems); i++)
					{
						if (strcmp(NetPDLDatabase->ADTList[i]->ADTName, NetPDLFieldElement->ADTRef) == 0)
							NetPDLElementADTWanted= NetPDLDatabase->ADTList[i];
					}

					if (NetPDLElementADTWanted == NULL)
					{
						// If the ADT has not been found, let's print an error
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
							"Adtfield points to field-ADT '%s' which does not exist.",
							NetPDLFieldElement->ADTRef);

						return nbFAILURE;
					}

					DuplicateADT(NetPDLFieldElement->ADTRef, NetPDLTempElement, NETPDL_GET_ELEMENT(NetPDLElementADTWanted->FirstChild), (struct _nbNetPDLElementReplace *) NETPDL_GET_ELEMENT(NetPDLTempElement->FirstChild));

					DELETE_PTR(NetPDLFieldElement->ADTRef);
					NetPDLFieldElement->ADTRef= NULL;
				}

			}; break;
		}

		if (NetPDLTempElement->FirstChild)
			DuplicateInnerADT(NETPDL_GET_ELEMENT(NetPDLTempElement->FirstChild));

		NetPDLTempElement= NETPDL_GET_ELEMENT(NetPDLTempElement->NextSibling);
	}

	return nbSUCCESS;
}



/*!
	\brief Process the ADT calls and enables ADT-unawareness for the NetPDL 
	Decoder.

	\param ADTToDuplicate: NULL terminated string that keeps the name of the 
	current ADT to duplicate. (It is a trivial parameter at this time, but
	it becomes essential to deals with namespacing technique for ADTs.)

	\param NetPDLParentElement: NetPDL Element on which ADT copy has to link as
	child element.

	\param NetPDLFirstElementToLink: first NetPDL Element whose the original ADT 
	declaration is parent.

	\param FirstReplaceElement: first NetPDL Element that belongs to <replace> 
	element listed within NetPDL Element performing ADT call.

	\return nbSUCCESS if everything is fine, nbFAILURE in case of error.
*/
int CNetPDLSAXParser::DuplicateADT(char *ADTToDuplicate, struct _nbNetPDLElementBase *NetPDLParentElement, struct _nbNetPDLElementBase *NetPDLFirstElementToLink, struct _nbNetPDLElementReplace *FirstReplaceElement)
{
struct _nbNetPDLElementBase *NetPDLTempElementFromADT;
struct _nbNetPDLElementBase *NetPDLTempElementDup;
uint32_t NetPDLFirstChildPtr= nbNETPDL_INVALID_ELEMENT;
struct _nbNetPDLElementBase *NetPDLPreviousElementDup= NULL;
struct _nbNetPDLElementFieldBase *NetPDLElementField;
size_t SizeToDuplicate;

	if (NetPDLFirstElementToLink == NULL)
		return nbSUCCESS;

	NetPDLTempElementFromADT= NetPDLFirstElementToLink;

	while (NetPDLTempElementFromADT)
	{
		NetPDLElementField= NULL;

		// Let's check the right amount of bytes to allocate in order ti duplicate 
		// current element
		switch(NetPDLTempElementFromADT->Type)
		{
		case nbNETPDL_IDEL_FIELD:
			{
				NetPDLElementField= (struct _nbNetPDLElementFieldBase *) NetPDLTempElementFromADT;

				switch(NetPDLElementField->FieldType)
				{
				case nbNETPDL_ID_FIELD_FIXED:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldFixed); break;
				case nbNETPDL_ID_FIELD_BIT:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldBit); break;
				case nbNETPDL_ID_FIELD_VARIABLE:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldVariable); break;
				case nbNETPDL_ID_FIELD_TOKENENDED:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldTokenEnded); break;
				case nbNETPDL_ID_FIELD_TOKENWRAPPED:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldTokenWrapped); break;
				case nbNETPDL_ID_FIELD_LINE:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldLine); break;
				case nbNETPDL_ID_FIELD_PATTERN:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldPattern); break;
				case nbNETPDL_ID_FIELD_EATALL:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldEatall); break;
				case nbNETPDL_ID_FIELD_PADDING:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldPadding); break;
				case nbNETPDL_ID_FIELD_PLUGIN:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldPlugin); break;
				case nbNETPDL_ID_CFIELD_TLV:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementCfieldTLV); break;
				case nbNETPDL_ID_CFIELD_DELIMITED:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementCfieldDelimited); break;
				case nbNETPDL_ID_CFIELD_LINE:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementCfieldLine); break;
				case nbNETPDL_ID_CFIELD_HDRLINE:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementCfieldHdrline); break;
				case nbNETPDL_ID_CFIELD_DYNAMIC:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementCfieldDynamic); break;
				case nbNETPDL_ID_CFIELD_ASN1:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementCfieldASN1); break;
				case nbNETPDL_ID_CFIELD_XML:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementCfieldXML); break;
				case nbNETPDL_ID_FIELDMATCH:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldmatch); break;
				default:
					{
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
							"ADT contains invalid field element.");

						return nbFAILURE;
					}
				}
			}; break;
		case nbNETPDL_IDEL_SUBFIELD:
			{
				NetPDLElementField= (struct _nbNetPDLElementFieldBase *) NetPDLTempElementFromADT;

				SizeToDuplicate= sizeof(struct _nbNetPDLElementSubfield);
			}; break;
		case nbNETPDL_IDEL_FIELDMATCH:
			{
				NetPDLElementField= (struct _nbNetPDLElementFieldBase *) NetPDLTempElementFromADT;

				SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldmatch);
			}; break;
		case nbNETPDL_IDEL_MAP:
			{
			struct _nbNetPDLElementFieldBase *NetPDLFieldBase= (struct _nbNetPDLElementFieldBase *) NetPDLTempElementFromADT;

				switch(NetPDLFieldBase->FieldType)
				{
				case nbNETPDL_ID_FIELD_MAP_XML_PI:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementMapXMLPI); break;
				case nbNETPDL_ID_FIELD_MAP_XML_DOCTYPE:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementMapXMLDoctype); break;
				case nbNETPDL_ID_FIELD_MAP_XML_ELEMENT:
					SizeToDuplicate= sizeof(struct _nbNetPDLElementMapXMLElement); break;
				default:
					{
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
							"ADT contains invalid map element.");

						return nbFAILURE;
					}
				}
			}; break;
		case nbNETPDL_IDEL_BLOCK:
			SizeToDuplicate= sizeof(struct _nbNetPDLElementBlock); break;
		case nbNETPDL_IDEL_INCLUDEBLK:
			SizeToDuplicate= sizeof(struct _nbNetPDLElementIncludeBlk); break;
		case nbNETPDL_IDEL_IF:
			SizeToDuplicate= sizeof(struct _nbNetPDLElementIf); break;
		case nbNETPDL_IDEL_IFTRUE:
		case nbNETPDL_IDEL_IFFALSE:
		case nbNETPDL_IDEL_MISSINGPACKETDATA:
			SizeToDuplicate= sizeof(struct _nbNetPDLElementBase); break;
		case nbNETPDL_IDEL_SWITCH:
			SizeToDuplicate= sizeof(struct _nbNetPDLElementSwitch); break;
		case nbNETPDL_IDEL_CASE:
		case nbNETPDL_IDEL_DEFAULT:
			SizeToDuplicate= sizeof(struct _nbNetPDLElementCase); break;
		case nbNETPDL_IDEL_LOOP:
			SizeToDuplicate= sizeof(struct _nbNetPDLElementLoop); break;
		case nbNETPDL_IDEL_LOOPCTRL:
			SizeToDuplicate= sizeof(struct _nbNetPDLElementLoopCtrl); break;
		case nbNETPDL_IDEL_SET:
			SizeToDuplicate= sizeof(struct _nbNetPDLElementSet); break;
		case nbNETPDL_IDEL_EXITWHEN:
			SizeToDuplicate= sizeof(struct _nbNetPDLElementExitWhen); break;
		case nbNETPDL_IDEL_CHOICE:
			SizeToDuplicate= sizeof(struct _nbNetPDLElementChoice); break;
		case nbNETPDL_IDEL_DEFAULTITEM:
			SizeToDuplicate= sizeof(struct _nbNetPDLElementFieldmatch); break;
		default:
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
					"ADT contains invalid element.");

				return nbFAILURE;
			}
		}

		// Let's create a copy of current element from ADT
		NetPDLTempElementDup= (struct _nbNetPDLElementBase *) malloc(SizeToDuplicate);

		if (NetPDLTempElementDup == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
				"Not enough memory for creating internal structures.");

			return nbFAILURE;
		}

		memcpy(NetPDLTempElementDup, NetPDLTempElementFromADT, SizeToDuplicate);

		// Let's perform replacing, if it is needed
		if (NetPDLElementField)
		{
		struct _nbNetPDLElementReplace *NetPDLTempElementReplace;

			NetPDLTempElementReplace= FirstReplaceElement;

			while (NetPDLTempElementReplace)
			{
				if (strncmp(NetPDLElementField->Name, NetPDLTempElementReplace->FieldToRename, strlen(NetPDLTempElementReplace->FieldToRename)) == 0)
				{
					if (NetPDLTempElementReplace->Name)
						((struct _nbNetPDLElementFieldBase *) NetPDLTempElementDup)->Name= NetPDLTempElementReplace->Name;
					if (NetPDLTempElementReplace->LongName)
						((struct _nbNetPDLElementFieldBase *) NetPDLTempElementDup)->LongName= NetPDLTempElementReplace->LongName;
					if (NetPDLTempElementReplace->ShowTemplateName)
						((struct _nbNetPDLElementFieldBase *) NetPDLTempElementDup)->ShowTemplateName= NetPDLTempElementReplace->ShowTemplateName;
					if (NetPDLTempElementReplace->ShowTemplateInfo)
						((struct _nbNetPDLElementFieldBase *) NetPDLTempElementDup)->ShowTemplateInfo= NetPDLTempElementReplace->ShowTemplateInfo;
					break;
				}

				NetPDLTempElementReplace= (struct _nbNetPDLElementReplace *) NETPDL_GET_ELEMENT(NetPDLTempElementReplace->NextSibling);
			}
		}

		// Let's update NetPDL Database with new duplicated element from ADT
		NetPDLTempElementDup->ElementNumber= NetPDLDatabase->GlobalElementsListNItemsPlusADTCopies;
		NetPDLDatabase->GlobalElementsList[NetPDLDatabase->GlobalElementsListNItemsPlusADTCopies]= NetPDLTempElementDup;

		// Let's update pointers of current duplicated element from ADT
		if (NetPDLFirstChildPtr == nbNETPDL_INVALID_ELEMENT)
		{
			NetPDLFirstChildPtr= NetPDLDatabase->GlobalElementsListNItemsPlusADTCopies;
		}
		else
		{
			NetPDLPreviousElementDup->NextSibling= NetPDLDatabase->GlobalElementsListNItemsPlusADTCopies;
			NetPDLTempElementDup->PreviousSibling= NetPDLPreviousElementDup->ElementNumber;
		}

		NetPDLTempElementDup->Parent= NetPDLParentElement->ElementNumber;
		NetPDLTempElementDup->FirstChild= nbNETPDL_INVALID_ELEMENT;

		// Let's check if next probable duplicate element from ADT can be allocated
		NetPDLDatabase->GlobalElementsListNItemsPlusADTCopies++;

		if (NetPDLDatabase->GlobalElementsListNItemsPlusADTCopies == NETPDL_MAX_NELEMENTS)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
				"Internal error: reached the maximum number of NetPDL elements.");

			return nbFAILURE;
		}

		// Recursive call if current element has children
		if (NetPDLTempElementFromADT->FirstChild)
		{
		int RetVal;

			RetVal= DuplicateADT(ADTToDuplicate, 
						NetPDLTempElementDup, 
						NETPDL_GET_ELEMENT(NetPDLTempElementFromADT->FirstChild), 
						FirstReplaceElement);

			if (RetVal != nbSUCCESS)
				return RetVal;
		}

		// Select next element to continue processing
		NetPDLPreviousElementDup= NetPDLTempElementDup;
		NetPDLTempElementFromADT= NETPDL_GET_ELEMENT(NetPDLTempElementFromADT->NextSibling);
	}

	// Let's assign first duplicated element to its parent
	NetPDLParentElement->FirstChild= NetPDLFirstChildPtr;

	return nbSUCCESS;
}



//---------------------------------------------------------------------------
// Debugging function
//---------------------------------------------------------------------------
#ifdef _DEBUG_LOUD
void CNetPDLSAXParser::PrintElements(int CurrentLevel, uint32 CurrentElementIdx)
{
	while (CurrentElementIdx)
	{
		for (int i= 0; i < CurrentLevel; i++)
			printf("   ");

		printf("<%s", NetPDLDatabase->GlobalElementsList[CurrentElementIdx]->ElementName);

		// In case this is a protocol, print the protocol name
		switch (NetPDLDatabase->GlobalElementsList[CurrentElementIdx]->Type)
		{
			case nbNETPDL_IDEL_PROTO:
			{
				printf(" name='%s'>\n", ((struct _nbNetPDLElementProto *) NetPDLDatabase->GlobalElementsList[CurrentElementIdx])->Name);
				break;
			}

			case nbNETPDL_IDEL_FIELD:
			{
			struct _nbNetPDLElementFieldBase *NetPDLFieldElement;
				
				NetPDLFieldElement= (struct _nbNetPDLElementFieldBase *) NetPDLDatabase->GlobalElementsList[CurrentElementIdx];
				printf(" type='%d' name='%s'>\n", NetPDLFieldElement->FieldType, NetPDLFieldElement->Name);
				break;
			}

			default:
			{
				printf(">\n");
				break;
			}
		}

		if (NetPDLDatabase->GlobalElementsList[CurrentElementIdx]->FirstChild)
			PrintElements(CurrentLevel+1, NetPDLDatabase->GlobalElementsList[CurrentElementIdx]->FirstChild);

		CurrentElementIdx= NetPDLDatabase->GlobalElementsList[CurrentElementIdx]->NextSibling;
	}
}
#endif
