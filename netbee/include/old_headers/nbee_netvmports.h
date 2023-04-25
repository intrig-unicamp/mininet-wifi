/*
 * Copyright (c) 2002 - 2004
 * NetGroup, Politecnico di Torino (Italy)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Politecnico di Torino nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */


#ifndef __NETBEE_NETVMPORTS__
#define __NETBEE_NETVMPORTS__


#if _MSC_VER > 1000
#pragma once
#endif

#ifdef NETPDL_EXPORTS
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
		#define DLL_EXPORT __declspec(dllexport)
		#define DLL_FILE_NAME "NetBee.dll"
	#endif
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
		#define DLL_EXPORT
		#define DLL_FILE_NAME ""
	#endif
#endif

/*!
\mainpage NetBee Exported Interface

	This module keeps all the classes that are exported by NetBee.

		\ref NetBeeNetVM

		This section provides some classes that can be used to interact with the NetVM virtual machine. These
		classes are rather low-level classes because basically provides a C++ interface to the NetVM and it defines
		some components (NetVM, NetPEs, Ports) that must be used to control the machine.<p>
		These classes provides the maximum flexibility in terms of controlling the behaviour of the NetVM, e.g. in
		terms of NetPE installed and the program executed by each of them, altough they are not very simple to use.<p>
		For standard applications, programmers do not have to use the primitives defined in this section; they should use the 
		classes defined in section NetBeePacketProcessing, which are appropriate for most of the needs of
		standard network applications.
*/



/*! \addtogroup NetBeeNetVM 
	\{
*/


////////////////////////////
//   Typedef definition   //
////////////////////////////

#define nbSUCCESS 0		///< Doc
#define nbFAILURE -1	///< Doc
#define nbNO_DATA -2	///< Doc


/*!
	\brief Buffer used in conjunction with a NetPE port
*/
typedef u_int8_t nbNetVmData;

/*!
	\brief Header of a buffer used in conjunction with a NetPE port
*/
typedef nvmExchangeBufferHeader nbNetPktHdr;

/*!
	\brief Callback called by a collector Data Device, or by a Port.
	
	This is the callback function that gets called when a collector or a push 
	port should report some data to the application.
	This function must be implemented by an application that uses an 
	GenericDataDevice working in collector push mode to receive data
	(the callback should be registered with 
	\ref nbGenericDataDevice::RegisterDataDstCallback()), or by another 
	deriving from the class \ref nbPort and wanting to receive data through
	a callback.
	
	This function is applicable only in case of a PUSH connection.

	\note The user-defined callback should keep into consideration the flags 
	passed in the flag parameter, TODO!! in particular it must check if the 
	bit "NET_VM_EXPORTER_CB_CAN_RETAIN_DATA" is set 
	(*flag & NET_VM_CALLBACK_CAN_RETAIN_DATA); if set, it can hold the received
	data by setting the bit "NET_VM_EXPORTER_CB_RETAIN_DATA" in the flag 
	parameter (*flag |= NET_VM_EXPORTER_CB_RETAIN_DATA;).
	
	\param CustomPtr void pointer that was passed to 
			\ref nbGenericDataDevice:: RegisterDataDstCallbackFunction()
			when the callback was registered.
	\param PacketHdr Contains various informations relative to the PacketData
			field TODO: it must be an header of some sort, this is weird GV
	\param PacketData data passed to the receiver callback
	\param Flag [in/out] used to pass additional parameters in and out of the 
			callback
	\return 
			- \ref nbSUCCESS if everything is fine, 
			- \ref nbFAILURE in case of errors.

	\note This callback is a duplication of the one available in NetVM.h. Only
			for documentation requirements.
*/
typedef u_int32_t (nbDataDstCallback)(void *CustomPtr, nbNetPktHdr PacketHdr, void* PacketData, int* Flag);

/*!
	\brief Callback called by an exporter Data Device or a nbPort.
	
	This is the function that gets called  port when an exporter or a pull 
	port must obtain some data from the outside world. This function must be 
	implemented by an application that uses an GenericDataDevice 
	working in exporter pull mode to send data (the callback should be 
	registered with \ref nbGenericDataDevice::RegisterDataSrcCallback()),
	or by another class deriving from the class nbPort and wanting to send 
	data through a callback.
	
	This function is applicable only in case of a PULL connection.

	\note the user-defined callback should keep into consideration the flags 
			passed in the flag parameter, for the copy semantics of this method

	\param CustomPtr void pointer that was passed to 
			\ref nbGenericDataDevice::RegisterDataSrcCallback() 
			when the callback was registered.
	\param PacketHdr [out] pointer to a structure that contains various 
			informations relative to the PacketData field 
			TODO: it must be an header of some sort, this is weird GV
	\param PacketData [out] data passed back to the caller
	\param Flag [in/out] used to pass additional parameters in and out of the 
			callback
	\return 
		- \ref nbSUCCESS if everything is fine, 
		- \ref nbFAILURE in case of errors, 
		- \ref nbNO_DATA if the callback cannot provide any data.

	\note This callback is a duplication of the one available in NetVM.h. 
			Only for documentation requirements.
*/
typedef u_int32_t (nbDataSrcCallback) (void* CustomPtr, nbNetPktHdr *PacketHdr, void **PacketData, int* Flag); 

/*!
	\brief GV_NOT_TOUCHED Class keeping the addresses associated to a network interface.
	In case the interface has more addresses, the 'Next' pointer allow to store them all.
*/
class DLL_EXPORT nbNetAddress
{
public:
	nbNetAddress();
	virtual ~nbNetAddress();

