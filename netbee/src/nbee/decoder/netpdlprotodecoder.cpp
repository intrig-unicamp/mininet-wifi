/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <pcre.h>
#include <nbprotodb.h>
#include <nbprotodb_defs.h>
#include <xercesc/util/XMLString.hpp>


#include "netpdlprotodecoder.h"
#include "decoderplugin/decoder_plugin.h"
#include "../utils/netpdlutils.h"

#include "../globals/globals.h"
#include "../globals/debug.h"


// Global variables
extern struct _nbNetPDLDatabase *NetPDLDatabase;
extern struct _PluginList PluginList[];



/*!
	\brief Default constructor; it creates a new instance of the protocol decoder.

	\param NetPDLVars: pointer to the class which contains the run-time variables, that must be shared between
	the classes (the NetPDL parser has to access it).

	\param ExprHandler: pointer to the class for expression management (shared among all the NetPDL 
	classes).

	\param PDMLMaker: a pointer to the CPDMLMaker class, which is in charge of creating the detailed
	view of the packet.

	\param PSMLMaker: a pointer to the CPSMLMaker class, which is in charge of creating the summary
	view of the packet.

	\param ErrBuf: pointer to a user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer belongs to the class that creates this one.
	Data stored in this buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.
*/
CNetPDLProtoDecoder::CNetPDLProtoDecoder(CNetPDLVariables *NetPDLVars, CNetPDLExpression *ExprHandler, 
										CPDMLMaker *PDMLMaker, CPSMLMaker *PSMLMaker, char *ErrBuf, int ErrBufSize)
{
	m_netPDLVariables= NetPDLVars;
	m_exprHandler= ExprHandler;
	m_PDMLMaker= PDMLMaker;
	m_PSMLMaker= PSMLMaker;

	// Store internally the pointer to the error buffer. This buffer belongs to the class that creates this one.
	m_errbuf= ErrBuf;
	m_errbufSize= ErrBufSize;
}



//! Default destructor
CNetPDLProtoDecoder::~CNetPDLProtoDecoder()
{
}



/*!
	\brief It updates the general variables in order to be able to decode a new protocol
	This funcion re-initializes almost all the data structures, avoiding to create a new 
	NetPDLProtoDecoder instance for every protocol.

	This code cold be included into the DecodeProto() method; however, the first protocol
	(i.e. 'startproto') does not have any field to decode. So, this method decouples the 
	initialization from the decoding.

	\param myCurrentProtoItem: the index of the current protocol within the protocol list
	(contained into the GlobalData class).

	\param PcapHeader: header of the sumbitted packet, according to the WinPcap definition.
*/
void CNetPDLProtoDecoder::Initialize(int myCurrentProtoItem, const struct pcap_pkthdr *PcapHeader)
{
	// initialize class global variables
	m_currentProtoItem= myCurrentProtoItem;
	m_pcapHeader= PcapHeader;

	// At the beginning, the pointer to the current PDML fragment is NULL
	m_PDMLProtoItem= NULL;
}



/*!
	\brief It decodes a packet; it returns a PDML fragment containing the parsed protocol.

	This is the main method of this class: it accepts a binary m_packet dump and it parses the data
	according to the NetPDL protocol description.
	The pointer to the NetPDL protocol that has to be used has already been done by means of a call to
	the Initialize() method.

	\param Packet: pointer to the binary dump of the m_packet (the pkt_data returned by the pcap_read() ).

	\param SnapLen: length of the current dump, i.e. the 'snaplen' of the 'struct pcap_header'.
	
	\param Offset: position of the first byte to be decoded according to the current protocol into the
	bynary dump.

	\return: nbSUCCESS if the protocol was decoded without any problem, nbFAILURE otherwise (for example 
	in case the &lt;presentif&gt; node told us it is the wrong proto). The error message is stored 
	in the m_errbuf buffer.<br>
	nbWARNING is returned in case we got some non-critical error (e.g. we were unable to parse correctly
	the packet because the packet buffer ended, e.g. in case of a application-layer message split across
	several packets). In this case we must continue the parsing, withough raising an error.
*/
int CNetPDLProtoDecoder::DecodeProto(const unsigned char *Packet, bpf_u_int32 SnapLen, bpf_u_int32 Offset)
{
struct _nbNetPDLElementExecuteX *BeforeElement, *AfterElement;
struct _nbNetPDLElementBase *FirstFieldElement;
unsigned int Len;	// Number of bytes (of the current protocol) that have been decoded
int RetVal;

	m_protoStartingOffset= Offset;
	m_packet= (unsigned char *) Packet;

	FirstFieldElement= NetPDLDatabase->ProtoList[m_currentProtoItem]->FirstField;

	// Let's execute 'before' code (if it exists)
	BeforeElement= NetPDLDatabase->ProtoList[m_currentProtoItem]->FirstExecuteBefore;

	while(BeforeElement)
	{
	int RetVal;
	unsigned int Result= 1;

		if (BeforeElement->WhenExprTree)
		{
			RetVal= m_exprHandler->EvaluateExprNumber(BeforeElement->WhenExprTree, NULL, &Result);

			if (RetVal != nbSUCCESS)
				return RetVal;
		}

		if (Result)
		{
			RetVal= ExecuteProtoCode(nbNETPDL_GET_ELEMENT(NetPDLDatabase, BeforeElement->FirstChild));

			if (RetVal != nbSUCCESS)
				return RetVal;
		}

		// We may even have to execute several sections (if more 'when' attributes matches)
		BeforeElement= BeforeElement->NextExecuteElement;
	}
	
	// Create a new PDML header. This will be the root element that will keep all the fields
	// belonging to the current protocol
	m_PDMLProtoItem= m_PDMLMaker->HeaderElementInitialize(Offset, m_currentProtoItem);
	if (m_PDMLProtoItem == NULL)
		return nbFAILURE;

	// Set to zero the number of bytes we have already decoded within the current protocol
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, 0);
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, Offset);

	int LoopCtrl= nbNETPDL_ID_LOOPCTRL_NONE;

	// Decode all the fields contained in this header
	RetVal= DecodeFields(FirstFieldElement, SnapLen - 1, NULL /* No parent */, &LoopCtrl, &Len);

	if (RetVal != nbSUCCESS)
	{
		if (RetVal == nbFAILURE)
			return nbFAILURE;

		// If we get a nbWARNING, this is probably due to some truncated fields. Please note that
		// application-layer protocols may have messages which spans over several packets, so
		// this case may be really common.
		// In this case, we have to proceed and save what we actually got till now.
		//
		// As an example, let's imagine having a protocol that begins with "if($packet[$currentoffset:4] == "PORT")
		// and the packet contains only two bytes.
		// The expression cannot be evaluated, because we do not have enough data in the packet buffer, 
		// hence this function will return a nbWARNING. In this case, we may have a protocol with 'zero'
		// length, and we must continue our processing without raising an error.
		// This problem can be avoided in case we have a <missing-packetdata> tag, but we do not use this tag
		// extensively in the NetPDL code, hence we must cope with this error here.
		//
		// However, we need to check that the protocol length is zero. This happens if the 'warning'
		// occurs immediately at the beginning of the protocol, so that no data in the protocol is parsed.
		// In this case, we need to return right now. Otherwise, we need to continue and save the fields
		// we parsed so far.

		if ((RetVal == nbWARNING) && (Len == 0))
			return nbWARNING;
	}

	// Let's check if we the protocol resulted with 'zero' length. This should never happen anyway
	if (Len <= 0)
	{
#ifdef _DEBUG
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "DEBUG WARNING: please check if this code has still to be maintained.");
#endif
		return nbFAILURE;
//		return nbWARNING;
	}

	// Tell the PDMLMaker that the current fragment must be saved
	if (m_PDMLMaker->HeaderElementUpdate(Len) == nbFAILURE)
		return nbFAILURE;

	// If PSML creation is required
	if (m_PSMLMaker)
	{
	unsigned int CurrentOffset;

		m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffset);

		// Update the summary view
		// Please note that this method requires the caplen (instead of FrameLength) as 3rd parameter
		RetVal= m_PSMLMaker->AddHeaderFragment(m_PDMLProtoItem, m_currentProtoItem, 
					NULL, CurrentOffset, m_pcapHeader->caplen);

		if (RetVal != nbSUCCESS)
			return RetVal;
	}

	// Let's execute 'after' code (if it exists)
	AfterElement= NetPDLDatabase->ProtoList[m_currentProtoItem]->FirstExecuteAfter;

	while(AfterElement)
	{
	int RetVal;
	unsigned int Result= 1;

		if (AfterElement->WhenExprTree)
		{
			RetVal= m_exprHandler->EvaluateExprNumber(AfterElement->WhenExprTree, m_PDMLProtoItem->FirstField, &Result);

			if (RetVal != nbSUCCESS)
				return RetVal;
		}

		if (Result)
		{
			RetVal= ExecuteProtoCode(nbNETPDL_GET_ELEMENT(NetPDLDatabase, AfterElement->FirstChild));

			if (RetVal != nbSUCCESS)
				return RetVal;
		}

		// We may even have to execute several sections (if more 'when' attributes matches)
		AfterElement= AfterElement->NextExecuteElement;
	}

	return nbSUCCESS;
}



int CNetPDLProtoDecoder::DecodeBlock(struct _nbNetPDLElementBlock *BlockElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent, int *LoopCtrl/*, int *Len*/)
{
//struct _nbNetPDLElementExecuteX *BeforeElement, *AfterElement;
struct _nbNetPDLElementBase *FirstField;
struct _nbPDMLField *PDMLElement;
unsigned int StartingOffset;
int RetVal;
unsigned int BlockLen;
int ReturnCode= nbSUCCESS;

/*
	// Let's execute 'before' code (if it exists)
	BeforeElement= NetPDLDatabase->ProtoList[m_currentProtoItem]->FirstExecuteBefore;

	while(BeforeElement)
	{
	int RetVal;
	int Result= 1;

		if (BeforeElement->WhenExprTree)
		{
			RetVal= m_exprHandler->EvaluateExprNumber(BeforeElement->WhenExprTree, NULL, &Result);

			if (RetVal != nbSUCCESS)
				return RetVal;
		}

		if (Result)
		{
			RetVal= ExecuteProtoCode(nbNETPDL_GET_ELEMENT(NetPDLDatabase, BeforeElement->FirstChild));

			if (RetVal != nbSUCCESS)
				return RetVal;
		}

		// We may even have to execute several sections (if more 'when' attributes matches)
		BeforeElement= BeforeElement->NextExecuteElement;
	}
*/

	m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &StartingOffset);

	FirstField= nbNETPDL_GET_ELEMENT(NetPDLDatabase, BlockElement->FirstChild);

	// Create a Field node and append it to PDML tree
	PDMLElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

	// Decode all the fields contained into the header
	RetVal= DecodeFields(FirstField, MaxOffsetToBeDecoded, PDMLElement, LoopCtrl, &BlockLen);

	if (RetVal != nbSUCCESS)
	{
		if (RetVal == nbFAILURE)
			return nbFAILURE;
		else
			// We have to go on and complete the processing, as much as we can
			ReturnCode= nbWARNING;
	}

	if (BlockLen > 0)
	{
	unsigned int NextFieldOffset;

		// Update the common fields of a PDML element (name, longname, ...)
		RetVal= m_PDMLMaker->PDMLBlockElementUpdate(PDMLElement, BlockElement, BlockLen, StartingOffset);

		if (RetVal != nbSUCCESS)
			return RetVal;

		if (m_PSMLMaker)
		{
			m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &NextFieldOffset);

			// In case of a block, we have to update the packet summary, if one.
			RetVal= m_PSMLMaker->AddHeaderFragment(m_PDMLProtoItem, m_currentProtoItem, 
									(struct _nbNetPDLElementFieldBase *) BlockElement, NextFieldOffset, m_pcapHeader->caplen);

			if (RetVal != nbSUCCESS)
			{
				if (RetVal == nbFAILURE)
					return nbFAILURE;
				else
					// We have to go on and complete the processing, as much as we can
					ReturnCode= nbWARNING;
			}
		}
	}
	else
		m_PDMLMaker->PDMLElementDiscard(PDMLElement);


/*
	// Let's execute 'after' code (if it exists)
	AfterElement= NetPDLDatabase->ProtoList[m_currentProtoItem]->FirstExecuteAfter;

	while(AfterElement)
	{
	int RetVal;
	int Result= 1;

		if (AfterElement->WhenExprTree)
		{
			RetVal= m_exprHandler->EvaluateExprNumber(AfterElement->WhenExprTree, m_PDMLProtoItem->FirstField, &Result);

			if (RetVal != nbSUCCESS)
				return RetVal;
		}

		if (Result)
		{
			RetVal= ExecuteProtoCode(nbNETPDL_GET_ELEMENT(NetPDLDatabase, AfterElement->FirstChild));

			if (RetVal != nbSUCCESS)
				return RetVal;
		}

		// We may even have to execute several sections (if more 'when' attributes matches)
		AfterElement= AfterElement->NextExecuteElement;
	}
*/
	return ReturnCode;
}


