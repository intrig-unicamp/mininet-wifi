/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

/*!
	\file nbee_pxmlreader.h

	Keeps the definitions and exported functions related to the module that handles PDML and PSML files.
*/



#ifdef NBEE_EXPORTS
	// We may have include header files of some other libraries, which do not have the right xxxx_EXPORTS
	// and then defined DLL_EXPORT as ""; So, let's define this in any case, and let's suppress the warning
	#pragma warning(disable: 4005)	
	#define DLL_EXPORT __declspec(dllexport)
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT
	#endif
#endif




/*!
	\defgroup NetBeePxMLReader NetBee PSML and PDML Readers
*/

/*! \addtogroup NetBeePxMLReader
	\{
*/

/*!
	\brief This class implements a set of methods to return packets formatted according to the PSML language to the user.

	This class can be used to a given PSML file and return their content to the caller, in a format which is
	more friendly than using XML parsers.

	\note Please note that this class cannot be instantiated directly: you have to use either the nbAllocatePSMLReader()
	function to get a pointer to this class, or ask the nbPacketDecoder to return a pointer to it (the nbPacketDecoder
	creates an instance of this class for its internal use, and it can return it to the caller).
	For cleanup, in case the class was allocated through the nbAllocatePSMLReader(), the programmer must use the 
	nbDeallocatePSMLReader() function. If this class was returned by the nbPacketDecoder, it will be deleted automatically.
*/
class DLL_EXPORT nbPSMLReader
{
public:
	nbPSMLReader() {};
	virtual ~nbPSMLReader() {};

	/*!
		\brief It returns an ascii buffer containing the name of the sections defined into the &lt;showsumstruct&gt;
		section of the NetPDL file.

		The returned buffer will keep the names of the sections separated by a NULL character, e.g.:

		<pre>
		Number\\0Time\\0Data Link\\0Network\\0Transport\\0Application\\0
		</pre>

		The number of sections present in the buffer are returned as well.

		\param SummaryPtr: pointer to a 'char *' in which PSMLReader will put the will put the structure 
		of the summary. This buffer will always be NULL terminated.

		\return The number of sections (e.g. '6' in previous example), nbFAILURE if some error occurred.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning The returned buffer is valid until the user calls a new method of the nbPSMLReader class.
	*/
	virtual int GetSummary(char **SummaryPtr) = 0;


	/*!
		\brief It returns an ascii buffer containing the PSML structure of the summary formatted with XML tags.

		\param SummaryPtr: pointer to a 'char *' in which PSMLReader will put the will put the structure 
		of the summary. This buffer will always be NULL terminated.

		\param SummaryLength: upon return, it will contain the length of the valid data in the buffer
		pointed by 'SummaryPtr', not counting the string terminator.

		\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning The returned buffer is valid until the user calls a new method of the nbPSMLReader class.
	*/
	virtual int GetSummaryXML(char* &SummaryPtr, unsigned int &SummaryLength) = 0;


	/*!
		\brief It returns an ascii buffer containing the summary related to the current packet.

		This function is used only if the PSMLReader source is a NetBeePacketDecoder; in this case, this function
		returns the summary related to the last decoded packet.

		The returned buffer will keep the names of the sections separated by a NULL character, e.g.:

		<pre>
		0\\016:35:13.984880\\0Eth: 0080C7-CB439A => FFFFFF-FFFFFF\\0ARP Request\\0Eth. Padding\\0NULL\\0
		</pre>

		The number of sections present in the buffer are returned as well.

		\param PacketPtr: pointer to a 'char *' in which PSMLReader will put the will put the summary 
		related to the current packet. This buffer will always be NULL terminated.
	
		\return The number of sections (e.g. '6' in previous example) contained in the packet,
		nbFAILURE if some error occurred.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning The returned buffer is valid until the user calls a new method of the nbPSMLReader class.
	*/
	virtual int GetCurrentPacket(char **PacketPtr) = 0;


