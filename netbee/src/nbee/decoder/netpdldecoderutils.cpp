/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <xercesc/util/PlatformUtils.hpp>

#include <nbprotodb.h>
#include <nbprotodb_defs.h>
#include "netpdldecoderutils.h"
#include "showplugin/native_showfunctions.h"		// For native printing functions
#include "../misc/initialize.h"
#include "../globals/globals.h"
#include "../globals/debug.h"


// Global variable
extern struct _nbNetPDLDatabase *NetPDLDatabase;




//! Standard constructor
CNetPDLDecoderUtils::CNetPDLDecoderUtils()
{
	memset(m_errbuf, 0, sizeof(m_errbuf));

	// Instantiate a run-time variable Handler
	m_netPDLVariables = new CNetPDLVariables(m_errbuf, sizeof(m_errbuf));

	// Instantiates an Expression Handler
	m_exprHandler= new CNetPDLExpression(m_netPDLVariables, m_errbuf, sizeof(m_errbuf));

	// Initializes the class that generates the detailed view
	m_PDMLMaker= new CPDMLMaker (m_exprHandler, NULL /* PDMLReader is not needed here */, nbDECODER_GENERATEPDML_COMPLETE, m_errbuf, sizeof(m_errbuf));

	if (m_PDMLMaker)
		// Warning: it does not check for errors, here (which is not a good thing)
		m_PDMLMaker->Initialize(m_netPDLVariables);

	// Warning: it does not check for errors, here (which is not a good thing)
	InitializeNetPDLInternalStructures(m_exprHandler, m_netPDLVariables, m_errbuf, sizeof(m_errbuf));
}



//! Standard distructor
CNetPDLDecoderUtils::~CNetPDLDecoderUtils()
{
	delete m_netPDLVariables;
	delete m_exprHandler;
	delete m_PDMLMaker;
}


// Documented in the base class
int CNetPDLDecoderUtils::FormatNetPDLField(const char *ProtoName, const char *FieldName, const unsigned char *FieldDumpPtr, 
										   unsigned int FieldSize, char *FormattedField, int FormattedFieldSize)
{
	m_netPDLVariables->SetVariableRefBuffer(m_netPDLVariables->m_defaultVarList.PacketBuffer, (unsigned char *) FieldDumpPtr, 0, FieldSize);

	for (unsigned int j= 0; j < NetPDLDatabase->ProtoListNItems; j++)
	{
		if (strcmp(NetPDLDatabase->ProtoList[j]->Name, ProtoName) == 0)
		{
		struct _nbNetPDLElementFieldBase *NetPDLElement;

			NetPDLElement= LookForNetPDLFieldDefinition(NetPDLDatabase->ProtoList[j]->FirstField, FieldName, j);

			if (NetPDLElement)
				return FormatPDMLElement(FormattedField, FormattedFieldSize, NetPDLElement, FieldSize, FieldDumpPtr);
			else if (strcmp(FieldName,"allfields") == 0)
				return nbSUCCESS;
			else
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The field named '%s' cannot be found in the current NetPDL description.", FieldName);
				return nbFAILURE;
			}
		}
	}

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The protocol named '%s' cannot be found in the current NetPDL description.", ProtoName);
	return nbFAILURE;
}


// Documented in the base class
int CNetPDLDecoderUtils::FormatNetPDLField(const char *TemplateName, const unsigned char *FieldDumpPtr, 
										   unsigned int FieldSize, int FieldIsInNetworkByteOrder, char *FormattedField, int FormattedFieldSize)
{
	m_netPDLVariables->SetVariableRefBuffer(m_netPDLVariables->m_defaultVarList.PacketBuffer, (unsigned char *) FieldDumpPtr, 0, FieldSize);

	for (unsigned int j= 0; j < NetPDLDatabase->ShowTemplateNItems; j++)
	{
		if (strcmp(NetPDLDatabase->ShowTemplateList[j]->Name, TemplateName) == 0)
		{
		struct _nbNetPDLElementFieldBase NetPDLElement;

			memset(&NetPDLElement, 0, sizeof(struct _nbNetPDLElementFieldBase));

			// Create a fake NetPDL element and fill in the needed information
			NetPDLElement.ShowTemplateInfo= NetPDLDatabase->ShowTemplateList[j];
			NetPDLElement.IsInNetworkByteOrder= FieldIsInNetworkByteOrder;

			return FormatPDMLElement(FormattedField, FormattedFieldSize, &NetPDLElement, FieldSize, FieldDumpPtr);
		}
	}

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), 
					"The visualization template named '%s' cannot be found in the current NetPDL description.", TemplateName);
	return nbFAILURE;
}