/*!
	\brief 	This method handles all the elements that can be found within a &lt;fields&gt; section.

	It loops through the fields, starting from 'FieldElement' to the end of the section. This function
	is able to process also the child elements found within the section.

	This function can be used also to process a 'subsection', e.g. the content of a &lt;case&gt; 
	element or such. If 'FieldElement' is the starting element of one such section, it decodes all
	the element contained in it (and all the sub-elements).

	\param FieldElement: pointer to the first field that has to be decoded.

	\param MaxOffsetToBeDecoded: max number of bytes (within the packet) that have to be decoded.
	This corresponds to the *offset* of the last byte that has to be decoded (i.e. in a 64 bytes packet, 
	the offset of the last byte is 63).
	This parameter is used in order to stop decoding when the packet ends (or when a sub-field
	is expected to end, e.g. because it reached the parent field length limit).
	For instance, if StartingOffset is 10 and MaxOffsetToBeDecoded is 20, only 10 bytes have
	still to be decoded.

	\param PDMLParent: the parent node (in the PDML document) on which the new decoded field will 
	have to be appended. This node is not always the root PDML header; for instance, in case we're
	decoding a block, the PDMLParent node is a '&lt;field &gt;' element, which will keep all the
	fields of the block as childs.

	\param LoopCtrl: it is needed to handle loops. A loop can contain an arbitrary number of fields,
	other nested loops and so on. It follows that there could be the need to break the loop and return
	back to the caller (for example because a codition is matched). The 'loopctrl' variable is used 
	for that: if this function returns due to a loop (or such), this variable keeps the type of break
	we encountered.

	\param Len: it will keep the length of the decoded structure (in bytes). Please note that the input
	value does not matter, since the value of this variable is zeroed at the beginning of this function.
	
	\return nbSUCCESS if everything is fine, nbFAILURE in case of error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CNetPDLProtoDecoder::DecodeFields(struct _nbNetPDLElementBase *FieldElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent, int *LoopCtrl, unsigned int *Len)
{
unsigned int StartingOffset;
unsigned int CurrentOffset;

	*Len= 0;

	m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffset);
	StartingOffset= CurrentOffset;

	// Decode all the fields contained into the header
	while( (FieldElement != NULL) && (CurrentOffset <= MaxOffsetToBeDecoded) )
	{
	int RetVal;

		RetVal= DecodeField(FieldElement, MaxOffsetToBeDecoded, PDMLParent, LoopCtrl);

		if (RetVal != nbSUCCESS)
		{
			// This code is needed expecially for nbWARNING, in which we have to return the piece that
			// has been decoded so far
			m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffset);
			*Len= CurrentOffset - StartingOffset;

			return RetVal;
		}

		// It we encounter a "loopctrl", we have to exit immediately from this decoding loop
		if (*LoopCtrl != nbNETPDL_ID_LOOPCTRL_NONE)
			break;

		FieldElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, FieldElement->NextSibling);
		m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffset);
	}

	*Len= CurrentOffset - StartingOffset;
	return nbSUCCESS;
}


/*!
	\brief 	This method handles all the elements that can be found within a &lt;fields&gt; section.

	It processes just one of these structures for each call.

	One of the problem of this funtion is to handle the &lt;loopctrl&gt; tag. This tag is used to
	force an exit from within a loop. The problem is that this stop can be inside an switch/case
	section. In case you found such a tag, you have to return immediately.

	\param FieldElement: pointer to the current field that has to be decoded.
	\param MaxOffsetToBeDecoded: please refer to the same parameter in function DecodeFields().
	\param PDMLParent: please refer to the same parameter in function DecodeFields().
	\param LoopCtrl: please refer to the same parameter in function DecodeFields().

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CNetPDLProtoDecoder::DecodeField(struct _nbNetPDLElementBase *FieldElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent, int *LoopCtrl)
{
int RetVal;

	if (LoopCtrl)			// loopctrl can be NULL in case the calling function does not need that info
		*LoopCtrl= nbNETPDL_ID_LOOPCTRL_NONE;	// Initialize the loopctrl, to be used in case of "break"

	switch (FieldElement->Type)
	{
		case nbNETPDL_IDEL_SWITCH:
		{
		struct _nbNetPDLElementCase *CaseNodeInfo;

			// This test must be at the beginning: if the selected element is not a 'true' field but
			// it is a field switch, we need to update the 'field' variable in order to point to the
			// proper field (selected according to the result of the field switch)
			RetVal= m_exprHandler->EvaluateSwitch((struct _nbNetPDLElementSwitch *) FieldElement, m_PDMLProtoItem->FirstField, &CaseNodeInfo);

			if (RetVal != nbSUCCESS)
				return RetVal;

			if ( (RetVal == nbSUCCESS) && (CaseNodeInfo) )
			{
			struct _nbNetPDLElementBase *FirstCaseField;
			unsigned int DecodedLen;

				// move to the first child of the <case> node (i.e. to the first field)
				FirstCaseField= nbNETPDL_GET_ELEMENT(NetPDLDatabase, CaseNodeInfo->FirstChild);

				RetVal= DecodeFields(FirstCaseField, MaxOffsetToBeDecoded, PDMLParent, LoopCtrl, &DecodedLen);

				if (RetVal != nbSUCCESS)
					return RetVal;
			}

			break;
		}

		case nbNETPDL_IDEL_LOOP:
		{
			RetVal= DecodeLoop((struct _nbNetPDLElementLoop *) FieldElement, MaxOffsetToBeDecoded, PDMLParent);
			
			if (RetVal != nbSUCCESS)
				return RetVal;

			break;
		}

		case nbNETPDL_IDEL_LOOPCTRL:
		{
			*LoopCtrl= ((struct _nbNetPDLElementLoopCtrl *) FieldElement)->LoopCtrlType;

			// In this case we have to exit immediately
			break;
		}

		case nbNETPDL_IDEL_INCLUDEBLK:
		{
		struct _nbNetPDLElementIncludeBlk *IncludeBlockElement;
		struct _nbNetPDLElementBlock *BlockElement;

			IncludeBlockElement= (struct _nbNetPDLElementIncludeBlk *) FieldElement;
			BlockElement= (struct _nbNetPDLElementBlock*) IncludeBlockElement->IncludedBlock;

			RetVal= DecodeBlock(BlockElement, MaxOffsetToBeDecoded, PDMLParent, LoopCtrl);

			if (RetVal != nbSUCCESS)
				return RetVal;

			break;
		}

		case nbNETPDL_IDEL_BLOCK:
		{
			RetVal= DecodeBlock((struct _nbNetPDLElementBlock *) FieldElement, MaxOffsetToBeDecoded, PDMLParent, LoopCtrl);

			if (RetVal != nbSUCCESS)
				return RetVal;

			break;
		}

		case nbNETPDL_IDEL_IF:
		// field is an 'if' field
		{
		struct _nbNetPDLElementBase *FirstIfField = NULL;
		struct _nbNetPDLElementIf *IfNode;
		unsigned int Result;
		unsigned int DecodedLen;

			IfNode= (struct _nbNetPDLElementIf *) FieldElement;

			RetVal= m_exprHandler->EvaluateExprNumber(IfNode->ExprTree, m_PDMLProtoItem->FirstField, &Result);

			switch (RetVal)
			{
				case nbFAILURE:
					return nbFAILURE;

				case nbWARNING:
				{
					FirstIfField= IfNode->FirstValidChildIfMissingPacketData;

					if (FirstIfField == NULL)
						return nbWARNING;

				}; break;

				case nbSUCCESS:
				{
					if (Result)
						FirstIfField= IfNode->FirstValidChildIfTrue;
					else
						FirstIfField= IfNode->FirstValidChildIfFalse;

					if (FirstIfField == NULL)
						return nbSUCCESS;

				}; break;

			}

			RetVal= DecodeFields(FirstIfField, MaxOffsetToBeDecoded, PDMLParent, LoopCtrl, &DecodedLen);

			if (RetVal != nbSUCCESS)
				return RetVal;

			break;
		}

		case nbNETPDL_IDEL_ASSIGNVARIABLE:
		{
			RetVal= m_exprHandler->EvaluateAssignVariable((struct _nbNetPDLElementAssignVariable*) FieldElement, m_PDMLProtoItem->FirstField);

			return RetVal;

			break;
		}

		case nbNETPDL_IDEL_ASSIGNLOOKUPTABLE:
		{
			RetVal= m_exprHandler->EvaluateAssignLookupTable((struct _nbNetPDLElementAssignLookupTable*) FieldElement, m_PDMLProtoItem->FirstField);

			return RetVal;

			break;
		}

		case nbNETPDL_IDEL_UPDATELOOKUPTABLE:
		{
			RetVal= m_exprHandler->EvaluateLookupTable((struct _nbNetPDLElementUpdateLookupTable*) FieldElement, m_PDMLProtoItem->FirstField);

			// The EvaluateLookupTable() can return nbWARNING in case the key element is not present
			// However, this is not an error condition.
			return RetVal;

			break;
		}

		case nbNETPDL_IDEL_FIELD:
		{
		unsigned int StartingOffsetOfThisField;
		struct _nbPDMLField *CurrentPDMLElement;

			RetVal= DecodeStandardField((struct _nbNetPDLElementFieldBase *) FieldElement, MaxOffsetToBeDecoded, PDMLParent, &CurrentPDMLElement, &StartingOffsetOfThisField);

			if (RetVal != nbSUCCESS)
				return RetVal;

			break;
		}
	
		case nbNETPDL_IDEL_SET:
		{
			return DecodeSet((struct _nbNetPDLElementSet *) FieldElement, MaxOffsetToBeDecoded, PDMLParent);
		}

		case nbNETPDL_IDEL_CHOICE:
		{
			return DecodeFieldChoice((struct _nbNetPDLElementChoice *) FieldElement, MaxOffsetToBeDecoded, PDMLParent);
		}

		default:
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "The NetPDL description contains a field which has not been recognized.");
			return nbFAILURE;
		}
	}

	return nbSUCCESS;
}


/*!
	\brief It handles the 'loop' tag.

	According with the type of the loop (size, times2repeat, while or do-while), it does different things:
		- [size] computes the size of the block and decode the fields until the size is reached;
		- [times2repeat] decodes the fields for 'times2repeat' times;
		- [while] decodes the fields while the expression in looptype is false (the expression must not be zero);
		- [do-while] decodes the fields until the expression in looptype is false (the expression must be zero)
		  This type of loop guarantees that the the block is decoded at least one time.

	It appends the PDML fragments generated from the fields in the block to the current PDML fragment

	\param LoopElement: pointer to the internal structure that contains the most important data
	related to each field.

	\param MaxOffsetToBeDecoded: please refer to the same parameter in function DecodeFields().

	\param PDMLParent: please refer to the same parameter in function DecodeFields().

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CNetPDLProtoDecoder::DecodeLoop(struct _nbNetPDLElementLoop *LoopElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent)
{
struct _nbNetPDLElementBase *FirstLoopField;
unsigned int Value;
int RetVal;
unsigned int CurrentOffset;

	// Get the starting offset of this field
	m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffset);

	// Get the first field
	FirstLoopField= LoopElement->FirstValidChildInLoop;

	switch (LoopElement->LoopType)
	{
		case nbNETPDL_ID_LOOP_T2R:
		{
		unsigned int NumRepeated;

			// retrieve the times the block has to be repeated
			RetVal= m_exprHandler->EvaluateExprNumber(LoopElement->ExprTree, m_PDMLProtoItem->FirstField, &Value);

			if (RetVal == nbFAILURE)
				return nbFAILURE;
			if (RetVal == nbWARNING)
				goto MissingPacketData;

			NumRepeated= 0;

			// We have to repeat the decoding N times
			// unless the m_packet has been completely decoded
			while ( (NumRepeated < Value) && (CurrentOffset <= MaxOffsetToBeDecoded) )
			{
			int TmpLoopCtrl;
			unsigned int DecodedLen;

				RetVal= DecodeFields(FirstLoopField, MaxOffsetToBeDecoded, PDMLParent, &TmpLoopCtrl, &DecodedLen);

				if (RetVal != nbSUCCESS)
					return RetVal;

				// If we encounter a "loopctrl", we have to execute it immediately
				if (TmpLoopCtrl != nbNETPDL_ID_LOOPCTRL_NONE)
				{
					if (TmpLoopCtrl == nbNETPDL_ID_LOOPCTRL_BREAK)
						break;

					if (TmpLoopCtrl == nbNETPDL_ID_LOOPCTRL_CONTINUE)
						continue;
				}

				// If for some reasons there are no more fields to be decoded, let's exit right now
				// It could be the case of some conditional fields that do not exist
				if (DecodedLen == 0)
					return nbSUCCESS;

				// Get the starting offset of this field
				m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffset);

				NumRepeated++;
			}
			break;
		}

		case nbNETPDL_ID_LOOP_WHILE:
		{
			// retrieve the value of the expression
			RetVal= m_exprHandler->EvaluateExprNumber(LoopElement->ExprTree, m_PDMLProtoItem->FirstField, &Value);

			if (RetVal == nbFAILURE)
				return nbFAILURE;
			if (RetVal == nbWARNING)
				goto MissingPacketData;

			// We have to repeat the decoding if the expression is true
			// unless the m_packet has been completely decoded
			while ( (Value) && (CurrentOffset <= MaxOffsetToBeDecoded) )
			{
			int TmpLoopCtrl;
			unsigned int DecodedLen;

				RetVal= DecodeFields(FirstLoopField, MaxOffsetToBeDecoded, PDMLParent, &TmpLoopCtrl, &DecodedLen);

				if (RetVal != nbSUCCESS)
					return RetVal;

				// If we encounter a "loopctrl", we have to execute it immediately
				if (TmpLoopCtrl != nbNETPDL_ID_LOOPCTRL_NONE)
				{
					if (TmpLoopCtrl == nbNETPDL_ID_LOOPCTRL_BREAK)
						break;

					if (TmpLoopCtrl == nbNETPDL_ID_LOOPCTRL_CONTINUE)
						continue;
				}

				// If for some reasons there are no more fields to be decoded, let's exit right now
				// It could be the case of some conditional fields that do not exist
				if (DecodedLen == 0)
					return nbSUCCESS;

				RetVal= m_exprHandler->EvaluateExprNumber(LoopElement->ExprTree, m_PDMLProtoItem->FirstField, &Value);

				if (RetVal == nbFAILURE)
					return nbFAILURE;
				if (RetVal == nbWARNING)
					goto MissingPacketData;

				// Get the starting offset of this field
				m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffset);
			}
			break;
		}
	
		case nbNETPDL_ID_LOOP_DOWHILE:
		{
			Value = 1;

			// We have to repeat the decoding at least 1 time
			// unless the m_packet has been completely decoded
			while ( (Value) && (CurrentOffset <= MaxOffsetToBeDecoded) )
			{
			int TmpLoopCtrl;
			unsigned int DecodedLen;

				RetVal= DecodeFields(FirstLoopField, MaxOffsetToBeDecoded, PDMLParent, &TmpLoopCtrl, &DecodedLen);

				if (RetVal != nbSUCCESS)
					return RetVal;

				// If we encounter a "loopctrl", we have to execute it immediately
				if (TmpLoopCtrl != nbNETPDL_ID_LOOPCTRL_NONE)
				{
					if (TmpLoopCtrl == nbNETPDL_ID_LOOPCTRL_BREAK)
						break;

					if (TmpLoopCtrl == nbNETPDL_ID_LOOPCTRL_CONTINUE)
						continue;
				}

				// If for some reasons there are no more fields to be decoded, let's exit right now
				// It could be the case of some conditional fields that do not exist
				if (DecodedLen == 0)
					return nbSUCCESS;

				RetVal= m_exprHandler->EvaluateExprNumber(LoopElement->ExprTree, m_PDMLProtoItem->FirstField, &Value);

				if (RetVal == nbFAILURE)
					return nbFAILURE;
				if (RetVal == nbWARNING)
					goto MissingPacketData;

				// Get the starting offset of this field
				m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffset);
			}
			break;
		}

		case nbNETPDL_ID_LOOP_SIZE:
		{
		unsigned int LoopLen;

			// retrieve the maximum block-size
			RetVal= m_exprHandler->EvaluateExprNumber(LoopElement->ExprTree, m_PDMLProtoItem->FirstField, &Value);

			if (RetVal == nbFAILURE)
				return nbFAILURE;
			if (RetVal == nbWARNING)
				goto MissingPacketData;

			LoopLen= 0;

			// We have a maximum size of the block; decode until this limit is reached
			// or the m_packet has been completely decoded
			while ( (LoopLen < Value) && (CurrentOffset <= MaxOffsetToBeDecoded) )
			{
			int TmpLoopCtrl;
			unsigned int DecodedLen;

				RetVal= DecodeFields(FirstLoopField, MaxOffsetToBeDecoded, PDMLParent, &TmpLoopCtrl, &DecodedLen);

				if (RetVal != nbSUCCESS)
					return RetVal;

				LoopLen+= DecodedLen;

				// If we encounter a "loopctrl", we have to execute it immediately
				if (TmpLoopCtrl != nbNETPDL_ID_LOOPCTRL_NONE)
				{
					if (TmpLoopCtrl == nbNETPDL_ID_LOOPCTRL_BREAK)
						break;

					if (TmpLoopCtrl == nbNETPDL_ID_LOOPCTRL_CONTINUE)
						continue;
				}

				// If for some reasons there are no more fields to be decoded, let's exit right now
				// It could be the case of some conditional fields that do not exist
				if (DecodedLen == 0)
					return nbSUCCESS;

				// Get the starting offset of this field
				m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffset);
			};
			break;
		}
#ifdef _DEBUG
		default:
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Loop type not recognized.");
			return nbFAILURE;
		}
#endif
	}

	return nbSUCCESS;

MissingPacketData:

	if (LoopElement->FirstValidChildIfMissingPacketData)
	{
	int TmpLoopCtrl;
	unsigned int DecodedLen;

		// The value of TmpLoopCtrl doesn't matter; if we find such a tag, we have to exit anyway
		// By the way, also the 'DecodedLen' does not matter, here.
		RetVal= DecodeFields(LoopElement->FirstValidChildIfMissingPacketData, MaxOffsetToBeDecoded, PDMLParent, &TmpLoopCtrl, &DecodedLen);

		return RetVal;
	}

	return nbWARNING;
}



/*!
	\brief This method returns basic parameters for every NetPDL type of field: 
	- amount of initial bytes to discard
	- amount of real bytes belonging to the field (the 'footprint' of the field itself) 
	- amount of ending bytes to discard

	It DOES NOT update the variables related to the packet offset, but it requires all related internal variables correctly updated.

	\param FieldElement: pointer to the current field that has to be decoded.

	\param MaxOffsetToBeDecoded: please refer to the same parameter in function DecodeFields().
	
	\param StartingBytesToDiscard: pointer to integer in which this method stores amount of initial bytes to discard.
	\param FieldLen: pointer to integer in which this method stores amount of bytes belonging to field.
	\param EndingBytesToDiscard: pointer to integer in which this method stores amount of ending bytes to discard.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
*/
int CNetPDLProtoDecoder::GetFieldParams(struct _nbNetPDLElementFieldBase *FieldElement, unsigned int MaxOffsetToBeDecoded, unsigned int *StartingBytesToDiscard, unsigned int *FieldLen, unsigned int *EndingBytesToDiscard)
{
unsigned int StartingFieldOffset;
int RetVal;

	// Get the starting offset of this field
	m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &StartingFieldOffset);

	// Initialize values to return
	(*StartingBytesToDiscard)= 0;
	(*FieldLen)= 0;
	(*EndingBytesToDiscard)= 0;

	switch (FieldElement->FieldType)
	{
		case nbNETPDL_ID_FIELD_FIXED:
		{
			(*FieldLen)= ((struct _nbNetPDLElementFieldFixed *) FieldElement)->Size;
			break;
		}

		case nbNETPDL_ID_FIELD_BIT:
		{
		struct _nbNetPDLElementFieldBit *BitField;

			BitField= (struct _nbNetPDLElementFieldBit *) FieldElement;

			(*FieldLen)= BitField->Size;

			//// TEMP FABIO 27/06/2008: l'esecuzione di queste istruzioni � differita nel chiamante
			//if (BitField->IsLastBitField == 0)
			//	UpdateStartingOffsetVariable= false;

			break;
		}

		case nbNETPDL_ID_FIELD_LINE:
		{
			// It scans the entire m_packet, until it finds the '0d0a' string (\r\n) (or '\n')
			while ( ((*FieldLen) + StartingFieldOffset) <= MaxOffsetToBeDecoded)
			{
				// It checks if the line is UNIX-like
				if (m_packet[StartingFieldOffset + (*FieldLen)] == '\n')
				{
					(*FieldLen)++;
					break;
				}

				(*FieldLen)++;

				// This case is already included in previous one
				// It checks if the line is DOS-like
				//if ( (m_packet[StartingOffset + FieldLen] == '\r') && (m_packet[StartingOffset + FieldLen + 1] == '\n') )
				//{
				//	FieldLen+= 2;
				//	break;
				//}
			}

			break;
		}

		case nbNETPDL_ID_FIELD_VARIABLE:
		{
			RetVal= m_exprHandler->EvaluateExprNumber(
							((struct _nbNetPDLElementFieldVariable *)FieldElement)->ExprTree, m_PDMLProtoItem->FirstField, FieldLen);

			if (RetVal != nbSUCCESS)
				return RetVal;

			// Check that we do not exceed packet boundaries
			if ((*FieldLen) > (MaxOffsetToBeDecoded - StartingFieldOffset + 1))
				*FieldLen= MaxOffsetToBeDecoded - StartingFieldOffset + 1;

			break;
		}

		case nbNETPDL_ID_FIELD_TOKENENDED:
		{
		struct _nbNetPDLElementFieldTokenEnded *FieldTokenEndedElement;

			FieldTokenEndedElement= (struct _nbNetPDLElementFieldTokenEnded *) FieldElement;

			// Warning: at the moment, the StartingOffset is not taken into consideration

			if (FieldTokenEndedElement->EndTokenString)
			{
				// It scans the entire m_packet, until it finds the sentinel string
				while ( ((*FieldLen) + StartingFieldOffset) <= MaxOffsetToBeDecoded )
				{
					// Check if the first field byte matches
					if (m_packet[StartingFieldOffset + (*FieldLen)] == FieldTokenEndedElement->EndTokenString[0])
					{
						// If so, let's check the remaining of the field
						if (memcmp( (void *) &m_packet[StartingFieldOffset + (*FieldLen)], (void *) FieldTokenEndedElement->EndTokenString, 
									FieldTokenEndedElement->EndTokenStringSize) == 0)
						{
							(*FieldLen)+= FieldTokenEndedElement->EndTokenStringSize;
							break;
						}
						else
							(*FieldLen)++;
					}
					else
						(*FieldLen)++;
				}

				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenEndTokenLen, FieldTokenEndedElement->EndTokenStringSize);
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenFieldLen, (*FieldLen) - FieldTokenEndedElement->EndTokenStringSize);
			}
			else
			{
			int RegExpReturnCode;
			int MatchingOffset[MATCHING_OFFSET_COUNT];

				RegExpReturnCode= pcre_exec(
					(pcre *) FieldTokenEndedElement->EndPCRECompiledRegExp,		// the compiled pattern
					NULL,									// no extra data - we didn't study the pattern
					(char *) &m_packet[StartingFieldOffset],				// the subject string
					MaxOffsetToBeDecoded - StartingFieldOffset + 1,	// the length of the subject
					0,									// start at offset 0 in the subject
					0,									// default options
					MatchingOffset,						// output vector for substring information
					MATCHING_OFFSET_COUNT);				// number of elements in the output vector

				if (RegExpReturnCode >= 0)	// Match found
				{
					(*FieldLen)= MatchingOffset[1];
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenEndTokenLen, MatchingOffset[1] - MatchingOffset[0]);
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenFieldLen, MatchingOffset[0]);
				}

				if (RegExpReturnCode == -1)	// Match not found
				{
					(*FieldLen)= MaxOffsetToBeDecoded - StartingFieldOffset;
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenEndTokenLen, 0);
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenFieldLen, (*FieldLen));
				}
			}

			// Please note that a sentinel field *always* exist (i.e. it cannot have len= 0)
			// This is because in case the sentinel field has not been found, the variable
			// field spans over all the remaining m_packet.

			// Now, let's check if we have to modify ending offset, which, in our case
			// corresponds to modify the field length
			if (FieldTokenEndedElement->EndOffsetExprTree)
			{
				RetVal= m_exprHandler->EvaluateExprNumber(
								FieldTokenEndedElement->EndOffsetExprTree, m_PDMLProtoItem->FirstField, FieldLen);

				if (RetVal != nbSUCCESS)
					return RetVal;
			}

			if (FieldTokenEndedElement->EndDiscardExprTree)
			{
				RetVal= m_exprHandler->EvaluateExprNumber(
								FieldTokenEndedElement->EndDiscardExprTree, m_PDMLProtoItem->FirstField, EndingBytesToDiscard);

				if (RetVal != nbSUCCESS)
					return RetVal;
			}

			break;
		}

		case nbNETPDL_ID_FIELD_TOKENWRAPPED:
		{
		struct _nbNetPDLElementFieldTokenWrapped *FieldTokenWrappedElement;
		unsigned int FieldStartingOffset;
		unsigned int StartingTokenLen;
		unsigned int NewBeginOffset;

			FieldTokenWrappedElement= (struct _nbNetPDLElementFieldTokenWrapped *) FieldElement;

			// Fake initializations to avoid compiler warnings
			FieldStartingOffset= 0;
			StartingTokenLen= 0;
			NewBeginOffset= 0;

			if (FieldTokenWrappedElement->BeginTokenString)
			{
				if (memcmp( (void *) &m_packet[StartingFieldOffset], (void *) FieldTokenWrappedElement->BeginTokenString, 
							FieldTokenWrappedElement->BeginTokenStringSize) == 0)
				{
					// Set the starting offset of this field
					FieldStartingOffset= StartingFieldOffset;
					(*FieldLen)= FieldTokenWrappedElement->BeginTokenStringSize;
				}
				else
					break;

				StartingTokenLen= FieldTokenWrappedElement->BeginTokenStringSize;
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenBeginTokenLen, StartingTokenLen);
			}
			else
			{
			int RegExpReturnCode;
			int MatchingOffset[MATCHING_OFFSET_COUNT];

				RegExpReturnCode= pcre_exec(
					(pcre *) FieldTokenWrappedElement->BeginPCRECompiledRegExp,		// the compiled pattern
					NULL,									// no extra data - we didn't study the pattern
					(char *) &m_packet[StartingFieldOffset],				// the subject string
					MaxOffsetToBeDecoded - StartingFieldOffset + 1,	// the length of the subject
					0,									// start at offset 0 in the subject
					0,									// default options
					MatchingOffset,						// output vector for substring information
					MATCHING_OFFSET_COUNT);				// number of elements in the output vector

				if (RegExpReturnCode >= 0)	// Match found
				{
					FieldStartingOffset= StartingFieldOffset + MatchingOffset[0];
					(*FieldLen)= MatchingOffset[1] - MatchingOffset[0];

					StartingTokenLen= MatchingOffset[1] - MatchingOffset[0];
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenBeginTokenLen, StartingTokenLen);
				}

				if (RegExpReturnCode == -1)	// Match not found
					break;
			}

			// If FieldLen == 0, the starting tag has not been found. In this case, let's exit right now (the field does not exist)
			if ((*FieldLen) == 0)
				break;

			if (FieldTokenWrappedElement->EndTokenString)
			{
				// It scans the entire m_packet, until it finds the ending string
				while ( (FieldStartingOffset + (*FieldLen)) <= MaxOffsetToBeDecoded )
				{
					// Check if the first field byte matches
					if (m_packet[FieldStartingOffset + (*FieldLen)] == FieldTokenWrappedElement->EndTokenString[0])
					{
						// If so, let's check the remaining of the field
						if (memcmp( (void *) &m_packet[FieldStartingOffset + (*FieldLen)], (void *) FieldTokenWrappedElement->EndTokenString, 
									FieldTokenWrappedElement->EndTokenStringSize) == 0)
						{
							(*FieldLen)+= FieldTokenWrappedElement->EndTokenStringSize;
							break;
						}
						else
							(*FieldLen)++;
					}
					else
						(*FieldLen)++;
				}

				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenEndTokenLen, FieldTokenWrappedElement->EndTokenStringSize);
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenFieldLen, (*FieldLen) - FieldTokenWrappedElement->EndTokenStringSize - StartingTokenLen);
			}
			else
			{
			int RegExpReturnCode;
			int MatchingOffset[MATCHING_OFFSET_COUNT];

				RegExpReturnCode= pcre_exec(
					(pcre *) FieldTokenWrappedElement->EndPCRECompiledRegExp,		// the compiled pattern
					NULL,									// no extra data - we didn't study the pattern
					(char *) &m_packet[FieldStartingOffset + (*FieldLen)],				// the subject string
					MaxOffsetToBeDecoded - FieldStartingOffset - (*FieldLen) + 1,	// the length of the subject
					0,									// start at offset 0 in the subject
					0,									// default options
					MatchingOffset,						// output vector for substring information
					MATCHING_OFFSET_COUNT);				// number of elements in the output vector

				if (RegExpReturnCode >= 0)	// Match found
				{
					(*FieldLen)+= MatchingOffset[1];
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenEndTokenLen, MatchingOffset[1] - MatchingOffset[0]);
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenFieldLen, MatchingOffset[0]);
				}

				if (RegExpReturnCode == -1)	// Match not found
				{
					(*FieldLen)= MaxOffsetToBeDecoded - FieldStartingOffset;
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenEndTokenLen, 0);
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenFieldLen, (*FieldLen));
				}
			}

			// Update 'Starting Offset', which is no longer the parameter we used for this field, because
			// this field may begin at another offset (if some dust exists between the previous field and the next one)
			(*StartingBytesToDiscard)= StartingFieldOffset;
			StartingFieldOffset= FieldStartingOffset;

			// Now, let's check if we have to modify starting and ending offset of this field
			if (FieldTokenWrappedElement->BeginOffsetExprTree)
			{
				RetVal= m_exprHandler->EvaluateExprNumber(
								FieldTokenWrappedElement->BeginOffsetExprTree, m_PDMLProtoItem->FirstField, &NewBeginOffset);

				if (RetVal != nbSUCCESS)
					return RetVal;

				//// TEMP FABIO 27/06/2008: was this previous expression right??? I replaced it with its following.
				//StartingFieldOffset+= NewBeginOffset;
				StartingFieldOffset= NewBeginOffset;
				(*StartingBytesToDiscard)= NewBeginOffset - (*StartingBytesToDiscard);
			}
			else
				(*StartingBytesToDiscard)= FieldStartingOffset - (*StartingBytesToDiscard);

			if (FieldTokenWrappedElement->EndOffsetExprTree)
			{
			unsigned int NewEndOffset;

				RetVal= m_exprHandler->EvaluateExprNumber(
								FieldTokenWrappedElement->EndOffsetExprTree, m_PDMLProtoItem->FirstField, &NewEndOffset);

				if (RetVal != nbSUCCESS)
					return RetVal;

				(*FieldLen)= NewEndOffset - NewBeginOffset;
			}

			if (FieldTokenWrappedElement->EndDiscardExprTree)
			{
				RetVal= m_exprHandler->EvaluateExprNumber(
								FieldTokenWrappedElement->EndDiscardExprTree, m_PDMLProtoItem->FirstField, EndingBytesToDiscard);

				if (RetVal != nbSUCCESS)
					return RetVal;
			}

			break;
		}

		case nbNETPDL_ID_FIELD_PATTERN:
		{
		struct _nbNetPDLElementFieldPattern *FieldPatternElement;
		int RegExpReturnCode;
		int MatchingOffset[MATCHING_OFFSET_COUNT];
		int PCREFlags= PCRE_PARTIAL;

			FieldPatternElement= (struct _nbNetPDLElementFieldPattern *) FieldElement;

			RegExpReturnCode= pcre_exec(
				(pcre *) FieldPatternElement->PatternPCRECompiledRegExp,	// the compiled pattern
				NULL,										// no extra data - we didn't study the pattern
				(char *) &m_packet[StartingFieldOffset],			// the subject string
				MaxOffsetToBeDecoded - StartingFieldOffset + 1,	// the length of the subject
				0,									// start at offset 0 in the subject
				PCREFlags,							// default options
				MatchingOffset,						// output vector for substring information
				MATCHING_OFFSET_COUNT);				// number of elements in the output vector

			if ( (RegExpReturnCode >= 0 && MatchingOffset[0] == 0)	// Match found and it starts from current offset
				|| (FieldPatternElement->OnPartialDecodingContinue  // Partial match found and it starts from current offset
				   && RegExpReturnCode == PCRE_ERROR_PARTIAL
				   && MatchingOffset[0] == 0) )
			{
				(*FieldLen)= MatchingOffset[1] - MatchingOffset[0];
			}

			break;
		}

		case nbNETPDL_ID_FIELD_EATALL:
		{
			(*FieldLen)= MaxOffsetToBeDecoded - StartingFieldOffset + 1;
			break;
		}

		case nbNETPDL_ID_FIELD_PADDING:
		{
		int AlignSize;

			AlignSize= ((struct _nbNetPDLElementFieldPadding *)FieldElement)->Align;
			(*FieldLen)= AlignSize - (StartingFieldOffset - m_protoStartingOffset) % AlignSize;
			break;
		}

		case nbNETPDL_ID_FIELD_PLUGIN:
		{
		struct _PluginParams PluginParams;
		int PluginIndex;

			PluginParams.CapturedLength= m_pcapHeader->caplen;
			PluginParams.PDMLProto= m_PDMLProtoItem;
			PluginParams.ErrBuf= m_errbuf;
			PluginParams.ErrBufSize= m_errbufSize;
			PluginParams.FieldOffset= StartingFieldOffset;
			PluginParams.Packet= m_packet;
			PluginParams.NetPDLVariables= m_netPDLVariables;

			PluginIndex= ((struct _nbNetPDLElementFieldPlugin *) FieldElement)->PluginIndex;
			if ( (PluginList[PluginIndex].PluginHandler) (&PluginParams) == nbFAILURE)
				return nbFAILURE;

			(*FieldLen)= PluginParams.Size;

			break;
		}

		case nbNETPDL_ID_CFIELD_TLV:
		{
		struct _nbNetPDLElementCfieldTLV *CfieldTLVElement;
		unsigned int IterLengthOffset, IterLengthSize;

			CfieldTLVElement= (struct _nbNetPDLElementCfieldTLV *) FieldElement;

			// It scans the m_packet to retrieve the size of 'Value' part
			IterLengthOffset= StartingFieldOffset + CfieldTLVElement->TypeSize;
			IterLengthSize= CfieldTLVElement->LengthSize;
			while ( IterLengthOffset <= MaxOffsetToBeDecoded && IterLengthSize > 0 )
			{
				(*FieldLen)<<= 8; // same as (*FieldLen)*= 256, but faster
				(*FieldLen)+= m_packet[IterLengthOffset];
				IterLengthOffset++;
				IterLengthSize--;
			}

			if (IterLengthSize == 0)
			{
				// The 'Length' part was not fragmented, let's update the whole length of the field
				(*FieldLen)= (*FieldLen) + CfieldTLVElement->TypeSize + CfieldTLVElement->LengthSize;
				if ((*FieldLen) > MaxOffsetToBeDecoded)
				{
					(*FieldLen)= MaxOffsetToBeDecoded + 1;
					return nbWARNING;
				}
			}
			else
			{
				(*FieldLen)= MaxOffsetToBeDecoded + 1;
				return nbWARNING;
			}

			break;
		}

		case nbNETPDL_ID_CFIELD_DELIMITED:
		{
		struct _nbNetPDLElementCfieldDelimited *CfieldDelimitedElement;
		unsigned int SubFieldStartingOffset;
		int RegExpReturnCode;
		int MatchingOffset[MATCHING_OFFSET_COUNT];
		//int PCREFlags= 0;

			CfieldDelimitedElement= (struct _nbNetPDLElementCfieldDelimited *) FieldElement;

			SubFieldStartingOffset= StartingFieldOffset;

			if (CfieldDelimitedElement->BeginRegularExpression)
			{
				RegExpReturnCode= pcre_exec(
					(pcre *) CfieldDelimitedElement->BeginPCRECompiledRegExp,		// the compiled pattern
					NULL,											// no extra data - we didn't study the pattern
					(char *) &m_packet[StartingFieldOffset],				// the subject string
					MaxOffsetToBeDecoded - StartingFieldOffset + 1,		// the length of the subject
					0,									// start at offset 0 in the subject
					0,									// default options
					MatchingOffset,						// output vector for substring information
					MATCHING_OFFSET_COUNT);				// number of elements in the output vector

				if (RegExpReturnCode >= 0 && MatchingOffset[0] == 0)
				{
					(*StartingBytesToDiscard)= MatchingOffset[1];
					SubFieldStartingOffset+= MatchingOffset[1];
				}
				else
				{
					if (!CfieldDelimitedElement->OnMissingBeginContinue)
					{
						(*StartingBytesToDiscard)= 0;
						(*FieldLen)= 0;
						(*EndingBytesToDiscard)= 0;
						return nbSUCCESS;
					}
				}
			}

			RegExpReturnCode= pcre_exec(
				(pcre *) CfieldDelimitedElement->EndPCRECompiledRegExp,		// the compiled pattern
				NULL,												// no extra data - we didn't study the pattern
				(char *) &m_packet[SubFieldStartingOffset],			// the subject string
				MaxOffsetToBeDecoded - SubFieldStartingOffset + 1,	// the length of the subject
				0,									// start at offset 0 in the subject
				0,									// default options
				MatchingOffset,						// output vector for substring information
				MATCHING_OFFSET_COUNT);				// number of elements in the output vector

			if (RegExpReturnCode == -1 && CfieldDelimitedElement->OnMissingEndContinue)	// Match not found
			{
				(*FieldLen)= MaxOffsetToBeDecoded - SubFieldStartingOffset + 1;
			}
			else
			{
				if (RegExpReturnCode >= 0)	// Match found
				{
					(*FieldLen)= MatchingOffset[0];
					(*EndingBytesToDiscard)= MatchingOffset[1] - MatchingOffset[0];
				}
				else
				{
					(*StartingBytesToDiscard)= 0;
					(*FieldLen)= 0;
					(*EndingBytesToDiscard)= 0;
					return nbSUCCESS;
				}
			}

			break;
		}

		case nbNETPDL_ID_CFIELD_LINE:
		{
                  // struct _nbNetPDLElementCfieldLine *CfieldLineElement;

                  // CfieldLineElement= (struct _nbNetPDLElementCfieldLine *) FieldElement;

			// It scans the entire m_packet, until it finds the '0d0a' string (\r\n) (or '\n')
			while ( ((*FieldLen) + StartingFieldOffset) <= MaxOffsetToBeDecoded )
			{
				if (m_packet[StartingFieldOffset + (*FieldLen)] == '\n')
				{
					(*FieldLen)++;
					break;
				}
				
				(*FieldLen)++;
			}

			if ( ((*FieldLen) + StartingFieldOffset) == MaxOffsetToBeDecoded )
			{
				(*FieldLen)++;
				return nbWARNING;
			}
			else
			{
				(*FieldLen)-= 2;
				(*EndingBytesToDiscard)= 2;
			}

			break;
		}

		case nbNETPDL_ID_CFIELD_HDRLINE:
		{
                  // struct _nbNetPDLElementCfieldHdrline *CfieldHdrlineElement;

                  // CfieldHdrlineElement= (struct _nbNetPDLElementCfieldHdrline *) FieldElement;

			// It scans the entire m_packet, until it finds the '0d0a' string (\r\n) (or '\n') not followed by ' ' or '\t'
			while ( ((*FieldLen) + StartingFieldOffset) <= MaxOffsetToBeDecoded )
			{
				if ( m_packet[StartingFieldOffset + (*FieldLen)] == '\n'
					&& (StartingFieldOffset + (*FieldLen) == MaxOffsetToBeDecoded 
						|| (m_packet[StartingFieldOffset + (*FieldLen) + 1] != '\t' && m_packet[StartingFieldOffset + (*FieldLen) + 1] != ' ')) )
				{
					(*FieldLen)++;
					break;
				}

				(*FieldLen)++;
			}

			if ( ((*FieldLen) + StartingFieldOffset) == MaxOffsetToBeDecoded )
			{
				(*FieldLen)++;
				return nbWARNING;
			}
			else
			{
				(*FieldLen)-= 2;
				(*EndingBytesToDiscard)= 2;
			}

			break;
		}

		case nbNETPDL_ID_CFIELD_DYNAMIC:
		{
		struct _nbNetPDLElementCfieldDynamic *CfieldDynamicElement;
		int RegExpReturnCode;
		int MatchingOffset[MATCHING_OFFSET_COUNT];
		//int PCREFlags= 0;

			CfieldDynamicElement= (struct _nbNetPDLElementCfieldDynamic *) FieldElement;

			RegExpReturnCode= pcre_exec(
				(pcre *) CfieldDynamicElement->PatternPCRECompiledRegExp,	// the compiled pattern
				NULL,										// no extra data - we didn't study the pattern
				(char *) &m_packet[StartingFieldOffset],			// the subject string
				MaxOffsetToBeDecoded - StartingFieldOffset + 1,	// the length of the subject
				0,									// start at offset 0 in the subject
				0,									// default options
				MatchingOffset,						// output vector for substring information
				MATCHING_OFFSET_COUNT);				// number of elements in the output vector

			if (RegExpReturnCode >= 0 && MatchingOffset[0] == 0)	// Match found
			{
				(*FieldLen)= MatchingOffset[1];
			}
			else
				return nbFAILURE;

			break;
		}

		case nbNETPDL_ID_CFIELD_ASN1:
		{
                  // struct _nbNetPDLElementCfieldASN1 *CfieldASN1Element;

                  // CfieldASN1Element= (struct _nbNetPDLElementCfieldASN1 *) FieldElement;

			//// TEMP FABIO 27/06/2008: ASN.1 encoding support suspended
			//switch (CfieldASN1Element->Encoding)
			//{
			//	case nbNETPDL_ID_ENCODING_RAW:
			//	case nbNETPDL_ID_ENCODING_BER: // default case
			//	case nbNETPDL_ID_ENCODING_DER:
			//	case nbNETPDL_ID_ENCODING_CER:
			//	{

					//// TEMP FABIO 27/06/2008: this uncommented section contain ASN.1 decoding based on its specification

					if ( (StartingFieldOffset + (*StartingBytesToDiscard)) > MaxOffsetToBeDecoded )
					{
						(*StartingBytesToDiscard)= MaxOffsetToBeDecoded + 1;
						return nbWARNING;
					}

					if ( (m_packet[StartingFieldOffset + (*StartingBytesToDiscard)] & 0x1F) == 0x1F )
					{
						(*StartingBytesToDiscard)++;

						// It scans the entire m_packet, until it finds the last octet of Identifier Octet
						while ( (++(*StartingBytesToDiscard)) + StartingFieldOffset <= MaxOffsetToBeDecoded
								&&  (m_packet[StartingFieldOffset + (*StartingBytesToDiscard)] & 0x80) != 0x00 );
					}

					(*StartingBytesToDiscard)++;

					if ( (StartingFieldOffset + (*StartingBytesToDiscard)) > MaxOffsetToBeDecoded )
					{
						(*StartingBytesToDiscard)= MaxOffsetToBeDecoded + 1;
						return nbWARNING;
					}

					if ( (m_packet[StartingFieldOffset + (*StartingBytesToDiscard)] & 0x80) == 0x00 )
					{
						(*FieldLen)= (unsigned int) m_packet[StartingFieldOffset + (*StartingBytesToDiscard)];
					}
					else
					{
					unsigned int Index= StartingFieldOffset + (*StartingBytesToDiscard);

						if ( (m_packet[StartingFieldOffset + (*StartingBytesToDiscard)] & 0x7F) != 0x00 )
						{
						unsigned int IterLengthOctet;

							IterLengthOctet= (unsigned int) (m_packet[Index] & 0x7F);
							(*StartingBytesToDiscard)+= IterLengthOctet;

							while ( (++Index) <= MaxOffsetToBeDecoded && IterLengthOctet > 0 )	
							{
								(*FieldLen)<<= 8;	// Same as 'ValueLen*= 256;', but faster
								(*FieldLen)+= (unsigned int) m_packet[Index];
								IterLengthOctet--;
							}

							if (IterLengthOctet != 0)
							{
								(*FieldLen)= MaxOffsetToBeDecoded + 1;
								return nbWARNING;
							}
						}
						else
						{
						unsigned int EocRecognizedBytes= 0;

							while ( (++Index) <= MaxOffsetToBeDecoded
									&&  EocRecognizedBytes != 2 )
							{
								switch (EocRecognizedBytes)
								{
								case 0:
									{
										if ( m_packet[Index] == 0x00 )
											EocRecognizedBytes++;
									};break;
								case 1:
									{
										EocRecognizedBytes= m_packet[Index] == 0x00 ?
											EocRecognizedBytes + 1 : 0;
									};break;
								}
							}

							if ( Index > MaxOffsetToBeDecoded )
							{
								(*FieldLen)= MaxOffsetToBeDecoded + 1;
								return nbWARNING;
							}

							(*FieldLen)= (++Index) - (StartingFieldOffset + (*StartingBytesToDiscard) + (*EndingBytesToDiscard));
							(*EndingBytesToDiscard)= 2;
						}
					}

					(*StartingBytesToDiscard)++;

			//		break;
			//	}
			//}

			break;
		}

		//// TEMP FABIO 27/06/2008: XML under construction
		case nbNETPDL_ID_CFIELD_XML:
		{
		struct _nbNetPDLElementCfieldXML *CfieldXMLElement;

			CfieldXMLElement= (struct _nbNetPDLElementCfieldXML *) FieldElement;

			if (CfieldXMLElement->SizeExprString)
			{
				return m_exprHandler->EvaluateExprNumber(
										CfieldXMLElement->SizeExprTree, m_PDMLProtoItem->FirstField, FieldLen);
			}
			else
			{
			void *PatternPCRECompiledRegExp;
			const char *PCREErrorPtr;
			int PCREErrorOffset;
			int RegExpReturnCode;
			int MatchingOffset[MATCHING_OFFSET_COUNT];
			int PCREFlags= PCRE_DOTALL;

				PatternPCRECompiledRegExp= (void *) pcre_compile(".+</.+?>", PCREFlags, &PCREErrorPtr, &PCREErrorOffset, NULL);

				if (PatternPCRECompiledRegExp == NULL)
					return nbFAILURE;

				RegExpReturnCode= pcre_exec(
					(pcre *) PatternPCRECompiledRegExp,			// the compiled pattern
					NULL,										// no extra data - we didn't study the pattern
					(char *) &m_packet[StartingFieldOffset],			// the subject string
					MaxOffsetToBeDecoded - StartingFieldOffset + 1,	// the length of the subject
					0,									// start at offset 0 in the subject
					0,									// default options
					MatchingOffset,						// output vector for substring information
					MATCHING_OFFSET_COUNT);				// number of elements in the output vector

				if (RegExpReturnCode >= 0 && MatchingOffset[0] == 0)	// Match found
				{
					(*FieldLen)= MatchingOffset[1];
				}
				else
					return nbFAILURE;
			}

			break;
		}
		default:
			break;
	}

	return nbSUCCESS;
}