	/*!
		\brief It returns an ascii buffer containing the summary of the current PSML packet formatted with XML tags.

		\param PacketPtr: pointer to a 'char *' in which PSMLReader will put the will put the PSML packet summary.
		This buffer will always be NULL terminated.

		\param PacketLength: upon return, it will contain the length of the valid data in the buffer
		pointed by 'PacketPtr', not counting the string terminator.

		\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning The returned buffer is valid until the user calls a new method of the nbPSMLReader class.
	*/
	virtual int GetCurrentPacketXML(char* &PacketPtr, unsigned int &PacketLength) = 0;


	/*!
		\brief It returns an ascii buffer containing the summary related a given packet.

		The returned buffer will keep the names of the sections separated by a NULL character, e.g.:

		<pre>
		0\\016:35:13.984880\\0Eth: 0080C7-CB439A => FFFFFF-FFFFFF\\0ARP Request\\0Eth. Padding\\0NULL\\0
		</pre>

		The number of sections present in the buffer are returned as well.

		\param PacketNumber: number of the packet (starting from '1') that we want to know its summary.

		\param PacketPtr: pointer to a 'char *' in which PSMLReader will put the will put the summary 
		related to the given packet. This buffer will always be NULL terminated.

		\return The number of sections (e.g. '6' in previous example), contained in the packet, 
		nbFAILURE if some error occurred, nbWARNING if the user asked for a packet that is out of range.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning This function returns an error if the PSMLReader source is a NetBeePacketDecoder, but it has been 
		configured not to keep all packets. In this case, the PSMLReader cannot have access to previously decoded
		packet that have already been descarded.

		\warning The returned buffer is valid until the user calls a new method of the nbPSMLReader class.
	*/
	virtual int GetPacket(unsigned long PacketNumber, char **PacketPtr) = 0;


	/*!
		\brief It returns an ascii buffer containing the summary of the given PSML packet formatted with XML tags.

		\param PacketNumber: number of the packet (starting from '1') that we want to know its summary.

		\param PacketPtr: pointer to a 'char *' in which PSMLReader will put the will put the PSML packet summary.
		This buffer will always be NULL terminated.

		\param PacketLength: upon return, it will contain the length of the valid data in the buffer
		pointed by 'PacketPtr', not counting the string terminator.

		\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred,
		nbWARNING in case the requested packet is out of range.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning The returned buffer is valid until the user calls a new method of the nbPSMLReader class.
	*/
	virtual int GetPacketXML(unsigned long PacketNumber, char* &PacketPtr, unsigned int &PacketLength) = 0;


	/*!
		\brief It deletes an element from the list of packets managed by the current instance of the class.

		This method is provided in order to allow delections of some of the packets from within the 
		PSML file.

		When the function returns, the new index of the packets will take into account of the deleted one.
		In other words, if the index was from 0 to N, it is now is from 0 to (N-1).

		\param PacketNumber: the ordinal number of the packet (packets start at '1') that has to
		be deleted.

		\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int RemovePacket(unsigned long PacketNumber) = 0;


	/*!
		\brief It dumps the current PSML document on file.

		This method creates a <i>copy</i> of the current file, skipping the elements that 
		have then deleted using the methods available in this class (e.g. RemovePacket()).

		Please note that the original file is either the one opened using the allocator of
		this class (which is kept unmodified), or a temp file created automatically by the library.
		For instance, if the nbPacketDecoder() is asked to keep all the PSML data, a temp
		file is created on disk and this method creates a copy of that file.

		\param Filename: name of the file that will be used for the dump.

		\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int SaveDocumentAs(const char *Filename) = 0;

	/*! 
		\brief Return a string keeping the last error message that occurred within the current instance of the class

		\return A buffer that keeps the last error message.
		This buffer will always be NULL terminated.
	*/
	virtual char *GetLastError() = 0;
};




// Just to avoid compiler errors
struct _nbPDMLField;
struct _nbPDMLProto;
struct _nbPDMLPacket;