	/*!
		\brief Clone the content of the current class (including all objects pointed by the 'Next' member).

		This method clones the entire content of the current class and it returns a pointer to a new object.
		The new object (and its entire content) is still valid also if the original object is being destroyed.

		The user must deallocate the returned object manually, calling the DeleteClone() method.
	*/
	nbNetAddress* Clone();

	/*!
		\brief Delete the nbNetAddress pointer passed as parameter.
		\param NetAddress: pointer to the object that has to be deallocated.
		\note This method has to be called only for the pointers cloned through the Clone() method.
		TODO: NON HO ASSOLUTAMENTE CAPITO L'UTILITA' DI UN METODO DEL GENERE.
	*/
	static void DeleteClone(nbNetAddress* NetAddress) { if (NetAddress) delete NetAddress;};

	//! Return a string keeping the last error message that occurred within the current instance of the class.
	char *GetLastError() {return m_errbuf;}

public:
	int AddressFamily;		//!< Address Family (AF_INET in case of an IPv4 address, AF_INET6 in case of an IPv6 address).
	char *Address;			//!< Pointer to the numeric address in its 'printable' form (e.g. '10.1.1.1').
	char *Netmask;			//!< Pointer to the netmask it its 'printable' form (e.g. '255.0.0.0'); valid only in case of IPv4 addresses.
	int PrefixLen;			//!< Prefix length; valid only in case of IPv4 addresses.
	nbNetAddress *Next;		//!< Pointer to another address; this is used to create a linked list of addresses for interfaces that have more than one address.

private:
	char m_errbuf[2048];	//!< Buffer that keeps the error message (if any)
};

/*!
	\brief It contains the various types for a \ref nbPort
*/
enum nbPortType
{
	/*!
		\brief An input port, i.e. a port that feeds data into a \ref nbNetPe
			or a \ref nbDataDevice	
	*/
	nbINPUT_PORT	= 0,	
	/*!
		\brief An output port, i.e. a port that a \ref nbNetPe or a \ref 
			nbDataDevice uses to send data out of itself.
	*/
	nbOUTPUT_PORT	= 1,	
};

/*!
	\brief It represents the various types for a Data Device (\ref nbDataDevice)

	\note Values are mapped directly to the ones of the enumaration \ref nbPortType.
*/
enum nbDataDeviceType
{
	/*!
		\brief An exporter \ref nbDataDevice, i.e. an interface by which an 
			application, a network adapter, or any other entity can export data
			to the NetVM.
	*/
	nbEXPORTER		= nbOUTPUT_PORT,	
	/*!
		\brief A collector \ref nbDataDevice, i.e.  an interface by which an 
			application, a network adapter or any other entity collects data 
			from the NetVM.
	*/
	nbCOLLECTOR		= nbINPUT_PORT,
};

/*!
	\brief It represents the possible types of a connection between 
		\ref nbPort's.
*/
enum nbConnectionType
{
	/*!
		\brief A push connection, i.e. a connection where the data sender 
			"pushes" data to the receiver by sending a "data ready" signal.
	*/
	nbPUSH_CONNECTION	= 0,	
	/*!
		\brief A pull connection, i.e. a connection where the data receiver
			requests data from the sender by sending a "data request" signal.
	*/
	nbPULL_CONNECTION	= 2,
};



/*!
	\brief This structure represents the statistics that can be retrieved out
			of a \ref nbPort.
*/
typedef _nbPortStatistics
{
	/*!
		Number of bytes received from the connection attached to this port.
	*/
	u_int64_t	ReceivedBytes;		

	/*!
		Number of buffers (usually packets) received from the connection 
		attached to this port.
	*/
	u_int64_t	ReceivedBuffers;

	/*!
		Number of bytes trasmitted through the connection attached to this 
		port.
	*/
	u_int64_t	TransmittedBytes;

	/*!
		Number of buffers (usually packets) transmitted through the connection
		attached to this port.
	*/
	u_int64_t	TransmittedBuffers;

	/*!
		Type of the port. Possible values are
			- \ref nbINPUT_PORT
			- \ref nbOUTPUT_PORT
	*/
	nbPortType	PortType;
	/*!
		Type of the connection. Possible values are:
			- \ref nbPUSH_CONNECTION
			- \ref nbPULL_CONNECTION
	*/
	nbConnectionType	ConnectionType;

	//More statistics for a port?
}
	nbPortStatistics;

/*!
	\brief It represents a port, the basic entity to enable the communication
			between NetPEs (\ref nbNetPe) and DataDevices (\ref nbDataDevice).

	\note
	-	This class is not instanceable directly, but only through a class that
		derives from it. This is used to better insulate the concept of port. 
	-	This class has all the methods to transmit and receive data on the 
		connection, whose format is proprietary and not known to the 
		application (\ref Read(), \ref Write(), \ref RegisterDataSrcCallback(), 
		\ref RegisterDataDstCallback()). These methods are protected, so that an 
		application that owns a reference to a \ref nbPort cannot directly write
		or read to the connection. Only a class deriving from \ref nbPort has 
		access to these methods. This level of protection allows to optimize 
		the implementation of the connections, since applications do not have 
		direct control to the data flowing on the connection.
*/
class DLL_EXPORT nbPort
{
	/*!
		Used to allow the \ref nbNetVM class to access the internals of a port.
	*/
	friend class nbNetVM;

public:
	virtual ~nbPort();

protected:
	/*!
		\brief It creates a new instance of a port. 

		\param PortType Type of the port. Possible values are
			- \ref nbINPUT_PORT
			- \ref nbOUTPUT_PORT
	*/
	nbPort(nbPortType PortType);