//// TEMP FABIO 27/06/2008: I leave intentionally this section for sake of readability respect to
//// past versions of library, therefore someone has to perform its deleting after check out!
//// Whatever following now belongs to past!
/*!
	\brief This method handles the standard types of fields (i.e. not conditional elements)

	It updates also the variables related to the packet offset.

	\param FieldElement: pointer to the current field that has to be decoded.

	\param MaxOffsetToBeDecoded: please refer to the same parameter in function DecodeFields().
	\param PDMLParent: please refer to the same parameter in function DecodeFields().
	\param CurrentPDMLElement: please refer to the same parameter in function DecodeFields().

	\param CurrentFieldStartingOffset: In case of 'tokenwrapped' fields, the actual starting offset
	may be different from the original one (simply because we're going to locate the beginning token,
	which may not be at the beginning of the current packet buffer). Therefore, we have to return back
	the actual starting offset of this field.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
//int CNetPDLProtoDecoder::DecodeStandardFieldOLD(struct _nbNetPDLElementFieldBase *FieldElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent, struct _nbPDMLField **CurrentPDMLElement, unsigned int *CurrentFieldStartingOffset)
//{
//struct _nbPDMLField *PDMLElement;
//int UpdateStartingOffsetVariable;
//unsigned int StartingOffset;
//unsigned int FieldLen;
//unsigned int BytesToDiscard;	// This is used only in tokenended and tokenwrapped fields
//int RetVal;
//
//	// Get the starting offset of this field
//	m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &StartingOffset);
//
//	FieldLen= 0;
//	BytesToDiscard= 0;
//
//	UpdateStartingOffsetVariable= true;
//
//	// Create a Field node and append it to PDML tree
//	PDMLElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);
//
//
//	switch (FieldElement->FieldType)
//	{
//		case nbNETPDL_ID_FIELD_FIXED:
//		{
//			FieldLen= ((struct _nbNetPDLElementFieldFixed *) FieldElement)->Size;
//			break;
//		}
//
//		case nbNETPDL_ID_FIELD_BIT:
//		{
//		struct _nbNetPDLElementFieldBit *BitField;
//
//			BitField= (struct _nbNetPDLElementFieldBit *) FieldElement;
//
//			FieldLen= BitField->Size;
//
//			if (BitField->IsLastBitField == 0)
//				UpdateStartingOffsetVariable= false;
//
//			break;
//		}
//
//		case nbNETPDL_ID_FIELD_LINE:
//		{
//			// It scans the entire m_packet, until it finds the '0d0a' string (\r\n) (or '\n')
//			while ( (FieldLen + StartingOffset) <= MaxOffsetToBeDecoded)
//			{
//				// It checks if the line is UNIX-like
//				if (m_packet[StartingOffset + FieldLen] == '\n')
//				{
//					FieldLen++;
//					break;
//				}
//
//// This case is already included in previous one
//				// It checks if the line is DOS-like
////				if ( (m_packet[StartingOffset + FieldLen] == '\r') && (m_packet[StartingOffset + FieldLen + 1] == '\n') )
////				{
////					FieldLen+= 2;
////					break;
////				}
//				
//				FieldLen++;
//			}
//
//			break;
//		}
//
//		case nbNETPDL_ID_FIELD_VARIABLE:
//		{
//			RetVal= m_exprHandler->EvaluateExprNumber(
//							((struct _nbNetPDLElementFieldVariable *)FieldElement)->ExprTree, m_PDMLProtoItem->FirstField, &FieldLen);
//
//			if (RetVal != nbSUCCESS)
//				return RetVal;
//
//			break;
//		}
//
//		case nbNETPDL_ID_FIELD_TOKENENDED:
//		{
//		struct _nbNetPDLElementFieldTokenEnded *FieldTokenEndedElement;
//
//			FieldTokenEndedElement= (struct _nbNetPDLElementFieldTokenEnded *) FieldElement;
//
//			// Warning: at the moment, the StartingOffset is not taken into consideration
//
//			if (FieldTokenEndedElement->EndTokenString)
//			{
//				// It scans the entire m_packet, until it finds the sentinel string
//				while ( (FieldLen + StartingOffset) <= MaxOffsetToBeDecoded )
//				{
//					// Check if the first field byte matches
//					if (m_packet[StartingOffset + FieldLen] == FieldTokenEndedElement->EndTokenString[0])
//					{
//						// If so, let's check the remaining of the field
//						if (memcmp( (void *) &m_packet[StartingOffset + FieldLen], (void *) FieldTokenEndedElement->EndTokenString, 
//									FieldTokenEndedElement->EndTokenStringSize) == 0)
//						{
//							FieldLen+= FieldTokenEndedElement->EndTokenStringSize;
//							break;
//						}
//						else
//							FieldLen++;
//					}
//					else
//						FieldLen++;
//				}
//
//				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenEndTokenLen, FieldTokenEndedElement->EndTokenStringSize);
//				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenFieldLen, FieldLen - FieldTokenEndedElement->EndTokenStringSize);
//			}
//			else
//			{
//			int RegExpReturnCode;
//			int MatchingOffset[MATCHING_OFFSET_COUNT];
//
//				RegExpReturnCode= pcre_exec(
//					(pcre *) FieldTokenEndedElement->EndPCRECompiledRegExp,		// the compiled pattern
//					NULL,									// no extra data - we didn't study the pattern
//					(char *) &m_packet[StartingOffset],				// the subject string
//					MaxOffsetToBeDecoded - StartingOffset + 1,	// the length of the subject
//					0,									// start at offset 0 in the subject
//					0,									// default options
//					MatchingOffset,						// output vector for substring information
//					MATCHING_OFFSET_COUNT);				// number of elements in the output vector
//
//				if (RegExpReturnCode >= 0)	// Match found
//				{
//					FieldLen= MatchingOffset[1];
//					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenEndTokenLen, MatchingOffset[1] - MatchingOffset[0]);
//					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenFieldLen, MatchingOffset[0]);
//				}
//
//				if (RegExpReturnCode == -1)	// Match not found
//				{
//					FieldLen= MaxOffsetToBeDecoded - StartingOffset;
//					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenEndTokenLen, 0);
//					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenFieldLen, FieldLen);
//				}
//			}
//
//			// Please note that a sentinel field *always* exist (i.e. it cannot have len= 0)
//			// This is because in case the sentinel field has not been found, the variable
//			// field spans over all the remaining m_packet.
//
//			// Now, let's check if we have to modify ending offset, which, in our case
//			// corresponds to modify the field length
//			if (FieldTokenEndedElement->EndOffsetExprTree)
//			{
//				RetVal= m_exprHandler->EvaluateExprNumber(
//								FieldTokenEndedElement->EndOffsetExprTree, m_PDMLProtoItem->FirstField, &FieldLen);
//
//				if (RetVal != nbSUCCESS)
//					return RetVal;
//			}
//
//			if (FieldTokenEndedElement->EndDiscardExprTree)
//			{
//				RetVal= m_exprHandler->EvaluateExprNumber(
//								FieldTokenEndedElement->EndDiscardExprTree, m_PDMLProtoItem->FirstField, &BytesToDiscard);
//
//				if (RetVal != nbSUCCESS)
//					return RetVal;
//			}
//
//			break;
//		}
//
//		case nbNETPDL_ID_FIELD_TOKENWRAPPED:
//		{
//		struct _nbNetPDLElementFieldTokenWrapped *FieldTokenWrappedElement;
//		unsigned int FieldStartingOffset;
//		unsigned int StartingTokenLen;
//		unsigned int NewBeginOffset;
//
//			FieldTokenWrappedElement= (struct _nbNetPDLElementFieldTokenWrapped *) FieldElement;
//
//			// Fake initializations to avoid compiler warnings
//			FieldStartingOffset= 0;
//			StartingTokenLen= 0;
//			NewBeginOffset= 0;
//
//			if (FieldTokenWrappedElement->BeginTokenString)
//			{
//				if (memcmp( (void *) &m_packet[StartingOffset], (void *) FieldTokenWrappedElement->BeginTokenString, 
//							FieldTokenWrappedElement->BeginTokenStringSize) == 0)
//				{
//					// Set the starting offset of this field
//					FieldStartingOffset= StartingOffset;
//					FieldLen= FieldTokenWrappedElement->BeginTokenStringSize;
//				}
//				else
//					break;
//
//				StartingTokenLen= FieldTokenWrappedElement->BeginTokenStringSize;
//				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenBeginTokenLen, StartingTokenLen);
//			}
//			else
//			{
//			int RegExpReturnCode;
//			int MatchingOffset[MATCHING_OFFSET_COUNT];
//
//				RegExpReturnCode= pcre_exec(
//					(pcre *) FieldTokenWrappedElement->BeginPCRECompiledRegExp,		// the compiled pattern
//					NULL,									// no extra data - we didn't study the pattern
//					(char *) &m_packet[StartingOffset],				// the subject string
//					MaxOffsetToBeDecoded - StartingOffset + 1,	// the length of the subject
//					0,									// start at offset 0 in the subject
//					0,									// default options
//					MatchingOffset,						// output vector for substring information
//					MATCHING_OFFSET_COUNT);				// number of elements in the output vector
//
//				if (RegExpReturnCode >= 0)	// Match found
//				{
//					FieldStartingOffset= StartingOffset + MatchingOffset[0];
//					FieldLen= MatchingOffset[1] - MatchingOffset[0];
//
//					StartingTokenLen= MatchingOffset[1] - MatchingOffset[0];
//					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenBeginTokenLen, StartingTokenLen);
//				}
//
//				if (RegExpReturnCode == -1)	// Match not found
//					break;
//			}
//
//			// If FieldLen == 0, the starting tag has not been found. In this case, let's exit right now (the field does not exist)
//			if (FieldLen == 0)
//				break;
//
//			if (FieldTokenWrappedElement->EndTokenString)
//			{
//				// It scans the entire m_packet, until it finds the ending string
//				while ( (FieldStartingOffset + FieldLen) <= MaxOffsetToBeDecoded )
//				{
//					// Check if the first field byte matches
//					if (m_packet[FieldStartingOffset + FieldLen] == FieldTokenWrappedElement->EndTokenString[0])
//					{
//						// If so, let's check the remaining of the field
//						if (memcmp( (void *) &m_packet[FieldStartingOffset + FieldLen], (void *) FieldTokenWrappedElement->EndTokenString, 
//									FieldTokenWrappedElement->EndTokenStringSize) == 0)
//						{
//							FieldLen+= FieldTokenWrappedElement->EndTokenStringSize;
//							break;
//						}
//						else
//							FieldLen++;
//					}
//					else
//						FieldLen++;
//				}
//
//				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenEndTokenLen, FieldTokenWrappedElement->EndTokenStringSize);
//				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenFieldLen, FieldLen - FieldTokenWrappedElement->EndTokenStringSize - StartingTokenLen);
//			}
//			else
//			{
//			int RegExpReturnCode;
//			int MatchingOffset[MATCHING_OFFSET_COUNT];
//
//				RegExpReturnCode= pcre_exec(
//					(pcre *) FieldTokenWrappedElement->EndPCRECompiledRegExp,		// the compiled pattern
//					NULL,									// no extra data - we didn't study the pattern
//					(char *) &m_packet[FieldStartingOffset + FieldLen],				// the subject string
//					MaxOffsetToBeDecoded - FieldStartingOffset - FieldLen + 1,	// the length of the subject
//					0,									// start at offset 0 in the subject
//					0,									// default options
//					MatchingOffset,						// output vector for substring information
//					MATCHING_OFFSET_COUNT);				// number of elements in the output vector
//
//				if (RegExpReturnCode >= 0)	// Match found
//				{
//					FieldLen+= MatchingOffset[1];
//					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenEndTokenLen, MatchingOffset[1] - MatchingOffset[0]);
//					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenFieldLen, MatchingOffset[0]);
//				}
//
//				if (RegExpReturnCode == -1)	// Match not found
//				{
//					FieldLen= MaxOffsetToBeDecoded - FieldStartingOffset;
//					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenEndTokenLen, 0);
//					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TokenFieldLen, FieldLen);
//				}
//			}
//
//			// Update 'Starting Offset', which is no longer the parameter we used for this field, because
//			// this field may begin at another offset (if some dust exists between the previous field and the next one)
//			StartingOffset= FieldStartingOffset;
//
//			// Now, let's check if we have to modify starting and ending offset of this field
//			if (FieldTokenWrappedElement->BeginOffsetExprTree)
//			{
//				RetVal= m_exprHandler->EvaluateExprNumber(
//								FieldTokenWrappedElement->BeginOffsetExprTree, m_PDMLProtoItem->FirstField, &NewBeginOffset);
//
//				if (RetVal != nbSUCCESS)
//					return RetVal;
//
//				StartingOffset+= NewBeginOffset;
//			}
//
//			if (FieldTokenWrappedElement->EndOffsetExprTree)
//			{
//			unsigned int NewEndOffset;
//
//				RetVal= m_exprHandler->EvaluateExprNumber(
//								FieldTokenWrappedElement->EndOffsetExprTree, m_PDMLProtoItem->FirstField, &NewEndOffset);
//
//				if (RetVal != nbSUCCESS)
//					return RetVal;
//
//				FieldLen= NewEndOffset - NewBeginOffset;
//			}
//
//			if (FieldTokenWrappedElement->EndDiscardExprTree)
//			{
//				RetVal= m_exprHandler->EvaluateExprNumber(
//								FieldTokenWrappedElement->EndDiscardExprTree, m_PDMLProtoItem->FirstField, &BytesToDiscard);
//
//				if (RetVal != nbSUCCESS)
//					return RetVal;
//			}
//
//			break;
//		}
//
//		case nbNETPDL_ID_FIELD_PATTERN:
//		{
//		struct _nbNetPDLElementFieldPattern *FieldPatternElement;
//		int RegExpReturnCode;
//		int MatchingOffset[MATCHING_OFFSET_COUNT];
//
//			FieldPatternElement= (struct _nbNetPDLElementFieldPattern *) FieldElement;
//
//			RegExpReturnCode= pcre_exec(
//				(pcre *) FieldPatternElement->PatternPCRECompiledRegExp,	// the compiled pattern
//				NULL,										// no extra data - we didn't study the pattern
//				(char *) &m_packet[StartingOffset],			// the subject string
//				MaxOffsetToBeDecoded - StartingOffset + 1,	// the length of the subject
//				0,									// start at offset 0 in the subject
//				0,									// default options
//				MatchingOffset,						// output vector for substring information
//				MATCHING_OFFSET_COUNT);				// number of elements in the output vector
//
//			if (RegExpReturnCode >= 0 && MatchingOffset[0] == 0)	// Match found and it starts from current offset
//			{
//				FieldLen= MatchingOffset[1] - MatchingOffset[0];
//			}
//
//			break;
//		}
//
//		case nbNETPDL_ID_FIELD_EATALL:
//		{
//			FieldLen= MaxOffsetToBeDecoded - StartingOffset + 1;
//			break;
//		}
//
//		case nbNETPDL_ID_FIELD_PADDING:
//		{
//		int AlignSize;
//
//			AlignSize= ((struct _nbNetPDLElementFieldPadding *)FieldElement)->Align;
//			FieldLen= AlignSize - (StartingOffset - m_protoStartingOffset) % AlignSize;
//			break;
//		}
//
//		case nbNETPDL_ID_FIELD_PLUGIN:
//		{
//		struct _PluginParams PluginParams;
//		int PluginIndex;
//
//			PluginParams.CapturedLength= m_pcapHeader->caplen;
//			PluginParams.PDMLProto= m_PDMLProtoItem;
//			PluginParams.ErrBuf= m_errbuf;
//			PluginParams.ErrBufSize= m_errbufSize;
//			PluginParams.FieldOffset= StartingOffset;
//			PluginParams.Packet= m_packet;
//			PluginParams.NetPDLVariables= m_netPDLVariables;
//
//			PluginIndex= ((struct _nbNetPDLElementFieldPlugin *) FieldElement)->PluginIndex;
//			if ( (PluginList[PluginIndex].PluginHandler) (&PluginParams) == nbFAILURE)
//				return nbFAILURE;
//
//			FieldLen= PluginParams.Size;
//
//			break;
//		}
//	}
//
//
//	if (FieldLen <= 0)
//	{
//		// If len <= 0, I have to remove this field,
//		// since it contains no data (so, it does not exist in the real packet)
//		// This is the case of a variable field whose size is equal to zero.
//		m_PDMLMaker->PDMLElementDiscard(PDMLElement);
//
//		// This value must be returned to the caller
//		*CurrentPDMLElement= NULL;
//
//		return nbSUCCESS;
//	}
//	else
//	{
//		// Saves this value and returns it to the caller function
//		*CurrentFieldStartingOffset= StartingOffset;
//
//		//	Update the common fields of a PDML element (name, longname, ...)
//		RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLElement, (struct _nbNetPDLElementFieldBase *) FieldElement, FieldLen, StartingOffset, &m_packet[StartingOffset]);
//
//		if (RetVal != nbSUCCESS)
//			return RetVal;
//
//		// Save this value, that must be returned to the caller
//		*CurrentPDMLElement= PDMLElement;
//
//		if (UpdateStartingOffsetVariable)
//		{
//		unsigned int NewOffset;
//
//			NewOffset= StartingOffset + FieldLen + BytesToDiscard;
//
//			// Check that we do not exceed packet boundaries
//			if (NewOffset > MaxOffsetToBeDecoded)
//				NewOffset= MaxOffsetToBeDecoded + 1;
//
//			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, NewOffset);
//			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, NewOffset - m_protoStartingOffset);
//		}
//		return nbSUCCESS;
//	}
//}
//
//
//



/*!
	\brief This method handles the simple subfields

	It updates also the variables related to the packet offset.

	\param SubfieldElement: pointer to the current subfield that has to be decoded.

	\param StartingOffset: the starting offset of the current subfield.
	\param Size: the size of the current subfield.
	\param CurrentPDMLElement: please refer to the same parameter in function DecodeFields().

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CNetPDLProtoDecoder::DecodeSubfield(struct _nbNetPDLElementSubfield *SubfieldElement, unsigned int StartingOffset, unsigned int Size, _nbPDMLField *PDMLParent, int *LoopCtrl, struct _nbPDMLField *AlreadyAllocatedPDMLElement)
{
		if (SubfieldElement->ComplexSubfieldInfo)
		{
		struct _nbPDMLField *CurrentPDMLElement;
		int RetVal;
		unsigned int CurrentOffsetBuffer;
		unsigned int CurrentFieldStartingOffset;

			// Save the offset of the next sibling field; we need to restore this offset as soon as we decoded child fields
			m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffsetBuffer);

			// Reset packet offsets to the value that was present before decoding this subfield
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, StartingOffset);
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, StartingOffset - m_protoStartingOffset);

			RetVal= DecodeStandardField(SubfieldElement->ComplexSubfieldInfo, StartingOffset + Size - 1, PDMLParent, &CurrentPDMLElement, &CurrentFieldStartingOffset);

			if (RetVal != nbSUCCESS)
				return RetVal;

			// Reset packet offsets to the value that was present after decoding this subfield
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, CurrentOffsetBuffer);
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, CurrentOffsetBuffer - m_protoStartingOffset);
		}
		else
		{
		struct _nbPDMLField *PDMLElement;
		int RetVal;
		unsigned int CurrentOffsetBuffer;

			if (AlreadyAllocatedPDMLElement == NULL)
			{
				// Create a Field node and append it to PDML tree
				PDMLElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);
			}
			else
			{
				PDMLElement= AlreadyAllocatedPDMLElement;
			}

			//	Update the common fields of a PDML element (name, longname, ...)
			RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLElement, (struct _nbNetPDLElementFieldBase *) SubfieldElement, Size, StartingOffset, &m_packet[StartingOffset]);

			if (RetVal != nbSUCCESS)
				return RetVal;

			// In case the field does not exist (e.g. it's a conditional field or such),
			// it does not dif for childrens, because CurrentPDMLElement is NULL
			if ((SubfieldElement->FirstChild != nbNETPDL_INVALID_ELEMENT) && (PDMLElement != NULL))
			{
			unsigned int DecodedLen;
			struct _nbNetPDLElementBase *ChildFieldElement;

				ChildFieldElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, SubfieldElement->FirstChild);

				// Save the offset the next sibling field; we need to restore this offset as soon as we decoded child fields
				m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffsetBuffer);

				// Reset packet offsets to the value that was present before decoding this subfield
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, StartingOffset);
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, StartingOffset - m_protoStartingOffset);

				RetVal= DecodeFields(ChildFieldElement, StartingOffset + Size - 1, PDMLElement, LoopCtrl, &DecodedLen);

				if (RetVal != nbSUCCESS)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
						"Decoding of inner elements within subfield '%s' failed.", SubfieldElement->Name);
				}

				// Reset packet offsets to the value that was present after decoding this subfield
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, CurrentOffsetBuffer);
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, CurrentOffsetBuffer - m_protoStartingOffset);
			}
		}

	return nbSUCCESS;
}



/*!
	\brief This method handles all types of fields (i.e. &lt;field&gt;/&lt;subfield&gt; elements)

	It updates also the variables related to the packet offset.

	\param FieldElement: pointer to the current field that has to be decoded.

	\param MaxOffsetToBeDecoded: please refer to the same parameter in function DecodeFields().
	\param PDMLParent: please refer to the same parameter in function DecodeFields().
	\param CurrentPDMLElement: please refer to the same parameter in function DecodeFields().

	\param CurrentFieldStartingOffset: In case of 'delimited' or 'dynamic' fields, the actual starting 
	offset may be different from the original one (simply because we're going to locate the beginning 
	token, which may not be at the beginning of the current packet buffer). Therefore, we have to return 
	back the actual starting offset of this field.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CNetPDLProtoDecoder::DecodeStandardField(struct _nbNetPDLElementFieldBase *FieldElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent, struct _nbPDMLField **CurrentPDMLElement, unsigned int* CurrentFieldStartingOffset)
{
struct _nbPDMLField *PDMLElement;
struct _nbNetPDLElementBase *CurrentSubfieldElement;
unsigned int CurrentPortionStartingOffset;
unsigned int CurrentPortionSize;
unsigned int StartingBytesToDiscard, FieldLen, EndingBytesToDiscard;
struct _nbNetPDLElementSubfield DefaultSubfield= {0};
struct _nbNetPDLElementShowTemplate DefaultShowTemplate= {0};
int LoopCtrl= nbNETPDL_ID_LOOPCTRL_NONE;
unsigned int DecodedLen= 0;
int RetVal;

	// Initialize auxiliary data structures for default subfields
	DefaultSubfield.ShowTemplateInfo= &DefaultShowTemplate;

	RetVal= GetFieldParams(FieldElement, MaxOffsetToBeDecoded, &StartingBytesToDiscard, &FieldLen, &EndingBytesToDiscard);

	if ( RetVal != nbSUCCESS )
		return RetVal;

	if (FieldLen <= 0 && FieldElement->FieldType != nbNETPDL_ID_CFIELD_ASN1)
	{
		// This value must be returned to the caller
		*CurrentPDMLElement= NULL;

		return nbSUCCESS;
	}
	else
	{
	unsigned int StartingOffset;
	unsigned int NewOffset;

		// Get the starting offset of this field
		m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &StartingOffset);

		NewOffset= StartingOffset;

		// If there are initial bytes to discard, let's decode them as "ghost" field
		// This condition can result 'true' only for the following field types:
		//   tokenwrapped, delimited, dynamic, asn1
		if (StartingBytesToDiscard > 0)
		{
#ifdef AllowGhostFieldDecode
			struct _nbNetPDLElementSubfield TrivialField= {0};
			struct _nbNetPDLElementShowTemplate DefaultShowTemplate= {0};
			
			TrivialField.Name= (char *) NETPDL_FIELDBASE_NAME_TRIVIAL;
			TrivialField.IsInNetworkByteOrder= FieldElement->IsInNetworkByteOrder;
			TrivialField.ShowTemplateInfo= &DefaultShowTemplate;
			
			DecodeSubfield(&TrivialField, StartingOffset, StartingBytesToDiscard, PDMLParent, &LoopCtrl, NULL);
#endif

			NewOffset+= StartingBytesToDiscard;

			// Check that we do not exceed packet boundaries
			if (NewOffset > MaxOffsetToBeDecoded)
				NewOffset= MaxOffsetToBeDecoded + 1;

			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, NewOffset);
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, NewOffset - m_protoStartingOffset);
		}

		// Create a Field node and append it to PDML tree
		PDMLElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

		//	Update the common fields of a PDML element (name, longname, ...)
		RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLElement, (struct _nbNetPDLElementFieldBase *) FieldElement, FieldLen, StartingOffset + StartingBytesToDiscard, &m_packet[StartingOffset+StartingBytesToDiscard]);

		if (RetVal != nbSUCCESS)
			return RetVal;

		// Save this value, that must be returned to the caller
		*CurrentPDMLElement= PDMLElement;

		switch (FieldElement->FieldType)
		{
		case nbNETPDL_ID_FIELD_BIT:
			{
			struct _nbNetPDLElementFieldBit *FieldBitElement;

				FieldBitElement= (struct _nbNetPDLElementFieldBit *) FieldElement;
				
				CurrentSubfieldElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, FieldBitElement->FirstChild);
				CurrentPortionStartingOffset= StartingOffset + StartingBytesToDiscard;
                CurrentPortionSize= FieldLen;
				
				if (CurrentSubfieldElement)
					DecodeFields(CurrentSubfieldElement, CurrentPortionStartingOffset + CurrentPortionSize - 1, PDMLElement, &LoopCtrl, &DecodedLen);

				if (!FieldBitElement->IsLastBitField)
					return nbSUCCESS;

				break;
			}
			case nbNETPDL_ID_CFIELD_TLV:
			{
			struct _nbNetPDLElementCfieldTLV *CfieldTLVElement;

				CfieldTLVElement= (struct _nbNetPDLElementCfieldTLV *) FieldElement;

				// Let's decode the 'Type' portion
				CurrentSubfieldElement= CfieldTLVElement->TypeSubfield;
				CurrentPortionStartingOffset= StartingOffset;
				CurrentPortionSize= CfieldTLVElement->TypeSize;

				if (CurrentSubfieldElement)
				{
					DecodeSubfield((struct _nbNetPDLElementSubfield *) CurrentSubfieldElement, CurrentPortionStartingOffset, CurrentPortionSize, PDMLElement, &LoopCtrl, NULL);
				}
				else
				{
					DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_TLV_TYPE;
					DefaultSubfield.IsInNetworkByteOrder= CfieldTLVElement->IsInNetworkByteOrder;
					
					DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLElement, &LoopCtrl, NULL);
				}

				// Let's decode the 'Length' portion
				CurrentSubfieldElement= CfieldTLVElement->LengthSubfield;
				CurrentPortionStartingOffset+= CfieldTLVElement->TypeSize;
				CurrentPortionSize= CfieldTLVElement->LengthSize;

				if (CurrentSubfieldElement)
				{
					DecodeSubfield((struct _nbNetPDLElementSubfield *) CurrentSubfieldElement, CurrentPortionStartingOffset, CurrentPortionSize, PDMLElement, &LoopCtrl, NULL);
				}
				else
				{
					DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_TLV_LENGTH;
					DefaultSubfield.IsInNetworkByteOrder= CfieldTLVElement->IsInNetworkByteOrder;
					
					DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLElement, &LoopCtrl, NULL);
				}

				// Let's decode the 'Value' portion
				CurrentSubfieldElement= CfieldTLVElement->ValueSubfield;
				CurrentPortionStartingOffset+= CfieldTLVElement->LengthSize;
				CurrentPortionSize= FieldLen - CfieldTLVElement->TypeSize - CfieldTLVElement->LengthSize;
				
				if (CurrentSubfieldElement)
				{
					DecodeSubfield((struct _nbNetPDLElementSubfield *) CurrentSubfieldElement, CurrentPortionStartingOffset, CurrentPortionSize, PDMLElement, &LoopCtrl, NULL);
				}
				else
				{
					DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_TLV_VALUE;
					DefaultSubfield.IsInNetworkByteOrder= CfieldTLVElement->IsInNetworkByteOrder;
					
					DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLElement, &LoopCtrl, NULL);
				}

				break;
			}

			case nbNETPDL_ID_CFIELD_HDRLINE:
			{
			struct _nbNetPDLElementCfieldHdrline *CfieldHdrlineElement;
			int RegExpReturnCode;
			int MatchingOffset[MATCHING_OFFSET_COUNT];

				CfieldHdrlineElement= (struct _nbNetPDLElementCfieldHdrline *) FieldElement;

				// Let's decode the 'Header Name' portion
				CurrentSubfieldElement= CfieldHdrlineElement->HeaderNameSubfield;
				CurrentPortionStartingOffset= StartingOffset;

				RegExpReturnCode= pcre_exec(
					(pcre *) CfieldHdrlineElement->SeparatorPCRECompiledRegExp,		// the compiled pattern
					NULL,											// no extra data - we didn't study the pattern
					(char *) &m_packet[StartingOffset],				// the subject string
					FieldLen,										// the length of the subject
					0,									// start at offset 0 in the subject
					0,									// default options
					MatchingOffset,						// output vector for substring information
					MATCHING_OFFSET_COUNT);				// number of elements in the output vector

				if (RegExpReturnCode == -1)	// Match not found
				{
					CurrentPortionSize= FieldLen;

					if (CurrentSubfieldElement != NULL)
					{
						DecodeSubfield((struct _nbNetPDLElementSubfield *) CurrentSubfieldElement, CurrentPortionStartingOffset, CurrentPortionSize, PDMLElement, &LoopCtrl, NULL);
					}
					else
					{
						DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_HDRLINE_HNAME;
						DefaultSubfield.IsInNetworkByteOrder= CfieldHdrlineElement->IsInNetworkByteOrder;
						
						DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLElement, &LoopCtrl, NULL);
					}

					break;
				}

				if (RegExpReturnCode >= 0)	// Match found
				{
					CurrentPortionSize= MatchingOffset[0];

					if (CurrentSubfieldElement != NULL)
					{
						DecodeSubfield((struct _nbNetPDLElementSubfield *) CurrentSubfieldElement, CurrentPortionStartingOffset, CurrentPortionSize, PDMLElement, &LoopCtrl, NULL);
					}
					else
					{
						DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_HDRLINE_HNAME;
						DefaultSubfield.IsInNetworkByteOrder= CfieldHdrlineElement->IsInNetworkByteOrder;
						
						DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLElement, &LoopCtrl, NULL);
					}

					// Let's decode the 'Header Value' portion
					CurrentSubfieldElement= CfieldHdrlineElement->HeaderValueSubfield;
					CurrentPortionStartingOffset+= (unsigned int) MatchingOffset[1];
					CurrentPortionSize= FieldLen - (unsigned int) MatchingOffset[1];

					if (CurrentSubfieldElement != NULL)
					{
								DecodeSubfield((struct _nbNetPDLElementSubfield *) CurrentSubfieldElement, CurrentPortionStartingOffset, CurrentPortionSize, PDMLElement, &LoopCtrl, NULL);
					}
					else
					{
						DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_HDRLINE_HVALUE;
						DefaultSubfield.IsInNetworkByteOrder= CfieldHdrlineElement->IsInNetworkByteOrder;
						
						DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLElement, &LoopCtrl, NULL);
					}
				}

				break;
			}

			case nbNETPDL_ID_CFIELD_DYNAMIC:
			{
			struct _nbNetPDLElementCfieldDynamic *CfieldDynamicElement;
			int RegExpReturnCode;
			int MatchingOffset[MATCHING_OFFSET_COUNT];
			//int PCREFlags= 0;
			unsigned int DefaultSubfieldIndex= 0;

				CfieldDynamicElement= (struct _nbNetPDLElementCfieldDynamic *) FieldElement;

				RegExpReturnCode= pcre_exec(
					(pcre *) CfieldDynamicElement->PatternPCRECompiledRegExp,	// the compiled pattern
					NULL,										// no extra data - we didn't study the pattern
					(char *) &m_packet[StartingOffset],			// the subject string
					MaxOffsetToBeDecoded - StartingOffset + 1,	// the length of the subject
					0,									// start at offset 0 in the subject
					0,									// default options
					MatchingOffset,						// output vector for substring information
					MATCHING_OFFSET_COUNT);				// number of elements in the output vector

				// Let's decode subpattern-related portions
				CurrentSubfieldElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, CfieldDynamicElement->FirstChild);
				while (CurrentSubfieldElement != NULL)
				{
					RegExpReturnCode= pcre_get_stringnumber(
						(pcre *) CfieldDynamicElement->PatternPCRECompiledRegExp,						// the compiled pattern
						((struct _nbNetPDLElementSubfield *) CurrentSubfieldElement)->PortionName);		// the name of subpattern

					if (RegExpReturnCode >= 0)	// Match found
					{
						CurrentPortionStartingOffset= StartingOffset + MatchingOffset[2*RegExpReturnCode];
						CurrentPortionSize= MatchingOffset[2*RegExpReturnCode+1] - MatchingOffset[2*RegExpReturnCode];

						DecodeSubfield((struct _nbNetPDLElementSubfield *) CurrentSubfieldElement, CurrentPortionStartingOffset, CurrentPortionSize, PDMLElement, &LoopCtrl, NULL);
					}

					CurrentSubfieldElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, CurrentSubfieldElement->NextSibling);
				}

				while (DefaultSubfieldIndex < CfieldDynamicElement->NamesListNItems)
				{
					if (CfieldDynamicElement->NamesList[DefaultSubfieldIndex] != NULL)
					{
					struct _nbNetPDLElementSubfield DefaultSubfield= {0};
					//struct _nbNetPDLElementShowTemplate DefaultShowTemplate= {0};

						RegExpReturnCode= pcre_get_stringnumber(
							(pcre *) CfieldDynamicElement->PatternPCRECompiledRegExp,		// the compiled pattern
							CfieldDynamicElement->NamesList[DefaultSubfieldIndex]);			// the name of subpattern

						DefaultSubfield.Name= CfieldDynamicElement->NamesList[DefaultSubfieldIndex];
						DefaultSubfield.IsInNetworkByteOrder= CfieldDynamicElement->IsInNetworkByteOrder;
						
						CurrentPortionStartingOffset= StartingOffset + MatchingOffset[2*RegExpReturnCode];
						CurrentPortionSize= MatchingOffset[2*RegExpReturnCode+1] - MatchingOffset[2*RegExpReturnCode];

						DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLElement, &LoopCtrl, NULL);
					}

					DefaultSubfieldIndex++;
				}

				break;
			}

			case nbNETPDL_ID_CFIELD_XML:
			{
			struct _nbNetPDLElementCfieldXML *CfieldXMLElement;

				CfieldXMLElement= (struct _nbNetPDLElementCfieldXML *) FieldElement;

				DecodeMapTree(CfieldXMLElement, PDMLElement, PDMLElement->Position, PDMLElement->Position + PDMLElement->Size - 1);

				break;
			}

			default:
			{
			struct _nbNetPDLElementFieldBase *FieldBaseElement;

				FieldBaseElement= (struct _nbNetPDLElementFieldBase *) FieldElement;

				CurrentSubfieldElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, FieldBaseElement->FirstChild);
				CurrentPortionStartingOffset= StartingOffset + StartingBytesToDiscard;
				CurrentPortionSize= FieldLen;

				if (CurrentSubfieldElement)
					DecodeFields(CurrentSubfieldElement, CurrentPortionStartingOffset + CurrentPortionSize - 1, PDMLElement, &LoopCtrl, &DecodedLen);

				break;
			}
		}

		// Update offset to carry on decoding
		NewOffset+= FieldLen;

		// Check that we do not exceed packet boundaries
		if (NewOffset > MaxOffsetToBeDecoded)
			NewOffset= MaxOffsetToBeDecoded + 1;

		m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, NewOffset);
		m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, NewOffset - m_protoStartingOffset);

		// If there are final bytes to discard, let's decode them as "ghost" field
		// This condition can result 'true' only for the following field types:
		//   tokenended, tokenwrapped, delimited, line, hdrline, dynamic
		if (EndingBytesToDiscard > 0)
		{
#ifdef AllowGhostFieldDecode
			struct _nbNetPDLElementSubfield TrivialField= {0};
			struct _nbNetPDLElementShowTemplate DefaultShowTemplate= {0};
			
			TrivialField.Name= (char *) NETPDL_FIELDBASE_NAME_TRIVIAL;
			TrivialField.IsInNetworkByteOrder= FieldElement->IsInNetworkByteOrder;
			TrivialField.ShowTemplateInfo= &DefaultShowTemplate;
			
			DecodeSubfield(&TrivialField, NewOffset, EndingBytesToDiscard, PDMLParent, &LoopCtrl, NULL);
#endif

			NewOffset+= EndingBytesToDiscard;

			// Check that we do not exceed packet boundaries
			if (NewOffset > MaxOffsetToBeDecoded)
				NewOffset= MaxOffsetToBeDecoded + 1;

			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, NewOffset);
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, NewOffset - m_protoStartingOffset);
		}

		return nbSUCCESS;
	}
}



//// TEMP FABIO 27/06/2008: Sorry, here they are in a 'working in progress' section
/*!
	\brief This method handles mapping (i.e. &lt;map&gt; element)
*/
void CNetPDLProtoDecoder::DecodeMapTree(struct _nbNetPDLElementCfieldXML *CfieldXMLElement, struct _nbPDMLField *PDMLFieldXML, unsigned int MinOffsetToBeDecoded, unsigned int MaxOffsetToBeDecoded)
{
//struct _nbNetPDLElementCfieldMap *TempMapElement;
//
//	TempMapElement= (struct _nbNetPDLElementCfieldMap *) nbNETPDL_GET_ELEMENT(NetPDLDatabase, CfieldXMLElement->FirstChild);
//
//	while (TempMapElement) 
//	{
//
//		//DecodeMapTree(struct _nbNetPDLElementCfieldXML *CfieldXMLElement, struct _nbPDMLField *PDMLFieldXML)
//
//		TempMapElement= (struct _nbNetPDLElementCfieldMap *) nbNETPDL_GET_ELEMENT(NetPDLDatabase, CfieldXMLElement->NextSibling);
//	}
}