/*!
	\brief Structure that keeps the most important info related to a PDML packet.

	This structure contains the links to properly scan the content (protocols and fields)
	of the current packet.
	This structure keeps also the information associated to the entire packet (e.g. length, etc).
*/
typedef struct _nbPDMLPacket
{
	unsigned long Number;				//!< Ordinal number of the current packet (starting from '1').
	unsigned long Length;				//!< Total length (off wire) of the current packet.
	unsigned long CapturedLength;		//!< Number of bytes captured from the current packet.
	unsigned long TimestampSec;			//!< Timestamp (seconds) of the current packet, starting midnight, January 1, 1970 (same as UNIX struct timeval).
	unsigned long TimestampUSec;		//!< Timestamp (microseconds) of the current packet (same as UNIX struct timeval).
	struct _nbPDMLProto *FirstProto;	//!< Pointer to the first protocol present in the packet.
	const unsigned char* PacketDump;	//!< Pointer to the raw packet dump, formatted according to the standard libpcap/Winpcap libraries. This hex buffer has size 'CapturedLength'.
} _nbPDMLPacket;


/*!
	\brief Structure that keeps the most important info related to a PDML protocol.

	This structure contains the links to properly scan the content (other protocols and fields
	of the current proto) of the current protocol.
*/
typedef struct _nbPDMLProto
{
	char *Name;						//!< Internal name of the current protocol according to the NetPDL definition.
	char *LongName;					//!< Name of the current protocol according to the NetPDL definition.
	unsigned long Position;			//!< Position of the current protocol in the packed dump (offset start from '0').
	unsigned long Size;				//!< Size of the current protocol in bytes.
	struct _nbPDMLPacket *PacketSummary;	//!< Pointer to the structure that keeps the most important information about the packet.
	struct _nbPDMLProto *PreviousProto;		//!< Pointer to the protocol that precedes the current one (if any).
	struct _nbPDMLProto *NextProto;			//!< Pointer to the protocol that follows the current one (if any).
	struct _nbPDMLField *FirstField;		//!< Pointer to the first field contained in the packet.
} _nbPDMLProto;


/*!
	\brief Structure that keeps the most important info related to a PDML field.

	This structure contains the links to properly scan the content of the current protocol
	(e.g. all the other fields contained in the protocol).

	Please note that members Value and Mask are returned as ascii strings. In case
	the user wants to get access to the hex value of the field, it can use the Position/Size members
	and get the value directly from the hex dump of the packet.
*/
typedef struct _nbPDMLField
{
	char *Name;						//!< Internal name of the current field according to the NetPDL definition.
	char *LongName;					//!< Name of the current field according to the NetPDL definition.
	char *ShowValue;				//!< Value of the 'showvalue' attribute of the PDML field.
	char *ShowDetails;				//!< Value of the 'showdtl' attribute of the PDML field.
	char *ShowMap;					//!< Value of the 'showmap' attribute of the PDML field.
	char *Value;					//!< Value of the 'value' attribute of the PDML field as ascii hex buffer (e.g. value 16 is stored as '10', using two chars).
	char *Mask;						//!< Value of the 'mask' attribute of the PDML field (e.g. value 16 is stored as '10', using two chars).
	unsigned long Position;			//!< Position of the current field in the packed dump (offset start from '0').
	unsigned long Size;				//!< Size of the current protocol in bytes.
	bool isField;                   //!< 'true' if the current element is a field, 'false' if it is a block
	struct _nbPDMLProto *ParentProto;		//!< Pointer to the protocol that contains the current field.
	struct _nbPDMLField *ParentField;		//!< Pointer to the field which is the parent of the current one (if any).
	struct _nbPDMLField *PreviousField;		//!< Pointer to the field that precedes the current one (if any).
	struct _nbPDMLField *NextField;			//!< Pointer to the field that follows the current one (if any).
	struct _nbPDMLField *FirstChild;		//!< Pointer to the first child field contained in the packet.
} _nbPDMLField;




/*!
	\brief This class implements a set of methods to return packets formatted according to the PDML language to the user.

	This class can be used to parse a given PDML file and return their content to the caller, in a format which is
	more friendly than using XML parsers.

	\note Please note that this class cannot be instantiated directly: you have to use either the nbAllocatePDMLReader()
	function to get a pointer to this class, or ask the nbPacketDecoder to return a pointer to it (the nbPacketDecoder
	creates an instance of this class for its internal use, and it can return it to the caller).
	For cleanup, in case the class was allocated through the nbAllocatePDMLReader(), the programmer must use the 
	nbDeallocatePDMLReader() function. If this class was returned by the nbPacketDecoder, it will be deleted automatically.
*/
class DLL_EXPORT nbPDMLReader
{
public:
	nbPDMLReader() {};
	virtual ~nbPDMLReader() {};

