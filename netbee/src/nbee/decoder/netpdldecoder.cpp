/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <xercesc/util/PlatformUtils.hpp>

#include <nbee.h>

#include "netpdldecoder.h"
#include "../globals/globals.h"
#include "../globals/debug.h"
#include "../misc/initialize.h"


extern struct _nbNetPDLDatabase *NetPDLDatabase;


/*!
	\brief Default constructor.

	\param NetPDLFlags: a set of flags (OR-ed) defined in #nbPacketDecoderFlags.
*/
CNetPDLDecoder::CNetPDLDecoder(int NetPDLFlags)
{
	m_netPDLFlags= NetPDLFlags;

	m_exprHandler= NULL;
	m_netPDLVariables= NULL;

	m_protoDecoder= NULL;

	m_PSMLMaker= NULL;
	m_PDMLMaker= NULL;

	m_PSMLReader= NULL;
	m_PDMLReader= NULL;

	memset(m_errbuf, 0, sizeof(m_errbuf));
}


//! Default destructor
CNetPDLDecoder::~CNetPDLDecoder()
{
	if (m_exprHandler)
		delete m_exprHandler;
	if (m_netPDLVariables)
		delete m_netPDLVariables;

	if (m_protoDecoder)
		delete m_protoDecoder;

	if (m_PSMLReader)
		delete m_PSMLReader;
	if (m_PDMLReader)
		delete m_PDMLReader;

	if (m_PSMLMaker)
		delete m_PSMLMaker;
	if (m_PDMLMaker)
		delete m_PDMLMaker;
}


int CNetPDLDecoder::Initialize()
{
	// Instantiate a run-time variable Handler
	m_netPDLVariables = new CNetPDLVariables(m_errbuf, sizeof(m_errbuf));
	if (m_netPDLVariables == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to allocate internal variables.");
		return nbFAILURE;
	}

	// Instantiates an Expression Handler
	m_exprHandler= new CNetPDLExpression(m_netPDLVariables, m_errbuf, sizeof(m_errbuf));
	if (m_exprHandler == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to allocate the m_exprHandler internal variable.");
		return nbFAILURE;
	}

	// Initializes the class that manages PSML files. This class is created only if we want to create the PSML
	if (m_netPDLFlags & nbDECODER_GENERATEPSML)
	{
		m_PSMLReader= new CPSMLReader();
		if (m_PSMLReader == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to allocate the m_PSMLReader internal variable.");
			return nbFAILURE;
		}

		if (m_PSMLReader->Initialize() == nbFAILURE)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "%s", m_PSMLReader->GetLastError() );
			return nbFAILURE;
		}

		// Initializes the class that generates the summary view
		m_PSMLMaker= new CPSMLMaker(m_exprHandler, m_PSMLReader, m_netPDLFlags, m_errbuf, sizeof(m_errbuf));
		if (m_PSMLMaker == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to allocate the m_PSMLMaker internal variable.");
			return nbFAILURE;
		}

		if (m_PSMLMaker->Initialize() == nbFAILURE)
			return nbFAILURE;
	}

	// Check that the user asked to generate the proper PDML (either reduced or with visualization primitives)
	if ( ((m_netPDLFlags & nbDECODER_GENERATEPDML) == 0) && ((m_netPDLFlags & nbDECODER_GENERATEPDML_COMPLETE) == 0))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), 
			"Error allocating the packet decoder: the PDML generation (either simple or with visualization " \
			"primitives) has to be turned on in order to be able to allocate the Packet Decoder.");
		return nbFAILURE;
	}

	// Initializes the class that manages PDML files. This class must always be present.
	m_PDMLReader= new CPDMLReader();
	if (m_PDMLReader == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to allocate the m_PDMLReader internal variable.");
		return nbFAILURE;
	}
	if (m_PDMLReader->Initialize() == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "%s", m_PDMLReader->GetLastError() );
		return nbFAILURE;
	}

	// Initializes the class that generates the detailed view
	m_PDMLMaker= new CPDMLMaker (m_exprHandler, m_PDMLReader, m_netPDLFlags, m_errbuf, sizeof(m_errbuf));
	if (m_PDMLMaker == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to allocate the m_PDMLMaker internal variable.");
		return nbFAILURE;
	}

	if (m_PDMLMaker->Initialize(m_netPDLVariables) == nbFAILURE)
		return nbFAILURE;

	// Instantiates a new NetPDLProtoDecoder
	m_protoDecoder= new CNetPDLProtoDecoder(m_netPDLVariables, m_exprHandler, m_PDMLMaker, m_PSMLMaker, m_errbuf, sizeof(m_errbuf));
	if (m_protoDecoder == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), 
						"Not enough memory to allocate the m_protoDecoder internal variable.");
		return nbFAILURE;
	}

	return InitializeNetPDLInternalStructures(m_exprHandler, m_netPDLVariables, m_errbuf, sizeof(m_errbuf));
}



