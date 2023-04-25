//
#include <stdio.h>
#include <nbnetvm.h>


#define PACKET_SIZE	1500

size_t current_packet;
char *packet_name;


int temp=1;

int32_t ApplicationCallback(nvmExchangeBuffer *xbuffer)
{
/*
	unsigned char *b = (unsigned char *) (xbuffer ->PacketBuffer );
	printf("Bytes 12-14: %d %d %d - Length: %d\n", b[12], b[13], b[14], xbuffer -> PacketLen);

	printf("fatta la callback: %d\n",temp);
	temp++;
	*/
	return nvmSUCCESS;
}

void CreateIPARPPacket(u_int8_t *payload, u_int32_t length, u_int32_t flag)
{
	int i;

	for (i = 0 ; i < (int)length; i++)
		payload[i] = (u_int8_t)i;
	
	if (flag == 1)
	{ // IP
		payload[12] = 0x08;
		payload[13] = 0x00;
	}
	else
	{ //not IP, it's ARP
		payload[12] = 0x08;
		payload[13] = 0x06;
	}

	strcpy(payload,"ciao signor USER ftp come USER va il tuo  ftp ?");
}




int main(int argc, char *argv[])
{
	nvmByteCode     * BytecodeHandle = NULL;


	char		errbuf[250];
	nvmNetVM	*NetVM=NULL;
	nvmNetPE	*NetPE1=NULL;
	nvmNetPE	*NetPE2=NULL;
	nvmSocket	*SocketIn=NULL;
	nvmSocket	*SocketOut=NULL;
	nvmRuntimeEnvironment	*RT=NULL;
	nvmAppInterface *InInterf;
	nvmAppInterface *OutInterf;
	char userData[250];

	
	NetVM = nvmCreateVM(0, errbuf);

	BytecodeHandle = nvmAssembleNetILFromFile(argv[1], errbuf);

	//nvmDumpBytecodeInfo(stdout, BytecodeHandle);
	
	NetPE1=nvmCreatePE(NetVM, BytecodeHandle, errbuf);
	NetPE2=nvmCreatePE(NetVM, BytecodeHandle, errbuf);

	SocketIn = nvmCreateSocket(NetVM, errbuf);
	SocketOut = nvmCreateSocket(NetVM, errbuf);
	
	//connetto In su porta 0
	nvmConnectSocket2PE(NetVM, SocketIn, NetPE1, 0, errbuf);
	//connect pe1 to pe2
	nvmConnectPE2PE (NetVM, NetPE1, 1, NetPE2, 0, errbuf);

	//connetto Out su porta 1
	nvmConnectSocket2PE(NetVM, SocketOut, NetPE2, 1, errbuf);
	
	RT= nvmCreateRTEnv(NetVM, 0, errbuf);
	
	InInterf=nvmCreateAppInterfacePushIN(RT, errbuf);
	OutInterf=nvmCreateAppInterfacePushOUT(RT,ApplicationCallback,errbuf);

	if (nvmBindAppInterf2Socket(InInterf,SocketIn)!= nvmSUCCESS)
		{
			printf("Cannot bind the interface\n");
			goto end_failure;

		}
	if (nvmBindAppInterf2Socket(OutInterf,SocketOut)!= nvmSUCCESS)
	{
		printf("Cannot bind the interface\n");
		goto end_failure;
	}

	nvmNetStart(NetVM, RT, 0, 0, 0, errbuf);
	

	int	i = 0;


//	nvmStartPhysInterface(PhysInterfaceIn,SocketIn,errbuf);



	while(i < 10)
	{
		u_int8_t buff[PACKET_SIZE];
		CreateIPARPPacket(buff, PACKET_SIZE, i % 2);
		printf("writing packet %u\n", i);
		nvmWriteAppInterface(InInterf, buff, PACKET_SIZE, userData, errbuf);
		i++;
	}

	
	nvmDestroyRTEnv(RT);
	
	nvmDestroyBytecode(BytecodeHandle);

	nvmDestroyVM(NetVM);
	
#ifdef WIN32
	system("pause");
#endif
	return nvmSUCCESS;

end_failure:
#ifdef WIN32
	system("pause");
#endif
	return nvmFAILURE;
}