// Documented in the base class
int CNetPDLDecoderUtils::GetFastPrintingFunctionCode(const char *ProtoName, const char *FieldName, int *FastPrintingFunctionCode)
{
	for (unsigned int j= 0; j < NetPDLDatabase->ProtoListNItems; j++)
	{
		if (strcmp(NetPDLDatabase->ProtoList[j]->Name, ProtoName) == 0)
		{
		struct _nbNetPDLElementFieldBase *NetPDLElement;

			NetPDLElement= LookForNetPDLFieldDefinition(NetPDLDatabase->ProtoList[j]->FirstField, FieldName, j);

			if (NetPDLElement)
			{
			struct _nbNetPDLElementShowTemplate *ShowTemplate= NetPDLElement->ShowTemplateInfo;

				if (ShowTemplate->ShowNativeFunction == 0)
				{
					// The requested field does not support fast printing
					return nbWARNING;
				}
				else
				{
					*FastPrintingFunctionCode= (int) ShowTemplate->ShowNativeFunction;
					return nbSUCCESS;
				}
			}
			else if(strcmp(FieldName,"allfields")==0)
				return nbSUCCESS;
			else
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The field named '%s' cannot be found in the current NetPDL description.", FieldName);
				return nbFAILURE;
			}
		}
	}

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The protocol named '%s' cannot be found in the current NetPDL description.", ProtoName);
	return nbFAILURE;
}



// Documented in the base class
int CNetPDLDecoderUtils::FormatNetPDLField(int FastPrintingFunctionCode, const unsigned char *FieldDumpPtr, unsigned int FieldSize, char *FormattedField, int FormattedFieldSize)
{
unsigned long FormattedFieldLength;		//Required, but never actually used here

	if (FormattedFieldSize == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Internal error: the buffer required to print field value is too small.");
		return nbFAILURE;
	}

	switch (FastPrintingFunctionCode)
	{
		case nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_IPV4:
			return NativePrintIPv4Address(FieldDumpPtr, FormattedField, FormattedFieldSize, &FormattedFieldLength, m_errbuf, sizeof(m_errbuf));

		case nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_ASCII:
			return NativePrintAsciiField(FieldDumpPtr, FieldSize, FormattedField, FormattedFieldSize, &FormattedFieldLength, m_errbuf, sizeof(m_errbuf));

		case nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_ASCIILINE:
			return NativePrintAsciiLineField(FieldDumpPtr, FieldSize, FormattedField, FormattedFieldSize, &FormattedFieldLength, m_errbuf, sizeof(m_errbuf));

		case nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_HTTPCONTENT:
			return NativePrintHTTPContentField(FieldDumpPtr, FieldSize, FormattedField, FormattedFieldSize, &FormattedFieldLength, m_errbuf, sizeof(m_errbuf));

		// No native printing function, so let's print the error
		default:
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "A fast printing function is called for a field, but the function code has not been recognized.");
			return nbFAILURE;
		}
	}

}


/*!
	\brief Given a field name, it locates the definition within the NetPDL files.

	This function is able to scan a given section of the NetPDL definition by means of a recursive method.
	This function checks the node type and:
	- if it is a standard field (e.g. a 'fixed'): it checks if the field name corresponds to the field we want;
	if so, it returns the field, otherwise it moves to the next field
	- if if is a 'null' field (e.g. 'looptype'), it moves to the next field
	- it is is a 'container' field (e.g. a 'case'), it moves to the first field contained within the
	block, and it calls itself (hence the recursive method).

	This code is included within a 'while' loop, which ends when the protocol do not have any other fields
	to be examined.

	\param FirstElement: pointer to the first element of the NetPDL definition (e.g. the first field
	in case we're starting from the beginning of the protocol). In case this method has been recursively called,
	this parameter can be the first field of the block that we want to examine (e.g. the first field
	contained within a 'case' block).

	\param FieldName: ASCII string that contains the name of the field that we want to locate.
	
	\param CurrentProtoItem: an integer that keeps the position of the current protocol within the protocol list
	(the NetPDLDatabase global variable).

	\return A pointer to the definition of the NetPDL element if it has been found, NULL otherwise.
*/
struct _nbNetPDLElementFieldBase *CNetPDLDecoderUtils::LookForNetPDLFieldDefinition(struct _nbNetPDLElementBase *FirstElement, const char *FieldName, int CurrentProtoItem)
{
struct _nbNetPDLElementBase *CurrentField;