	/*!
		\brief It returns a pointer to a set of binary structures that can be used to operate on the current PDML packet
		without the need of XML parsers.

		This function is used only if the PDMLReader source is a NetBeePacketDecoder; in this case, this function
		returns a pointer to the last decoded packet.

		The returned data will be a _nbPDMLPacket and a set of _nbPDMLProto and _nbPDMLFields linked together.
		The number of protocols contained in the packet are returned as well.

		\param PDMLPacket: pointer to a '_nbPDMLPacket *' in which PDMLReader will put the returned data.

		\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning The returned structures are valid until the user calls a new method of the nbPDMLReader class.
	*/
	virtual int GetCurrentPacket(_nbPDMLPacket **PDMLPacket) = 0;


	/*!
		\brief It returns a pointer to a buffer that contains the XML dump of the current PDML packet.

		This function is used only if the PDMLReader source is a NetBeePacketDecoder; in this case, this function
		returns a pointer to the last decoded packet.

		The returned data will be an ascii buffer that has to be further parsed according to the PDML definition.
		The resulting document will start with the &lt;packet&gt; tag, and so on.

		\param PacketPtr: pointer to a buffer in which PDMLReader will put the returned data.
		This buffer will always be NULL terminated.

		\param PacketLength: upon return, it will contain the length of the valid data in the buffer
		pointed by 'PacketPtr', not counting the string terminator.

		\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning The returned structures are valid until the user calls a new method of the nbPDMLReader class.
	*/
	virtual int GetCurrentPacketXML(char* &PacketPtr, unsigned int &PacketLength) = 0;


	/*!
		\brief It returns a pointer to a set of binary structures that can be used to operate on a given PDML packet
		without the need of XML parsers.

		The returned data will be a _nbPDMLPacket and a set of _nbPDMLProto and _nbPDMLFields linked together.
		The number of protocols contained in the packet are returned as well.

		\param PacketNumber: number of the packet (starting from '1') that we want to get.

		\param PDMLPacket: pointer to a 'PDMLPacket *' in which PDMLReader will put the returned PDML data.

		\return A positive number containing the number of protocols contained in the packet,
		nbFAILURE if some error occurred, nbWARNING if the user asked for a packet that is out of range.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning This function returns an error if the PDMLReader source is a NetBeePacketDecoder, but it has been 
		configured not to keep all packets. In this case, the PDMLReader cannot have access to previously decoded
		packet that have already been descarded.

		\warning The returned structures are valid until the user calls a new method of the nbPDMLReader class.
	*/
	virtual int GetPacket(unsigned long PacketNumber, _nbPDMLPacket **PDMLPacket) = 0;


	/*!
		\brief It returns a pointer to a buffer that contains the XML dump of a given PDML packet.

		The returned data will be an ascii buffer that has to be further parsed according to the PDML definition.
		The resulting document will start with the &lt;packet&gt; tag, and so on.
	
		\param PacketNumber: number of the packet (starting from '1') that we want to get.

		\param PacketPtr: pointer to a buffer in which PDMLReader will put the returned data.
		This buffer will always be NULL terminated.

		\param PacketLength: upon return, it will contain the length of the valid data in the buffer
		pointed by 'PacketPtr', not counting the string terminator.

		\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred,
		nbWARNING in case the requested packet is out of range.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning The returned structures are valid until the user calls a new method of the nbPDMLReader class.
	*/
	virtual int GetPacketXML(unsigned long PacketNumber, char* &PacketPtr, unsigned int &PacketLength) = 0;