	/*!
		\brief Reads from a port. The type of this port should be pull input.

		It reads a data block (that could be a packet or a table of 
		information) from this pull input port, that is connected to a pull 
		output port.

		\param PacketHdr Contains various information about the data contained
				in PacketData
		\param PacketData Address of a user allocated to the pointer that will 
				be filled with the address of a chunk of memory which contains
				the data block (more documentation about the copy semantics of
				this call will be provided later)
		\param Flags (in/out) this parameter is used to control the behavior 
				of the read operation, and to retrieve back some info about 
				that. At the moment it is not used and can be set to NULL. 
				(TODO Flags in \ref Read() )

		\return 
				- \ref nbSUCCESS if everything is fine, 
				- \ref nbFAILURE in case of errors.
				In case of error, the error message can be retrieved by the 
				\ref GetLastError() method.

		\note This method will fail if the port is not a pull input one.
	*/
	int Read(nbNetPktHdr **PacketHdr, void **PacketData, int *Flags);

	/*!
		\brief Writes to a port. The type of this port should be push output.

		It writes a data block (that could be a packet or other stuff) to this 
		push output port, that is connected to push input port.

		\param PacketHdr it contains information about the data to be written.
		\param PacketData pointer to the memory which contains the data block
				to be written 
		\param Flags (in/out) this parameter is used to control the behavior 
				of the Write operation, and to retrieve back some info about 
				that. At the moment it is not used and can be set to NULL. 
				(TODO Flags in \ref Write() )

		\return 
				- \ref nbSUCCESS if everything is fine, 
				- \ref nbFAILURE in case of errors.
				In case of error, the error message can be retrieved with the 
				\ref GetLastError() method.

		\note This method will fail if the port is not a push output one.
	*/
	int Write(nbNetPktHdr PacketHdr, void* PacketData, int *Flags);

	/*!
		\brief Registers the callback that will be called by the port when it 
				needs some data to be fed into the connection.

		This function registers the function that is called by the port to 
		inject data into the connection of the port. When the registered 
		function is called, it should prepare a packet and return it through
		the appropriate parameters of the callback.
		If no packet is available, the callback should return \ref nbNO_DATA.

		\param CallbackFunction pointer to the function that has to be called
				when some data need to be fed into the connection used by the 
				port.
		\param CustomPtr Void pointer that will be passed unmodified to the 
				callback in order to customize its behaviour.

		\return 
				- \ref nbSUCCESS if everything is fine, 
				- \ref nbFAILURE in case of errors.
				In case of error, the error message can be retrieved with the
				\ref GetLastError() method.

		\note This method will fail if the port is not a pull output one.
	*/
	int RegisterDataSrcCallback(nbDataSrcCallback CallbackFunction, void* CustomPtr);

	/*!
		\brief Registers the callback that will be called by the port when some
				data are available to be processed at this endpoint of the 
				connection.

		This function registers the function that is called by the port to 
		report some available data into the connection of the port. When the 
		registered function is called, it should process the data, and return
		\ref nbSUCCESS. The callback should return \ref nbFAILURE only in case
		of a severe internal error that prevents it from working correclty on 
		any further call.

		\param CallbackFunction Pointer to the function that has to be called
				when some data are available in the port.
		\param CustomPtr void pointer that will be passed unmodified to the 
				callback in order to customize its behaviour.

		\return 
			- \ref nbSUCCESS if everything is fine, 
			- \ref nbFAILURE in case of errors.
				In case of error, the error message can be retrieved with the
				\ref GetLastError() method.

		\note This method will fail if the port is not a push input one.
	*/
	int RegisterDataDstCallback(nbDataDstCallback CallbackFunction, void* CustomPtr);


	///*!
	//	\ brief Gets the Port information data structure.

	//	\return A pointer to the Port information data structure, or NULL in case of errors.
	//
	//	This method has been removed (in this form) as it exposed the internals of the port. 
	//	The following new method exposes a custom structure of information.

	//	nvmExchangePortState *GetPortInfo(void) { return m_PortStruct; };
	//*/

	/*!
		\brief Returns TRUE if the entity owning the port (e.g. a NetPe or a 
			Data Device) has been started or not.

		\return
			- TRUE if the entity owning the port has been started.
			- FALSE if the entity owning the port has not been started, yet.
	*/
	virtual bool IsStarted();

public:

	/*!
		\brief Connects two ports

		It connects two ports exported by NetPEs or Data Devices. 

		\param firstPort pointer to the first port to be connected.
		\param secondPort pointer to the second port to be connected.
		\param ConnectionType Type of connection. Possible values are:
			- \ref nbPUSH_CONNECTION
			- \ref nbPULL_CONNECTION
		
		\return 
			- \ref nbSUCCESS if everything is fine, 
			- \ref nbFAILURE in case of errors.
			In case of error, the error message can be retrieved with the 
			\ref GetLastError() method on either the two ports.
	*/
	static int Connect(nbPort *firstPort, nbPort *secondPort, nbConnectionType ConnectionType);

	/*!
		\brief It returns some statistics about the port 

		\return the tx/rx statistics about the current port
	*/
	nbPortStatistics* GetPortStatistics(void);

	/*!
		\brief It returns the port that is at the other end of the connection.

		\return A reference to the other port, or NULL if the port is not 
				connected.
	*/
	nbPort* GetConnectedPort(void);

	/*!	
		\brief Returns a string keeping the last error message that occurred
				within the current instance of the class.
	*/
	char *GetLastError() { return m_errbuf; }

protected:
	char m_errbuf[2048];							//!< Buffer that keeps the error message (if any)
private:
	//Dependent on the implementation. However, they should not be exposed, even to a derived class.
	nvmExchangeBufferState *m_OwnedExchangeBuffer; //!< exchange buffer pointer
	nvmExchangePortState *m_PortStruct;				//!< Port's data structure

};

