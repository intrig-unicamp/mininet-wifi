#include "rt_environment.h"
#include "nbnetvm.h"
#include "arch/octeon/octeon_coprocessors.h"
#include <stdio.h>
#include <stdlib.h>

typedef u_int64_t AddrType;

int main()
{
	nvmExchangeBuffer ExchangeBuffer;
	nvmHandlerState	HandlerState;
	tmp_nvmPEState	PEState;
	nvmMemDescriptor MemDescriptor;
	nvmCoprocessorState		CopState;
	AddrType offset;
	OcteonCoprocessorMapEntry octeon_map_entry;

	offset = ((AddrType)&ExchangeBuffer.PacketBuffer) - ((AddrType)&ExchangeBuffer);
	printf("ExchangeBuffer.PacketBuffer: %ld\n",offset);
	offset = ((AddrType)&ExchangeBuffer.InfoData) - ((AddrType)&ExchangeBuffer);
	printf("ExchangeBuffer.InfoData: %ld\n",offset);
	offset = ((AddrType)&ExchangeBuffer.PacketLen) - ((AddrType)&ExchangeBuffer);
	printf("ExchangeBuffer.PacketLen: %ld\n",offset);
	offset = ((AddrType)&ExchangeBuffer.InfoLen) - ((AddrType)&ExchangeBuffer);
	printf("ExchangeBuffer.InfoLen: %ld\n",offset);

	offset = ((AddrType) &HandlerState.PEState) - ((AddrType)&HandlerState);
	printf("PEHandlerState.PEState: %ld\n",offset);

	offset = ((AddrType) &PEState.DataMem - (AddrType) &PEState);
	printf("PEState.DataMem: %ld\n",offset );
	offset = ((AddrType) &PEState.ShdMem - (AddrType) &PEState);
	printf("PEState.SharedMem: %ld\n",offset );
	offset = ((AddrType) &PEState.CoprocTable - (AddrType) &PEState);
	printf("PEState.CoprocTable: %ld\n",offset );

	offset = ((AddrType) &MemDescriptor.Base - (AddrType)&MemDescriptor);
	printf("MemDescriptor.Base: %ld\n",offset );

	offset      =   ((AddrType)&CopState.registers) - ((AddrType)&CopState);
	printf("CoprocessorState.regsOffs     : %ld\n",offset      );
	offset      =   ((AddrType)&CopState.init) - ((AddrType)&CopState);
	printf("CoprocessorState.init     : %ld\n",offset);
	offset      =   ((AddrType)&CopState.write) - ((AddrType)&CopState);
	printf("CoprocessorState.write     : %ld\n",offset);
	offset      =   ((AddrType)&CopState.read) - ((AddrType)&CopState);
	printf("CoprocessorState.read     : %ld\n",offset);
	offset      =   ((AddrType)&CopState.invoke) - ((AddrType)&CopState);
	printf("CoprocessorState.invoke     : %ld\n",offset);
	offset      =   ((AddrType)&CopState.xbuf) - ((AddrType)&CopState);
	printf("CoprocessorState.xbuff     : %ld\n",offset);

	offset = (AddrType) sizeof(CopState);
	printf("CoproStateSize = : %ld\n",offset);
	offset      =   ((AddrType)&octeon_map_entry.copro_create) - ((AddrType)&octeon_map_entry);
	printf("octeonmap_entry.copro_create = : %ld\n", offset);
	return 0;
}