/*!
	\brief This method handles field sets (i.e. &lt;set&gt; element)

	It updates also the variables related to the packet offset.

	\param SetElement: pointer to the current set that has to be decoded.

	\param MaxOffsetToBeDecoded: please refer to the same parameter in function DecodeFields().
	\param PDMLParent: please refer to the same parameter in function DecodeFields().

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CNetPDLProtoDecoder::DecodeSet(struct _nbNetPDLElementSet *SetElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent)
{
unsigned int StartingBytesToDiscard, FieldLen, EndingBytesToDiscard;
struct _nbPDMLField *PDMLStartingElement= NULL;
struct _nbPDMLField *PDMLFieldElement= NULL;
struct _nbPDMLField *PDMLEndingElement= NULL;
unsigned int ExitCondValue;
int RetVal;
int LoopCtrl= nbNETPDL_ID_LOOPCTRL_NONE;
unsigned int DecodedLen;
uint8_t InTheMiddleFailure= false;

	/*
		Evaluation of the 'set'-related exit condition
	*/

	// Let's evaluate the exit condition
	RetVal= m_exprHandler->EvaluateExprNumber(SetElement->ExitWhen->ExitExprTree, m_PDMLProtoItem->FirstField, &ExitCondValue);

	if (RetVal != nbSUCCESS)
	{
		if (SetElement->FirstValidChildIfMissingPacketData == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
				"Decoding of set fails due to evaluation of '%s' exit condition.", SetElement->ExitWhen->ExitExprString);
			return nbFAILURE;
		}

		return DecodeFields(SetElement->FirstValidChildIfMissingPacketData, MaxOffsetToBeDecoded, PDMLParent, &LoopCtrl, &DecodedLen);
	}

	// Declare other variables aimed at decoding process
	unsigned int CurrentOffset;
	struct _nbNetPDLElementFieldBase SpeculativeField= {0};
	struct _nbNetPDLElementSubfield DefaultSubfield= {0};
	struct _nbNetPDLElementShowTemplate DefaultShowTemplate= {0};
	unsigned int NewOffset;
	struct _nbNetPDLElementFieldmatch *TempFieldmatchElement;
	unsigned int MatchExprValue;

	// Initialize data structures for speculative field decoding
	SpeculativeField.IsInNetworkByteOrder= SetElement->FieldToDecode->IsInNetworkByteOrder;
	SpeculativeField.ShowTemplateInfo= &DefaultShowTemplate;
	DefaultSubfield.IsInNetworkByteOrder= SetElement->FieldToDecode->IsInNetworkByteOrder;
	DefaultSubfield.ShowTemplateInfo= &DefaultShowTemplate;

	// Get the starting offset of the set
	m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffset);

	// We have to repeat the decoding if the expression is true
	// unless the m_packet has been completely decoded
	while ( (!ExitCondValue) && (CurrentOffset <= MaxOffsetToBeDecoded) )
	{
		/*
			Speculative decoding of the field specified by set based on its 
			own type
		*/

		RetVal= GetFieldParams(SetElement->FieldToDecode, MaxOffsetToBeDecoded, &StartingBytesToDiscard, &FieldLen, &EndingBytesToDiscard);

		if (RetVal != nbSUCCESS)
		{
			if (SetElement->FirstValidChildIfMissingPacketData == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
					"Decoding of set fails due to speculative decoding of the set-related item.");
				return nbFAILURE;
			}

			return DecodeFields(SetElement->FirstValidChildIfMissingPacketData, MaxOffsetToBeDecoded, PDMLParent, &LoopCtrl, &DecodedLen);
		}

		// If the current set-related field is not recognized, the set-related item is probably fragmented
		if (FieldLen <= 0 && SetElement->FieldToDecode->FieldType != nbNETPDL_ID_CFIELD_ASN1)
			return nbSUCCESS;

		// Let's initialize the offset value for current item
		NewOffset= CurrentOffset;

		// If there are initial bytes to discard, let's decode them as "ghost" field
		if (StartingBytesToDiscard > 0)
		{
#ifdef AllowGhostFieldDecode
		struct _nbNetPDLElementFieldBase TrivialField= {0};

			TrivialField.Name= (char *) NETPDL_FIELDBASE_NAME_TRIVIAL;
			TrivialField.IsInNetworkByteOrder= SetElement->FieldToDecode->IsInNetworkByteOrder;
			TrivialField.ShowTemplateInfo= &DefaultShowTemplate;
			
			// Create a Field node and append it to PDML tree
			PDMLStartingElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

			//	Update the common fields of a PDML element (name, longname, ...)
			RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLStartingElement, &TrivialField, StartingBytesToDiscard, NewOffset, &m_packet[NewOffset]);
			// RetVal is not checked here