// To avoid a compiler error
class nbNetPe;

/*!
	\brief Default port index for \ref nbDataDevice::GetPort()
*/
#define nbDEFAULT_PORT	0

#define nbDEFAULT_PORT_NAME	"default_port"

/*!	
	\brief This class represents a generic entity that acts as an "impedance" 
			adapter between the NetVM and any external entity (a trace file, 
			an application, a NIC)

	\note It is not a specialization of the \ref nbPort class, rather it 
			contains a \ref nbPort, and exports it through \ref GetPort(). We 
			use a nested, private class to have an internal \ref nbPort 
			internally. This naive approach avoids the misconception that a 
			Data Device is a port. 
*/
class DLL_EXPORT nbDataDevice
{
public:

	/*!
		\brief It returns one of the \ref nbPort associated with this data 
			device.

		\param Index Index of the port to be returned. If this parameter is
			omitted, the method uses the default value \ref nbDEFAULT_PORT,
			which is equal to 0.

		\return A reference to a valid nbPort, or NULL.
	*/
	virtual nbPort* GetPort(unsigned int Index = nbDEFAULT_PORT) = 0;

	/*!
		\brief It returns one of the \ref nbPort associated with this data 
			device.

		\param Name NULL terminated string containing the name of the port
			that has to be returned. The name of the default port is defined
			as \ref nbDEFAULT_PORT_NAME.

		\return A reference to a valid nbPort, or NULL.
	*/
	virtual nbPort* GetPort(char* Name) = 0;

public:

	/*!
		\brief GV_NOT_TOUCHED Clone the content of the current class (including all objects pointed by the 'Next' member).

		This method clones the entire content of the current class and it returns a pointer to a new object.
		The new object (and its entire content) is still valid also if the original object is being destroyed.

		The user must deallocate the returned object manually, calling the DeleteClone() method.

		\return A pointer to a cloned object.
		The object returned by this method belongs to the same class of the calling object. To make the returned
		pointer usable, the user may need to cast this object in the proper class object.<br>	
		In case of error, NULL is returned and the error message can be retrieved by the GetLastError() method.
	*/
	virtual nbDataDevice *Clone()= 0;

	/*!
		\brief GV_NOT_TOUCHED Delete the nbNetDevice pointer passed as parameter.
		\param DataDevice pointer to the object that has to be deallocated.
		\note This method has to be called only for the pointers cloned through the Clone() method.

		Non ho capito assolutamente che senso abbia. Perche' devo usare un metodo per la cancellazione del clone??
		L'implementazione non deve essere qui, se non si volgiono avere casini con il C RunTime
	*/
	void DeleteClone(nbDataDevice* DataDevice) { if (DataDevice) delete DataDevice; };
};

/*!
	\brief This class represents a Data Device used by an application to 
			send/receive data.

	This class can be used to implement a new Data Device, by specializing
	or containing it. As a matter of facts, it acts as a mere wrapper over the 
	nbPort class. We have this wrapper in order to insulate the concept of Port
	from the one of Data Device (that acts as a port on one side, only).
*/
class nbGenericDataDevice : public nbDataDevice
{
private:
	/*!
		\brief Nested class used to access the protected methods of \ref 
		nbPort. It's declared private since the external world should not be 
		able to create it (and eventually access some methods exported by this
		class).
		
		\note An alternative way, to avoid this nested class, would be
		to declare \ref nbGenericDataDevice as a friend class of 
		\ref nbPort. This solution was not chosen since \ref nbPort would 
		"smell" too much because of nonsense friend declarations.
	 */
	class nbInternalPort : public nbPort
	{
		/*!
			\brief This friend is necessary so that the containing class can
			access all the members/methods of the contained class.
		 */
		friend class nbGenericDataDevice;
	};
	
	/*!
		\brief Internal port. Used to connect this Data Device to another
		Data Device or to a port of a NetPE.
	*/
	nbInternalPort m_Port;

public:

	virtual int Start();  ///< Documented in the base class.
	virtual int Stop();	  ///< Documented in the base class.

	/*!
		\brief It creates a new instance of a Generic Data Device.
		\param type One of the possible types of a DataDevice:
			- \ref nbCOLLECTOR
			- \ref nbEXPORTER
	*/
	nbGenericDataDevice(nbDataDeviceType type);
	
	nbPort* GetPort(unsigned int Index);	///< Documented in the base class.
	nbPort* GetPort(char* Name);	///< Documented in the base class.
	
	/*!
		\brief It writes data into an push exporter Data Device.

		It writes a data block (that could be a packet or other stuff) to this 
		Data Device.

		\param PacketHdr It contains information about the data to be written.
		\param PacketData Pointer to the memory which contains the data block 
				to be written 
		\param Flags (in/out) this parameter is used to control the behavior 
				of the Write operation, and to retrieve back some info about 
				that. At the moment it is not used and can be set to NULL. 
				(TODO Flags in \ref Write() )

		\return 
				- \ref nbSUCCESS if everything is fine, 
				- \ref nbFAILURE in case of errors.
				In case of error, the error message can be retrieved with the 
				\ref GetLastError() method.

		\note This method will fail if the DataDevice is not a push 
				exporter one.
	*/
	inline int Write(nbNetPktHdr PacketHdr, void* PacketData, int *Flags)
	{
		//TODO the code should not be here, and we need to propagate the error outside the port
		return m_Port.Write(PacketHdr, PacketData, Flags);
		
	}

