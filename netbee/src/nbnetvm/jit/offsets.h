/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#pragma once

#include "rt_environment.h"
#include "nbnetvm.h"

namespace jit 
{
	
	//!class containing relative offsets in netvm structure
	template<typename _AddrType>
	class nvmStructOffsets {

		public:
		typedef _AddrType AddrType; //!address type

		typedef struct	nvmPEHandlerStateOffsets
		{
			AddrType PEState;
		}nvmPEHandlerStateOffsets;

		typedef struct nvmPEStateOffsets
		{
			AddrType DataMem;
			AddrType SharedMem;
			AddrType CoprocTable;
		} nvmPEStateOffsets;

		typedef struct nvmMemDescriptorOffsets
		{
			AddrType Base;
		}nvmMemDescriptorOffsets; 

		typedef struct nvmExchangeBufferOffsets
		{
			AddrType PacketBuffer;
			AddrType InfoData;
			AddrType PacketLen;
			AddrType InfoLen;
			AddrType TStamp_s;
			AddrType TStamp_us;
		} nvmExchangeBufferOffsets;

		typedef struct nvmCoprocFieldOffsets
		{
			AddrType regsOffs;
			AddrType init;
			AddrType write;
			AddrType read;
			AddrType invoke;
			AddrType xbuff;
		} nvmCoprocFieldOffsets;

		nvmExchangeBufferOffsets 		ExchangeBuffer;
		nvmPEHandlerStateOffsets		PEHandlerState;
		nvmPEStateOffsets				PEState;
		nvmMemDescriptorOffsets			MemDescriptor;
		nvmCoprocFieldOffsets			CoprocessorState;
		AddrType 						CoproStateSize;

		//!function to override in a backend to fill this structure offsets
		virtual void init();
		virtual ~nvmStructOffsets() {}
	};

}//namespace jit

	template<typename AddrType>
void jit::nvmStructOffsets<AddrType>::init()
{
	nvmExchangeBuffer ExchangeBuffer;
	nvmHandlerState	HandlerState;
	tmp_nvmPEState	PEState;
	nvmMemDescriptor MemDescriptor;
	nvmCoprocessorState		CopState;

	this->ExchangeBuffer.PacketBuffer = ((AddrType)&ExchangeBuffer.PacketBuffer) - ((AddrType)&ExchangeBuffer);
	this->ExchangeBuffer.InfoData = ((AddrType)&ExchangeBuffer.InfoData) - ((AddrType)&ExchangeBuffer);
	this->ExchangeBuffer.PacketLen = ((AddrType)&ExchangeBuffer.PacketLen) - ((AddrType)&ExchangeBuffer);
	this->ExchangeBuffer.InfoLen = ((AddrType)&ExchangeBuffer.InfoLen) - ((AddrType)&ExchangeBuffer);
	this->ExchangeBuffer.TStamp_s = ((AddrType)&ExchangeBuffer.TStamp_s) - ((AddrType)&ExchangeBuffer);
	this->ExchangeBuffer.TStamp_us = ((AddrType)&ExchangeBuffer.TStamp_us) - ((AddrType)&ExchangeBuffer);

	this->PEHandlerState.PEState = ((AddrType) &HandlerState.PEState) - ((AddrType)&HandlerState);

	this->PEState.DataMem = ((AddrType) &PEState.DataMem - (AddrType) &PEState);
	this->PEState.SharedMem = ((AddrType) &PEState.ShdMem - (AddrType) &PEState);
	this->PEState.CoprocTable = ((AddrType) &PEState.CoprocTable - (AddrType) &PEState);

	this->MemDescriptor.Base = ((AddrType) &MemDescriptor.Base - (AddrType)&MemDescriptor);

	this->CoprocessorState.regsOffs      =   ((AddrType)&CopState.registers) - ((AddrType)&CopState);
	this->CoprocessorState.init      =   ((AddrType)&CopState.init) - ((AddrType)&CopState);
	this->CoprocessorState.write      =   ((AddrType)&CopState.write) - ((AddrType)&CopState);
	this->CoprocessorState.read      =   ((AddrType)&CopState.read) - ((AddrType)&CopState);
	this->CoprocessorState.invoke      =   ((AddrType)&CopState.invoke) - ((AddrType)&CopState);
	this->CoprocessorState.xbuff      =   ((AddrType)&CopState.xbuf) - ((AddrType)&CopState);

	this->CoproStateSize = (AddrType) sizeof(CopState);
}