#endif

			NewOffset+= StartingBytesToDiscard;

			// Check that we do not exceed packet boundaries
			if (NewOffset > MaxOffsetToBeDecoded)
				NewOffset= MaxOffsetToBeDecoded + 1;

			// Update the current offset to process subsequent item of the set
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, NewOffset);
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, NewOffset - m_protoStartingOffset);
		}

		// Let's perform type-based speculative decoding 
		switch (SetElement->FieldToDecode->FieldType)
		{
			case nbNETPDL_ID_CFIELD_TLV:
			{ // Let's perform speculative decoding of current 'tlv' complex field
			struct _nbNetPDLElementCfieldTLV *CfieldTLVElement;
			unsigned int CurrentPortionStartingOffset= 0;
			unsigned int CurrentPortionSize= 0;

				CfieldTLVElement= (struct _nbNetPDLElementCfieldTLV *) SetElement->FieldToDecode;

				SpeculativeField.Name= (char *) NETPDL_CFIELD_NAME_TLV;

				// Create a Field node and append it to PDML tree
				PDMLFieldElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

				//	Update the common fields of a PDML element (name, longname, ...)
				RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, &SpeculativeField, FieldLen, NewOffset, &m_packet[NewOffset]);

				if (RetVal != nbSUCCESS)
					goto DecodeSet_SpeculativeDecodingFailure;

				// Let's perform speculative decoding of the 'Type' portion
				CurrentPortionStartingOffset= CurrentOffset;
				CurrentPortionSize= CfieldTLVElement->TypeSize;

				DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_TLV_TYPE;
				
				RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

				if (RetVal != nbSUCCESS)
					goto DecodeSet_SpeculativeDecodingFailure;

				// Let's perform speculative decoding of the 'Length' portion
				CurrentPortionStartingOffset+= CfieldTLVElement->TypeSize;
				CurrentPortionSize= CfieldTLVElement->LengthSize;

				DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_TLV_LENGTH;
				
				RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

				if (RetVal != nbSUCCESS)
					goto DecodeSet_SpeculativeDecodingFailure;

				// Let's perform speculative decoding of the 'Value' portion
				CurrentPortionStartingOffset+= CfieldTLVElement->LengthSize;
				CurrentPortionSize= FieldLen - CfieldTLVElement->TypeSize - CfieldTLVElement->LengthSize;

				DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_TLV_VALUE;
				
				RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

				if (RetVal != nbSUCCESS)
					goto DecodeSet_SpeculativeDecodingFailure;

				break;
			}

			case nbNETPDL_ID_CFIELD_DELIMITED:
			{ // Let's perform speculative decoding of current 'delimited' complex field

				SpeculativeField.Name= (char *) NETPDL_CFIELD_NAME_DELIMITED;

				// Create a Field node and append it to PDML tree
				PDMLFieldElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

				//	Update the common fields of a PDML element (name, longname, ...)
				RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, &SpeculativeField, FieldLen, NewOffset, &m_packet[NewOffset]);

				if (RetVal != nbSUCCESS)
					goto DecodeSet_SpeculativeDecodingFailure;

				break;
			}

			case nbNETPDL_ID_CFIELD_LINE:
			{ // Let's perform speculative decoding of current 'line' complex field

				SpeculativeField.Name= (char *) NETPDL_CFIELD_NAME_LINE;

				// Create a Field node and append it to PDML tree
				PDMLFieldElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

				//	Update the common fields of a PDML element (name, longname, ...)
				RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, &SpeculativeField, FieldLen, NewOffset, &m_packet[NewOffset]);

				if (RetVal != nbSUCCESS)
					goto DecodeSet_SpeculativeDecodingFailure;

				break;
			}

			case nbNETPDL_ID_CFIELD_HDRLINE:
			{ // Let's perform speculative decoding of current 'hdrline' complex field
			struct _nbNetPDLElementCfieldHdrline *CfieldHdrlineElement;
			unsigned int CurrentPortionStartingOffset= 0;
			unsigned int CurrentPortionSize= 0;
			int RegExpReturnCode;
			int MatchingOffset[MATCHING_OFFSET_COUNT];
			//int PCREFlags= 0;

				CfieldHdrlineElement= (struct _nbNetPDLElementCfieldHdrline *) SetElement->FieldToDecode;

				SpeculativeField.Name= (char *) NETPDL_CFIELD_NAME_HDRLINE;

				// Create a Field node and append it to PDML tree
				PDMLFieldElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

				//	Update the common fields of a PDML element (name, longname, ...)
				RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, &SpeculativeField, FieldLen, NewOffset, &m_packet[NewOffset]);

				if (RetVal != nbSUCCESS)
					goto DecodeSet_SpeculativeDecodingFailure;

				// Let's perform speculative decoding of the 'Header Name' and 'Header Value' portion
				CurrentPortionStartingOffset= NewOffset;

				RegExpReturnCode= pcre_exec(
					(pcre *) CfieldHdrlineElement->SeparatorPCRECompiledRegExp,		// the compiled pattern
					NULL,											// no extra data - we didn't study the pattern
					(char *) &m_packet[NewOffset],					// the subject string
					FieldLen,										// the length of the subject
					0,									// start at offset 0 in the subject
					0,									// default options
					MatchingOffset,						// output vector for substring information
					MATCHING_OFFSET_COUNT);				// number of elements in the output vector

				if (RegExpReturnCode == -1)	// Match not found, 'Header Value' is missing
				{ 
					// Let's perform speculative decoding of the 'Header Name' portion
					CurrentPortionSize= FieldLen;

					DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_HDRLINE_HNAME;
					
					RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

					if (RetVal != nbSUCCESS)
						goto DecodeSet_SpeculativeDecodingFailure;

					break;
				}

				if (RegExpReturnCode >= 0)	// Match found, there are both 'Header Name' and 'Header Value'
				{
					// Let's perform speculative decoding of the 'Header Name' portion
					CurrentPortionSize= MatchingOffset[0];

					DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_HDRLINE_HNAME;
					
					RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

					if (RetVal != nbSUCCESS)
						goto DecodeSet_SpeculativeDecodingFailure;

					// Let's perform speculative decoding of the 'Header Value' portion
					CurrentPortionStartingOffset+= (unsigned int) MatchingOffset[1];
					CurrentPortionSize= FieldLen - (unsigned int) MatchingOffset[1];

					DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_HDRLINE_HVALUE;
					
					RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

					if (RetVal != nbSUCCESS)
						goto DecodeSet_SpeculativeDecodingFailure;
				}

				break;
			}

			case nbNETPDL_ID_CFIELD_DYNAMIC:
			{ // Let's perform speculative decoding of current 'dynamic' complex field
			struct _nbNetPDLElementCfieldDynamic *CfieldDynamicElement;
			unsigned int CurrentPortionStartingOffset= 0;
			unsigned int CurrentPortionSize= 0;
			int RegExpReturnCode;
			int MatchingOffset[MATCHING_OFFSET_COUNT];
			//int PCREFlags= 0;
			unsigned int DefaultSubfieldIndex= 0;

				CfieldDynamicElement= (struct _nbNetPDLElementCfieldDynamic *) SetElement->FieldToDecode;

				SpeculativeField.Name= (char *) NETPDL_CFIELD_NAME_DYNAMIC;

				// Create a Field node and append it to PDML tree
				PDMLFieldElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

				// Update the common fields of a PDML element (name, longname, ...)
				RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, &SpeculativeField, FieldLen, NewOffset, &m_packet[NewOffset]);

				if (RetVal != nbSUCCESS)
					goto DecodeSet_SpeculativeDecodingFailure;

				// Let's retrieve the matching offset vector to decode subfield
				RegExpReturnCode= pcre_exec(
					(pcre *) CfieldDynamicElement->PatternPCRECompiledRegExp,	// the compiled pattern
					NULL,											// no extra data - we didn't study the pattern
					(char *) &m_packet[PDMLFieldElement->Position],	// the subject string
					PDMLFieldElement->Size,				// the length of the subject
					0,									// start at offset 0 in the subject
					0,									// default options
					MatchingOffset,						// output vector for substring information
					MATCHING_OFFSET_COUNT);				// number of elements in the output vector

				// Let's perform speculative decoding of the 'dynamic'-related portions
				while (DefaultSubfieldIndex < CfieldDynamicElement->NamesListNItems)
				{
					RegExpReturnCode= pcre_get_stringnumber(
						(pcre *) CfieldDynamicElement->PatternPCRECompiledRegExp,		// the compiled pattern
						CfieldDynamicElement->NamesList[DefaultSubfieldIndex]);			// the name of subpattern

					DefaultSubfield.Name= CfieldDynamicElement->NamesList[DefaultSubfieldIndex];
					
					CurrentPortionStartingOffset= NewOffset + MatchingOffset[2*RegExpReturnCode];
					CurrentPortionSize= MatchingOffset[2*RegExpReturnCode+1] - MatchingOffset[2*RegExpReturnCode];

					RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

					if (RetVal != nbSUCCESS)
						goto DecodeSet_SpeculativeDecodingFailure;

					DefaultSubfieldIndex++;
				}

				break;
			}

			case nbNETPDL_ID_CFIELD_ASN1:
			{ // Let's perform speculative decoding of current 'asn1' complex field
			unsigned int CurrentPortionStartingOffset= 0;
			unsigned int CurrentPortionSize= 0;

				SpeculativeField.Name= (char *) NETPDL_CFIELD_NAME_ASN1;

				// Create a Field node and append it to PDML tree
				PDMLFieldElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

				//	Update the common fields of a PDML element (name, longname, ...)
				RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, &SpeculativeField, FieldLen, NewOffset, &m_packet[NewOffset]);

				if (RetVal != nbSUCCESS)
					goto DecodeSet_SpeculativeDecodingFailure;

				// This is a trick to make available encoding-related meta-data contents
				CurrentPortionStartingOffset= CurrentOffset;
				CurrentPortionSize= StartingBytesToDiscard;

				DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_ASN1_ENCRULEDATA;

				RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

				if (RetVal != nbSUCCESS)
					goto DecodeSet_SpeculativeDecodingFailure;

				break;
			}

		default:
			{
				SpeculativeField.Name= (char *) NETPDL_FIELD_NAME_ITEM;

				// Create a Field node and append it to PDML tree
				PDMLFieldElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

				//	Update the common fields of a PDML element (name, longname, ...)
				RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, &SpeculativeField, FieldLen, NewOffset, &m_packet[NewOffset]);

				if (RetVal != nbSUCCESS)
					goto DecodeSet_SpeculativeDecodingFailure;

				break;
			}
		}

		/*
			Evaluation of 'set'-related match conditions
		*/
		TempFieldmatchElement= SetElement->FirstMatchElement;

		while (TempFieldmatchElement->FieldType != nbNETPDL_ID_DEFAULT_MATCH)
		{
			RetVal= m_exprHandler->EvaluateExprNumber(TempFieldmatchElement->MatchExprTree, PDMLFieldElement, &MatchExprValue);

			if (RetVal != nbSUCCESS)
			{
				RollbackSpeculativeDecoding(PDMLStartingElement, PDMLFieldElement, PDMLEndingElement);

				// Restore the current offset at last error-free decoded item of the set
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, CurrentOffset);
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, CurrentOffset - m_protoStartingOffset);

				if (SetElement->FirstValidChildIfMissingPacketData == NULL)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
						"Decoding of set fails due to evaluation of matching expression '%s'.", TempFieldmatchElement->MatchExprString);
					return nbFAILURE;
				}

				return DecodeFields(SetElement->FirstValidChildIfMissingPacketData, MaxOffsetToBeDecoded, PDMLParent, &LoopCtrl, &DecodedLen);
			}

			if ( MatchExprValue )
			{
			struct _nbNetPDLElementFieldBase *NetPDLElementField= (struct _nbNetPDLElementFieldBase *) TempFieldmatchElement;

				//	Update the common fields of a PDML element (name, longname, ...)
				RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, NetPDLElementField, PDMLFieldElement->Size, PDMLFieldElement->Position, &m_packet[PDMLFieldElement->Position]);

				if (RetVal != nbSUCCESS)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
						"Decoding of set fails due to decoding completion of the set-related item.");

					// Restore the current offset at last error-free decoded item of the set
					RollbackSpeculativeDecoding(PDMLStartingElement, PDMLFieldElement, PDMLEndingElement);

					// Restore the current offset at last error-free decoded item of the set
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, CurrentOffset);
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, CurrentOffset - m_protoStartingOffset);

					return nbFAILURE;
				}

				break;
			}

			TempFieldmatchElement= TempFieldmatchElement->NextFieldmatch;
		}

		// If nothing fields-related expression match, let's update current 'set'-related field relying on 'default-item'
		if ( TempFieldmatchElement->FieldType == nbNETPDL_ID_DEFAULT_MATCH )
		{	
		struct _nbNetPDLElementFieldBase *NetPDLElementField= (struct _nbNetPDLElementFieldBase *) TempFieldmatchElement;

			//	Update the common fields of a PDML element (name, longname, ...)
			RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, NetPDLElementField, PDMLFieldElement->Size, PDMLFieldElement->Position, &m_packet[PDMLFieldElement->Position]);

			if (RetVal != nbSUCCESS)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
					"Decoding of set fails due to decoding completion of the set-related item.");

				// Let's rollback to previous consistent state
				RollbackSpeculativeDecoding(PDMLStartingElement, PDMLFieldElement, PDMLEndingElement);

				// Restore the current offset at last error-free decoded item of the set
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, CurrentOffset);
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, CurrentOffset - m_protoStartingOffset);

				return nbFAILURE;
			}
		}

		/*
			Special case for ASN.1 fields: let's deallocate the subfield 
			provided for matching purposes
		*/
		if (SetElement->FieldToDecode->FieldType == nbNETPDL_ID_CFIELD_ASN1)
		{
			m_PDMLMaker->PDMLElementDiscard(PDMLFieldElement->FirstChild);

			PDMLFieldElement->FirstChild= NULL;
		}

		/* 
			Let's update subfields accordingly to selected item: for those field 
			types that have subfields is necessary to check the presence of their 
			definition and to perform relevant specified decoding
		*/
		struct _nbNetPDLElementBase *TempNetPDLChildElement;

		TempNetPDLChildElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, TempFieldmatchElement->FirstChild);

		if (TempNetPDLChildElement)
		{
		struct _nbPDMLField *PDMLTempSubfieldElement;

			switch (SetElement->FieldToDecode->FieldType)
			{
			case nbNETPDL_ID_CFIELD_TLV:
				{
					while (TempNetPDLChildElement)
					{
						switch(TempNetPDLChildElement->Type)
						{
						case nbNETPDL_IDEL_SUBFIELD:
							{
							struct _nbNetPDLElementSubfield *NetPDLElementSubfield;

								NetPDLElementSubfield= (struct _nbNetPDLElementSubfield *) TempNetPDLChildElement;

								if ( strncmp(NetPDLElementSubfield->PortionName, NETPDL_SUBFIELD_NAME_TLV_TYPE, strlen(NETPDL_SUBFIELD_NAME_TLV_TYPE)) == 0 )
									{ PDMLTempSubfieldElement= PDMLFieldElement->FirstChild; goto DecodeSet_EndTLVPortionCheck; }
								if ( strncmp(NetPDLElementSubfield->PortionName, NETPDL_SUBFIELD_NAME_TLV_LENGTH, strlen(NETPDL_SUBFIELD_NAME_TLV_LENGTH)) == 0 )
									{ PDMLTempSubfieldElement= PDMLFieldElement->FirstChild->NextField; goto DecodeSet_EndTLVPortionCheck; }
								if ( strncmp(NetPDLElementSubfield->PortionName, NETPDL_SUBFIELD_NAME_TLV_VALUE, strlen(NETPDL_SUBFIELD_NAME_TLV_VALUE)) == 0 )
									{ PDMLTempSubfieldElement= PDMLFieldElement->FirstChild->NextField->NextField; goto DecodeSet_EndTLVPortionCheck; }

								InTheMiddleFailure= true;
								break;

DecodeSet_EndTLVPortionCheck:

								if (NetPDLElementSubfield->ComplexSubfieldInfo)
								{
									//	Update the common fields of a PDML element (name, longname, ...)
									RetVal= m_PDMLMaker->PDMLElementUpdate(
										PDMLTempSubfieldElement, 
										(struct _nbNetPDLElementFieldBase *) TempNetPDLChildElement, 
										PDMLTempSubfieldElement->Size, 
										PDMLTempSubfieldElement->Position, 
										&m_packet[PDMLFieldElement->Position]);
								}
							
								RetVal= DecodeSubfield(
									NetPDLElementSubfield, 
									PDMLTempSubfieldElement->Position, 
									PDMLTempSubfieldElement->Size, 
									PDMLFieldElement, 
									&LoopCtrl, 
									NetPDLElementSubfield->ComplexSubfieldInfo? NULL : PDMLTempSubfieldElement);

								InTheMiddleFailure+= RetVal != nbSUCCESS;

								break;
							}
						}

						TempNetPDLChildElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, TempNetPDLChildElement->NextSibling);
					}
				}; break;
			case nbNETPDL_ID_CFIELD_HDRLINE:
				{
					while (TempNetPDLChildElement)
					{
						switch(TempNetPDLChildElement->Type)
						{
						case nbNETPDL_IDEL_SUBFIELD:
							{
							struct _nbNetPDLElementSubfield *NetPDLElementSubfield;

								NetPDLElementSubfield= (struct _nbNetPDLElementSubfield *) TempNetPDLChildElement;

								if ( strncmp(NetPDLElementSubfield->PortionName, NETPDL_SUBFIELD_NAME_HDRLINE_HNAME, strlen(NETPDL_SUBFIELD_NAME_HDRLINE_HNAME)) == 0 )
									{ PDMLTempSubfieldElement= PDMLFieldElement->FirstChild; goto DecodeSet_EndHdrlinePortionCheck; }
								if ( strncmp(NetPDLElementSubfield->PortionName, NETPDL_SUBFIELD_NAME_HDRLINE_HVALUE, strlen(NETPDL_SUBFIELD_NAME_HDRLINE_HVALUE)) == 0 )
									{ if ((PDMLTempSubfieldElement= PDMLFieldElement->FirstChild->NextField) != NULL) goto DecodeSet_EndHdrlinePortionCheck; }

								InTheMiddleFailure= true;
								break;

DecodeSet_EndHdrlinePortionCheck:

								if (NetPDLElementSubfield->ComplexSubfieldInfo)
								{
									//	Update the common fields of a PDML element (name, longname, ...)
									RetVal= m_PDMLMaker->PDMLElementUpdate(
										PDMLTempSubfieldElement, 
										(struct _nbNetPDLElementFieldBase *) TempNetPDLChildElement, 
										PDMLTempSubfieldElement->Size, 
										PDMLTempSubfieldElement->Position, 
										&m_packet[PDMLFieldElement->Position]);
								}
							
								RetVal= DecodeSubfield(
									NetPDLElementSubfield, 
									PDMLTempSubfieldElement->Position, 
									PDMLTempSubfieldElement->Size, 
									PDMLFieldElement, 
									&LoopCtrl, 
									NetPDLElementSubfield->ComplexSubfieldInfo? NULL : PDMLTempSubfieldElement);

								InTheMiddleFailure+= RetVal != nbSUCCESS;

								break;
							}
						}

						TempNetPDLChildElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, TempNetPDLChildElement->NextSibling);
					}
				}; break;
			case nbNETPDL_ID_CFIELD_DYNAMIC:
				{
				struct _nbNetPDLElementSubfield *NetPDLElementSubfield;

					while (TempNetPDLChildElement)
					{
						NetPDLElementSubfield= (struct _nbNetPDLElementSubfield *) TempNetPDLChildElement;
						PDMLTempSubfieldElement= PDMLFieldElement->FirstChild;

						while (PDMLTempSubfieldElement)
						{
							if ( strncmp(NetPDLElementSubfield->PortionName, PDMLTempSubfieldElement->Name, strlen(PDMLTempSubfieldElement->Name)) == 0 )
							{
								switch(TempNetPDLChildElement->Type)
								{
								case nbNETPDL_IDEL_SUBFIELD:
									{
										if (NetPDLElementSubfield->ComplexSubfieldInfo)
										{
											//	Update the common fields of a PDML element (name, longname, ...)
											RetVal= m_PDMLMaker->PDMLElementUpdate(
												PDMLTempSubfieldElement, 
												(struct _nbNetPDLElementFieldBase *) TempNetPDLChildElement, 
												PDMLTempSubfieldElement->Size, 
												PDMLTempSubfieldElement->Position, 
												&m_packet[PDMLFieldElement->Position]);
										}

										RetVal= DecodeSubfield(
											NetPDLElementSubfield, 
											PDMLTempSubfieldElement->Position, 
											PDMLTempSubfieldElement->Size, 
											PDMLFieldElement, 
											&LoopCtrl, 
											NetPDLElementSubfield->ComplexSubfieldInfo? NULL : PDMLTempSubfieldElement);

										InTheMiddleFailure+= RetVal != nbSUCCESS;
									}
								}
							
								break;
							}

							PDMLTempSubfieldElement= PDMLTempSubfieldElement->NextField;
						}

						TempNetPDLChildElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, TempNetPDLChildElement->NextSibling);
					}
				}; break;
			default:
				{
				unsigned int CurrentOffsetBuffer;

					// Save the offset of the next sibling field; we need to restore this offset as soon as we decoded child fields
					m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffsetBuffer);

					// Reset packet offsets to the value that was present before decoding this subfield
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, PDMLFieldElement->Position);
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, PDMLFieldElement->Position - m_protoStartingOffset);

					RetVal= DecodeFields(TempNetPDLChildElement, PDMLFieldElement->Position + PDMLFieldElement->Size - 1, PDMLFieldElement, &LoopCtrl, &DecodedLen);

					InTheMiddleFailure+= RetVal != nbSUCCESS;

					// Reset packet offsets to the value that was present after decoding this subfield
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, CurrentOffsetBuffer);
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, CurrentOffsetBuffer - m_protoStartingOffset);
				}
			}
		}

		/*
			Update offset to complete successful speculative decoding
		*/
		NewOffset+= FieldLen;

		// Check that we do not exceed packet boundaries
		if (NewOffset > MaxOffsetToBeDecoded)
			NewOffset= MaxOffsetToBeDecoded + 1;

		// Update the current offset to process subsequent item of the set
		m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, NewOffset);
		m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, NewOffset - m_protoStartingOffset);

		// If there are final bytes to discard, let's decode them as "ghost" field
		if (EndingBytesToDiscard > 0)
		{
#ifdef AllowGhostFieldDecode
		struct _nbNetPDLElementFieldBase TrivialField= {0};

			TrivialField.Name= (char *) NETPDL_FIELDBASE_NAME_TRIVIAL;
			TrivialField.IsInNetworkByteOrder= SetElement->FieldToDecode->IsInNetworkByteOrder;
			TrivialField.ShowTemplateInfo= &DefaultShowTemplate;
			
			// Create a Field node and append it to PDML tree
			PDMLEndingElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

			//	Update the common fields of a PDML element (name, longname, ...)
			RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLEndingElement, &TrivialField, EndingBytesToDiscard, NewOffset, &m_packet[NewOffset]);
			// RetVal is not checked here
