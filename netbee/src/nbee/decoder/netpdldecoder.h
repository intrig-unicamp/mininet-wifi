/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include "netpdlexpression.h"

#include "netpdlprotodecoder.h"
#include "psmlmaker.h"
#include "pdmlmaker.h"
#include "netpdldecoderutils.h"


/*!
	\brief This class implements a NetPDL decoding engine which is able to decode and print
	a network packet according to the NetPDL and PDML descriptions.

	This class creates a NetPDL parser; it needs an XML file containing the NetPDL description, 
	and it will parse the file creating the corresponding PDML file.
	The user must call the DecodePacket() method in order to submit to the NetPDL engine
	the packet to parse, according to the NetPDL description.
*/
class CNetPDLDecoder : public nbPacketDecoder
{
	friend class CPSMLReader;
	friend class CPDMLReader;

public:
	CNetPDLDecoder(int NetPDLFlags);
	virtual ~CNetPDLDecoder();

	int Initialize();

	int DecodePacket(nbNetPDLLinkLayer_t LinkLayerType, int PacketCounter,
		const struct pcap_pkthdr *PcapHeader, const unsigned char *PcapPktData);

	nbPSMLReader* GetPSMLReader();
	nbPDMLReader* GetPDMLReader();

	nbPacketDecoderVars* GetPacketDecoderVars();
	nbPacketDecoderLookupTables* GetPacketDecoderLookupTables();

private:
	//! Keeps the flags (e.g. if we want to generate PSML files, and more) chosen by the user.
	int m_netPDLFlags;

	//! Class that manages NetPDL run-time variables
	CNetPDLVariables *m_netPDLVariables;

	//! Creates an expression handler; you can reuse this class throughout all the decoding (i.e. faster)
	CNetPDLExpression *m_exprHandler;

	//! Pointer to the object that decodes protocols.
	CNetPDLProtoDecoder *m_protoDecoder;

	//! Pointer to the class that generates the summary view
	CPSMLMaker *m_PSMLMaker;

	//! Pointer to the class that generates the detailed view
	CPDMLMaker *m_PDMLMaker;

	//! Pointer to the class that manages PSML files
	CPSMLReader *m_PSMLReader;

	//! Pointer to the class that manages PDML files
	CPDMLReader *m_PDMLReader;
};

