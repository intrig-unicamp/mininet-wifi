/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include <nbee_netpdlutils.h>
#include "netpdlexpression.h"
#include "pdmlmaker.h"




/*!
	\file netpdldecoderutils.h

	This file include some methods that can be of general use for a NetPDL engine, but that could 
	be called only sometimes from some programs.
*/


/*!
	\brief This class implements some methods that can be of general use for NetPDL-based tools.
	Exported methods have been declared into the 'virtual container' nbNetPDLUtils class.
*/
class CNetPDLDecoderUtils : public nbNetPDLUtils
{
public:
	CNetPDLDecoderUtils();
	virtual ~CNetPDLDecoderUtils();

	int FormatNetPDLField(const char *ProtoName, const char *FieldName, const unsigned char *FieldDumpPtr,
		unsigned int FieldSize, char *FormattedField, int FormattedFieldSize);

	int FormatNetPDLField(const char *TemplateName, const unsigned char *FieldDumpPtr,
		unsigned int FieldSize, int FieldIsInNetworkByteOrder, char *FormattedField, int FormattedFieldSize);

	int FormatNetPDLField(int FastPrintingFunctionCode, const unsigned char *FieldDumpPtr,
		unsigned int FieldSize, char *FormattedField, int FormattedFieldSize);

	int GetHexValueNetPDLField(const char *ProtoName, const char *FieldName, const char *FormattedField, 
		unsigned char *FieldHexValue, unsigned int *FieldHexSize, bool BinaryEncoding);

	int GetFastPrintingFunctionCode(const char *ProtoName, const char *FieldName, int *FastPrintingFunctionCode);

	int GetHexValueNetPDLField(const char *TemplateName, const char *FormattedField, 
		unsigned char *FieldHexValue, unsigned int *FieldHexSize, bool BinaryEncoding);

private:
	struct _nbNetPDLElementFieldBase *LookForNetPDLFieldDefinition(struct _nbNetPDLElementBase *FirstElement, const char *FieldName, int CurrentProtoItem);

	int FormatPDMLElement(char *FormattedField, int FormFieldSize, struct _nbNetPDLElementFieldBase *NetPDLElement,
		int Len, const unsigned char *FieldBinHexDump);

	int FormatBinDumpElement(const char *FormattedField, struct _nbNetPDLElementFieldBase *NetPDLElement,
		char *BufferHexDump, unsigned int *BufferHexDumpSize, bool BinaryEncoding);

	//! Pointer to a vector which keeps trace of the NetPDL run-time variables.
	CNetPDLVariables *m_netPDLVariables;

	//! Pointer to the expression handler.
	CNetPDLExpression *m_exprHandler;

	//! Pointer to the class that generates the detailed view.
	CPDMLMaker *m_PDMLMaker;
};

