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



#include "nbnetvm.h"
#include "nbnetvmports.h"

/*!
	\defgroup NetBeePacketProcessing NetBee classes for packet processing (e.g. capture)
*/

/*! \addtogroup NetBeePacketProcessing 
	\{
*/

/*!
	\brief This class should never used by the programmer because it only provides some common methods for derived classes.

	This class provides some method (such as error management, interface listing) that can be useful
	when interacting with the network.
	
	Since most of the other classes (e.g. packet capture) inherit from this one, the features provided here will
	be available to all the other classes.
*/
class DLL_EXPORT nbPacketProcessBase
{
public:

	nbPacketProcessBase();
	virtual ~nbPacketProcessBase();

	nbNetVmPortLocalAdapter *GetListLocalAdapters();	//!< Return list of local adapter
	nbNetVmPortRemoteAdapter *GetListRemoteAdapters();	//!< Return list of remote adapter
	char *GetLastError() {return m_errbuf;}			//!< Return the error messages (if any)
private:
	nbNetVmPortDataDevice *m_returnedDeviceList;		//!< Return list of devices
protected:
	char m_errbuf[2048];					//!< Buffer that keeps the error message (if any)
};


/*!
	\brief This class defines the methods needed to capture and filter network packets.

	This class can be used to define a source for network packets (e.g. a network adapter), a destination (e.g.
	a file or an application). Additionally, a filter can be applied to the incoming packets so that only a subset
	of the incoming packets get delivered to the application.

	The most common use of this class is to create a program that calls the methods according to the following order:
	- GetListLocalAdapters() to retrieve the list of available network devices
	- SetSrcDataDevice() to define the network device that is the source of the network packets; this function is not needed in case user application reads data through the ReadPacket() method
	- SetDstDataDevice() to define the network device that will become the destination of the network packets; this function is not needed in case user application reads data through the ReadPacket() method
	- InjectFilter() to define a boolean rule (e.g. 'accept only arp packets') that is used to discard packets that do not satisfy the filter
	- Start() to create the NetVM and start the capture
	- ReadPacket() to read packets filtered by NetVM
	- WritePacket() to write a packet inside the NetVM
	- CapturePacket() to capture packet from Local or Remote Adapter.
*/
class DLL_EXPORT nbPacketCapture : public nbPacketProcessBase
{
public:
	nbPacketCapture();
	virtual ~nbPacketCapture();

	int SetSrcDataDevice(nbNetVmPortDataDevice *NetDevice, char *Name, char *Description, int Flags);  //!< set source device
	int SetDstDataDevice(nbNetVmPortDataDevice *NetDevice, char *Name, char *Description, int Flags);  //!< set destination device 
	int RegisterCallbackFunction(nbDataDstCallbackFunction *DataDstCallbackFunction, void *CustomPtr); //!< register callback function
	int InjectFilter(char *FilterString);		//!< inject filter code 
	int Start(); 					//!< start capturing packet
	void Stop(); 					//!< stop capturing packet
	int SetCaptureParams(); 			//!< set capture parameter
	int ReadPacket(nbNetPkt **Packet, int *Flags); 	//!< Read filtered capture packets 
	int WritePacket(nbNetPkt *Packet);		//!< Write capture packet fro filtering	
	nbNetPkt * GetPacket();				//!< Obtain packet structure
	int ReleaseExBuf(nbNetPkt *Packet);		//!< release Exchange Buffer

public:
	//! Network device currently in use as a source device
	nbNetVmPortDataDevice * m_srcNetDevice;

	//! Network device currently in use as a desstination device
	nbNetVmPortDataDevice * m_dstNetDevice;

	//! Handle to a NetVM virtual machine.
	nbNetVm *m_netVMHandle;

	//! Handle to a NetPE processing element; may be NULL in case the user does not set any filter.
	nbNetPe *m_netPEHandle;

	//! Handle to an objects that keeps the bytecode for filtering, in case it is needed.
	nbNetVmByteCode *m_byteCodeHandle;

	//! 'true' if the NetVM is up and running, 'false' otherwise.
	bool m_nvmIsRunning;
};

/*!
	\}
*/