	/*!
		\brief It deletes an element from the list of packets managed by the current instance of the class.

		This method is provided in order to allow delections of some of the packets from within the 
		PDML file.

		When the function returns, the new index of the packets will take into account of the deleted one.
		In other words, if the index was from 0 to N, it is now is from 0 to (N-1).

		\param PacketNumber: the ordinal number of the packet (packets start at '0') that has to
		be deleted.

		\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int RemovePacket(unsigned long PacketNumber) = 0;


	/*!
		\brief It dumps the current PDML document on file.

		This method creates a <i>copy</i> of the current file, skipping the elements that 
		have then deleted using the methods available in this class (e.g. RemovePacket()).

		Please note that the original file is either the one opened using the allocator of
		this class (which is kept unmodified), or a temp file created automatically by the library.
		For instance, if the nbPacketDecoder() is asked to keep all the PDML data, a temp
		file is created on disk and this method creates a copy of that file.

		\param Filename: name of the file that will be used for the dump.

		\return nbSUCCESS if everything is fine, nbFAILURE' if some error occurred.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int SaveDocumentAs(const char *Filename) = 0;

	/*!
		\brief This function returns a pointer to a PDML field, which can be used to get access to field values.

		This function is able to scan a given decoded packet and to locate a field named
		'FieldName' within a given protocol. Then, it returns a pointer to that field.

		This function can be used to access to the content of a specific field (e.g. the field
		value) in order to make some further processing.

		Please note that the packet must already been decoded in order to be able to call this
		function. In other words, you should:
		- submit a packet to the decoding engine (through the nbPacketDecoder::DecodePacket() )
		- call this function in order to get some of the fields within this packet.

		This function can be called several times within the same packet. This can be useful in
		case the decoded packet contains several fields that have the same name. In this case,
		this function returns the first encountered field the first time is called.
		Then, the function can be called again in order to return the following items. However,
		in this case the first parameter ('FirstField') must be set to the previously located field
		in order to tell the analysis engine that it should scan the packets starting from 
		the given field onward.

		\param FieldName: string that contains the name of the field (in the PDML file) 
		we are looking for.

		\param ProtoName: string that contains the name of the protocol (in the PDML file) 
		in which the field we are looking for is contained. This can be NULL in case we specify
		the field to start with (i.e. if the 'FirstField' parameter is non-NULL).

		\param FirstField: in case the field named 'FieldName' occurs more than one time in the
		current protocol, this parameter tells this function that it must start searching from this
		field onward. Basically, the value of this field is the same value returned by this
		function in the previous round.

		\param ExtractedField: this variable keeps the return value of the function. This variable
		is a pointer to a _nbPDMLField, passed by reference (i.e. &(_nbPDMLField *) ).
		When the function return, this variable will contain a pointer to the field we are looking for.

		\return nbSUCCESS if everything is fine  and the field has been located, nbFAILURE if some error occurred,
		nbWARNING if there are no more occurencies of the field within the current protocol.
		The latest return code is used only when the 'FirstField' variable is a valid pointer.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\note Since variable 'FirstField' is used only at the beginning of the function, while
		'ExtractedField' keeps the returned value, these parameters can make use of a single variable.
		In other words, this function can be called using the following fragment of code:
		<pre>
		struct _nbPDMLField *PDMLField= NULL;
		GetPDMLField("proto", "myfield", PDMLField, &PDMLField);
		</pre>
	*/
	virtual int GetPDMLField(char *ProtoName, char *FieldName, struct _nbPDMLField *FirstField, struct _nbPDMLField **ExtractedField) = 0;


