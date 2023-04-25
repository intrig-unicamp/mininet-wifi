/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "octeon-offset.h"

using namespace jit;
using namespace octeon;

void octeonOffset::init()
{
	ExchangeBuffer.PacketBuffer   = 8;
	ExchangeBuffer.InfoData		  = 24;
	ExchangeBuffer.PacketLen	  = 16;
	ExchangeBuffer.InfoLen		  = 32;
	PEHandlerState.PEState		  = 40;
	PEState.DataMem				  = 0;
	PEState.SharedMem			  = 8;
	PEState.CoprocTable			  = 24;
	MemDescriptor.Base			  = 0;
	CoprocessorState.regsOffs     = 100;
	CoprocessorState.init     	  = 232;
	CoprocessorState.write     	  = 240;
	CoprocessorState.read     	  = 248;
	CoprocessorState.invoke       = 256;
	CoprocessorState.xbuff     	  = 272;
	CoproStateSize 				  = 280;
	octeon_copro_map_entry.create = 64;
	octeon_copro_map_entry.size   = 72;
}

octeonOffset jit::octeon::offsets;