#endif

			NewOffset+= EndingBytesToDiscard;

			// Check that we do not exceed packet boundaries
			if (NewOffset > MaxOffsetToBeDecoded)
				NewOffset= MaxOffsetToBeDecoded + 1;

			// Update the current offset to process subsequent item of the set
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, NewOffset);
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, NewOffset - m_protoStartingOffset);
		}

		// Update the current offset to process subsequent item of the set
		m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, NewOffset);
		m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, NewOffset - m_protoStartingOffset);

		// Set current offset for next iteration
		m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffset);

		// Re-evaluate the exit condition for next iteration
		RetVal= m_exprHandler->EvaluateExprNumber(SetElement->ExitWhen->ExitExprTree, m_PDMLProtoItem->FirstField, &ExitCondValue);

		if (RetVal != nbSUCCESS)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
				"Decoding of set fails due to evaluation of '%s' exit condition.", SetElement->ExitWhen->ExitExprString);

			if (SetElement->FirstValidChildIfMissingPacketData == NULL)
				return nbWARNING;

			return DecodeFields(SetElement->FirstValidChildIfMissingPacketData, MaxOffsetToBeDecoded, PDMLParent, &LoopCtrl, &DecodedLen);
		}
	}

	if (InTheMiddleFailure)
	{
		// If a subfield-related decoding fails, let's print a warning
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
			"Decoding of elements within set-related item '%s' was not completed.", PDMLFieldElement->Name);

		return nbWARNING;
	}

	return nbSUCCESS;


DecodeSet_SpeculativeDecodingFailure:

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
		"Decoding of set fails due to speculative decoding of the set-related item.");

	RetVal= nbFAILURE;

	/*
		If processing fails, there is a need to rollback to a correct status 
		of PDML tree
	*/
	RollbackSpeculativeDecoding(PDMLStartingElement, PDMLFieldElement, PDMLEndingElement);

	// Restore the current offset at last error-free decoded item of the set
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, CurrentOffset);
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, CurrentOffset - m_protoStartingOffset);

	return RetVal;
}



/*!
	\brief This method handles field alternatives (i.e. &lt;choice&gt; element)

	It updates also the variables related to the packet offset.

	\param FieldElement: pointer to the current alternative that has to be decoded.

	\param MaxOffsetToBeDecoded: please refer to the same parameter in function DecodeFields().
	\param PDMLParent: please refer to the same parameter in function DecodeFields().

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CNetPDLProtoDecoder::DecodeFieldChoice(struct _nbNetPDLElementChoice *ChoiceElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent)
{
unsigned int StartingBytesToDiscard, FieldLen, EndingBytesToDiscard;
struct _nbPDMLField *PDMLStartingElement= NULL;
struct _nbPDMLField *PDMLFieldElement= NULL;
struct _nbPDMLField *PDMLEndingElement= NULL;
int RetVal;
unsigned int CurrentOffset;
struct _nbNetPDLElementFieldBase SpeculativeField= {0};
struct _nbNetPDLElementSubfield DefaultSubfield= {0};
struct _nbNetPDLElementShowTemplate DefaultShowTemplate= {0};
int LoopCtrl= nbNETPDL_ID_LOOPCTRL_NONE;
unsigned int NewOffset;
struct _nbNetPDLElementFieldmatch *TempFieldMatchElement;
unsigned int MatchExprValue;
unsigned int DecodedLen;
uint8_t InTheMiddleFailure= false;

	// Initialize data structures for speculative field decoding
	SpeculativeField.IsInNetworkByteOrder= ChoiceElement->FieldToDecode->IsInNetworkByteOrder;
	SpeculativeField.ShowTemplateInfo= &DefaultShowTemplate;
	DefaultSubfield.IsInNetworkByteOrder= ChoiceElement->FieldToDecode->IsInNetworkByteOrder;
	DefaultSubfield.ShowTemplateInfo= &DefaultShowTemplate;

	// Get the starting offset of the choice
	m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffset);

	/*
		Speculative decoding of the field specified by set based on its 
		own type
	*/

	RetVal= GetFieldParams(ChoiceElement->FieldToDecode, MaxOffsetToBeDecoded, &StartingBytesToDiscard, &FieldLen, &EndingBytesToDiscard);

	if (RetVal != nbSUCCESS)
	{
		if (ChoiceElement->FirstValidChildIfMissingPacketData == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
				"Decoding of choice fails due to speculative decoding of the set-related item.");
			return nbFAILURE;
		}

		return DecodeFields(ChoiceElement->FirstValidChildIfMissingPacketData, MaxOffsetToBeDecoded, PDMLParent, &LoopCtrl, &DecodedLen);
	}

	// If the current set-related field is not recognized, the set is probably fragmented
	if (FieldLen <= 0 && ChoiceElement->FieldToDecode->FieldType != nbNETPDL_ID_CFIELD_ASN1)
		return nbSUCCESS;

	// Let's initialize the offset value for 'choice'-related speculative field
	NewOffset= CurrentOffset;

	// If there are initial bytes to discard, let's decode them as "ghost" field
	if (StartingBytesToDiscard > 0)
	{
#ifdef AllowGhostFieldDecode
	struct _nbNetPDLElementFieldBase TrivialField= {0};

		TrivialField.Name= (char *) NETPDL_FIELDBASE_NAME_TRIVIAL;
		TrivialField.IsInNetworkByteOrder= ChoiceElement->FieldToDecode->IsInNetworkByteOrder;
		TrivialField.ShowTemplateInfo= &DefaultShowTemplate;
		
		// Create a Field node and append it to PDML tree
		PDMLStartingElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

		//	Update the common fields of a PDML element (name, longname, ...)
		RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLStartingElement, &TrivialField, StartingBytesToDiscard, NewOffset, &m_packet[NewOffset]);

		if (RetVal != nbSUCCESS)
		{
			RetVal= nbFAILURE;
			goto DecodeChoice_SpeculativeDecodingFailure;
		}