	/*!
		\brief This function returns a pointer to a PDML field, which can be used to get access to field values.

		This function is able to scan a given decoded packet and to locate a field named
		'FieldName' within a given protocol. Then, it returns a pointer to that field.

		This function can be used to access to the content of a specific field (e.g. the field
		value) in order to make some further processing.

		This function overloads the other GetPDMLField() method and it is useful when you have a set
		of decoded packets (e.g. a PDML file on disk) and you want to access to some information 
		contained within these packets.

		In this case, you do not need to call the nbPacketDecoder::DecodePacket() before being able
		to access to PDML packets; you need just to make sure that the packet 'PacketNumber' exists
		in PDML format.

		This function can be called several times within the same packet. This can be useful in
		case the decoded packet contains several fields that have the same name. In this case,
		this function returns the first encountered field the first time is called.
		Then, the function can be called again in order to return the following items. However,
		in this case the first parameter ('FirstField') must be set to the previously located field
		in order to tell the analysis engine that it should scan the packets starting from 
		the given field onward.

		\param PacketNumber: the ordinal number of the packet (packets start at '0') that is being
		selected.

		\param FieldName: string that contains the name of the field (in the PDML file) 
		we are looking for.

		\param ProtoName: string that contains the name of the protocol (in the PDML file) 
		in which the field we are looking for is contained. This can be NULL in case we specify
		the field to start with (i.e. if the 'FirstField' parameter is non-NULL).

		\param FirstField: in case the field named 'FieldName' occurs more than one time in the
		current protocol, this parameter tells this function that it must start searching from this
		field onward. Basically, the value of this field is the same value returned by this
		function in the previous round.

		\param ExtractedField: this variable keeps the return value of the function. This variable
		is a pointer to a _nbPDMLField, passed by reference (i.e. &(_nbPDMLField *) ).
		When the function return, this variable will contain a pointer to the field we are looking for.

		\return nbSUCCESS if everything is fine  and the field has been located, nbFAILURE if some error occurred,
		nbWARNING if there are no more occurencies of the field within the current protocol.
		The latest return code is used only when the 'FirstField' variable is a valid pointer.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\note Since variable 'FirstField' is used only at the beginning of the function, while
		'ExtractedField' keeps the returned value, these parameters can make use of a single variable.
		In other words, this function can be called using the following fragment of code:
		<pre>
		struct _nbPDMLField *PDMLField= NULL;
		GetPDMLField(PacketNumber, "proto", "myfield", PDMLField, &PDMLField);
		</pre>
	*/
	virtual int GetPDMLField(unsigned long PacketNumber, char *ProtoName, char *FieldName, _nbPDMLField *FirstField, _nbPDMLField **ExtractedField) = 0;

	/*! 
		\brief Return a string keeping the last error message that occurred within the current instance of the class

		\return A buffer that keeps the last error message.
		This buffer will always be NULL terminated.
	*/
	virtual char *GetLastError() = 0;
};



/*!
	\brief It creates a new instance of a nbPSMLReader object and returns it to the caller.

	This class can be used to parse a given PSML file and return their content to the caller, in a format which is
	more friendly than using XML parsers.

	\param FileName: string containing the PDML file to open.

	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.

	\return A pointer to a nbPSMLReader, or NULL if something was wrong.
	In that case, the error message will be returned in the ErrBuf buffer.

	\warning Be carefully that the returned object must be deallocated through the nbDeallocatePSMLReader().
*/
DLL_EXPORT nbPSMLReader* nbAllocatePSMLReader(char *FileName, char *ErrBuf, int ErrBufSize);


/*!
	\brief It deallocates the instance of nbPSMLReader created through the nbAllocatePSMLReader().

	\param PSMLReader: pointer to the object that has to be deallocated.
*/
DLL_EXPORT void nbDeallocatePSMLReader(nbPSMLReader* PSMLReader);



/*!
	\brief It creates a new instance of a nbPDMLReader object and returns it to the caller.

	This class can be used to parse a given PDML file and return their content to the caller, in a format which is
	more friendly than using XML parsers.

	\param FileName: string containing the PDML file to open.

	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.

	\return A pointer to a nbPDMLReader, or NULL if something was wrong.
	In that case, the error message will be returned in the ErrBuf buffer.

	\warning Be carefully that the returned object must be deallocated through the nbDeallocatePDMLReader().
*/
DLL_EXPORT nbPDMLReader* nbAllocatePDMLReader(char *FileName, char *ErrBuf, int ErrBufSize);


/*!
	\brief It deallocates the instance of nbPDMLReader created through the nbAllocatePDMLReader().

	\param PDMLReader: pointer to the object that has to be deallocated.
*/
DLL_EXPORT void nbDeallocatePDMLReader(nbPDMLReader* PDMLReader);


/*!
	\}
*/

