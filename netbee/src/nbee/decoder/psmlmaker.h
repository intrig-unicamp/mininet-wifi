/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include "psmlreader.h"
#include "netpdlexpression.h"
#include "../utils/asciibuffer.h"

/*!
	\brief It provides the interface to create a PSML file.

	This class includes all the methods that are needed in order to create a PSML file, i.e.
	a file that keeps a brief description of the captured packets.

	\warning The PSML document will be deallocated all at once when this class terminates
	(i.e. when the destructor is being called).
*/
class CPSMLMaker
{
public:
	CPSMLMaker(CNetPDLExpression *ExprHandler, CPSMLReader *PSMLReader, int NetPDLFlags, char *ErrBuf, int ErrBufSize);
	virtual ~CPSMLMaker();

	//! Initialize internal variables in order to get another packet
	void PacketInitialize(_nbPDMLPacket *PDMLPacket);
	int PacketDecodingEnded();
	int Initialize();

	int AddHeaderFragment(_nbPDMLProto *PDMLProtoItem, int NetPDLProtoItem, struct _nbNetPDLElementFieldBase *CurrentNetPDLBlock, 
		int CurrentOffset, int SnapLen);

	// This function is called also from other classes, but there are no errors in there (so we do not define ErrBuf and such)
	int GetSummaryAscii(char *Buffer, int BufferSize);
	int GetCurrentPacketAscii(char *Buffer, int BufferSize);


	/*!
		\brief Variable used to store the text contained general structure of the summary

		This var is an array of strings (i.e. a vector of string vector), whose size is given by the
		number of sections in the summary view.
	*/
	char **m_summaryStructData;

	/*!
		\brief Temporary variable, used to store the text prior to setting it into the PSML document

		This var is an array of string (i.e. a vector of string vector), whose size is given by the
		number of sections in the summary view.
		However, there is a nasty problem: sometimes, we can have a 'block' field that wants to
		add something to the summary view. To overcome this problem, this variable has size equal
		to the number of sections *plus one*, which is used to store temporarily the string
		the block wants to append.

		For instance, the biggest problem is that the &lt;showsum&gt; tag is called for the block
		before being called for the whole protocol; therefore the text inserted by the block will be 
		inserted *before* the text inserted by the protocol.

		In this way, we place the text of the block in a temporary location, then we move that text later.
	*/
	char **m_summaryItemsData;

private:
	int PrintProtoBlockDetails(_nbPDMLProto *PDMLProtoItem, int CurrentOffset, int SnapLen, struct _nbNetPDLElementBase *ShowSummaryFirstItem);

	//! Keeps the current section, i.e. the one in which the data will be appended
	unsigned int m_currentSection;

	//! Pointer to the same expression handler we have into the NetPDL decoder
	CNetPDLExpression *m_exprHandler;

	//! Value that indicates if the user wants to create also visualization extension within the PDML file.
	int m_isVisExtRequired;

	//! Value that indicates if the user wants to keep all the packets stored or just the last one.
	int m_keepAllPackets;

	//! Pointer to a PSMLReader; needed to manage PSML files (storing data and such)
	CPSMLReader *m_PSMLReader;

	// Pointer to the PDML packet
	_nbPDMLPacket *m_PDMLPacket;

	//! Variable needed to save data on file
	CAsciiBuffer m_tempAsciiBuffer;

	//! Pointer to the buffer that will keep the error message (if any); this buffer belongs to the class that creates this one.
	char *m_errbuf;

	//! Size of the buffer that will keep the error message (if any); this buffer belongs to the class that creates this one.
	int m_errbufSize;
};

