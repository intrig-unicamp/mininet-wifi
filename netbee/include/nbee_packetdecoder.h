/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


/*!
	\file nbee_packetdecoder.h

	Keeps the definitions and exported functions related to the Packet Decoder module of the NetBee library.
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



#include <nbee_pxmlreader.h>
#include <nbee_packetdecoderutils.h>


/*!
	\defgroup NetBeePacketDecoder NetBee Packet Decoder

	This group includes all the classes related to the packet decoding engine included in the NetBee library.
	The main class is the nbPacketDecoder, which implements the packet decoder itself. In addition, you can use
	the nbPacketDecoderVars and nbPacketDecoderLookupTables classes, which allow to manage variables and lookup
	tables used by the packet decoder itself. For instance, these classes allow to add or modify the content of
	a variable or a lookup table, and are shared between "user space" and the nbPacketDecoder class (hence, any
	modification will influence the behaviour of the packet decoder).

	Please note that the nbPacketDecoderLookupTables can be used also alone, without any active instance of the 
	nbPacketDecoder class, although this feature has not been widely tested.
*/

/*! \addtogroup NetBeePacketDecoder
	\{
*/




/*! 
	\brief Values used for the initialization of a nbPacketDecoder class.

	The most common settings are nbDECODER_GENERATEPDML_COMPLETE | nbDECODER_GENERATEPSML, 
	which tells the Packet Decoder that it has to generate both PDML (complete) and PSML documents;
	however, only the fragment related to the last decoded packet is kept in memory.

	If the user wants to keep the data related to all the packets, it has to add the 
	nbDECODER_KEEPALLPDML and nbDECODER_KEEPALLPSML flags. In this case, the user will be able to
	dump on disk the PSML and PDML files related to the packets being decoded through the
	appropriate use of the nbPSMLReader and nbPDMLReader classes.

	In case nbDECODER_GENERATEPDML_COMPLETE is not set, the PDML document is created with the basic tags.
	In case nbDECODER_GENERATEPSML is not set, the PSML document is not created at all.

	If the user does not want any of these flags, it has to call the nbAllocatePacketDecoder() with a parameter
	equal to 'zero'.
*/
enum nbPacketDecoderFlags
{
	/*!
		\brief The NetBee Packet Decoder will generate the PDML fragment associated to each packet.
		
		The PDML fragment does not contain the visualization primitives
		(hence it does not require a NetPDL file with visualization primitives).
	*/
	nbDECODER_GENERATEPDML= 1,

	/*!
		\brief The NetBee Packet Decoder will generate the PDML fragment associated to each packet.
		
		The PDML fragment contains also the visualization primitives
		(hence it requires a NetPDL file with visualization primitives).
	*/
	nbDECODER_GENERATEPDML_COMPLETE= 2,

	/*!
		\brief The NetBee Packet Decoder will also include the raw dump of the packet in the generated PDML fragment.
		
		The PDML fragment will include also the raw packet dump in addition to the 
		detailed view of the packet. The raw packet dump might be needed in order to show
		the actual bytes included in the packet.
		This flag can be turned on with either the PDML 'base' or the one that
		includes the visualization primitives.
	*/
	nbDECODER_GENERATEPDML_RAWDUMP= 4,

	/*!
		\brief The NetBee Packet Decoder will generate the PSML fragment associated to each packet.
		
		This requires a NetPDL file with visualization primitives.
	*/
	nbDECODER_GENERATEPSML= 8,

	/*!
		\brief The NetBee PacketDecoder will keep a copy of all the generated PDML fragments.
		
		In this case, the PDML fragments related to all the packets submitted to 
		the NetBee Packet Decoder are stored internally (i.e. in a temporary file).
		This allow the user to ask the NetBee library for the PDML fragment associated
		to any packet already decoded.
		
		The user is allowed to dump the entire PDML file on disk (through the use of
		nbPDMLReader::SaveDocumentAs()) only when this flag is turned on.
	*/
	nbDECODER_KEEPALLPDML= 16,

	/*!
		\brief The NetBee PacketDecoder will keep a copy of the entire PSML file
		
		In this case, the PSML fragments related to all the packets submitted to 
		the NetBee Packet Decoder are stored internally (i.e. in a temporary file).
		This allow the user to ask the NetBee library for the PSML fragment associated
		to any packet already decoded.

		The user is allowed to dump the entire PSML file on disk (through the use of
		nbPSMLReader::SaveDocumentAs()) only when this flag is turned on.
	*/
	nbDECODER_KEEPALLPSML= 32
};