	CurrentField= FirstElement;

	while (CurrentField)
	{
		switch (CurrentField->Type)
		{
			case nbNETPDL_IDEL_FIELD:
			{
			struct _nbNetPDLElementFieldBase *FieldElement;

				FieldElement= (struct _nbNetPDLElementFieldBase *) CurrentField;

				if (strcmp(FieldElement->Name, FieldName) == 0)
					return (struct _nbNetPDLElementFieldBase *) CurrentField;

				// Let's give a look inside in case we've some bitfield inside
				if (CurrentField->FirstChild != nbNETPDL_INVALID_ELEMENT)
				{
					FieldElement= LookForNetPDLFieldDefinition(nbNETPDL_GET_ELEMENT(NetPDLDatabase, CurrentField->FirstChild), FieldName, CurrentProtoItem);

					// If we locate the correct element, please return it.
					if (FieldElement)
						return FieldElement;
				}

				break;
			};

			// This code indicates a nested block of fields
			case nbNETPDL_IDEL_SWITCH:
			case nbNETPDL_IDEL_CASE:
			case nbNETPDL_IDEL_DEFAULT:
			case nbNETPDL_IDEL_LOOP:
			case nbNETPDL_IDEL_BLOCK:
			{
			struct _nbNetPDLElementFieldBase *FieldElement;

				// Let's give a look inside
				FieldElement= LookForNetPDLFieldDefinition(nbNETPDL_GET_ELEMENT(NetPDLDatabase, CurrentField->FirstChild), FieldName, CurrentProtoItem);

				// If we locate the correct element, please return it.
				if (FieldElement)
					return FieldElement;

				// Otherwise, we have to continue
				break;
			};

			case nbNETPDL_IDEL_IF:
			{
			struct _nbNetPDLElementFieldBase *FieldElement;
			struct _nbNetPDLElementIf *IfElement;

				IfElement= (struct _nbNetPDLElementIf *) CurrentField;

				// Let's give a look inside the 'true' branch
				FieldElement= LookForNetPDLFieldDefinition(IfElement->FirstValidChildIfTrue, FieldName, CurrentProtoItem);

				// If we locate the correct element, please return it.
				if (FieldElement)
					return FieldElement;

				// Let's give a look inside the 'false' branch
				FieldElement= LookForNetPDLFieldDefinition(IfElement->FirstValidChildIfFalse, FieldName, CurrentProtoItem);

				// If we locate the correct element, please return it.
				if (FieldElement)
					return FieldElement;

				// Otherwise, we have to continue
				break;
			};


			// This is not a valid field; move to the next one
			case nbNETPDL_IDEL_LOOPCTRL:
			{
				break;
			};

			// This points to a separate block field
			case nbNETPDL_IDEL_INCLUDEBLK:
			{
			struct _nbNetPDLElementIncludeBlk *IncludeBlkElement;
			struct _nbNetPDLElementFieldBase *FieldElement;

				IncludeBlkElement= (struct _nbNetPDLElementIncludeBlk *) CurrentField;

				// Let's give a look inside
				FieldElement= LookForNetPDLFieldDefinition(nbNETPDL_GET_ELEMENT(NetPDLDatabase,
									(IncludeBlkElement->IncludedBlock)->FirstChild), FieldName, CurrentProtoItem);

				// If we locate the correct element, please return it.
				if (FieldElement)
					return FieldElement;

				// Otherwise, we have to continue
				break;
			};
		}			// end switch

		
		// Move to the next child
		CurrentField= nbNETPDL_GET_ELEMENT(NetPDLDatabase, CurrentField->NextSibling);
	}

	return NULL;
}



