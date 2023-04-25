/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


/*!
	\file nbee_packetdumpfiles.h

	Keeps the definitions and exported functions related to the module that handles NetBee packet dumps.
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
	\defgroup NetBeePacketDumpFiles NetBee Packet Dump Files Manager

	This group includes all the classes related to the packet dump file management, i.e. classes that are used
	to open, close and manage (read packets, write packets, etc.) packet capture dumps.

	These classes are abstract and cannot be instantiated directly. The programmer can get an instance of these
	classes using the nbAllocatePacketDumpFilePcap() functions, and it has to delete them through the appropriate
	nbDeallocatePacketDumpFilePcap().
*/


/*! \addtogroup NetBeePacketDumpFiles
	\{
*/




/*!
	\brief This class defines an object that is able to manage capture files saved in the WinPcap/libpcap format.

	This class defines a set of methods that aim at managing the binary dump of a network packet.
	It allows to create, read, write, scan the file. It provides an easy way to get access to WinPcap/libpcap
	files. In addition, it provides also indexing, i.e. it allows accessing to a random packet in the dump, while
	WinPcap/libpcap allows only progressive scan.

	Please note that this is an abstract class; please use the nbAllocatePacketDumpFilePcap() method in order to create
	an instance of this class. For cleanup, the programmer must use the nbDeallocatePacketDumpFilePcap() function.
*/

class DLL_EXPORT nbPacketDumpFilePcap
{
public:
	nbPacketDumpFilePcap() {};
	virtual ~nbPacketDumpFilePcap() {};

	/*!
		\brief Open an existing capture file.

		This method allows opening a capture file that is already on disk.
		Please note that this file is 'read-only', i.e. we cannot append packets to it.

		\param FileName: name of the file that has to be opened.

		\param CreateIndexing: 'true' if we want to create an index in memory. When the indexing
		is turned on, we can get random access to the packets in there through the GetPacket() method.
		Vice versa, only sequential access (through GetNextPacket) is allowed.

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int OpenDumpFile(const char* FileName, bool CreateIndexing= false)= 0;

	/*!
		\brief Create a new capture file.

		This method allows creating a new capture file on disk. If an existing file already exists,
		it is overwrittem.

		Please note that this file is opened in read-write mode (i.e. you can read previously written data).

		\param FileName: name of the file that has to be created.

		\param LinkLayerType: sets the link layer type of the captured packets. This parameter is needed
		in order to be able to read packets according to the correct link-layer type.

		\param CreateIndexing: 'true' if we want to create an index in memory. When the indexing
		is turned on, we can get random access to the packets in there through the GetPacket() method.
		Vice versa, only sequential access (through GetNextPacket) is allowed.

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int CreateDumpFile(const char* FileName, int LinkLayerType, bool CreateIndexing= false)= 0;

	/*!
		\brief Close a dump file, either created through the CreateDumpFile(), or opened through the OpenDumpFile().

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int CloseDumpFile()= 0;

	/*!
		\brief Return the link layer type of the packets stored in the current dump file.

		Values are defined according to the well-known libpcap/Winpcap convention; returned
		values are the ones available in #nbNetPDLLinkLayer_t.

		\param LinkLayerType Upon return, it will contain the link-layer type of the
		packets stored in the current dump file.

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int GetLinkLayerType(nbNetPDLLinkLayer_t &LinkLayerType)= 0;

	/*!
		\brief Append a packet to a newly created capture dump.

		\param PktHeader: structure containing the packet header, according to the WinPcap/libpcap format.

		\param PktData: buffer containing the packet dump, in hex.

		\param FlushData: 'true' if we want to flush data to disk immediately, without relying on the
		buffering provided by the operating system. By default, this parameter is turned 'false',
		which guarantees better performance.
		In this case you may experience some delays in writing data to disk, as the operating system
		can wait till some tens of KBytes are buffered before dumping them on disk.

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int AppendPacket(const struct pcap_pkthdr* PktHeader, const unsigned char* PktData, bool FlushData= false)= 0;

	/*!
		\brief Return the packet selected through the 'PacketNumber' parameters.

		Packet numbering starts from '1'. Please note that this function succeeds only if the indexing is turned on.

		\param PacketNumber: ordinal number of the packet that has to be returned.

		\param PktHeader: pointer to a structure pcap_pkthdr, passed by reference, that will contain the packet header
		when the function returns.

		\param PktData: a pointer, passed by reference, that will contain the packet dump (in hex)
		when the function returns.

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int GetPacket(unsigned long PacketNumber, struct pcap_pkthdr** PktHeader, const unsigned char** PktData)= 0;

	/*!
		\brief Return the next packet in the current packet dump.

		\param PktHeader: pointer to a structure pcap_pkthdr, passed by reference, that will contain the packet header
		when the function returns.

		\param PktData: a pointer, passed by reference, that will contain the packet dump (in hex)
		when the function returns.

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int GetNextPacket(struct pcap_pkthdr** PktHeader, const unsigned char** PktData)= 0;

	/*!
		\brief Remove a packet from the current capture dump.

		This function succeeds only if the indexing is turned on. In this case, the packet index is updated
		so that packet 'N+1' becomes now 'N'.

		Please note that this function does not remove the packet, but it cancels it from the packet dump.
		Therefore the file needs to be saved again in order to apply these changes.

		\param PacketNumber: ordinal number of the packet that has to be removed.

		\return nbSUCCESS if everything is fine, nbWARNING if no more packets are currenty available in
		the current file (i.e., no packet has been returned, but this does not represent an error as we 
		reached the end-of-file), nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int RemovePacket(unsigned long PacketNumber)= 0;

	//! Return the error messages (if any)
	virtual char *GetLastError()= 0;
};



/*!
	\brief It creates a new instance of a nbPacketDumpFilePcap object and returns it to the caller.

	This function is needed in order to create the proper object derived from nbPacketDumpFilePcap.
	For instance, nbPacketDumpFilePcap is a virtual class and it cannot be instantiated.
	The returned object has the same interface of nbPacketDumpFilePcap, but it is a real object.

	A nbPacketDumpFilePcap class aims at managint capture files according to the WinPcap/libpcap format.

	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will eventually 
	keep an error message (if one).

	\param ErrBufSize: the length of the buffer that keeps the error message.

	\return A pointer to real object that has the same interface of the nbPacketDumpFilePcap abstract class,
	or NULL in case of error. In case of failure, the error message is returned into the ErrBuf buffer.

	\warning Be carefully that the returned object must be deallocated through the nbDeallocatePacketDumpFilePcap().
*/
DLL_EXPORT nbPacketDumpFilePcap* nbAllocatePacketDumpFilePcap(char *ErrBuf, int ErrBufSize);


/*!
	\brief It deallocates the instance of nbPacketDumpFilePcap created through the nbAllocatePacketDumpFilePcap().

	\param PacketDumpFilePcap: pointer to the object that has to be deallocated.
*/
DLL_EXPORT void nbDeallocatePacketDumpFilePcap(nbPacketDumpFilePcap* PacketDumpFilePcap);

/*!
	\}
*/