	/*!
		\brief It reads data from a pull collector Data Device.

		It reads a data block (that could be a packet or a table of 
		information) from this Data Device.

		\param PacketHdr Contains various information about the data contained
				in PacketData
		\param PacketData Address of a user allocated to the pointer that will 
				be filled with the address of a chunk of memory which contains
				the data block (more documentation about the copy semantics of
				this call will be provided later)
		\param Flags (in/out) this parameter is used to control the behavior 
				of the read operation, and to retrieve back some info about 
				that. At the moment it is not used and can be set to NULL. 
				(TODO Flags in \ref Read() )

		\return 
				- \ref nbSUCCESS if everything is fine, 
				- \ref nbFAILURE in case of errors.
				In case of error, the error message can be retrieved by the 
				\ref GetLastError() method.

		\note This method will fail if the Data Device is a pull 
				collector one.
	*/
	inline int Read(nbNetPktHdr **PacketHdr, void **PacketData, int *Flags)
	{
		//TODO the code should not be here, and we need to propagate the error outside the port
		return m_Port.Read(PacketHdr, PacketData, Flags); 
	}

	/*!
		\brief Registers the callback that will be called by the data device 
			when some data are available to be processed.

		This function registers the function that is called by the data device
		to report some available data into the underlying connection
		between this data device and another data device or a NetPE. When the 
		registered function is called, it should process the data, and return
		\ref nbSUCCESS. The callback should return \ref nbFAILURE only in case
		of a severe internal error that prevents it from working correclty on 
		any further call.

		\param CallbackFunction pointer to the function that has to be called 
				when some data are available in the Data Device.
		\param CustomPtr void pointer that will be passed unmodified to the 
				callback in order to customize its behaviour.

		\return 
				- \ref nbSUCCESS if everything is fine, 
				- \ref nbFAILURE in case of errors.
				In case of error, the error message can be retrieved with the 
				\ref GetLastError() method.

		\note This method will fail if the Data Device is not a push 
				collector one.
	*/
	int RegisterDataDstCallback(nbDataDstCallback *CallbackFunction, void *CustomPtr)
	{
		//TODO the code should not be here, and we need to propagate the error outside the port
		return m_Port.RegisterDataDstCallback(CallbackFunction, CustomPtr); 
	}

	/*!
		\brief Register the callback that will be called by the data device to
			feed data into the connection in case of a pull exporter Data 
			Device.

		This function registers the function that is called by the port to 
		inject data into the underlying connection between this data device and 
		another data device or a NetPE. When the registered function is called, 
		it should prepare a packet and return it through the appropriate 
		parameters of the callback. If no packet is available, the callback 
		should return \ref nbNO_DATA.

		\param CallbackFunction pointer to the function that has to be called
				when some data need to be fed into the Data Device.
		\param CustomPtr void pointer that will be passed unmodified to the 
				callback in order to customize its behaviour.

		\return 
				- \ref nbSUCCESS if everything is fine, 
				- \ref nbFAILURE in case of errors.
				In case of error, the error message can be retrieved with the
				\ref GetLastError() method.

		\note This method will fail if the Data Device is not a pull 
				exporter one.
	*/
	int RegisterDataSrcCallback(nbDataSrcCallback *CallbackFunction, void *CustomPtr)
	{
		//TODO the code should not be here, and we need to propagate the error outside the port
		return m_Port.RegisterDataSrcCallback(CallbackFunction, CustomPtr); 
	}

	/*!
		\brief Return a message containing the details of the last error 
			occurred in the Data Device.

		\return The error message
	*/
	char* GetLastError()
	{
		return m_Port.GetLastError();
	}

/*
	//codice vecchio di FULVIO

	//Puo' avere un senso dare un nome e/o descrizione di un Data Device?
	//tuttavia non so se ha senso avere un metodo initialize cosi' generico che riceve nomi di file o di adattatori
	int Initialize(char *Name, char *Description);

	//Sostituiti dai metodi  Start() e Stop() della porta.
	virtual int OpenAsDataSrc() = 0;
	virtual int OpenAsDataDst() = 0;
	virtual int Close() = 0;

	//non ho ben capito la loro utilita'. soprattutto adesso che la porta non coincide piu' con il Data Device
	virtual nbDataSrcPollingFunction *GetPollingFunction() = 0;
	virtual nbDataDstCallbackFunction *GetCallbackFunction()= 0;
	virtual void *GetFunctionCustomPointer() = 0;

public:
	//Vale il discorso su initialize. E in piu' non dovrebbero essere pubbliche. 
	char *Name;				//!< Name of the device (e.g. a network adapter) used as packet traffic source / destination.
	char *Description;		//!< Description of the device (e.g. a network adapter) used as packet traffic source / destination.

	//Quale e' l'utilita' di mantenere una lista di Data Device? (o DataDevice, che e' lo stesso).
	nbNetVmPortDataDevice *Next;
*/
};

/*!
	\brief This structure defines the statistics that can be retrieved from 
		a trace file class.
*/
typedef struct _nbTraceFileStatistics
{
	/*!
		Size of the file, if opened in read mode.
		If the file was opened in write mode, this field is meaningless, at 
		the moment.
		If the file size is unknown, this field has a value of -1.
	*/
	int64_t		FileSize;

	/*!
		Number of packets stored in a file opened in read mode.
		If such counter is unknown, this field has a value of -1.
	*/
	int64_t		TotalNumberOfPackets;
	
	/*!
		Number of bytes written to the file, if it was opened in write mode.
	*/
	u_int64_t	WrittenBytes;

	/*!
		Number of bytes read from the file, if it was opened in read mode.
	*/
	u_int64_t	ReadBytes;

	/*!
		Number of packets written to the file, if opened in write mode. 
		If the file was opened in append mode, this counter does NOT take
		the packets already existent written to the file into consideration.
	*/
	u_int64_t	WrittenPackets;

	/*!
		Number of packets read so far from the file, if opened in read mode.
	*/
	u_int64_t	ReadPackets;
	//TODO more to be defined
}
	nbTraceFileStatistics;