/*!
	\brief It creates a string that contains a field formatted with the proper NetPDL instructions.

	This method looks very similar to the CPDMLMaker::PDMLElementUpdate(). Given the hex value of an header
	field and the NetPDL definition of the field itself, this method formats the value according to the NetPDL
	directives (e.g. an IP address becomes '10.11.12.13').

	Unfortunately, this method is more limited compared to the CPDMLMaker::PDMLElementUpdate() because some
	elements may not be available in this printing process (e.g. run-time variables, because we do not decode
	the whole protocol headers here). Other attributes (like 'showmap' and 'showdtl') are not taken into 
	consideration because they do not aim to define how the field must be printed; insteady, they inserts some 
	additional info into the PDML fragment that may be used by the visualizer. This method generates only the 
	content of the 'show' attribute of the PDML.

	\param FormattedField: user-allocated buffer that will keep the result of the transformation.

	\param FormFieldSize: size of the previous user-allocated buffer.

	\param NetPDLElement: the original element in the NetPDL file that contains this description.
	In order words, the NetPDLElement keeps the specification of a given field, while 'FormattedField' 
	element keeps the result of the decoding process of the selected field.

	\param Len: keeps the length (in bytes) of the decoded field.

	\param FieldBinHexDump: a pointer to the buffer that contains the packet data (in the pcap format)
	related to the current field (i.e. the field value in binary hex dump format).

	\return nbSUCCESS if everything is fine, nbFAILURE in case of error.
	In case of error, the error message can be retrieved by the GetLastError() method.

	\warning Some of the parameters of the CPDMLMaker::PDMLElementUpdate() (such as 'Offset' and 
	'ProtoStartOffset') are not present in this method. This is because this method does not operate
	within a complete decoding process and these information may be unknown.
*/
int CNetPDLDecoderUtils::FormatPDMLElement(char *FormattedField, int FormFieldSize, struct _nbNetPDLElementFieldBase *NetPDLElement,
										   int Len, const unsigned char *FieldBinHexDump)
{
struct _nbPDMLField *PDMLElement;
//int NumOfCopiedBytes;

	m_PDMLMaker->m_capLen= Len;
	PDMLElement= m_PDMLMaker->m_fieldsList[0];

	// Initialize current element
	memset(PDMLElement, 0, sizeof (_nbPDMLField));

	m_PDMLMaker->PacketInitialize();
	if (m_PDMLMaker->PDMLElementUpdate(PDMLElement, NetPDLElement, Len, 0, FieldBinHexDump) == nbFAILURE)
		return nbFAILURE;

	sstrncpy(FormattedField, PDMLElement->ShowValue, FormFieldSize);

	return nbSUCCESS;
}



// Documented in the base class
int CNetPDLDecoderUtils::GetHexValueNetPDLField(const char *ProtoName, const char *FieldName, const char *FormattedField, unsigned char *FieldHexValue, unsigned int *FieldHexSize, bool BinaryEncoding)
{
	for (unsigned int j= 0; j < NetPDLDatabase->ProtoListNItems; j++)
	{
		if (strcmp(NetPDLDatabase->ProtoList[j]->Name, ProtoName) == 0)
		{
		struct _nbNetPDLElementFieldBase *NetPDLElement;

			NetPDLElement= LookForNetPDLFieldDefinition(NetPDLDatabase->ProtoList[j]->FirstField, FieldName, j);

			if (NetPDLElement)
			{
				return (FormatBinDumpElement(FormattedField, NetPDLElement, (char *) FieldHexValue, FieldHexSize, BinaryEncoding));
			}
			else if(strcmp(FieldName,"allfields")==0)
				return nbSUCCESS;

			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The field named '%s' cannot be found in the current NetPDL description.", FieldName);
			return nbFAILURE;
		}
	}

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The protocol named '%s' cannot be found in the current NetPDL description.", ProtoName);
	return nbFAILURE;
}


// Documented in the base class
int CNetPDLDecoderUtils::GetHexValueNetPDLField(const char *TemplateName, const char *FormattedField, unsigned char *FieldHexValue, unsigned int *FieldHexSize, bool BinaryEncoding)
{
	for (unsigned int j= 0; j < NetPDLDatabase->ShowTemplateNItems; j++)
	{
		if (strcmp(NetPDLDatabase->ShowTemplateList[j]->Name, TemplateName) == 0)
		{
		struct _nbNetPDLElementFieldBase NetPDLElement;

			memset(&NetPDLElement, 0, sizeof(struct _nbNetPDLElementFieldBase));
			NetPDLElement.ShowTemplateInfo= NetPDLDatabase->ShowTemplateList[j];

			return (FormatBinDumpElement(FormattedField, &NetPDLElement, (char *) FieldHexValue, FieldHexSize, BinaryEncoding));
		}
	}

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), 
					"The visualization template named '%s' cannot be found in the current NetPDL description.", TemplateName);
	return nbFAILURE;
}