// Documented into the base class
int CNetPDLDecoder::DecodePacket(nbNetPDLLinkLayer_t LinkLayerType, int PacketCounter, 
										const struct pcap_pkthdr *PcapHeader, const unsigned char *PcapPktData)
{
	unsigned int CurrentProtoItem, PreviousProtoItem;
	unsigned int CurrentOffset;
	int RetVal;
	unsigned int PacketLen;
	unsigned int BytesToBeDecoded;	// Total number of bytes we have to decode; it is usually equal to
								// 'snaplen' unless we have a short frame on Ethernet

	// First, let's perform a sanity check to see that the packet size does not exceeds our internal limits
	if (PcapHeader->caplen >= NETPDL_MAX_PACKET)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
				"Submitted a packet whose captured size (%d) is larger than the maximum size allowed in this library (%d).",
				PcapHeader->caplen, NETPDL_MAX_PACKET);
		return nbFAILURE;
	}

	// Cleanup variables whose validity has already expired (e.g. old sessions or var whose scope is a single packet)
	m_netPDLVariables->DoGarbageCollection(PcapHeader->ts.tv_sec);

	// Set default variables defined by the NetPDL specification
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.LinkType, LinkLayerType);
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.FrameLength, PcapHeader->caplen);
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.PacketLength, PcapHeader->caplen);
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TimestampSec, PcapHeader->ts.tv_sec);
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.TimestampUSec, PcapHeader->ts.tv_usec);
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, 0);
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentProtoOffset, 0);
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.NextProto, NetPDLDatabase->StartProtoIndex);

	m_netPDLVariables->SetVariableRefBuffer(m_netPDLVariables->m_defaultVarList.PacketBuffer, (unsigned char *) PcapPktData, 0, PcapHeader->caplen);

	m_PDMLMaker->PacketInitialize();
	// Create a PDML fragment that keeps the general info of the packet (timestamp, ...)
	m_PDMLMaker->PacketGenInfoInitialize(PcapHeader, PcapPktData, PacketCounter);

	if (m_PSMLMaker)
		m_PSMLMaker->PacketInitialize( m_PDMLMaker->GetPDMLPacketInfo() );

	if (m_PSMLMaker)
	{
		// The 'startproto' does not have any real field in it, so the first parameter 
		// (which keeps the decpded protocol fragment) is NULL
		RetVal= m_PSMLMaker->AddHeaderFragment(NULL, NetPDLDatabase->StartProtoIndex, 
			NULL /* it is not a block */, 0 /* Offset */, PcapHeader->caplen);

		if (RetVal != nbSUCCESS)
			return nbFAILURE;
	}

	// Set the current protocol to the first one (which is the 'startproto')
	m_protoDecoder->Initialize(0, PcapHeader);

	// Let's initialize $nextproto variable
	m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.NextProto, NetPDLDatabase->DefaultProtoIndex);

	RetVal= m_protoDecoder->GetNextProto((struct _nbNetPDLElementBase*) NetPDLDatabase->ProtoList[NetPDLDatabase->StartProtoIndex]->FirstEncapsulationItem, &CurrentProtoItem);
	if (RetVal == nbFAILURE)
		return nbFAILURE;

	// We check against the packet length, which can be less than the frame length in case we're on Ethernet 2.0
	// and our frame is smaller than the minimum Ethernet frame length
	m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.PacketLength, &PacketLen);

	if (PacketLen < PcapHeader->caplen)
		BytesToBeDecoded= PacketLen;
	else
		BytesToBeDecoded= PcapHeader->caplen;

	CurrentOffset= 0;
	PreviousProtoItem= NetPDLDatabase->StartProtoIndex;

	while (CurrentOffset < PcapHeader->caplen)
	{
		// Set the proper PrevProto variable
		m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.PrevProto, PreviousProtoItem);

		// Initializes the NetPDLProtoDecoder to the current values for this protocol
		m_protoDecoder->Initialize(CurrentProtoItem, PcapHeader);

		// Decode header and return a pointer to a PDML Header
		if (CurrentOffset < BytesToBeDecoded)
			RetVal= m_protoDecoder->DecodeProto(PcapPktData, BytesToBeDecoded, CurrentOffset);
		else
			RetVal= m_protoDecoder->DecodeProto(PcapPktData, PcapHeader->caplen, CurrentOffset);

		if (RetVal == nbFAILURE)
			goto error;

		if (RetVal == nbSUCCESS)
			PreviousProtoItem= CurrentProtoItem;

		m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.PacketLength, &PacketLen);

		if (PacketLen < PcapHeader->caplen)
			BytesToBeDecoded= PacketLen;
		else
			BytesToBeDecoded= PcapHeader->caplen;

		// Get current offset
		m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.CurrentOffset, &CurrentOffset);

		// Let's initialize $nextproto variable
		m_netPDLVariables->SetVariableNumber(m_netPDLVariables->m_defaultVarList.NextProto, NetPDLDatabase->DefaultProtoIndex);

		/*
			This part is really a mess. So, let's recap:
			- if we've still bytes to be decoded, let's get the nextproto in the 'standard' way
			- if we ended the packet, the problem is that we may still have some bytes to be decoded, which may
			  be the Ethernet padding
		*/

		if (CurrentOffset < BytesToBeDecoded)
		{
			RetVal= m_protoDecoder->GetNextProto((struct _nbNetPDLElementBase*) NetPDLDatabase->ProtoList[CurrentProtoItem]->FirstEncapsulationItem, &CurrentProtoItem);
			if (RetVal == nbFAILURE)
				return nbFAILURE;
		}
		else
		{
			// Should we check that we're on Ethernet?
			if (NetPDLDatabase->EtherpaddingProtoIndex)
				CurrentProtoItem= NetPDLDatabase->EtherpaddingProtoIndex;
		}