/*!
	\brief It represents the possible open modes for a trace file.
*/
enum nbTraceFileMode
{
	/*!
		The file should be opened in read mode. If the file does not exist,
		the method using this flag will fail.
	*/
	nbFILEMODE_READ,	
	/*!
		The file should be opened in write mode. If the file already exists,
		the method using this flag will fail.
	*/
	nbFILEMODE_WRITE,
	/*!
		The file should be opened in append mode. If the file already exists,
		the new packets will be appended at the end of the file.
	*/
	nbFILEMODE_APPEND,
	/*!
		The file should be opened in oveerwrite mode. If the file does not 
		exist, the method using this flag will fail.
	*/
	nbFILEMODE_OVERWRITE
	//TODO more to be defined
};

/*!
	\brief It represents the informations that can be set and retrieved to/from a 
	trace file.
*/
typedef struct _nbTraceFileInfo
{
	/*!
		Name of the network device used for the capture, or NULL.
	*/
	char* NetworkDeviceName;		
	/*!
		Description of the network device used for the capture, or NULL.
	*/
	char* NetworkDeviceDescription;
	/*!
		Name of the operating used to dump the capture, or NULL.
	*/
	char* OperatingSystem;

	/*!
		List of the network address(es) associated to the device used for the
		capture, or NULL.
	*/
	nbNetworkAddress *Addresses;
	//TODO more to be defined
}
	nbTraceFileInfo;

/*!
	\brief This class represents a trace file, either in read mode or in write
		mode.
*/
class DLL_EXPORT nbTraceFileDataDevice : public nbDataDevice
{
private:
	/*!
		\brief Nested class used to access the protected methods of 
		\ref nbPort. It's declared private since the extenral world should not
		be able to create it (and eventually access some methods exported by 
		this class).
	 */
	class nbInternalPort : public nbPort
	{
		/*!
			Needed to allow the outer class to access every method of the port
			class.
		*/
		friend nbTraceFileDataDevice;

	};

	/*!
		\brief Internal port. Used to connect this Data Device to another
		Data Device or to a port of a NetPE.
	*/
	nbInternalPort *m_Port;

public:

	/*!
		TODO Implementation problem. At the moment it's not clear if such
		method is needed for a trace file.

		\return 
				- \ref nbSUCCESS if everything is ok.
				- \ref nbFAILURE otherwise.
	*/
	virtual int Stop();

	/*!
		TODO Implementation problem. At the moment it's not clear if such
		method is needed for a trace file.

		\return 
				- \ref nbSUCCESS if everything is ok.
				- \ref nbFAILURE otherwise.
	*/
	virtual int Start();

	/*!
		Constructor. The construction of an instance of this class is done in
		two steps: first the object is created, then it's initialized with 
		\ref Initialize(), passing the name and mode of the trace file.
	*/
	nbTraceFileDataDevice();

	/*!
		\brief It initializes the internal structure of the class.

		\param FileName Name of the tracefile
		\param FileMode Possible values are the ones declared in the 
			\ref nbTraceFileMode enumeration, that are:
			- \ref nbFILEMODE_READ
			- \ref nbFILEMODE_WRITE
			- \ref nbFILEMODE_APPEND
			- \ref nbFILEMODE_OVERWRITE.

		\return
			- \ref nbSUCCESS if everything was ok.
			- \ref nbFAILURE otherwise. In this case a more meaningful error 
			message can be retrieved using the method \ref GetLastError().
	*/
	int Initialize(char* FileName, nbTraceFileMode FileMode);

	nbPort* GetPort(unsigned int Index);	///< Documented in the base class.
	nbPort* GetPort(char* Name);	///< Documented in the base class.

	/*!
		\brief It returns some statistics about the data read/written to this
			trace file.
		
		\return A pointer to a \ref nbTraceFileStatistics structure containing
			the statistics for the current trace file, or NULL in case of 
			errors.
	*/
	nbTraceFileStatistics* GetStatistics();

	/*!
		\brief It returns some info about the trace file, e.g. the name of the
		network device used for the capture, its description, the OS...

		\return A pointer to a \ref nbTraceFileInfo structure containing the 
			info of the trace file, or NULL in case of errors.
	*/
	nbTraceFileInfo* GetInfo();

	/*!
		\brief It sets the info about the current trace info to the file. Such
			info include the name and description of the network device used 
			for the capture, and the operating system.

		\param TraceFileInfo A \ref nbTraceFileInfo structure containing the 
			infos that should be saved in the trace file.
		\return
			- \ref nbSUCCESS if the function succeeds.
			- \ref nbFAILURE otherwise. In this case a more meaningful error 
			message can be retrieved using the method \ref GetLastError().
	*/
	int SetInfo(nbTraceFileInfo TraceFileInfo);

	/*!
		\brief Return a message containing the details of the last error 
		occurred in the Data Device.

		\return The error message
	*/
	char* GetLastError();
};

