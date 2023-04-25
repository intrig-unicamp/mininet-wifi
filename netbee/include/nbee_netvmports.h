/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once

#ifdef NETPDL_EXPORTS
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT __declspec(dllexport)
	#endif
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT
	#endif
#endif


////////////////////////////
//   Typedef definition   //
////////////////////////////

typedef nvmExchangeBufferState nbNetPkt;

typedef uint32_t (nbDataDstCallbackFunction)(void* voidDummy, nvmExchangeBufferState *xbuffer, int* intDummy);

typedef uint32_t (nbDataSrcPollingFunction) (void * ptr, nvmExchangeBufferState *buffer, int *dummy); 


/*!
	\defgroup NetBeeNetVmPorts Classes for defining NetVM source / destination devices
*/

/*! \addtogroup NetBeeNetVmPorts 
	\{
*/

/*!
	\brief List of flags that can be used when opening a network device getting a list of them.
*/
enum nbNetVmPortDataDeviceFlags
{
	//! Turned on when the network device is a file; turned off when the network device is a physical interface.
	nbNETDEVICE_ISFILE= 1,
	//! Turned on when we want the network interface to be in 'promiscuous mode'. This flag does not have any effect when the network device is a file.
	nbNETDEVICE_PROMISCUOUS= 2,
	//! Turned on when we want the network interface to capture only incoming traffic. This flag does not have any effect when the network device is a file.
	nbNETDEVICE_INBOUNDTRAFFIC= 4,
	//! Turned on when we want the network interface to capture only outcoming traffic. This flag does not have any effect when the network device is a file.
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
	\brief Class keeping the addresses associated to a network interface.
	In case the interface has more addresses, the 'Next' pointer allow to store them all.
*/
class DLL_EXPORT nbNetAddress
{

public:
	
	nbNetAddress();
	virtual ~nbNetAddress(); 

	nbNetAddress *Clone();		//!< Clone class returning a pointer to the new istance
	static void DeleteClone(nbNetAddress* NetAddress) { if (NetAddress) delete NetAddress;} //!< Delete a clone instance of this class
	
	char *GetLastError() {return m_errbuf;} //!< Return the error messages (if any)

public:
	
	int		AddressFamily;		//!< Address Family (AF_INET in case of an IPv4 address, AF_INET6 in case of an IPv6 address).
	char	*Address;			//!< Pointer to the numeric address in its 'printable' form (e.g. '10.1.1.1').
	char	*Netmask;			//!< Pointer to the netmask it its 'printable' form (e.g. '255.0.0.0'); valid only in case of IPv4 addresses.
	int		PrefixLen;			//!< Prefix length; valid only in case of IPv4 addresses.
	nbNetAddress *Next;			//!< Pointer to another address; this is used to create a linked list of addresses for interfaces that have more than one address.

private:
	
	char m_errbuf[2048];	//!< Buffer that keeps the error message (if any)

};


/*!
	\brief Port informations data structure
*/
class DLL_EXPORT nbNetVmPortBase
{

public:
	
	nbNetVmPortBase();
	virtual ~nbNetVmPortBase() {};

	char *GetLastError() { return m_errbuf; } //!< Return the error messages (if any)

protected:
	
	char m_errbuf[2048];	//!< Buffer that keeps the error message (if any)
};

/*!
	\brief Class keeping the information related to generic External NetVM port 

	In case you want to list all the network devices present within a given system, the 'Next' pointer allow to store them all
	and to give access to them.
*/
class DLL_EXPORT nbNetVmPortDataDevice : public nbNetVmPortBase
{

friend class nbNetVm;

public:
	
	nbNetVmPortDataDevice();
	virtual ~nbNetVmPortDataDevice();
	
	void DeleteClone(nbNetVmPortDataDevice* NetDataDevice) { if (NetDataDevice) delete NetDataDevice; }	//!< Delete a clone instance of this class
	int Initialize(char *Name, char *Description); 								//!< Initialize Name and Description of device
	int RegisterCallbackFunction(nbDataDstCallbackFunction *DataDstCallbackFunction, void *CustomPtr); 	//!< register CallBack function over this port
	int RegisterPollingFunction(nbDataSrcPollingFunction *DataSrcPollingFunction, void *CustomPtr); 	//!< register Polling function over this port
	int ReleaseExBuf(nbNetPkt *Packet);									//!< release Exchange Buffer to NetVM
	nvmExternalPort * GetNetVmPortDataDevice() {return ExternalPort;} 					//!< Return pointer to NetVM external Port	 

public:

	char *Name;				//!< Name of the device (e.g. a network adapter) used as packet traffic source / destination.
	char *Description;			//!< Description of the device (e.g. a network adapter) used as packet traffic source / destination.
	nbNetVmPortDataDevice *Next;		//!< List of devices
	nvmExternalPort * ExternalPort;		//!<  NetVM Externalport designated to receive pkts.
	
protected:
	
	char m_errbuf[2048];			//!< Buffer that keeps the error message (if any)
};

/*!
	\brief Class keeping the information related to a Local network device (e.g. a physical network interface).
*/
class DLL_EXPORT nbNetVmPortLocalAdapter : public nbNetVmPortDataDevice
{

public:

	nbNetVmPortLocalAdapter();
	virtual ~nbNetVmPortLocalAdapter();

	nbNetVmPortDataDevice	*Clone(char *Name, char *Description, int Flags); 				//!< Clone this class returning a pointer to the new istance
	nbNetVmPortLocalAdapter *GetNext() {return (nbNetVmPortLocalAdapter *) Next; }				//!< return the pointer to next Local Adapter
	int	Initialize(char *Name, char *Description, int Flags);						//!< Initialize Name Description and Flags
	static uint32_t SrcPollingFunctionWinPcap(void * ptr, nvmExchangeBufferState *buffer, int *dummy); 	//!< Pointer to packet capture fucntion (WinPcap)	
	int	Write(nbNetPkt *PacketData);		//!< Write packet in the NetVM
	int Close();					//!< Close packet capture fucntion (WinPcap)

private:

	int OpenAsDataSrc();	//!< Initialize packet capture 					
	int OpenAsDataDst();	//!< not implemented;

public:

	int Flags;			//!< Flags that modify how the network device operates (e.g. promiscuous mode, etc)
	nbNetAddress *AddressList;	//!< List of addresses associated to the current interface
	void *m_WinPcapDevice;		//!< Pointer to WinPcapDevice
};

/*!
	\brief Class keeping the information related to a Remote network device (e.g. a physical network interface).
*/
class DLL_EXPORT nbNetVmPortRemoteAdapter : public nbNetVmPortDataDevice
{
public:
	nbNetVmPortRemoteAdapter();
	virtual ~nbNetVmPortRemoteAdapter();
	
	nbNetVmPortDataDevice *Clone(char *Name, char *Description, int Flags); 		//!< Clone this class returning a pointer to the new istance 
	nbNetVmPortRemoteAdapter *GetNext() {return (nbNetVmPortRemoteAdapter *) Next; }	//!< Return pointer to next remoteAdapter device
	int Initialize(char *Name, char *Description, int Flags, char *Host, char *Port, int AuthType, char *AuthToken1, char *AuthToken2, char *AuthToken3); 					//!<  Initialize parameter of Remote Adapter
	int Close(); //!< Not Implemented

private:

	int OpenAsDataSrc(); //!< Not Implemented 
	int OpenAsDataDst(); //!< Not Implemented

public:
	int Flags;				//!< Flags that modify how the network device operates (e.g. promiscuous mode, etc)
	char *Host;				//!< Keeps the host name, in case of a remote capture
	char *Port; 				//!< Keeps the port on the remote host, in case of a remote capture
	int AuthType;				//!< Keeps the authentication type, in case of a remote capture
	char *AuthToken1;			//!< Keeps the first authentication token (e.g. username) needed to authenticate user in case of a remote capture
	char *AuthToken2;			//!< Keeps the second authentication token (e.g. password) needed to authenticate user, in case of a remote capture
	char *AuthToken3;			//!< Keeps the third authentication token (e.g. domain) needed to authenticate user, in case of a remote capture
	nbNetAddress *AddressList;		//!< List of addresses associated to the current interface
};


//! Timestamp
struct nbTStamp
{
	uint32_t sec;		//!< Timestamp (in seconds since 1970)
	uint32_t usec;		//!< Timestamp (the decimal part, in microseconds)

	nbTStamp(uint32_t s, uint32_t us)
		:sec(s), usec(us){}
};

/*!
	\brief Class keeping the information related to an Application port 
*/
class DLL_EXPORT nbNetVmPortApplication : public nbNetVmPortDataDevice
{
public:
	
	nbNetVmPortApplication(void);
	~nbNetVmPortApplication(void);
	
	nbNetVmPortApplication *Clone(char *Name, char *Description, int Flags) {return NULL;}	//!< Clone this class returning a pointer to the new istance  
	int Read(nbNetPkt **Packet,int *Flags); 						//!< Read packets from NetVm
	int Write(uint8_t *PacketData, uint32_t PacketLen, enum writeModes mode); 		//!< Write packet into NetVm
	int Write(nbTStamp *tstamp, uint8_t *PacketData, uint32_t PacketLen, enum writeModes mode); 		//!< Write packet into NetVm
};

/*!
	\}
*/
