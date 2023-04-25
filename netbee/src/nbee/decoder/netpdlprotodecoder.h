/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


/*** 
	For Fulvio: comment/uncomment if you want to respectively deny/allow 
	decoding of those trivial fields that "adjust" packet dump visualization
***/
#define AllowGhostFieldDecode


#include <pcap.h>
#include <nbprotodb.h>

#include "netpdlexpression.h"
#include "pdmlmaker.h"
#include "psmlmaker.h"




/*!
	\brief This class is devoded to decode an single protocol according to the NetPDL specification

	This class is called by the CNetPDLDecoder for each protocol to decode. This class creates
	a fragment of the PDML file according to the protocol decoded, then it returns back the control.
*/
class CNetPDLProtoDecoder  
{
public:
	CNetPDLProtoDecoder(CNetPDLVariables *NetPDLVars, CNetPDLExpression *ExprHandler, 
		CPDMLMaker *PDMLMaker, CPSMLMaker *PSMLMaker, char *ErrBuf, int ErrBufSize);
	virtual ~CNetPDLProtoDecoder();

	void Initialize(int myCurrentProtoItem, const struct pcap_pkthdr *PcapHeader);
	int DecodeProto(const unsigned char *Packet, bpf_u_int32 SnapLen, bpf_u_int32 Offset);
	int GetNextProto(struct _nbNetPDLElementBase *EncapElement, unsigned int* NextProto);

private:

	int DecodeFields(struct _nbNetPDLElementBase *FieldElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent, int *LoopCtrl, unsigned int *Len);
	int DecodeLoop(struct _nbNetPDLElementLoop *LoopElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent);
	int DecodeBlock(struct _nbNetPDLElementBlock *BlockElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent, int *LoopCtrl);
	int DecodeField(struct _nbNetPDLElementBase *FieldElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent, int *LoopCtrl);
	int GetFieldParams(struct _nbNetPDLElementFieldBase *FieldElement, unsigned int MaxOffsetToBeDecoded, unsigned int *StartingBytesToDiscard, unsigned int *FieldLen, unsigned int *EndingBytesToDiscard);
	int DecodeStandardField(struct _nbNetPDLElementFieldBase *FieldElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent, struct _nbPDMLField **CurrentPDMLElement, unsigned int* CurrentFieldStartingOffset);
	int DecodeSubfield(struct _nbNetPDLElementSubfield *SubfieldElement, unsigned int StartingOffset, unsigned int Size, _nbPDMLField *PDMLParent, int *LoopCtrl, struct _nbPDMLField *AlreadyAllocatedPDMLElement);
	void DecodeMapTree(struct _nbNetPDLElementCfieldXML *CfieldXMLElement, struct _nbPDMLField *PDMLFieldXML, unsigned int MinOffsetToBeDecoded, unsigned int MaxOffsetToBeDecoded);
	int DecodeSet(struct _nbNetPDLElementSet *SetElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent);
	int DecodeFieldChoice(struct _nbNetPDLElementChoice *ChoiceElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent);
	int DecodeSubfieldChoice(struct _nbNetPDLElementChoice *ChoiceElement, unsigned int MaxOffsetToBeDecoded, struct _nbPDMLField *PDMLParent);
	void RollbackSpeculativeDecoding(struct _nbPDMLField *PDMLStartingElement, struct _nbPDMLField *PDMLFieldElement, struct _nbPDMLField *PDMLEndingElement);


	int VerifyNextProto(unsigned int NextProtoIndex);

	int ExecuteProtoCode(struct _nbNetPDLElementBase *ExecuteElement);

	int ScanEncapsulationSection(struct _nbNetPDLElementBase *EncapElement, unsigned int* NextProto, int* FirstDeferredOrCandidateProto, int* FirstVerifyResult);

	//! Pointer to the run-time variables managed by the NetPDL engine
	CNetPDLVariables *m_netPDLVariables;

	//! Keeps a reference to the current protocol; it is a index into the 'GlobalData->ProtoInfo' list
	int m_currentProtoItem;

	/*!
		Pointer to the element that keeps the PDML fragment that has been created in the current decoding step;
		it is used to determine the next protocol, to evaluate expressions, and more
	*/
	struct _nbPDMLProto *m_PDMLProtoItem;

	/*!
		Pointer to an expression handler; it is used to make the expression evaluation faster (we do not need 
		to create a new NetPDLExpression object each time we have a new expression to evaluate)
	*/
	CNetPDLExpression *m_exprHandler;

	//! Offset (into the packet) corresponding to the first byte of this proto (used to determine the padding and for custom templates)
	bpf_u_int32 m_protoStartingOffset;
	
	//! packet (byte string) that has to be decoded
	unsigned char *m_packet;
	
	//! Header of the captured packet (it tells you the timestamp, the snaplen, caplen, ...).
	const struct pcap_pkthdr *m_pcapHeader;

	//! Pointer to the class that generates the detailed packet view (shared with NetPDLParser)
	CPDMLMaker *m_PDMLMaker;

	//! Pointer to the class that generates the summary view (shared with NetPDLParser)
	CPSMLMaker *m_PSMLMaker;

	//! Pointer to the buffer that will keep the error message (if any); this buffer belongs to the class that creates this one.
	char *m_errbuf;

	//! Size of the buffer that will keep the error message (if any); this buffer belongs to the class that creates this one.
	int m_errbufSize;

};