/*!
	\brief It creates the hex dump of the given field, given its value in 'human' form.

	This method is the dual of GetFormattedField(). Given the value of an header field in human form
	(e.g. '10.11.12.13' for an IP address), this method returns the hex dump of the field (in pcap format) 
	according to the NetPDL directives (e.g. the above IP address becomes the 4-bytes string '0A0B0C0D').

	\param FormattedField: user-allocated buffer that keeps the original string to be converted.

	\param NetPDLElement: the original element in the NetPDL file that contains this description.
	In order words, the NetPDLElement keeps the specification of a given field, while 'FieldBinHexDump' 
	element keeps the result of the decoding process of the selected field.

	\param BufferHexDump: a pointer to the buffer that will contain the packet data (in the pcap format)
	related to the current field (i.e. the field value in binary hex dump format).

	\param BufferHexDumpSize: size of the previous user-allocated buffer. When the function returns, it will
	keep the number of bytes that have currently been used in that buffer.

	\param BinaryEncoding: 'true' if we want to return a binary buffer, 'false' if we want to return an ascii
	buffer. The size (i.e. the value of variable 'BufferHexDumpSize') of the same data returned as ascii buffer 
	has a size which is two times the size of the same data returned as binary buffer.

	\return nbSUCCESS if the field has been converted without errors, nbFAILURE otherwise.
	In case of error, the error message can be retrieved by the GetLastError() method.

	\warning This method is able to operate on fields that have 'standard' visualization primitives. In other words, 
	it is not able to convert fields that use 'showplg' or such directives in the NetPDL protocol definition file.
*/
int CNetPDLDecoderUtils::FormatBinDumpElement(const char *FormattedField, struct _nbNetPDLElementFieldBase *NetPDLElement, 
											 char *BufferHexDump, unsigned int *BufferHexDumpSize, bool BinaryEncoding)
{
int Base;
int NSeparators;
int PrefixLength = 0;
int NumberOfDigit;
const char *ResidueString;
char NumberToBeConverted[NETPDL_MAX_STRING + 1];
struct _nbNetPDLElementFieldBase *FieldElement;


	FieldElement= (struct _nbNetPDLElementFieldBase *) NetPDLElement;

	BufferHexDump[0]= 0;

	// Check the base (2, 10, 16) of the number
	switch(FieldElement->ShowTemplateInfo->ShowMode)
	{
		case nbNETPDL_ID_TEMPLATE_DIGIT_BIN:
		{
			// we have to skip the characters '0b'
			PrefixLength = 2;
			Base= 2;
		}; break;

		case nbNETPDL_ID_TEMPLATE_DIGIT_HEX:
		{
			// we have to skip the characters '0x'
			PrefixLength = 2;
			Base= 16;
		}; break;

		case nbNETPDL_ID_TEMPLATE_DIGIT_HEXNOX:
		{
			Base= 16;
		}; break;

		case nbNETPDL_ID_TEMPLATE_DIGIT_ASCII:
		{
			ResidueString= FormattedField;

			while (*ResidueString)
			{
			char CurrentConversion[10];
			char CurrentChar= ResidueString[0];

				// print the character in hexadecimal format
				sprintf(CurrentConversion, "%02X", CurrentChar);

				// append current block converted to the whole conversion
				strncat(BufferHexDump, CurrentConversion, *BufferHexDumpSize);

				// skip to next char
				ResidueString += sizeof(char);
			}
			
			// retrieve the length of the string
			*BufferHexDumpSize = (int) strlen(BufferHexDump);

			return nbSUCCESS;
		}; break;

		case nbNETPDL_ID_TEMPLATE_DIGIT_DEC:
		{
			Base= 10;
		}; break;

		default:
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Cannot convert the given field: format unknown");
			return nbFAILURE;
		}; break;

	}

	NSeparators= 0;

	if (FieldElement->ShowTemplateInfo->Separator)
	{
		ResidueString= FormattedField;

		// Look for the total number of separators present within our field
		while (ResidueString)
		{
			ResidueString= strstr(ResidueString, FieldElement->ShowTemplateInfo->Separator);

			if (ResidueString)
			{
				// Update the counter that keeps the number of separators currently present into the field
				NSeparators++;

				// Move to the next char (to avoid infinite loop, since strstr will return the same pointer)
				ResidueString+= sizeof(char);
			}
		}
	}
	

	// Check the dump size. We have to dump 'size' * 2 / 'showgrp' digit
	// i.e. if we want to dump 2048 in a short representation we have to dump 0800 and not 800 so we have to dump
	// 5 digit padding with some '0' on the left of the dump. In this case 'size' = 2 bytes,
	// 'showgrp' = 1 so the number of digit needed is 4.
	if (FieldElement->ShowTemplateInfo->DigitSize)
		NumberOfDigit= FieldElement->ShowTemplateInfo->DigitSize;
	else
	{

		// Check if we've a fixed-length field
		if (FieldElement->FieldType == nbNETPDL_ID_FIELD_FIXED)
		{
		struct _nbNetPDLElementFieldFixed *FixedElement;

			FixedElement= (struct _nbNetPDLElementFieldFixed *) FieldElement;
			NumberOfDigit= FixedElement->Size;
		}
		else
		{
			if (FieldElement->FieldType == nbNETPDL_ID_FIELD_BIT)
			{
			struct _nbNetPDLElementFieldBit *FixedElement;

				FixedElement= (struct _nbNetPDLElementFieldBit *) FieldElement;
				NumberOfDigit= FixedElement->Size;
			}
			else
			{
				// There is no way to detect the size of the field
				// The only possibility is to get the length of the field that has to be formatted
				NumberOfDigit= (int) strlen(FormattedField) / 2;
			}
		}
	}


	// Let's convert the number
	char PrintfConverter[NETPDL_MAX_STRING];
	char CurrSubstring[NETPDL_MAX_STRING + 1];
	char *BeginOfTheString;
	int CharsToCopy;
	
	ResidueString= FormattedField;
	do
	{
		// Get the first digit for the conversion skipping chars of prefix (like '0x' or '0b')
		BeginOfTheString= ((char *) ResidueString) + PrefixLength * sizeof(char);

		// Remove current block (until the field separator) of digit from the whole number
		if (FieldElement->ShowTemplateInfo->Separator)
			ResidueString= strstr(ResidueString, FieldElement->ShowTemplateInfo->Separator);
		else
			ResidueString= NULL;

		// Obtain the current block length
		if (ResidueString)
			CharsToCopy= (int) (ResidueString - BeginOfTheString);
		else
		{
			CharsToCopy= 0;

			while (BeginOfTheString[CharsToCopy])
				CharsToCopy++;
		}

		// Copy the portion of the number to be converted into the 'NumberToBeConverted' buffer
		strcpy(NumberToBeConverted, BeginOfTheString);
		NumberToBeConverted[CharsToCopy] = 0;
			
		if (Base <= 10)
		{
			long Number = 0;

			// Get an integer representation of that number
			for (int i= 0; i<CharsToCopy; i++)
				Number = Number * Base + (NumberToBeConverted[i] - '0');

			// obtain the "%0[numberofdigit]x" string
			ssnprintf(PrintfConverter, sizeof(PrintfConverter), "%%0%dX", NumberOfDigit * 2);

			// Print the number in hexadecimal format
			ssnprintf(CurrSubstring, sizeof(CurrSubstring), PrintfConverter, Number);
		}
		else
			// 'Base' should be 16 so print directly
			// Please note that '+1' is due to cope with the '\0' char at the end (that counts '1')
			sstrncpy(CurrSubstring, NumberToBeConverted, CharsToCopy + 1);

		// append current block converted to the whole conversion
		sstrncat(BufferHexDump, CurrSubstring, *BufferHexDumpSize);

		// skip the separator from the Residue String
		ResidueString += sizeof(char);
		NSeparators--;
	}
	while (NSeparators >= 0);

	
	// retrieve the length of the string
	*BufferHexDumpSize = (int) strlen(BufferHexDump);

	// let's convert the result in a binary dump, if needed
	if (BinaryEncoding)
	{
		HexDumpAsciiToHexDumpBin(BufferHexDump, (unsigned char *) BufferHexDump, *BufferHexDumpSize /* this value is not important */);
		*BufferHexDumpSize= *BufferHexDumpSize / 2;
	}

	return nbSUCCESS;
}