#ifdef _DEBUG
		// Check (only in debug mode) that the current protocol is valid; it may be due to some user errors
		// when filling in the lookup table that stores the nextproto item.
		if ((CurrentProtoItem < 0) || (CurrentProtoItem >= NetPDLDatabase->ProtoListNItems))
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
				"Requested a protocol that is out of range.");
			return nbFAILURE;
		}
#endif
	}

	// Tell the PxDMLMakers that the packet decoding is ended
	if (m_PDMLMaker->PacketDecodingEnded() == nbFAILURE)
		return nbFAILURE;
	if (m_PSMLMaker)
	{
		if (m_PSMLMaker->PacketDecodingEnded() == nbFAILURE)
			return nbFAILURE;
	}

	return nbSUCCESS;

error:
	// In any case, we have to tell PSML and PDML makers that the current packet has ended
	m_PDMLMaker->PacketDecodingEnded();
	if (m_PSMLMaker)
		m_PSMLMaker->PacketDecodingEnded();

	return nbFAILURE;
}



// Documented in the base class
nbPSMLReader *CNetPDLDecoder::GetPSMLReader()
{
	if (m_PSMLReader == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The current instance of packet decoder has not been configured to generate PSML packets.");
		return NULL;
	}

	if (m_PSMLReader->InitializeDecoder( (nbPacketDecoder *) this) == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "%s", m_PSMLReader->GetLastError() );
		return NULL;
	}

	return (nbPSMLReader *) m_PSMLReader;
}

// Documented in the base class
nbPDMLReader *CNetPDLDecoder::GetPDMLReader()
{
	if (m_PDMLReader->InitializeDecoder( (nbPacketDecoder *) this) == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "%s", m_PDMLReader->GetLastError() );
		return NULL;
	}

	return (nbPDMLReader *) m_PDMLReader;
}


// Documented in base class
nbPacketDecoderVars* CNetPDLDecoder::GetPacketDecoderVars()
{
	return (nbPacketDecoderVars *) m_netPDLVariables;
}


// Documented in base class
nbPacketDecoderLookupTables* CNetPDLDecoder::GetPacketDecoderLookupTables()
{
	return (nbPacketDecoderLookupTables *) m_netPDLVariables;
}

