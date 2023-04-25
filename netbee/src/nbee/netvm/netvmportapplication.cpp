/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <nbee_netvm.h>
#include "../globals/globals.h"
#include "../globals/utils.h"
#include "../globals/debug.h"


nbNetVmPortApplication::nbNetVmPortApplication()
{
	// Create the port that implements the collector
	ExternalPort = nvmCreateExternalPort();
	if (ExternalPort == NULL)
	{
		printf("Cannot create the port\n");
	}
};


nbNetVmPortApplication::~nbNetVmPortApplication(void)
{

};


int nbNetVmPortApplication::Read(nbNetPkt **Packet, int *Flags)
{
	nvmRead(this->GetNetVmPortDataDevice(), Packet, Flags);
	return nbSUCCESS; 
}


int nbNetVmPortApplication::Write(u_int8_t *PacketData, u_int32_t PacketLen, enum writeModes mode)
{
	nvmWrite(this->GetNetVmPortDataDevice(), PacketData, PacketLen, mode);
	return nbSUCCESS;
}


int nbNetVmPortApplication::Write(nbTStamp *tstamp, u_int8_t *PacketData, u_int32_t PacketLen, enum writeModes mode)
{
	nvmExchangeBufferState *buf = nvmGetExBuf(this->GetNetVmPortDataDevice());
	buf->timestamp_sec = tstamp->sec;
	buf->timestamp_usec = tstamp->usec;
	buf->pkt_pointer = PacketData;
	buf->pktlen = PacketLen;
	buf->owner = nvmOWNER_NETVM;
	nvmWriteExchangeBuffer(this->GetNetVmPortDataDevice(), buf);
	return nbSUCCESS;
}