/*!
	\brief This class defines an object that is able to decode network packets according to the NetPDL description.

	This class defines a set of methods that aim at analyzing the binary dump of a network packet
	and return a decoded packet, according to the PDML and PSML descriptions. The decoding will be done
	through the use of a protocol description according to the NetPDL language.

	Proper classes (e.g. nbPSMLReader and nbPDMLReader) can be used to return the decoded packet to the caller
	in order to make some further elaboration. For instance, these classes can be used to get access to every packet
	field, and so on.

	The main method of this class is DecodePacket(), which is used to submit a packet to this class.

	Please note that this is an abstract class; please use the nbAllocatePacketDecoder() method in order to create
	an instance of this class. For cleanup, the programmer must use the nbDeallocatePacketDecoder() function.

	\warning You may have to set some global variable before decoding anything. In this case, you have to get a 
	pointer to the class that manages internal variables (i.e. GetPacketDecoderVars() ) and call the appropriate methods in there.
	This can be required if you want to customize some part of the decoding (e.g. if you want to display literal 
	names such as 'host.foo.bar' instead of IP addresses).
*/
class DLL_EXPORT nbPacketDecoder
{
public:
	nbPacketDecoder() {};
	virtual ~nbPacketDecoder() {};

	/*!
		\brief Entry point of the Packet Decoder; it accepts a packet for decoding.

		This method is invoked by external programs to submit a packet to the Packet
		Decoding engine.
		This method will decode network packets and it will manage some internal structures 
		that will be used to return the PDML and PSML fragments related to this packet.

		This method is pretty much useless if called alone. For instance, the output of the decoding
		(i.e. the new PDML and PSML fragments) is kept in some internal structures that are not
		returned by this call.
		To have access to this data, you have to obtain a nbPSMLReader or nbPDMLReader object by calling
		the GetPSMLReader() and GetPDMLReader() methods.

		The way PSML and PDML elements are stored internally and returned depends on the flags 
		(see section #nbPacketDecoderFlags) used to initialize the Packet Decoder.

		\param LinkLayerType: the value of the link-layer type of the submitted packet, according to
		the values defined in #nbNetPDLLinkLayer_t.

		\param PacketCounter: the ordinal number of the packet corresponding to this packet within
		the current capture. Usually, packets starts from '1'.

		\param PcapHeader: header of the submitted packet, according to the WinPcap definition.
		
		\param PcapPktData: buffer containing the packet, according to the WinPcap definition.

		\return nbSUCCESS if the decoding ended correctly, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved by the GetLastError() method.<br>
		In case of errors, the PDML and PSML buffers may contain some valid data.
		Basically, they will contain all the data related to the decoding that has been performed before the
		error occurred.
	*/

	virtual int DecodePacket(nbNetPDLLinkLayer_t LinkLayerType, int PacketCounter,
		const struct pcap_pkthdr *PcapHeader, const unsigned char *PcapPktData)= 0;


	/*!
		\brief It returns a pointer to a nbPSMLReader object.

		This method can be used get a nbPSMLReader object, which is needed to manage the PSML file created by the decoder.
		This class is able to parse PSML files and return their content to the caller, in a format which is
		more friendly than using XML parsers.

		\return A pointer to a nbPSMLReader, or NULL if something was wrong.
		In that case, the error message will be retrieved by the GetLastError() method.

		\warning The class returned by this method will be deallocated automatically when
		the nbPacketDecoder is deleted. Therefore, the user does not have to deallocate
		this object.
	*/
	virtual nbPSMLReader *GetPSMLReader()= 0;


	/*!
		\brief It returns a pointer to a nbPDMLReader object.

		This method can be used get a nbPDMLReader object, which is needed to manage the PDML file created by the decoder.
		This class is able to parse PDML files and return their content to the caller, in a format which is
		more friendly than using XML parsers.

		\return A pointer to a nbPDMLReader, or NULL if something was wrong.
		In this case, the error message will be retrieved by the GetLastError() method.

		\warning The class returned by this method will be deallocated automatically when
		the nbPacketDecoder is deleted. Therefore, the user does not have to deallocate
		this object.
	*/
	virtual nbPDMLReader *GetPDMLReader()= 0;


