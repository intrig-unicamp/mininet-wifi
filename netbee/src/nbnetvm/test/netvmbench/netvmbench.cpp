// NetVMBench.cpp : Defines the entry point for the console application.
//

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cassert>
#include <iomanip>
#include <string>
#include <nbnetvm.h>

using namespace std;

#define PACKET_SIZE	1500

size_t current_packet;
char *packet_name;

size_t LoadPacket(string src, u_int8_t buffer[], size_t buffer_size);

// Prints the packet after it has been processed.
// TODO: compare the processed packet with the expected values

extern "C" {

int32_t ApplicationCallback(nvmExchangeBuffer *xbuffer)
{
	//cout << "Output for packet '" << packet_name << "'\n";
	cout << "Packet size: " << xbuffer->PacketLen << endl;
	for(size_t i = 0; i < xbuffer->PacketLen; ++i) {
		cout << hex << setfill('0') << setw(2) << setprecision(2) << (int)((u_int8_t *)xbuffer->PacketBuffer)[i] << ' ';
	}
	cout << endl;

	// Verifies that the packet is correct
	u_int8_t buffer[PACKET_SIZE];
	string prefix("result_");
	size_t result_size;
	result_size = LoadPacket(prefix + packet_name, buffer, sizeof(buffer));
	bool match(true);

	if(result_size != xbuffer->PacketLen) {
		cout << "!!! Packet size mismatch: got " << xbuffer->PacketLen << ", expected: " << result_size << '\n';
		match = false;
	}

	for(size_t i = 0; i < (xbuffer->PacketLen > result_size ? result_size : xbuffer->PacketLen); ++i) {
		unsigned t((unsigned)((u_int8_t *)xbuffer->PacketBuffer)[i]);
		if(t != (unsigned)buffer[i]) {
			cout << "!!! Packet data mismatch (offset: " << i << "): got " << hex << setfill('0') << setw(2) << setprecision(2) <<t;
			cout << ", expected: " << hex << setfill('0') << setw(2) << setprecision(2) << (unsigned)buffer[i];
			cout << '\n';
			match = false;
		}
	}

	cout << dec << resetiosflags(ios_base::showbase | ios_base::uppercase);

	if(match)
		cout << "----- MATCH\n";
	else
		cout << "XXXXX MISMATCH\n";

	return nvmSUCCESS;
}

} /* extern "C" */

int main(int argc, char *argv[])
{
	nvmByteCode     * BytecodeHandle = NULL;

	assert(argc > 3);

	bool use_jit(false);
	
	int pe_count(atoi(argv[1]));
	
	if(pe_count > 0)
		use_jit = true;

	if(pe_count <= 0)
		pe_count = 1;
		
	cerr << "NetVMBench: going to compile " << pe_count << " NetPEs together\n";
	cerr << "NetVMBench execution: going to process " << argc-2-pe_count << " network packets\n";


	cerr << "NetVMBench is " << (use_jit ? "" : "not") << " using the JIT engine\n";

	char		errbuf[250];
	nvmNetVM	*NetVM=NULL;
	nvmNetPE	*NetPE=NULL, *last(0);
	nvmSocket	*SocketIn=NULL;
	nvmSocket	*SocketOut=NULL;
	nvmRuntimeEnvironment	*RT=NULL;
	nvmAppInterface *InInterf;
	nvmAppInterface *OutInterf;
	//char userData[250];

	
	NetVM = nvmCreateVM(0, errbuf);
	
	SocketIn = nvmCreateSocket(NetVM, errbuf);
	SocketOut = nvmCreateSocket(NetVM, errbuf);

	
	for(int i(0); i < pe_count; ++i) {
		BytecodeHandle = nvmAssembleNetILFromFile(argv[i+2], errbuf);

		if (BytecodeHandle == NULL){
		      printf("Cannot read bytecode.\n");
		      goto end_failure;
		}
		
		nvmDumpBytecodeInfo(stdout, BytecodeHandle);
	
		NetPE=nvmCreatePE(NetVM, BytecodeHandle, errbuf);

		if(i == 0) {
			//connetto In su porta 0
			nvmConnectSocket2PE(NetVM, SocketIn, NetPE, 0, errbuf);
		} else {
			// Connect port 1 of the previous PE to port 0 of the current one
			nvmConnectPE2PE(NetVM, last, 1, NetPE, 0, errbuf);
		}
		
		if(i+1 == pe_count) {
			//connetto Out su porta 1
			nvmConnectSocket2PE(NetVM, SocketOut, NetPE, 1, errbuf);
		}
		
		last = NetPE;
	}
	
	RT= nvmCreateRTEnv(NetVM, nvmRUNTIME_COMPILEANDEXECUTE, errbuf);
	
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

	nvmNetStart(NetVM, RT, nvmRUNTIME_COMPILEANDEXECUTE, (nvmDO_NATIVE), 1, errbuf);

	// Processes all the packets
	for(int i = 0; i < argc-2-pe_count; ++i) {
		u_int8_t buff[PACKET_SIZE];
		u_int8_t userData[250];

		current_packet = i+2+pe_count;
		packet_name = argv[current_packet];

		string prefix("test_");
		size_t packet_size = LoadPacket(prefix + argv[current_packet], buff, sizeof(buff));
		cerr << "Processing packet " << i << ", packets size: " << packet_size << " bytes.\n";
		nvmWriteAppInterface(InInterf, buff, packet_size, userData, errbuf);
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

size_t LoadPacket(string src, u_int8_t buffer[], size_t buffer_size)
{
	size_t packet_size = 0;

	ifstream s(src.c_str());

	if(!s.is_open()) {
		cerr << "Error while opening '" << src << "'\n";
		return 0;
	}

	int b;
	while(packet_size < buffer_size && s >> hex >> b) {
		buffer[packet_size++] = (u_int8_t)b;
	}

	cerr << "Input packet size: " << packet_size << endl;

	s.close();

	
	return packet_size;
}