/*!
	\brief It contains the flags that can be used on a local or remote network card.
*/
enum nbNetworkCardFlags
{
	//! The network interface should capture only traffic to/from this host.
	nbNETDEVICE_LOCALTRAFFIC= 1,
	//! Turned on when we want the network interface to be in 'promiscuous mode'.
	nbNETDEVICE_PROMISCUOUS= 2,
	//! Turned on when we want the network interface to capture only incoming traffic. 
	nbNETDEVICE_INBOUNDTRAFFIC= 4,
	//! Turned on when we want the network interface to capture only outcoming traffic.
	nbNETDEVICE_OUTBOUNDTRAFFIC= 8,
	//! Turned on when we want to use an unreliable connection for transferring packet back from the remote network device.
	//! This flag is meaningful only when capturing from a network interface on a remote machine, otherwise it is ignored.
	nbNETDEVICE_RPCAP_UDP= 16,
	//! Turned on when we want to use connect to a remote machine using the Remote Packet CApture Protocol (RPCAP) in active mode.
	//! I this case, the local machine waits for a network connection to be initiated by the remote machine, while usually is the local
	//! machine that starts the new connection toward the remote machine.
	//! This flag is meaningful only when capturing from a network interface on a remote machine, otherwise it is ignored.
	nbNETDEVICE_RPCAP_ACTIVE= 32
};

/*!
	\brief This structure contains the statistics that can be retrieved from a
		local or remote network card
*/
typedef struct _nbNetworkCardStatistics 
{
	/*!
		Number of packets received by the network card
	*/
	u_int64_t	ReceivedPackets;
	
	/*!
		Number of packets actually captured by the underlying captuing system.
	*/
	u_int64_t	CapturedPackets;
	
	/*!
		Number of bytes actually captured by the underlying capturing system.
	*/
	u_int64_t	CapturedBytes;
	
	/*!
		Number of packets dropped by the underlying capturing system for lack
		of space in the internal storage.
	*/
	u_int64_t	DroppedPackets;

	/*!
		Number of packets actually transmitted by the underlying transmitting
		system.
	*/
	u_int64_t	TransmittedPackets;
	
	/*!
		Number of bytes actually transmitted by the underlying transmitting
		system.
	*/
	u_int64_t	TransmittedBytes;
	
	/*!
		Number of packets not transmitted because of an error in the underlying
		transmitting system
	*/
	u_int64_t	NonTransmittedPackets;

	/*!
		Number of packet pending to be transmitted by the underlying 
		transmitting system.
	*/
	u_int64_t	PendingTransmitPackets;

	//TODO more to be defined
}
	nbNetworkCardStatistics;

/*!
	\brief It represents a network device (e.g. a physical network interface)
		present on the local machine.
*/
class DLL_EXPORT nbLocalNicDataDevice : public nbDataDevice
{
private:

	/*!
		\brief Nested class used to access the protected methods of 
			\ref nbPort. It's declared private since the extenral world should
			not be able to create it (and eventually access some methods 
			exported by this class).
	 */
	class nbInternalPort : public nbPort
	{
		/*!
			Needed to allow the outer class to access every method of the port
			class.
		*/
		friend nbLocalNicDataDevice;	

	};

public:

	/*!
		This overridden method actually starts the underlying capturing 
		system. 
		TODO implementation: we can choose to open the adapter with
		\ref nbLocalNicDataDevice::Initialize(), and enable 
		it/reset the filter/reset the kernel buffer here. Or we can choose 
		of opening the device and enable it here. If the open fails, we 
		simply return \ref nbFAILURE. Moreover, this method can be used to 
		spawn a thread that is used internally to interface the APIs used
		to receive packets, with the \ref nbPort. For example, if the user 
		wants a push port, we need a thread, since the normal APIs work in 
		pull mode (i.e. we perform a read to obtain data). In this case, we 
		should implement a thread that basically loops in this way:
		
			while(1)
			{
				receive_packet();
				nbPort::Write();
			}

		\return
			- \ref nbSUCCESS if the specific capturing system was opened 
			correctly
			- \ref nbFAILURE otherwise.
	*/
	virtual int Start();

	/*!
		This overridden method actually stops the underlying capturing 
		system. 
		TODO implementation: we can choose to close the adapter with
		in the distructor of the outer class, and disable it here in some
		way. Or we can choose of closing the device here. If we are using
		an internal thread to receive packets, this is the right moment
		to stop this thread.
		
		\return
			- \ref nbSUCCESS if the specific capturing system was opened 
			correctly
			- \ref nbFAILURE otherwise.
	*/
	virtual int Stop();

	/*!
		\brief It creates a new instance of the class. The construction of an
		instance of this class is done in two steps: first the object is 
		created, then it's initialized with \ref Initialize(), passing the 
		name, description and Flags for the network device.

		\param Type Type of the Data Device. Possible values are the
			ones declared in the enumeration \ref nbDataDeviceType, that 
			are:
			- \ref nbCOLLECTOR
			- \ref nbEXPORTER
			An exporter is used to receive data from the network.
			A collector is used to transmit data to the network.
	*/
	nbLocalNicDataDevice(nbDataDeviceType Type);

	/*!
		\brief It initializes the internal structure of the class.

		\param Name Name of the network device
		\param Description Description of the network device.
		\param Flags Possible values are the ones declared in the 
			\ref nbNetworkCardFlags enumeration, that are:
			- \ref nbNETDEVICE_LOCALTRAFFIC
			- \ref nbNETDEVICE_PROMISCUOUS
			- \ref nbNETDEVICE_INBOUNDTRAFFIC
			- \ref nbNETDEVICE_OUTBOUNDTRAFFIC
			- \ref nbNETDEVICE_RPCAP_UDP
			- \ref nbNETDEVICE_RPCAP_ACTIVE

		\return
			- \ref nbSUCCESS if everything was ok.
			- \ref nbFAILURE otherwise. In this case a more meaningful error
			message can be retrieved using the method \ref GetLastError().
	*/
	int Initialize(char *Name, char *Description, nbNetworkCardFlags Flags);