	/*!
		\brief It returns a pointer to a nbPacketDecoderVars object.

		This object is used to get access to the NetPDL decoder internal variables.

		\return A pointer to a nbPacketDecoderVars, or NULL if something was wrong.
		In this case, the error message will be retrieved by the GetLastError() method.

		\warning The class returned by this method will be deallocated automatically when
		the nbPacketDecoder is deleted. Therefore, the user does not have to deallocate
		this object.
	*/
	virtual nbPacketDecoderVars* GetPacketDecoderVars()= 0;


	/*!
		\brief It returns a pointer to a nbPacketDecoderLookupTables object.

		This object is used to get access to the NetPDL decoder internal lookup tables.
		For instance, this used can also be used to manage lookup tables, i.e. to use this class
		from outside the NetPDL decoder engine. This is useful when you have to create some piece
		of code that use lookup tables and you do not want to create this code from scratch.

		\return A pointer to a nbPacketDecoderLookupTables, or NULL if something was wrong.
		In this case, the error message will be retrieved by the GetLastError() method.

		\warning The class returned by this method will be deallocated automatically when
		the nbPacketDecoder is deleted. Therefore, the user does not have to deallocate
		this object.
	*/
	virtual nbPacketDecoderLookupTables* GetPacketDecoderLookupTables()= 0;


	/*! 
		\brief Return a string keeping the last error message that occurred within the current instance of the class

		\return A buffer that keeps the last error message.
		This buffer will always be NULL terminated.
	*/
	char *GetLastError() { return m_errbuf; }

protected:
	char m_errbuf[2048];			//!< Buffer that keeps the error message (if any)
};




/*!
	\brief It creates a new instance of a nbPacketDecoder object and returns it to the caller.

	This function is needed in order to create the proper object derived from nbPacketDecoder.
	For instance, nbPacketDecoder is a virtual class and it cannot be instantiated.
	The returned object has the same interface of nbPacketDecoder, but it is a real object.

	A nbPacketDecoder class is a NetPDL-based engine (i.e. a class that implements the 
	NetPDL specification) that aims at decoding packets.

	\param Flags: one or more values defined in #nbPacketDecoderFlags. Please note that
	the nbPacketDecoder must generate a PDML fragment for being able to work; so either the
	flag for basic PDML generation or the complete PDML generation (with visualization primitives)
	must be present.

	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will eventually 
	keep an error message (if one).

	\param ErrBufSize: the length of the buffer that keeps the error message.

	\return A pointer to real object that has the same interface of the nbPacketDecoder abstract class,
	or NULL in case of error. In case of failure, the error message is returned into the ErrBuf buffer.

	\warning Be carefully that the returned object must be deallocated through the nbDeallocatePacketDecoder().
*/
DLL_EXPORT nbPacketDecoder* nbAllocatePacketDecoder(int Flags, char *ErrBuf, int ErrBufSize);


/*!
	\brief It deallocates the instance of nbPacketDecoder created through the nbAllocatePacketDecoder().

	\param PacketDecoder: pointer to the object that has to be deallocated.
*/
DLL_EXPORT void nbDeallocatePacketDecoder(nbPacketDecoder* PacketDecoder);




/************************************************************/
/*       Functions that manage external call handlers       */
/************************************************************/


//! Function prototype for the callback function in the user code, which are called when a 'callhandle' is encountered within a NetPDL tag.
typedef void (nbPacketDecoderExternalCallHandler)(struct _nbNetPDLElementBase *);


/*!
	\brief Registers a callback function, which will be called when a 'callhandle' is encountered within a NetPDL tag.

	\param Namespace: namespace of the current function. This parameter must match the corresponding
	parameter present in the 'callhandle' attribute of the NetPDL element.

	\param Function: literal name of the current function. This parameter must match the corresponding
	parameter present in the 'callhandle' attribute of the NetPDL element.

	\param ExternalHandler: pointer to the function (of type nbPacketDecoderExternalCallHandler) that will handle the callback.

	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will eventually 
	keep an error message (if one).

	\param ErrBufSize: the length of the buffer that keeps the error message.

	\return nbSUCCESS if everything is fine, nbFAILURE if some error occurred.
	In case of failure, the error message is returned into the ErrBuf buffer.
*/
int nbRegisterPacketDecoderCallHandle(char* Namespace, char* Function, nbPacketDecoderExternalCallHandler* ExternalHandler, char *ErrBuf, int ErrBufSize);




/*!
	\}
*/