#endif

		NewOffset+= StartingBytesToDiscard;

		// Check that we do not exceed packet boundaries
		if (NewOffset > MaxOffsetToBeDecoded)
			NewOffset= MaxOffsetToBeDecoded + 1;

		// Update the current offset to process subsequent item of the set
		m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, NewOffset);
		m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, NewOffset - m_protoStartingOffset);
	}

	// Let's perform type-based speculative decoding 
	switch (ChoiceElement->FieldToDecode->FieldType)
	{
		case nbNETPDL_ID_CFIELD_TLV:
		{ // Let's perform speculative decoding of current 'tlv' complex field
		struct _nbNetPDLElementCfieldTLV *CfieldTLVElement;
		unsigned int CurrentPortionStartingOffset= 0;
		unsigned int CurrentPortionSize= 0;

			CfieldTLVElement= (struct _nbNetPDLElementCfieldTLV *) ChoiceElement->FieldToDecode;

			SpeculativeField.Name= (char *) NETPDL_CFIELD_NAME_TLV;

			// Create a Field node and append it to PDML tree
			PDMLFieldElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

			//	Update the common fields of a PDML element (name, longname, ...)
			RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, &SpeculativeField, FieldLen, NewOffset, &m_packet[NewOffset]);

			if (RetVal != nbSUCCESS)
				goto DecodeChoice_SpeculativeDecodingFailure;

			// Let's perform speculative decoding of the 'Type' portion
			CurrentPortionStartingOffset= CurrentOffset;
			CurrentPortionSize= CfieldTLVElement->TypeSize;

			DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_TLV_TYPE;
			
			RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

			if (RetVal != nbSUCCESS)
				goto DecodeChoice_SpeculativeDecodingFailure;

			// Let's perform speculative decoding of the 'Length' portion
			CurrentPortionStartingOffset+= CfieldTLVElement->TypeSize;
			CurrentPortionSize= CfieldTLVElement->LengthSize;

			DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_TLV_LENGTH;
			
			RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

			if (RetVal != nbSUCCESS)
				goto DecodeChoice_SpeculativeDecodingFailure;

			// Let's perform speculative decoding of the 'Value' portion
			CurrentPortionStartingOffset+= CfieldTLVElement->LengthSize;
			CurrentPortionSize= FieldLen - CfieldTLVElement->TypeSize - CfieldTLVElement->LengthSize;

			DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_TLV_VALUE;
			
			RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

			if (RetVal != nbSUCCESS)
				goto DecodeChoice_SpeculativeDecodingFailure;
		}; break;

		case nbNETPDL_ID_CFIELD_DELIMITED:
		{ // Let's perform speculative decoding of current 'delimited' complex field

			SpeculativeField.Name= (char *) NETPDL_CFIELD_NAME_DELIMITED;

			// Create a Field node and append it to PDML tree
			PDMLFieldElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

			//	Update the common fields of a PDML element (name, longname, ...)
			RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, &SpeculativeField, FieldLen, NewOffset, &m_packet[NewOffset]);

			if (RetVal != nbSUCCESS)
				goto DecodeChoice_SpeculativeDecodingFailure;

		}; break;

		case nbNETPDL_ID_CFIELD_LINE:
		{ // Let's perform speculative decoding of current 'line' complex field

			SpeculativeField.Name= (char *) NETPDL_CFIELD_NAME_LINE;

			// Create a Field node and append it to PDML tree
			PDMLFieldElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

			//	Update the common fields of a PDML element (name, longname, ...)
			RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, &SpeculativeField, FieldLen, NewOffset, &m_packet[NewOffset]);

			if (RetVal != nbSUCCESS)
				goto DecodeChoice_SpeculativeDecodingFailure;
		}; break;

		case nbNETPDL_ID_CFIELD_HDRLINE:
		{ // Let's perform speculative decoding of current 'hdrline' complex field
		struct _nbNetPDLElementCfieldHdrline *CfieldHdrlineElement;
		unsigned int CurrentPortionStartingOffset= 0;
		unsigned int CurrentPortionSize= 0;
		int RegExpReturnCode;
		int MatchingOffset[MATCHING_OFFSET_COUNT];
		//int PCREFlags= 0;

			CfieldHdrlineElement= (struct _nbNetPDLElementCfieldHdrline *) ChoiceElement->FieldToDecode;

			SpeculativeField.Name= (char *) NETPDL_CFIELD_NAME_HDRLINE;

			// Create a Field node and append it to PDML tree
			PDMLFieldElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

			//	Update the common fields of a PDML element (name, longname, ...)
			RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, &SpeculativeField, FieldLen, NewOffset, &m_packet[NewOffset]);

			if (RetVal != nbSUCCESS)
				goto DecodeChoice_SpeculativeDecodingFailure;

			// Let's perform speculative decoding of the 'Header Name' and 'Header Value' portion
			CurrentPortionStartingOffset= NewOffset;

			RegExpReturnCode= pcre_exec(
				(pcre *) CfieldHdrlineElement->SeparatorPCRECompiledRegExp,		// the compiled pattern
				NULL,											// no extra data - we didn't study the pattern
				(char *) &m_packet[NewOffset],					// the subject string
				FieldLen,										// the length of the subject
				0,									// start at offset 0 in the subject
				0,									// default options
				MatchingOffset,						// output vector for substring information
				MATCHING_OFFSET_COUNT);				// number of elements in the output vector

			if (RegExpReturnCode == -1)	// Match not found, 'Header Value' is missing
			{
				// Let's perform speculative decoding of the 'Header Name' portion
				CurrentPortionSize= FieldLen;

				DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_HDRLINE_HNAME;
					
				RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

				if (RetVal != nbSUCCESS)
					goto DecodeChoice_SpeculativeDecodingFailure;

				break;
			}

			if (RegExpReturnCode >= 0)	// Match found, there are both 'Header Name' and 'Header Value'
			{
				// Let's perform speculative decoding of the 'Header Name' portion
				CurrentPortionSize= MatchingOffset[0];

				DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_HDRLINE_HNAME;
				
				RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

				if (RetVal != nbSUCCESS)
					goto DecodeChoice_SpeculativeDecodingFailure;

				// Let's perform speculative decoding of the 'Header Value' portion
				CurrentPortionStartingOffset+= (unsigned int) MatchingOffset[1];
				CurrentPortionSize= FieldLen - (unsigned int) MatchingOffset[1];

				DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_HDRLINE_HVALUE;
				
				RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

				if (RetVal != nbSUCCESS)
					goto DecodeChoice_SpeculativeDecodingFailure;
			}
		}; break;

		case nbNETPDL_ID_CFIELD_DYNAMIC:
		{ // Let's perform speculative decoding of current 'dynamic' complex field
		struct _nbNetPDLElementCfieldDynamic *CfieldDynamicElement;
		unsigned int CurrentPortionStartingOffset= 0;
		unsigned int CurrentPortionSize= 0;
		int RegExpReturnCode;
		int MatchingOffset[MATCHING_OFFSET_COUNT];
		//int PCREFlags= 0;
		unsigned int DefaultSubfieldIndex= 0;

			CfieldDynamicElement= (struct _nbNetPDLElementCfieldDynamic *) ChoiceElement->FieldToDecode;

			SpeculativeField.Name= (char *) NETPDL_CFIELD_NAME_DYNAMIC;

			// Create a Field node and append it to PDML tree
			PDMLFieldElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

			// Update the common fields of a PDML element (name, longname, ...)
			RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, &SpeculativeField, FieldLen, NewOffset, &m_packet[NewOffset]);

			if (RetVal != nbSUCCESS)
				goto DecodeChoice_SpeculativeDecodingFailure;

			// Let's retrieve the matching offset vector to decode subfield
			RegExpReturnCode= pcre_exec(
				(pcre *) CfieldDynamicElement->PatternPCRECompiledRegExp,	// the compiled pattern
				NULL,											// no extra data - we didn't study the pattern
				(char *) &m_packet[PDMLFieldElement->Position],	// the subject string
				PDMLFieldElement->Size,				// the length of the subject
				0,									// start at offset 0 in the subject
				0,									// default options
				MatchingOffset,						// output vector for substring information
				MATCHING_OFFSET_COUNT);				// number of elements in the output vector

			// Let's perform speculative decoding of the 'dynamic'-related portions
			while (DefaultSubfieldIndex < CfieldDynamicElement->NamesListNItems)
			{
				RegExpReturnCode= pcre_get_stringnumber(
					(pcre *) CfieldDynamicElement->PatternPCRECompiledRegExp,		// the compiled pattern
					CfieldDynamicElement->NamesList[DefaultSubfieldIndex]);			// the name of subpattern

				DefaultSubfield.Name= CfieldDynamicElement->NamesList[DefaultSubfieldIndex];
				
				CurrentPortionStartingOffset= NewOffset + MatchingOffset[2*RegExpReturnCode];
				CurrentPortionSize= MatchingOffset[2*RegExpReturnCode+1] - MatchingOffset[2*RegExpReturnCode];

				RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

				if (RetVal != nbSUCCESS)
					goto DecodeChoice_SpeculativeDecodingFailure;

				DefaultSubfieldIndex++;
			}
		}; break;

		case nbNETPDL_ID_CFIELD_ASN1:
		{ // Let's perform speculative decoding of current 'asn1' complex field
                  // struct _nbNetPDLElementCfieldASN1 *CfieldASN1Element;
		unsigned int CurrentPortionStartingOffset= 0;
		unsigned int CurrentPortionSize= 0;

                // CfieldASN1Element= (struct _nbNetPDLElementCfieldASN1 *) ChoiceElement->FieldToDecode;

			SpeculativeField.Name= (char *) NETPDL_CFIELD_NAME_ASN1;

			// Create a Field node and append it to PDML tree
			PDMLFieldElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

			//	Update the common fields of a PDML element (name, longname, ...)
			RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, &SpeculativeField, FieldLen, NewOffset, &m_packet[NewOffset]);

			if (RetVal != nbSUCCESS)
				goto DecodeChoice_SpeculativeDecodingFailure;

			// This is a trick to make available encoding-related meta-data contents
			CurrentPortionStartingOffset= CurrentOffset;
			CurrentPortionSize= StartingBytesToDiscard;

			DefaultSubfield.Name= (char *) NETPDL_SUBFIELD_NAME_ASN1_ENCRULEDATA;

			RetVal= DecodeSubfield(&DefaultSubfield, CurrentPortionStartingOffset, CurrentPortionSize, PDMLFieldElement, &LoopCtrl, NULL);

			if (RetVal != nbSUCCESS)
				goto DecodeChoice_SpeculativeDecodingFailure;
		}; break;
		default:
			break;
	}

	/*
		Evaluation of 'choice'-related match conditions
	*/
	TempFieldMatchElement= ChoiceElement->FirstMatchElement;

	while (TempFieldMatchElement && TempFieldMatchElement->FieldType != nbNETPDL_ID_DEFAULT_MATCH)
	{
		RetVal= m_exprHandler->EvaluateExprNumber(TempFieldMatchElement->MatchExprTree, PDMLFieldElement/*m_PDMLProtoItem->FirstField*/, &MatchExprValue);

		if (RetVal != nbSUCCESS)
		{
			RollbackSpeculativeDecoding(PDMLStartingElement, PDMLFieldElement, PDMLEndingElement);

			// Restore the current offset at last error-free decoded item of the set
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, CurrentOffset);
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, CurrentOffset - m_protoStartingOffset);

			if (ChoiceElement->FirstValidChildIfMissingPacketData == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
					"Decoding of choice fails due to evaluation of matching expression '%s'.", TempFieldMatchElement->MatchExprString);
				return nbFAILURE;
			}

			return DecodeFields(ChoiceElement->FirstValidChildIfMissingPacketData, MaxOffsetToBeDecoded, PDMLParent, &LoopCtrl, &DecodedLen);
		}

		if ( MatchExprValue )
		{
		struct _nbNetPDLElementFieldBase *NetPDLElementField= (struct _nbNetPDLElementFieldBase *) TempFieldMatchElement;

			//	Update the common fields of a PDML element (name, longname, ...)
			RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, NetPDLElementField, PDMLFieldElement->Size, PDMLFieldElement->Position, &m_packet[PDMLFieldElement->Position]);

			if (RetVal != nbSUCCESS)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
					"Decoding of choice fails due to decoding completion of the choice-related item.");

				// Restore the current offset at last error-free decoded item of the set
				RollbackSpeculativeDecoding(PDMLStartingElement, PDMLFieldElement, PDMLEndingElement);

				// Restore the current offset at last error-free decoded item of the set
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, CurrentOffset);
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, CurrentOffset - m_protoStartingOffset);

				// Let's set the value to return and go to last part of this method
				RetVal= nbFAILURE;
				goto DecodeChoice_Rollback;
			}

			break;
		}

		TempFieldMatchElement= TempFieldMatchElement->NextFieldmatch;
	}

	// If all the fields-related matching expressions was evaluated without a positive match, let's restore before-'choice' state
	if ( TempFieldMatchElement == NULL ) 
	{
		RollbackSpeculativeDecoding(PDMLStartingElement, PDMLFieldElement, PDMLEndingElement);
		return nbSUCCESS;
	}

	// If nothing fields-related expression matches, let's update current 'choice'-related field relying on 'default-choice'
	if ( TempFieldMatchElement->FieldType == nbNETPDL_ID_DEFAULT_MATCH )
	{	
	struct _nbNetPDLElementFieldBase *NetPDLElementField= (struct _nbNetPDLElementFieldBase *) TempFieldMatchElement;

		//	Update the common fields of a PDML element (name, longname, ...)
		RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLFieldElement, NetPDLElementField, PDMLFieldElement->Size, PDMLFieldElement->Position, &m_packet[PDMLFieldElement->Position]);

		if (RetVal != nbSUCCESS)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
				"Decoding of choice fails due to decoding completion of the choice-related item.");

			// Let's rollback to previous consistent state
			RollbackSpeculativeDecoding(PDMLStartingElement, PDMLFieldElement, PDMLEndingElement);

			// Restore the current offset at last error-free decoded item of the set
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, CurrentOffset);
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, CurrentOffset - m_protoStartingOffset);

			// Let's set the value to return and go to last part of this method
			RetVal= nbFAILURE;
			goto DecodeChoice_Rollback;
		}
	}

	/*
		Special case for ASN.1 fields: let's deallocate the subfield 
		provided for matching purposes
	*/
	if (ChoiceElement->FieldToDecode->FieldType == nbNETPDL_ID_CFIELD_ASN1)
	{
		m_PDMLMaker->PDMLElementDiscard(PDMLFieldElement->FirstChild);

		PDMLFieldElement->FirstChild= NULL;
	}

	/* 
		Let's update subfields accordingly to selected item: for those field 
		types that have subfields is necessary to check the presence of their 
		definition and to perform relevant specified decoding
	*/
	struct _nbNetPDLElementBase *TempNetPDLChildElement;

	TempNetPDLChildElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, TempFieldMatchElement->FirstChild);

	if (TempNetPDLChildElement)
	{
	struct _nbPDMLField *PDMLTempSubfieldElement;

		switch (ChoiceElement->FieldToDecode->FieldType)
		{
		case nbNETPDL_ID_CFIELD_TLV:
			{
				while (TempNetPDLChildElement)
				{
					switch(TempNetPDLChildElement->Type)
					{
					case nbNETPDL_IDEL_SUBFIELD:
						{
						struct _nbNetPDLElementSubfield *NetPDLElementSubfield;

							NetPDLElementSubfield= (struct _nbNetPDLElementSubfield *) TempNetPDLChildElement;

							if ( strncmp(NetPDLElementSubfield->PortionName, NETPDL_SUBFIELD_NAME_TLV_TYPE, strlen(NETPDL_SUBFIELD_NAME_TLV_TYPE)) == 0 )
								{ PDMLTempSubfieldElement= PDMLFieldElement->FirstChild; goto DecodeChoice_EndTLVPortionCheck; }
							if ( strncmp(NetPDLElementSubfield->PortionName, NETPDL_SUBFIELD_NAME_TLV_LENGTH, strlen(NETPDL_SUBFIELD_NAME_TLV_LENGTH)) == 0 )
								{ PDMLTempSubfieldElement= PDMLFieldElement->FirstChild->NextField; goto DecodeChoice_EndTLVPortionCheck; }
							if ( strncmp(NetPDLElementSubfield->PortionName, NETPDL_SUBFIELD_NAME_TLV_VALUE, strlen(NETPDL_SUBFIELD_NAME_TLV_VALUE)) == 0 )
								{ PDMLTempSubfieldElement= PDMLFieldElement->FirstChild->NextField->NextField; goto DecodeChoice_EndTLVPortionCheck; }

							InTheMiddleFailure= true;
							break;

DecodeChoice_EndTLVPortionCheck:

							if (NetPDLElementSubfield->ComplexSubfieldInfo)
							{
								//	Update the common fields of a PDML element (name, longname, ...)
								RetVal= m_PDMLMaker->PDMLElementUpdate(
									PDMLTempSubfieldElement, 
									(struct _nbNetPDLElementFieldBase *) TempNetPDLChildElement, 
									PDMLTempSubfieldElement->Size, 
									PDMLTempSubfieldElement->Position, 
									&m_packet[PDMLFieldElement->Position]);
							}
						
							RetVal= DecodeSubfield(
								NetPDLElementSubfield, 
								PDMLTempSubfieldElement->Position, 
								PDMLTempSubfieldElement->Size, 
								PDMLFieldElement, 
								&LoopCtrl, 
								NetPDLElementSubfield->ComplexSubfieldInfo? NULL : PDMLTempSubfieldElement);

							InTheMiddleFailure+= RetVal != nbSUCCESS;

							break;
						}
					}

					TempNetPDLChildElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, TempNetPDLChildElement->NextSibling);
				}
			}; break;
		case nbNETPDL_ID_CFIELD_HDRLINE:
			{
				while (TempNetPDLChildElement)
				{
					switch(TempNetPDLChildElement->Type)
					{
					case nbNETPDL_IDEL_SUBFIELD:
						{
						struct _nbNetPDLElementSubfield *NetPDLElementSubfield;

							NetPDLElementSubfield= (struct _nbNetPDLElementSubfield *) TempNetPDLChildElement;

							if ( strncmp(NetPDLElementSubfield->PortionName, NETPDL_SUBFIELD_NAME_HDRLINE_HNAME, strlen(NETPDL_SUBFIELD_NAME_HDRLINE_HNAME)) == 0 )
								{ PDMLTempSubfieldElement= PDMLFieldElement->FirstChild; goto DecodeChoice_EndHdrlinePortionCheck; }
							if ( strncmp(NetPDLElementSubfield->PortionName, NETPDL_SUBFIELD_NAME_HDRLINE_HVALUE, strlen(NETPDL_SUBFIELD_NAME_HDRLINE_HVALUE)) == 0 )
								{ if ((PDMLTempSubfieldElement= PDMLFieldElement->FirstChild->NextField) != NULL) goto DecodeChoice_EndHdrlinePortionCheck; }

							InTheMiddleFailure= true;
							break;

DecodeChoice_EndHdrlinePortionCheck:

							if (NetPDLElementSubfield->ComplexSubfieldInfo)
							{
								//	Update the common fields of a PDML element (name, longname, ...)
								RetVal= m_PDMLMaker->PDMLElementUpdate(
									PDMLTempSubfieldElement, 
									(struct _nbNetPDLElementFieldBase *) TempNetPDLChildElement, 
									PDMLTempSubfieldElement->Size, 
									PDMLTempSubfieldElement->Position, 
									&m_packet[PDMLFieldElement->Position]);
							}
						
							RetVal= DecodeSubfield(
								NetPDLElementSubfield, 
								PDMLTempSubfieldElement->Position, 
								PDMLTempSubfieldElement->Size, 
								PDMLFieldElement, 
								&LoopCtrl, 
								NetPDLElementSubfield->ComplexSubfieldInfo? NULL : PDMLTempSubfieldElement);

							InTheMiddleFailure+= RetVal != nbSUCCESS;

							break;
						}
					}

					TempNetPDLChildElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, TempNetPDLChildElement->NextSibling);
				}
			}; break;
		case nbNETPDL_ID_CFIELD_DYNAMIC:
			{
			struct _nbNetPDLElementSubfield *NetPDLElementSubfield;

				while (TempNetPDLChildElement)
				{
					NetPDLElementSubfield= (struct _nbNetPDLElementSubfield *) TempNetPDLChildElement;
					PDMLTempSubfieldElement= PDMLFieldElement->FirstChild;

					while (PDMLTempSubfieldElement)
					{
						if ( strncmp(NetPDLElementSubfield->PortionName, PDMLTempSubfieldElement->Name, strlen(PDMLTempSubfieldElement->Name)) == 0 )
						{
							switch(TempNetPDLChildElement->Type)
							{
							case nbNETPDL_IDEL_SUBFIELD:
								{
									if (NetPDLElementSubfield->ComplexSubfieldInfo)
									{
										//	Update the common fields of a PDML element (name, longname, ...)
										RetVal= m_PDMLMaker->PDMLElementUpdate(
											PDMLTempSubfieldElement, 
											(struct _nbNetPDLElementFieldBase *) TempNetPDLChildElement, 
											PDMLTempSubfieldElement->Size, 
											PDMLTempSubfieldElement->Position, 
											&m_packet[PDMLFieldElement->Position]);
									}

									RetVal= DecodeSubfield(
										NetPDLElementSubfield, 
										PDMLTempSubfieldElement->Position, 
										PDMLTempSubfieldElement->Size, 
										PDMLFieldElement, 
										&LoopCtrl, 
										NetPDLElementSubfield->ComplexSubfieldInfo? NULL : PDMLTempSubfieldElement);

									InTheMiddleFailure+= RetVal != nbSUCCESS;

									break;
								}
							}
						
							break;
						}

						PDMLTempSubfieldElement= PDMLTempSubfieldElement->NextField;
					}

					TempNetPDLChildElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, TempNetPDLChildElement->NextSibling);
				}
			}; break;
		default:
			{
			unsigned int CurrentOffsetBuffer;

				// Save the offset of the next sibling field; we need to restore this offset as soon as we decoded child fields
				m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffsetBuffer);

				// Reset packet offsets to the value that was present before decoding this subfield
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, PDMLFieldElement->Position);
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, PDMLFieldElement->Position - m_protoStartingOffset);

				RetVal= DecodeFields(TempNetPDLChildElement, PDMLFieldElement->Position + PDMLFieldElement->Size - 1, PDMLFieldElement, &LoopCtrl, &DecodedLen);

				InTheMiddleFailure+= RetVal != nbSUCCESS;

				// Reset packet offsets to the value that was present after decoding this subfield
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, CurrentOffsetBuffer);
				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, CurrentOffsetBuffer - m_protoStartingOffset);
			}
		}
	}

	/*
		Update offset to complete successful speculative decoding
	*/
	NewOffset+= FieldLen;

	// Check that we do not exceed packet boundaries
	if (NewOffset > MaxOffsetToBeDecoded)
		NewOffset= MaxOffsetToBeDecoded + 1;

	// Update the current offset to process subsequent item of the set
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, NewOffset);
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, NewOffset - m_protoStartingOffset);

	// If there are initial bytes to discard, let's decode them as "ghost" field
	if (EndingBytesToDiscard > 0)
	{
#ifdef AllowGhostFieldDecode
	struct _nbNetPDLElementFieldBase TrivialField= {0};

		TrivialField.Name= (char *) NETPDL_FIELDBASE_NAME_TRIVIAL;
		TrivialField.IsInNetworkByteOrder= ChoiceElement->FieldToDecode->IsInNetworkByteOrder;
		TrivialField.ShowTemplateInfo= &DefaultShowTemplate;
		
		// Create a Field node and append it to PDML tree
		PDMLStartingElement= m_PDMLMaker->PDMLElementInitialize(PDMLParent);

		//	Update the common fields of a PDML element (name, longname, ...)
		RetVal= m_PDMLMaker->PDMLElementUpdate(PDMLStartingElement, &TrivialField, EndingBytesToDiscard, NewOffset, &m_packet[NewOffset]);
		// RetVal is not checked here
#endif

		NewOffset+= EndingBytesToDiscard;

		// Check that we do not exceed packet boundaries
		if (NewOffset > MaxOffsetToBeDecoded)
			NewOffset= MaxOffsetToBeDecoded + 1;

		// Update the current offset to process subsequent item of the set
		m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, NewOffset);
		m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, NewOffset - m_protoStartingOffset);
	}

	if (InTheMiddleFailure)
	{
		// If a subfield-related decoding fails, let's print a warning
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
			"Decoding of elements within choice-related item '%s' was not completed.", PDMLFieldElement->Name);

		return nbWARNING;
	}

	return nbSUCCESS;