	/*!
		\brief It returns some statistics about the current network card, like
			the received and transmitted packets, the dropped packets...

		\return A pointer to a \ref nbNetworkCardStatistics structure, 
			containing all the statistics, or NULL in case of errors. A more
			meaningful error message can be obtained by calling method 
			\ref GetLastError().
	*/
	nbNetworkCardStatistics* GetStatistics();

	nbPort* GetPort(unsigned int Index);	///< Documented in the base class.
	nbPort* GetPort(char* Name);	///< Documented in the base class.

	/*!
		\brief Returns a message containing the details of the last error 
		occurred in the Data Device.

		\return The error message
	*/
	char* GetLastError();

	/*! 
		\brief It returns a list of network addresses associated to this device

		\return A list of the network addresses, or NULL in case of errors. A
			more meaningful error message can be retrieved by calling method
			\ref GetLastError()
	*/
	nbNetAddress* GetAddressList();

};

/*!
	\brief It represents a network device (e.g. a physical network interface)
		present on a remote machine and running a remote capturing system.
*/
class DLL_EXPORT nbRemoteNicDataDevice : public nbDataDevice
{
private:

	/*!
		\brief Nested class used to access the protected methods of 
			\ref nbPort. It's declared private since the extenral world should
			not be able to create it (and eventually access some methods 
			exported by this class).
	 */
	class nbInternalPort : public nbPort
	{
		/*!
			Needed to allow the outer class to access every method of the port
			class.
		*/
		friend nbRemoteNicDataDevice;

	};

public: 

	/*!
		This overridden method actually starts the underlying capturing 
		system. 
		TODO implementation: we can choose to open the adapter with
		\ref nbLocalNicDataDevice::Initialize(), and enable 
		it/reset the filter/reset the kernel buffer here. Or we can choose 
		of opening the device and enable it here. If the open fails, we 
		simply return \ref nbFAILURE.

		\return
		- \ref nbSUCCESS if the specific capturing system was opened 
			correctly
		- \ref nbFAILURE otherwise.
	*/
	virtual int Start();

	/*!
		This overridden method actually stops the underlying capturing 
		system. 
		TODO implementation: we can choose to close the adapter with
		in the distructor of the outer class, and disable it here in some
		way. Or we can choose of closing the device here. 

		\return
		- \ref nbSUCCESS if the specific capturing system was opened 
			correctly
		- \ref nbFAILURE otherwise.
	*/
	virtual int Stop();

	/*!
		\brief It creates a new instance of the class. The construction of an
		instance of this class is done in two steps: first the object is 
		created, then it's initialized with \ref Initialize(), passing all 
		the infos needed to open the remote adapter.

		\param Type Type of the Data Device. Possible values are the
			ones declared in the enumeration \ref nbDataDeviceType, that 
			are:
			- \ref nbCOLLECTOR
			- \ref nbEXPORTER
			An exporter is used to receive data from the network (using the
			network card on the remote machine).
			A collector is used to transmit data to the network (using the
			network card on the remote machine).
	*/
	nbRemoteNicDataDevice(nbDataDeviceType Type);

	/*!
		\brief Sets the parameter needed to open a given adapter.

		This method is used to specify which real adapter has to be used, after creating this class.
		
		//TODO Ho fatto copia e incolla, ma non capito esattamente cosa si intendesse.
		In case some higher-level primitives are used (e.g. the nbPacketCapture class), this method may be ignored because
		the user retrieves the list of the adapters from nbPacketCapture::GetListLocalAdapters() method.
		The objects returned by the previous methods can be immediately used in the following calls, therefore the Initialize()
		method may be useless.

		\param Name: name of the adapter.
		\param Description: user-friendly description of the adapter.
		\param Flags: a set of flags that can be used to customize how the adapter works. The list of allowed flags is
		specified in #nbNetVmPortDataDeviceFlags. Please be careful that some of them are meaningless in some configurations.
		\param Host: name of the host on which the adapter is installed.
		\param Port: network port, specified in case the user does not want to use the standard port to connect to the remote host.
		If the default port must be used, it can be NULL.
		\param AuthType: Authentication method. Currently it is bot implemented.
		\param AuthToken1: First authentication token (e.g. username). It can be NULL if this parameter is not required.
		\param AuthToken2: Second authentication token (e.g. password). It can be NULL if this parameter is not required.
		\param AuthToken3: Third authentication token (e.g. domain). It can be NULL if this parameter is not required.

		\return nbSUCCESS if everything is fine, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	int Initialize(char *Name, char *Description, nbNetworkCardFlags Flags, char *Host, char *Port, int AuthType, char *AuthToken1, char *AuthToken2, char *AuthToken3);

	/*!
		\brief It returns some statistics about the network card on the remote
			machine, like the received and transmitted packets, the dropped 
			packets...

		\return A pointer to a \ref nbNetworkCardStatistics structure, 
			containing all the statistics, or NULL in case of errors. A more
			meaningful error message can be obtained by calling method 
			\ref GetLastError().
	*/
	nbNetworkCardStatistics* GetStatistics();

	nbPort* GetPort(unsigned int Index);	///< Documented in the base class.
	nbPort* GetPort(char* Name);	///< Documented in the base class.

	/*!
		\brief Returns a message containing the details of the last error 
		occurred in the data device.

		\return The error message
	*/
	char* GetLastError();

	/*! 
		\brief It returns a list of network addresses associated to this device

		\return A list of the network addresses, or NULL in case of errors. A
			more meaningful error message can be retrieved by calling method
			\ref GetLastError()
	*/
	nbNetAddress* GetAddressList();

};


/*! \addtogroup NetBeeNetVM 
	\}
*/



#endif