DecodeChoice_SpeculativeDecodingFailure:

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
		"Decoding of choice fails due to speculative decoding of the set-related item.");

	RetVal= nbFAILURE;

DecodeChoice_Rollback:

	/*
		If processing fails, there is a need to rollback to a correct status 
		of PDML tree
	*/
	RollbackSpeculativeDecoding(PDMLStartingElement, PDMLFieldElement, PDMLEndingElement);

	// Restore the current offset at last error-free decoded item of the set
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, CurrentOffset);
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, CurrentOffset - m_protoStartingOffset);

	return RetVal;
}



void CNetPDLProtoDecoder::RollbackSpeculativeDecoding(struct _nbPDMLField *PDMLStartingElement, struct _nbPDMLField *PDMLFieldElement, struct _nbPDMLField *PDMLEndingElement)
{
	if (PDMLStartingElement != NULL)
	{
		memset(PDMLStartingElement, 0, sizeof(struct _nbPDMLField));

		m_PDMLMaker->PDMLElementDiscard(PDMLStartingElement);
	}

	if (PDMLFieldElement != NULL)
	{
	struct _nbPDMLField *PDMLFieldElementTempChild= PDMLFieldElement->FirstChild;

		// If there are subfields-related PDML elements, let's discard them
		if (PDMLFieldElementTempChild != NULL)
		{
			// Reach last subfields-related PDML element allocated by speculative decoding that have got a sibling
			while (PDMLFieldElementTempChild->NextField)
				PDMLFieldElementTempChild= PDMLFieldElementTempChild->NextField;

			// Discard all subfields-related PDML elements allocated by speculative decoding
			while (PDMLFieldElementTempChild->PreviousField)
			{
				memset(PDMLFieldElementTempChild, 0, sizeof(struct _nbPDMLField));

				m_PDMLMaker->PDMLElementDiscard(PDMLFieldElementTempChild->NextField);

				PDMLFieldElementTempChild= PDMLFieldElementTempChild->PreviousField;
			}

			// At the end, discard first subfield
			memset(PDMLFieldElementTempChild, 0, sizeof(struct _nbPDMLField));

			m_PDMLMaker->PDMLElementDiscard(PDMLFieldElementTempChild);
		}

		memset(PDMLFieldElement, 0, sizeof(struct _nbPDMLField));

		m_PDMLMaker->PDMLElementDiscard(PDMLFieldElement);
	}

	if (PDMLEndingElement != NULL)
	{
		memset(PDMLEndingElement, 0, sizeof(struct _nbPDMLField));

		m_PDMLMaker->PDMLElementDiscard(PDMLEndingElement);
	}
}



/*!
	\brief Returns the position of the next protocol to decode.

	This method is used to determine, from the current protocol, which is the one that follows.

	\param EncapElement: first element in the encapsulation section that has to be checked.

	\param NextProto: this parameter is used to return back the index of the next protocol
	to decode. This value is valid only in case the function does not fail.

	\return nbSUCCESS if the protocol has been found, or nbWARNING if no "definitive" protocols 
	have been found (but no error occurred). In the latter case, we can have two options: we're
	still in the proces of selecting the best candidate/deferred, or no suitable protocols have 
	been found at all, therefore the 'defaultproto' is returned in the NextProto parameter.
	In any case, the NextProto parameter always contains a valid protocol in case of nbSUCCESS and
	nbWARNING.
	Vicerversa, in case of failure the return code is be nbFAILURE and the NextProto parameter does
	not contain valid data.

	\warning The $nextproto variable, which contains the result of this function, MUST
	be initialized to the default value outside this function. This is because this function
	is recursive, hence it cannot be initialized inside this code.
*/
int CNetPDLProtoDecoder::GetNextProto(struct _nbNetPDLElementBase *EncapElement, unsigned int* NextProto)
{
int FirstDeferredOrCandidateProto;
int FirstVerifyResult;
int RetVal;

	FirstDeferredOrCandidateProto= -1;

	RetVal= ScanEncapsulationSection(EncapElement, NextProto, &FirstDeferredOrCandidateProto, &FirstVerifyResult);

	if (RetVal == nbWARNING)
	{
		if (FirstDeferredOrCandidateProto != -1)
		{
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.NextProto, FirstDeferredOrCandidateProto);
			m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.ProtoVerifyResult, FirstVerifyResult);

			*NextProto= FirstDeferredOrCandidateProto;
		}
		else
		{
			// The protocol has not been found, however we did not experience any error
			// (we can still rely on the 'defaultproto')
			*NextProto= NetPDLDatabase->DefaultProtoIndex;
		}

		return nbWARNING;
	}

	// In case of RetVal == nbSUCCESS or nbFAILURE, we can return without any further processing
	return RetVal;
}


/*!
	\brief Scan a piece of the exanpsionation section and returns the position of the next protocol tp decode (if found).

	This method is used to determine, from the current protocol, which is the one that follows.
	This function is recursive.

	\param EncapElement: first element in the encapsulation section that has to be checked.
	This method is recursive (depth-first scanning), and it can be called also in case
	of subelements.

	\param NextProto: this parameter is used to return back the index of the next protocol
	to decode. This value is valid only in case the function returns nbSUCCESS.

	\param FirstDeferredOrCandidateProto: this parameter is used inside the function to keep track
	of any deferred or candidate element. It must be initialized to '-1' when this function is
	called the first time for a protocol. When the function returns, it can contain the
	index of the protocol that has been found being 'deferred' or 'candidate'.

	\param FirstVerifyResult: this parameter is used inside the function to keep track
	of the result of the 'verify' section. When the function returns, it can contain the
	retuls of the 'verify' section of the first protocol that was 'deferred' or 'candidate'.
	Its value does not matter in case the protocol has been found.

	\return nbSUCCESS if the protocol has been found, or nbWARNING if no "definitive" protocols 
	have been found (but no error occurred). In the latter case, we can have two options: we're
	still in the proces of selecting the best candidate/deferred, or no suitable protocols have 
	been found at all. In these two cases, the value of the NextProto parameter is meaningless.
	Vice versa, the NextProto parameter contains a valid protocol in case of nbSUCCESS;
	in case of failure the return code is nbFAILURE and the NextProto parameter does
	not contain valid data.

	\warning The $nextproto variable, which contains the result of this function, MUST
	be initialized to the default value outside this function. This is because this function
	is recursive, hence it cannot be initialized inside this code.
*/
int CNetPDLProtoDecoder::ScanEncapsulationSection(struct _nbNetPDLElementBase *EncapElement, unsigned int* NextProto, int* FirstDeferredOrCandidateProto, int* FirstVerifyResult)
{
struct _nbPDMLField *FirstPDMLField;
int RetVal;

	if (m_PDMLProtoItem)	// When we start decoding the packet, PDMLProtoItem is NULL
		FirstPDMLField= m_PDMLProtoItem->FirstField;
	else
		FirstPDMLField= NULL;

	// Scan the encapsulation section
	while (EncapElement != NULL)
	{
		switch (EncapElement->Type)
		{
			case nbNETPDL_IDEL_NEXTPROTO:
			{
			struct _nbNetPDLElementNextProto *NextProtoElement;
			unsigned int NextProtoIndex;

				NextProtoElement= (struct _nbNetPDLElementNextProto *) EncapElement;

				RetVal= m_exprHandler->EvaluateExprNumber(NextProtoElement->ExprTree, FirstPDMLField, &NextProtoIndex);

				if (RetVal == nbFAILURE)
					return nbFAILURE;

				// We may have 'warning' because of a truncated packet or something like this
				if (RetVal == nbWARNING)
					break;

				m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.NextProto, NextProtoIndex);

				*NextProto= NextProtoIndex;
				return nbSUCCESS;
			};
			break;

			case nbNETPDL_IDEL_NEXTPROTOCANDIDATE:
			{
			struct _nbNetPDLElementNextProto *NextProtoElement;
			unsigned int NextProtoIndex;
			unsigned int VerifyResult;

				NextProtoElement= (struct _nbNetPDLElementNextProto *) EncapElement;

				RetVal= m_exprHandler->EvaluateExprNumber(NextProtoElement->ExprTree, FirstPDMLField, &NextProtoIndex);

				if (RetVal == nbFAILURE)
					return nbFAILURE;

				// We may have 'warning' because of a truncated packet or something like this
				if (RetVal == nbWARNING)
					break;

				RetVal= VerifyNextProto(NextProtoIndex);

				if (RetVal == nbFAILURE)
					return nbFAILURE;

				// Otherwise, let's continue
				m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.ProtoVerifyResult, &VerifyResult);

				if (VerifyResult == NETPDL_PROTOVERIFYRESULT_FOUND)
				{
					m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.NextProto, NextProtoIndex);

					*NextProto= NextProtoIndex;
					return nbSUCCESS;
				}


				if ((VerifyResult == NETPDL_PROTOVERIFYRESULT_CANDIDATE) || (VerifyResult == NETPDL_PROTOVERIFYRESULT_DEFERRED))
				{
					// Save a pointer to the first candidate or deferred encountered
					// If not better protocols are found, this one will be used
					// Save also the value of the ProtoVerifyResult, which will be overwritten by any next call to VerifyNextProto()
					if (*FirstDeferredOrCandidateProto == -1)
					{
						*FirstDeferredOrCandidateProto= NextProtoIndex;
						*FirstVerifyResult= VerifyResult;
					}
				}
			};
			break;

			case nbNETPDL_IDEL_SWITCH:
			{
			struct _nbNetPDLElementCase *CaseNodeInfo;
			struct _nbNetPDLElementSwitch *SwitchElement;
			struct _nbNetPDLElementBase *NestedNextProtoElement;

				SwitchElement= (struct _nbNetPDLElementSwitch *) EncapElement;

				RetVal= m_exprHandler->EvaluateSwitch(SwitchElement, FirstPDMLField, &CaseNodeInfo);

				if (RetVal == nbFAILURE)
					return nbFAILURE;

				// We may have 'warning' because of a truncated packet or something like this
				if (RetVal == nbWARNING)
					break;


				if (CaseNodeInfo)
				{
					NestedNextProtoElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, CaseNodeInfo->FirstChild);

					RetVal= ScanEncapsulationSection(NestedNextProtoElement, NextProto, FirstDeferredOrCandidateProto, FirstVerifyResult);

					if (RetVal != nbWARNING)
						return RetVal;

					// if (RetVal == nbWARNING) let's continue; no error, but the correct protocol has not been found
				}
			}
			break;

			case nbNETPDL_IDEL_IF:
			{
			struct _nbNetPDLElementIf *IfElement;
			struct _nbNetPDLElementBase *NestedNextProtoElement;
			unsigned int Result;

				IfElement= (struct _nbNetPDLElementIf *) EncapElement;
			
				RetVal= m_exprHandler->EvaluateExprNumber(IfElement->ExprTree, FirstPDMLField, &Result);

				if (RetVal == nbFAILURE)
					return nbFAILURE;

				// We may have 'warning' because of a truncated packet or something like this
				if (RetVal == nbWARNING)
					break;


				if (Result)
					NestedNextProtoElement= IfElement->FirstValidChildIfTrue;
				else
					NestedNextProtoElement= IfElement->FirstValidChildIfFalse;

				if (NestedNextProtoElement)
				{
					RetVal= ScanEncapsulationSection(NestedNextProtoElement, NextProto, FirstDeferredOrCandidateProto, FirstVerifyResult);

					if (RetVal != nbWARNING)
						return RetVal;

					// if (RetVal == nbWARNING) let's continue; no error, but the correct protocol has not been found
				}
			};
			break;

			case nbNETPDL_IDEL_ASSIGNVARIABLE:
			{
				RetVal= m_exprHandler->EvaluateAssignVariable((struct _nbNetPDLElementAssignVariable*) EncapElement, FirstPDMLField);

				if (RetVal != nbSUCCESS)
					return RetVal;
	
			}; break;

			case nbNETPDL_IDEL_ASSIGNLOOKUPTABLE:
			{
				RetVal= m_exprHandler->EvaluateAssignLookupTable((struct _nbNetPDLElementAssignLookupTable*) EncapElement, FirstPDMLField);

				if (RetVal == nbFAILURE)
					return nbFAILURE;

			}; break;

			case nbNETPDL_IDEL_UPDATELOOKUPTABLE:
			{
				RetVal= m_exprHandler->EvaluateLookupTable((struct _nbNetPDLElementUpdateLookupTable*) EncapElement, FirstPDMLField);

				// The EvaluateLookupTable() can return nbWARNING in case the key element is not present
				// However, this is not an error condition.
				if (RetVal == nbFAILURE)
					return nbFAILURE;

			}; break;

			default:
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Field type not recognized.");

				return nbFAILURE;
			}
		}

		// The current nextproto does not satisfy the expression; so let's try with the next one

		// Let's move to the next sibling
		EncapElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, EncapElement->NextSibling);
	}

	return nbWARNING;
}


/*!
	\brief Checks the 'verify' items of the target protocol and it returns if this protocol is correct.

	\param NextProtoIndex: index of the protocol that has to be tested.

	\return nbSUCCESS if the function returns without error, nbFAILURE in case some error occurred.
	The result of the verification is stored in the VerifyResult variable, hence value must be
	retrieved from there.
*/
int CNetPDLProtoDecoder::VerifyNextProto(unsigned int NextProtoIndex)
{
struct _nbNetPDLElementExecuteX *VerifyElement;
struct _nbPDMLField *FirstPDMLField;

	if (m_PDMLProtoItem)	// When we start decoding the packet, PDMLProtoItem is NULL
		FirstPDMLField= m_PDMLProtoItem->FirstField;
	else
		FirstPDMLField= NULL;

	// Let's initialize the protocol verification to "not found"
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.ProtoVerifyResult, NETPDL_PROTOVERIFYRESULT_NOTFOUND);

	VerifyElement= (NetPDLDatabase->ProtoList[NextProtoIndex])->FirstExecuteVerify;

	while(VerifyElement)
	{
	int RetVal;
	unsigned int Result= 1;

		if (VerifyElement->WhenExprTree)
		{
			RetVal= m_exprHandler->EvaluateExprNumber(VerifyElement->WhenExprTree, FirstPDMLField, &Result);

			if (RetVal != nbSUCCESS)
				return RetVal;
		}

		if (Result)
		{
			RetVal= ExecuteProtoCode(nbNETPDL_GET_ELEMENT(NetPDLDatabase, VerifyElement->FirstChild));

			if (RetVal != nbSUCCESS)
				return RetVal;
		}

		// We may even have to execute several sections (if more 'when' attributes matches)
		VerifyElement= VerifyElement->NextExecuteElement;
	}

	return nbSUCCESS;
}

#define CHECK_PROPER_CALLBACK_EXIST(NetPDLElement, Event) ((NetPDLElement->CallHandlerInfo) && (NetPDLElement->CallHandlerInfo->CallHandler) && (NetPDLElement->CallHandlerInfo->FireOnEvent == Event))

/*!
	\brief Executes code that is required in the 'execute-code-before' and 'execute-code-after' sections.

	\param ExecuteElement: first valid element within the given section.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CNetPDLProtoDecoder::ExecuteProtoCode(struct _nbNetPDLElementBase *ExecuteElement)
{
int RetVal;
struct _nbPDMLField *FirstPDMLField;

	if (m_PDMLProtoItem)	// When we start decoding the packet, PDMLProtoItem is NULL
		FirstPDMLField= m_PDMLProtoItem->FirstField;
	else
		FirstPDMLField= NULL;

	while (ExecuteElement)
	{
		if (CHECK_PROPER_CALLBACK_EXIST(ExecuteElement, nbNETPDL_CALLHANDLE_EVENT_BEFORE))
		{
			// Jump to the callback function
			((nbPacketDecoderExternalCallHandler *) ExecuteElement->CallHandlerInfo->CallHandler) (ExecuteElement);
		}

		switch (ExecuteElement->Type)
		{
			case nbNETPDL_IDEL_SWITCH:
			{
			struct _nbNetPDLElementCase *CaseNodeInfo;

				RetVal= m_exprHandler->EvaluateSwitch((struct _nbNetPDLElementSwitch *) ExecuteElement, FirstPDMLField, &CaseNodeInfo);

				if (RetVal != nbSUCCESS)
					return RetVal;

				if ( (RetVal == nbSUCCESS) && (CaseNodeInfo) )
				{
					RetVal= ExecuteProtoCode(nbNETPDL_GET_ELEMENT(NetPDLDatabase, CaseNodeInfo->FirstChild));

					if (RetVal != nbSUCCESS)
						return RetVal;
				}

			}; break;

			case nbNETPDL_IDEL_IF:
			{
			struct _nbNetPDLElementIf *IfNode;
			unsigned int Result;

				IfNode= (struct _nbNetPDLElementIf *) ExecuteElement;

				RetVal= m_exprHandler->EvaluateExprNumber(IfNode->ExprTree, FirstPDMLField, &Result);

				if (RetVal != nbSUCCESS)
					return RetVal;

				// In case the 'if' returns 'true'
				if (Result)
				{
					if (IfNode->FirstValidChildIfTrue)
					{
						RetVal= ExecuteProtoCode(IfNode->FirstValidChildIfTrue);

						if (RetVal != nbSUCCESS)
							return RetVal;
					}
				}
				else
				{
					if (IfNode->FirstValidChildIfFalse)
					{
						RetVal= ExecuteProtoCode(IfNode->FirstValidChildIfFalse);

						if (RetVal != nbSUCCESS)
							return RetVal;
					}
				}

			}; break;

			case nbNETPDL_IDEL_ASSIGNVARIABLE:
			{
				RetVal= m_exprHandler->EvaluateAssignVariable((struct _nbNetPDLElementAssignVariable*) ExecuteElement, FirstPDMLField);

				if (RetVal != nbSUCCESS)
					return RetVal;
	
			}; break;

			case nbNETPDL_IDEL_ASSIGNLOOKUPTABLE:
			{
				RetVal= m_exprHandler->EvaluateAssignLookupTable((struct _nbNetPDLElementAssignLookupTable*) ExecuteElement, FirstPDMLField);

				if (RetVal == nbFAILURE)
					return nbFAILURE;

			}; break;

			case nbNETPDL_IDEL_UPDATELOOKUPTABLE:
			{
				RetVal= m_exprHandler->EvaluateLookupTable((struct _nbNetPDLElementUpdateLookupTable*) ExecuteElement, FirstPDMLField);

				// The EvaluateLookupTable() can return nbWARNING in case the key element is not present
				// However, this is not an error condition.
				if (RetVal == nbFAILURE)
					return nbFAILURE;

			}; break;

			default:
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "The <execute-code> section of NetPDL description contains an element which has not been recognized.");
				return nbFAILURE;
			}
		}

		if (CHECK_PROPER_CALLBACK_EXIST(ExecuteElement, nbNETPDL_CALLHANDLE_EVENT_AFTER))
		{
			// Jump to the callback function
			((nbPacketDecoderExternalCallHandler *) ExecuteElement->CallHandlerInfo->CallHandler) (ExecuteElement);
		}

		// Move to the next element
		ExecuteElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, ExecuteElement->NextSibling);
	}

	return nbSUCCESS;
}
