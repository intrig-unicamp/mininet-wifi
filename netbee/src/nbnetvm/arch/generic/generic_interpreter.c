/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





/** @file netvm_interpreter.c
 * \brief This file contains the functions that implement the interpreter of the bytecode.
 *
 */

#include "generic_interpreter.h"
#include "../../opcodes.h"
#include "../../../nbee/globals/debug.h"
#include "../../../nbee/globals/profiling-functions.h"
#include <stdlib.h>
#include <string.h>
#include "assert.h"
#if(defined (WIN32) || defined(_WIN64))
#include <sys/timeb.h>		//needed for the implementation of the TSTAMP opcode
#else
#include <sys/time.h>		//needed for the implementation of the TSTAMP opcode
#endif
#include "arch.h"

#ifdef ENABLE_NETVM_LOGGING
#include "codetable.h"
#endif


#ifdef COUNTERS_PROFILING
#define CODE_PROFILING_PKTCHECK() \
NetVMInstructionsProfiler.n_pkt_access++;
#define CODE_PROFILING_INFOCHECK() \
NetVMInstructionsProfiler.n_info_access++;
#define CODE_PROFILING_DATACHECK() \
NetVMInstructionsProfiler.n_data_access++;
#define CODE_PROFILING_COPROCHECK() \
NetVMInstructionsProfiler.n_copro_access++;
#define CODE_PROFILING_STACKCHECK() \
NetVMInstructionsProfiler.n_stack_access++;
#define CODE_PROFILING_INITEDMEMCHECK() \
NetVMInstructionsProfiler.n_initedmem_access++;
#define CODE_PROFILING_JUMPCHECK() \
NetVMInstructionsProfiler.n_jump++;
#else
#define CODE_PROFILING_PKTCHECK()
#define CODE_PROFILING_INFOCHECK()
#define CODE_PROFILING_DATACHECK()
#define CODE_PROFILING_COPROCHECK()
#define CODE_PROFILING_STACKCHECK()
#define CODE_PROFILING_INITEDMEMCHECK()
#define CODE_PROFILING_JUMPCHECK()
#endif

void CODE_PROFILING_INSTRUCTION_COUNTER()
{

#ifdef COUNTERS_PROFILING
	NetVMInstructionsProfiler.n_instruction++;
#endif

#ifdef RTE_DYNAMIC_PROFILE
	NetVMInstructionsProfiler.n_instruction++;
#endif

	return;
}

uint32_t pktlen = 0, infolen = 0, codelen = 0;



#ifdef _WIN32
#ifdef _DEBUG
#include <crtdbg.h>
#endif


uint32_t _gen_rotl(uint32_t value, uint32_t pos )
{
	return ( (value << pos) | ( value >> ( 32 - pos ) ) );
}


uint32_t _gen_rotr(uint32_t value, uint32_t pos )
{
	return ( (value >> pos) | (value << ( 32 - pos )) );
}
#else
#include <unistd.h>

uint32_t _gen_rotl(uint32_t value, uint32_t pos )
{
	return ( (value << pos) | ( value >> ( 32 - pos ) ) );
}


uint32_t _gen_rotr(uint32_t value, uint32_t pos )
{
	return ( (value >> pos) | (value << ( 32 - pos )) );
}
#endif


uint32_t do_pscanb(uint8_t *pkt, uint32_t offset, uint8_t value, uint32_t p_len)
{
uint32_t i;

	//printf("pscanb startoffs: %u, val: %u, hton: %u\n", offset, value, val);
	for (i = offset; i < p_len; i++)
	{
		//printf("\toffs: %u, pval: %u\n", i, pkt[i]);
		if (pkt[i] == value)
			return i;
	}
	return 0xFFFFFFFF;
}

uint32_t do_pscanw(uint8_t *pkt, uint32_t offset, uint16_t value, uint32_t p_len)
{
	uint32_t i;
	uint16_t val = nvm_ntohs(value);
	//printf("pscanw startoffs: %u, val: %u, hton: %u\n", offset, value, val);
	for (i = offset; i < (p_len-1); i++)
	{
		//printf("\toffs: %u, pval: %u\n", i, (*(uint16_t*)&pkt[i]));
		if ((*(uint16_t*)&pkt[i]) == val)
			return i;
	}
	return 0xFFFFFFFF;
}

uint32_t do_pscandw(uint8_t *pkt, uint32_t offset, uint32_t value, uint32_t p_len)
{
	uint32_t i;
	uint32_t val = nvm_ntohl(value);
	//printf("pscandw startoffs: %u, val: %u, hton: %u\n", offset, value, val);
	for (i = offset; i < (p_len-3); i++)
	{
		//printf("\toffs: %u, pval: %u\n", i, (*(uint32_t*)&pkt[i]));
		if ((*(uint32_t*)&pkt[i]) == val)
			return i;
	}
	return 0xFFFFFFFF;
}


// #define USE_EXPERIMENTAL_SWITCH

#ifndef USE_EXPERIMENTAL_SWITCH

void gen_do_switch(uint8_t  *pr_buf, uint32_t *pc, uint32_t value)
{
	uint32_t npairs = 0, key = 0, target = 0;
	uint32_t i = 0;
	uint32_t next_insn = 0;
	int32_t default_displ;

	i=0;
	default_displ = *(uint32_t*)&pr_buf[*pc];
	(*pc) += 4;
	npairs = *(int32_t*)&pr_buf[*pc];
	(*pc) +=4;
	next_insn = (*pc) + npairs * 8;

	for (i = 0; i < npairs; i++)
	{
		key = *(uint32_t*)&pr_buf[*pc];
		(*pc) += 4;
		target = *(uint32_t*)&pr_buf[*pc];
		(*pc) +=4;
		if (key == value)
		{
			(*pc) = next_insn + target;

			return;
		}
	}

	(*pc) = next_insn + default_displ;
}

#else

#warning "Using experimental switch implementation"

#include "../../assembler/hashtable.h"

#define SWITCH_CACHE_SIZE 50
hash_table *htswitches;

void gen_do_switch(uint8_t  *pr_buf, int32_t *pc, uint32_t value) {
	uint32_t npairs = 0, key = 0, *target = 0;
	uint32_t i = 0;
	uint32_t next_insn = 0;
	int32_t default_displ;
	hash_table *ht;
	uint32_t swid[2], *swidcopy;

	default_displ = *(uint32_t *) &pr_buf[*pc];
	(*pc) += 4;
	npairs = *(int32_t *) &pr_buf[*pc];
	(*pc) += 4;
	next_insn = (*pc) + npairs * 8;

	/* We identify the switch by the bytecode start address and the current PC */
	swid[0] = (uint32_t) pr_buf;
	swid[1] = (uint32_t) *pc;
	if (!hash_table_lookup (htswitches, swid, sizeof (uint32_t) * 2, (void *) &ht)) {
		printf ("Caching new switch, SWID: %u/%u, npairs: %u\n", swid[0], swid[1], npairs);
		ht = hash_table_new (npairs > 4096 ? 2048 : npairs / 2);		/* Is this OK? */

		/* Cache switch pairs */
		for (i = 0; i < npairs; i++) {
			hash_table_add (ht, &pr_buf[*pc], sizeof (uint32_t), &pr_buf[(*pc) + 4]);
			(*pc) += 8;
		}

		/* Add the switch to the cache */
		swidcopy = (uint32_t *) malloc (sizeof (uint32_t) * 2);
		memcpy (swidcopy, &swid, sizeof (uint32_t) * 2);
		hash_table_add (htswitches, swidcopy, sizeof (uint32_t) * 2, ht);
	}

	/* Do the switch */
	if (hash_table_lookup (ht, &value, sizeof (uint32_t), (void *) &target)) {
// 		printf ("- target: %u\n", *target);
		(*pc) = next_insn + *target;
	} else {
// 		printf ("- default: %u\n", default_displ);
		(*pc) = next_insn + default_displ;
	}

	return;
}

#endif

//--------------------------------------------------------------------------------------------
//					RUN INSTRUCTIONS
//--------------------------------------------------------------------------------------------

///< Macro to check if the code being executed belongs to the INIT segment of a PE
#define INIT_SEGMENT (HandlerState->Handler->HandlerType == INIT_HANDLER)

///< Macro to check if the code being executed belongs to the PUSH handler of a PE
#define PUSH_SEGMENT (HandlerState->Handler->HandlerType == PUSH_HANDLER)

///< Macro to be used in instructions that can only appear in the INIT segment of a PE
#define INIT_SEGMENT_ONLY \
	if (!INIT_SEGMENT) { \
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Instruction %x can only be used in the INIT segment\n", pr_buf[pc]); \
		return nvmFAILURE; \
	}

///< Macro to be used in instructions that can only appear in the PUSH or PULL handler of a PE
#define HANDLERS_ONLY \
	if (INIT_SEGMENT) { \
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Instruction %x can only be used in the PUSH or PULL handler\n", pr_buf[pc]); \
		return nvmFAILURE; \
	}


//--------------------------------------------------------------------------------------------
// BOUND_CHECK, STACK_CHECK, JUMP_CHECK (ENABLED BY DEFAULT)
//--------------------------------------------------------------------------------------------


///< Macro to perform bounds checking on the stack
#define NEED_STACK(n) \
	CODE_PROFILING_STACKCHECK(); \
	if ((sp > 0) && (sp >= stacksize)) { \
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Stack index (%d) exceeds stack size (%d)\n", sp, stacksize); \
		return nvmSTACKEX; \
	}; \
	if (sp < n) { \
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Instruction needs %d stack element(s) but stack only contains %d\n", n, sp); \
		return nvmSTACKEX;  \
	}

#define LOCALS_CHECK(n) \
	if ((n) >= localsnum) { \
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Trying to access local %d, but only %d local(s) allocated\n", n, localsnum); \
		return nvmFAILURE; \
	}


#define DATAMEM_CHECK(n , b) \
	CODE_PROFILING_DATACHECK(); \
	if ((n+b) > HandlerState->PEState->DataMem->Size) { \
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Trying to access data memory with an offset too big (%d > %d)\n", n, HandlerState->PEState->DataMem->Size); \
		return nvmDATAEX; \
	}


#define SHAREDMEM_CHECK(n , b) \
	if ((n+b) > sharedmem_size) { \
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Trying to access shared memory with an offset too big (%d > %d)\n", n, sharedmem_size); \
		return nvmFAILURE; \
	}

/*
#define INITEDMEM_CHECK(n, b) \
	CODE_PROFILING_INITEDMEMCHECK(); \
	if ((n+b) > initedmem_size) { \
			errorprintf(__FILE__, __FUNCTION__, __LINE__, "Trying to access initialised memory with an offset too big (%d > %d)\n", n, initedmem_size); \
			return nvmINITEDMEMERR;  \
	}
*/

#define COPROCESSOR_CHECK(copro, reg) \
	CODE_PROFILING_COPROCHECK(); \
	if (copro > HandlerState->PEState-> NCoprocRefs) { \
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "%d/%d: No such coprocessor: %u (> %u)\n", HandlerState->Handler->OwnerPE->Name, pidx, copro, HandlerState->PEState ->NCoprocRefs); \
		return nvmCOPROCERR; \
	} else if (reg > (HandlerState->PEState -> CoprocTable)[copro] . n_regs) { \
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "%d/%d: No such register in coprocessor %u: %u (> %u)\n", HandlerState->Handler->OwnerPE->Name, pidx, copro, reg, (HandlerState->PEState ->CoprocTable)[copro] . n_regs); \
		return nvmCOPROCERR; \
	}

#define PKTMEM_CHECK(n , b )  \
	CODE_PROFILING_PKTCHECK(); \
	if ((n+b) > pktlen) { \
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Trying to access packet memory with an offset too big (from %d to %d > %d)\n", n, n+b-1 , pktlen); \
		return nvmPKTEX; \
	}



#define INFOMEM_CHECK(n , b ) \
	CODE_PROFILING_INFOCHECK(); \
	if ((n+b) > infolen) { \
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Trying to access info memory with an offset too big (from %d to %d > %d)\n", n, n+b-1 , infolen); \
		return nvmINFOEX ; \
	}


#define JUMPCHECK(pc) \
	CODE_PROFILING_JUMPCHECK(); \
	if ((pc) < 0) { \
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Trying to access code memory with a negative offset (%d)\n", pc); \
		return nvmJUMPERR; \
	} else if (codelen == 0) { \
		errorprintf(__FILE__, __FUNCTION__, __LINE__,  "Code memory not initialised\n"); \
		return nvmJUMPERR; \
	} else if (pc >= codelen) { \
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Trying to access code memory with an offset too big (%d > %d)\n", pc , codelen); \
		return nvmJUMPERR;\
	}


#ifdef RTE_DYNAMIC_PROFILE
#define PROF_INFO_READ(a, s)		HandlerState->t= nbProfilerGetTime();fprintf(HandlerState->MemDataFile, "%llu\t%u\t%u\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\t%u\n", HandlerState->t, a, s, NetVMInstructionsProfiler.n_instruction)
#define PROF_INFO_WRITE(a, s, v)	HandlerState->t= nbProfilerGetTime();fprintf(HandlerState->MemDataFile, "%llu\tna\tna\tna\t%u\t%u\t%u\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\t%u\n", HandlerState->t, a, s, v, NetVMInstructionsProfiler.n_instruction)
#define PROF_PKT_READ(a, s)			HandlerState->t= nbProfilerGetTime();fprintf(HandlerState->MemDataFile, "%llu\tna\tna\tna\tna\tna\tna\t%u\t%u\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\t%u\n", HandlerState->t, a, s, NetVMInstructionsProfiler.n_instruction)
#define PROF_PKT_WRITE(a, s, v)		HandlerState->t= nbProfilerGetTime();fprintf(HandlerState->MemDataFile, "%llu\tna\tna\tna\tna\tna\tna\tna\tna\tna\t%u\t%u\t%u\tna\tna\tna\tna\tna\tna\t%u\n", HandlerState->t, a, s, v, NetVMInstructionsProfiler.n_instruction)
#define PROF_DATA_READ(a, s)		HandlerState->t= nbProfilerGetTime();fprintf(HandlerState->MemDataFile, "%llu\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\t%u\t%u\tna\tna\tna\tna\t%u\n", HandlerState->t, a, s, NetVMInstructionsProfiler.n_instruction)
#define PROF_DATA_WRITE(a, s, v)	HandlerState->t= nbProfilerGetTime();fprintf(HandlerState->MemDataFile, "%llu\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\tna\t%u\t%u\t%u\t%u\n", HandlerState->t, a, s, v, NetVMInstructionsProfiler.n_instruction)

#else
#define PROF_INFO_READ(a, s)
#define PROF_INFO_WRITE(a, s, v)
#define PROF_PKT_READ(a, s)
#define PROF_PKT_WRITE(a, s, v)
#define PROF_DATA_READ(a, s)
#define PROF_DATA_WRITE(a, s, v)

#endif



int32_t genRT_Execute_Handlers(nvmExchangeBuffer **exbuf, uint32_t port, nvmHandlerState *HandlerState)
{
uint8_t i, j, loop;
uint8_t overflow;	// Currently unused, but not sure... proabably we should take care of this when subtracting values
//uint32_t stack_top;
uint8_t *xbuffer, *xbufinfo, *datamem = NULL, *sharedmem, *initedmem;
int32_t ret = nvmFAILURE;
uint32_t utemp1, utemp2, utemp3, pidx, sharedmem_size = 0;
//uint32_t initedmem_size;

uint32_t *stack = NULL;
uint32_t *locals = NULL;
uint32_t localsnum = 0;
uint32_t stacksize = 0;

uint8_t *pr_buf;
uint32_t pc, sp;
uint32_t ctdPort;

#ifdef RTE_PROFILE_COUNTERS
uint32_t dopktsend=0;
uint32_t inithandler=0;
#endif

#ifndef CODE_PROFILING
	uint32_t ctd_port;
#endif

#ifdef _WIN32
	struct _timeb timebuffer;
#else
	struct timeval timebuffer;
#endif

        overflow = 0;
	sp = 0;
	pc = 0;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_NETIL_INTERPRETER, " ");
	logdata(LOG_NETIL_INTERPRETER, "Starting a new NetIL program");
	logdata(LOG_NETIL_INTERPRETER, "----------------------------");
#endif

#ifdef USE_EXPERIMENTAL_SWITCH
	if (!htswitches)
		htswitches = hash_table_new (SWITCH_CACHE_SIZE);
#endif

	//entrypoint of the bytecode
	pr_buf = HandlerState->Handler->ByteCode;

	// Dimensione codice
	codelen = HandlerState->Handler->CodeSize;

	if (HandlerState->PEState->DataMem)
		datamem = HandlerState->PEState->DataMem->Base;
	
	sharedmem = HandlerState->PEState->ShdMem->Base;
	
	if (HandlerState->PEState->InitedMem)
	{
		initedmem = HandlerState->PEState->InitedMem->Base;
//		initedmem_size = HandlerState->PEState->InitedMem->Size;
	}
	else
	{
		initedmem= NULL;
//		initedmem_size= 0;
	}

	if (HandlerState->NLocals + HandlerState->StackSize > 0)
	{
		if (HandlerState->Locals == NULL && HandlerState->Stack == NULL)
		{
			stacksize = HandlerState->StackSize;
			localsnum = HandlerState->NLocals;
			locals = (uint32_t *) calloc(stacksize + localsnum, sizeof(uint32_t));

			HandlerState->Locals=locals;
			/* Initialize stack pointer */
			if (locals != NULL)
			{
				stack = locals + localsnum;
				HandlerState->Stack = stack;
			}
			else
				return nvmFAILURE;
		}
		else
		{
			stacksize = HandlerState->StackSize;
			localsnum = HandlerState->NLocals;
			locals = HandlerState->Locals;
			stack = HandlerState->Stack;
			memset(HandlerState->Stack, 0,HandlerState->StackSize * sizeof(uint32_t));
			memset(HandlerState->Locals, 0, HandlerState->NLocals * sizeof(uint32_t));
		}
	}

	if (HandlerState->Handler->HandlerType == INIT_HANDLER)
	{
		/* Init segment */
#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_NETIL_INTERPRETER, "Beginning execution of the INIT section for PE");
#endif
#ifdef RTE_PROFILE_COUNTERS
		inithandler=1;
#endif
		xbuffer = NULL;
		xbufinfo = NULL;
		pidx = -1;
	}
	else
	{
		/* Push or pull segment */
		if (HandlerState->Handler->HandlerType == PUSH_HANDLER)
		{
			/* Push */
#ifdef ENABLE_NETVM_LOGGING
			logdata(LOG_NETIL_INTERPRETER, "Beginning execution of the PUSH handler for PE");
#endif
			xbuffer = (**exbuf).PacketBuffer;
			xbufinfo = (**exbuf).InfoData;
			pktlen = (**exbuf).PacketLen;
			infolen = (**exbuf).InfoLen;
		}
		else
		{
			/* Pull */
#ifdef ENABLE_NETVM_LOGGING
			logdata(LOG_NETIL_INTERPRETER, "Beginning execution of the PULL handler section for PE");
#endif
			xbuffer = NULL;
			xbufinfo = NULL;
		}
		
		pidx = port;

		if (stack == NULL)
		{
			errorprintf(__FILE__, __FUNCTION__, __LINE__, "The NetVM stack cannot be NULL.");
			return nvmFAILURE;
		}

		/* Push the called port ID on the top of the stack */
		stack[sp] = pidx;
		sp++;
	}

	NETVM_ASSERT (pr_buf != NULL, "pr_buf is NULL!");
#ifdef RTE_PROFILE_COUNTERS
	HandlerState->ProfCounters->TicksStart= nbProfilerGetTime();
#endif

	loop = 1;
	while (loop)
	{
#if 0
		uint8_t* insn = &pr_buf[pc];
		uint32_t line = 0;
		char insnBuff[256];
		printf("executing instruction at pc: %u ==> %s\n", pc, nvmDumpInsn(insnBuff, 256, &insn, &line));
#endif
		// Exit if pc > codelen
		if( pc > codelen)
		{
			return (ret);
		}

//		if (sp > 0)
//			stack_top = stack[sp-1];

		CODE_PROFILING_INSTRUCTION_COUNTER();

		switch (pr_buf[pc])
		{


//-------------------------------STACK MANAGEMENT INSTRUCTIONS---------------------------------------------------------

			// Push a constant value onto the stack
			case PUSH:
				NEED_STACK(0);
// Warning: this piece of code must be placed *before* any modification to 'pc', since it uses its value
// to get the current instruction. This warning refers to all the similar blocks in this file.
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Pushed value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,*(int32_t *)&pr_buf[pc+1]);
#endif
				pc++;
				stack[sp] =  *(int32_t *)&pr_buf[pc];
				sp++;
				pc+=4;
				break;

			// Push two constant values onto the stack
			  case PUSH2:
				NEED_STACK(0);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, First loaded value= %d, Second loaded value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, *(int32_t *)&pr_buf[pc+1], *(int32_t *)&pr_buf[pc+5]);
#endif
				pc++;
				stack[sp] = *(int32_t *)&pr_buf[pc];
				pc+=4;
				sp += 1;
				NEED_STACK(0);
				stack[sp+1] = *(int32_t *)&pr_buf[pc];
				sp += 1;
				pc+=4;
				break;

			// Remove the top of the stack
			case POP:
				NEED_STACK(1);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				sp--;
				pc++;
				break;

			// Remove the I top values of the stack
			case POP_I:
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				NEED_STACK(pr_buf[pc]);
				sp -= *(uint8_t *)&pr_buf[pc];
				pc++;
				break;

			// Push the constant 1 onto the stack
			case CONST_1:
				NEED_STACK(0);
				stack[sp] = 1;
				sp++;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Pushed value= 1",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Push the constant 2 onto the stack
			case CONST_2:
				NEED_STACK(0);
				stack[sp] = 2;
				sp++;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Pushed value= 2",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Push the constant 0 onto the stack
			case CONST_0:
				NEED_STACK(0);
				stack[sp] = 0;
				sp++;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Pushed value= 0",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Push the constant -1 onto the stack
			case CONST__1:
				NEED_STACK(0);
				stack[sp] = (uint32_t)(-1);
				sp++;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Pushed value= -1",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Store the timestamp (seconds) into the stack
			case TSTAMP_S:
				NEED_STACK(0);
#ifdef WIN32
				_ftime(&timebuffer);
				stack[sp] = (uint32_t)(timebuffer.time);
#else
				gettimeofday(&timebuffer, NULL);
				stack[sp] = (uint32_t)(timebuffer.tv_sec);
#endif
				sp += 1;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Store the timestamp (nanoseconds) into the stack
			case TSTAMP_US:
				NEED_STACK(0);
#ifdef WIN32
				_ftime(&timebuffer);
				stack[sp] = (uint32_t)(timebuffer.millitm * 1000);
#else
				gettimeofday(&timebuffer, NULL);
				stack[sp] = (uint32_t)(timebuffer.tv_usec);
#endif
				sp += 1;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Duplication of the top operand stack value
			case DUP:
				NEED_STACK(1);
				stack[sp] = stack[sp-1];
				sp++;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Swap the top two operands stack values
			case SWAP:
				NEED_STACK(2);
				utemp1 = stack[sp-1];
				stack[sp-1] = stack[sp-2];
				stack[sp-2] = utemp1;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Change the mode between little endian and big endian
			case IESWAP:
				NEED_STACK(1);
				stack[sp-1] = nvm_ntohl(stack[sp-1]);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Returns the size of the exchange buffer
			case PBL:
				HANDLERS_ONLY;
				NEED_STACK(0);
				// [GM] What is the right meaning of this opcode?! NetVMSnort uses it to return the packet length in bytes.
				//stack[sp] = port->exchange_buffer->local_buffer_length;
// 				stack[sp] = HandlerState->Handler->CodeSize;
				stack[sp] = (**exbuf).PacketLen;
				sp++;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Packet length= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1]);
#endif
				pc++;
				break;

//------------------------------- LOAD AND STORE INSTRUCTIONS:----------------------------------

			// Load a signed 8bit int from the Exchange Buffer segment onto the stack after conversion to a 32bit int
			case PBLDS:
				HANDLERS_ONLY;
				NEED_STACK(1);
				PROF_PKT_READ(stack[sp-1],1);
				PKTMEM_CHECK(stack[sp-1] , 1 );
				stack[sp-1] = (int32_t)xbuffer[stack[sp-1]];
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1]);
#endif
				pc++;
				break;

			// Load an unsigned 8bit int from the Exchange Buffer segment onto the stack after conversion to a 32bit int
			case PBLDU:
				HANDLERS_ONLY;
				NEED_STACK(1);
				PROF_PKT_READ(stack[sp-1],1);
				PKTMEM_CHECK(stack[sp-1] , 1 );
				stack[sp-1] = (uint32_t)xbuffer[stack[sp-1]];
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value %u",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Load a signed 16bit int from the Exchange Buffer segment onto the stack after conversion to a 32bit int
			case PSLDS:
				HANDLERS_ONLY;
				NEED_STACK(1);
				PROF_PKT_READ(stack[sp-1],2);
				PKTMEM_CHECK(stack[sp-1] , 2 );
				stack[sp-1] = (int32_t)(nvm_ntohs(*(int16_t *)&xbuffer[stack[sp-1]]));
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1]);
#endif
				pc++;
				break;

			// Load an unsigned 16bit int from the Exchange Buffer segment onto the stack after conversion to a 32bit int
			case PSLDU:
				HANDLERS_ONLY;
				NEED_STACK(1);
				PROF_PKT_READ(stack[sp-1],2);
				PKTMEM_CHECK(stack[sp-1] , 2 );
				stack[sp-1] = (uint32_t)(nvm_ntohs(*(uint16_t *)&xbuffer[stack[sp-1]]));
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %u",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1]);
#endif
				pc++;
				break;

			// Load a 32bit int from the Exchange Buffer segment onto the stack
			case PILD:
				HANDLERS_ONLY;
				NEED_STACK(1);
				PROF_PKT_READ(stack[sp-1],4);
				PKTMEM_CHECK(stack[sp-1] , 4 );
				stack[sp-1] = (uint32_t)(nvm_ntohl(*(uint32_t *)&xbuffer[stack[sp-1]]));
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1]);
#endif
				pc++;
				break;

			// Load a signed 8bit int from the data memory segment onto the stack after conversion to a 32bit int
			case DBLDS:
				NEED_STACK(1);
				DATAMEM_CHECK(stack[sp-1],1);
				PROF_DATA_READ(stack[sp-1],1);
				stack[sp-1] = (int32_t)datamem[stack[sp-1]];
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1]);
#endif
				pc++;
				break;

			// Load an unsigned 8bit int from the data memory segment onto the stack after conversion to a 32bit int
			case DBLDU:
				NEED_STACK(1);
				DATAMEM_CHECK(stack[sp-1], 1);
				PROF_DATA_READ(stack[sp-1],1);
				stack[sp-1] = (uint32_t)datamem[stack[sp-1]];
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value: %u ",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Load a signed 16bit int from the data memory segment onto the stack after conversion to a 32bit int
			case DSLDS:
				NEED_STACK(1);
				DATAMEM_CHECK(stack[sp-1], 2);
				PROF_DATA_READ(stack[sp-1],2);
				stack[sp-1] = (int32_t)(*(int16_t *)&datamem[stack[sp-1]]);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Load an unsigned 16bit int from the data memory segment onto the stack after conversion to a 32bit int
			case DSLDU:
				NEED_STACK(1);
				DATAMEM_CHECK(stack[sp-1],2);
				PROF_DATA_READ(stack[sp-1],2);
				stack[sp-1] = (uint32_t)(*(uint16_t *)&datamem[stack[sp-1]]);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %u",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Load a 32bit int from the data segment onto the stack
			case DILD:
				NEED_STACK(1);
				DATAMEM_CHECK(stack[sp-1],4);
				PROF_DATA_READ(stack[sp-1],4);
				stack[sp-1] = (uint32_t)(*(uint32_t *)&datamem[stack[sp-1]]);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Load a signed 8bit int from the shared memory segment onto the stack after conversion to a 32bit int
			case SBLDS:
				NEED_STACK(1);
				SHAREDMEM_CHECK(stack[sp-1], 1);
				stack[sp-1] = (int32_t)sharedmem[stack[sp-1]];
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Load an unsigned 8bit int from the shared memory segment onto the stack after conversion to a 32bit int
			case SBLDU:
				NEED_STACK(1);
				SHAREDMEM_CHECK(stack[sp-1],1);
				stack[sp-1] = (uint32_t)sharedmem[stack[sp-1]];
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %u",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Load a signed 16bit int from the shared memory segment onto the stack after conversion to a 32bit int
			case SSLDS:
				NEED_STACK(1);
				SHAREDMEM_CHECK(stack[sp-1],2);
				stack[sp-1] = (int32_t)(*(int16_t *)&sharedmem[stack[sp-1]]);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Load an unsigned 16bit int from the shared memory segment onto the stack after conversion to a 32bit int
			case SSLDU:
				NEED_STACK(1);
				SHAREDMEM_CHECK(stack[sp-1],2);
				stack[sp-1] = (uint32_t)(*(uint16_t *)&sharedmem[stack[sp-1]]);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %u",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Load a 32bit int from the shared memory segment onto the stack
			case SILD:
				NEED_STACK(1);
				SHAREDMEM_CHECK(stack[sp-1],4);
				stack[sp-1] = (uint32_t)(*(uint32_t *)&sharedmem[stack[sp-1]]);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Load a byte from the Exchange Buffer, masks it with the 0x0f constant and multiplies it for 4.
			// Can be used to calculate the lenght of a IPv4 header
			case BPLOAD_IH:
				HANDLERS_ONLY;
				NEED_STACK(1);
				PKTMEM_CHECK(stack[sp-1] , 1 );
				stack[sp-1] = (uint32_t)(((xbuffer[stack[sp-1]]) & 0x0f) << 2);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;


			// Store a signed 8bit int from the stack to data memory after conversion from a 32 bit int
			case DBSTR:
				NEED_STACK(2);
				DATAMEM_CHECK(stack[sp-1],1);
				PROF_DATA_WRITE(stack[sp-1],1, (uint8_t) stack[sp-2]);
				datamem[stack[sp-1]] = (int8_t) stack[sp-2];
				sp -= 2;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Stored value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Store a signed 16bit int from the stack to data memory after conversion from a 32bit int
			case DSSTR:
				NEED_STACK(2);
				DATAMEM_CHECK(stack[sp-1],2);
				PROF_DATA_WRITE(stack[sp-1],2, stack[sp-2]);
				*(int16_t *)&datamem[stack[sp-1]] = (stack[sp-2]);
				sp -= 2;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Stored value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp+2]);
#endif
				pc++;
				break;

			// Store a 32bit int from the stack to data memory
			case DISTR:
				NEED_STACK (2);
				DATAMEM_CHECK(stack[sp-1],4);
				PROF_DATA_WRITE(stack[sp-1],4, stack[sp-2]);
				*(int32_t *)&datamem[stack[sp-1]] = (stack[sp-2]);
				sp -= 2;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Stored value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Store a signed 8bit int from the stack to shared memory after conversion from a 32bit int
			case SBSTR:
				NEED_STACK(2);
				SHAREDMEM_CHECK(stack[sp-1],1);
				sharedmem[stack[sp-1]] = (int8_t) stack[sp-2];
				sp -= 2;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Store a signed 16bit int from the stack to shared memory after conversion from a 32bit int
			case SSSTR:
				NEED_STACK(2);
				SHAREDMEM_CHECK(stack[sp-1],2);
				*(int16_t *)&sharedmem[stack[sp-1]] =(stack[sp-2]);
				sp -= 2;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Store a 32bit int from the stack to shared memory
			case SISTR:
				NEED_STACK(2);
				SHAREDMEM_CHECK(stack[sp-1],4);
				*(int32_t *)&sharedmem[stack[sp-1]] = (stack[sp-2]);
				sp -= 2;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Store an unsigned 8bit int from the stack to Exchange Buffer after conversion from a 32bit int
			case PBSTR:
				HANDLERS_ONLY;
				NEED_STACK(2);
				PROF_PKT_WRITE(stack[sp-1],1, (uint8_t) stack[sp-2]);
				PKTMEM_CHECK(stack[sp-1] , 1 );
				if (xbuffer != NULL)
					xbuffer[stack[sp-1]] = (uint8_t) stack[sp-2];
				sp -= 2;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Stored value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp]);
#endif
				pc++;
				break;

			// Store an unsigned 16bit int from the stack to Exchange Buffer after conversion from a 32bit int
			case PSSTR:
				HANDLERS_ONLY;
				NEED_STACK(2);
				PROF_PKT_WRITE(stack[sp-1],2, stack[sp-2]);
				PKTMEM_CHECK(stack[sp-1] , 2 );
				*(uint16_t *)&xbuffer[stack[sp-1]] = nvm_ntohs(stack[sp-2]);
				sp -= 2;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Store an unsigned 32bit int from the stack to Exchange Buffer after conversion from a 32bit int
			case PISTR:
				HANDLERS_ONLY;
				NEED_STACK(2);
				PROF_PKT_WRITE(stack[sp-1],4, stack[sp-2]);
				PKTMEM_CHECK(stack[sp-1] , 4 );
				*(uint32_t *)&xbuffer[stack[sp-1]] = nvm_ntohl(stack[sp-2]);
				sp -= 2;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Puts a random integer at the top of the stack
			case IRND:
				NEED_STACK(0);
				stack[sp] = (uint32_t) rand();
				sp++;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

//------------------------------- BIT MANIPULATION INSTRUCTIONS:----------------------------------

			//Arithmetical left shift
			case SHL:
				NEED_STACK(2);
				stack[sp-2] = ((int32_t) stack[sp-2]) << stack[sp-1];
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Arithmetical right shift
			case SHR:
				NEED_STACK(2);
				stack[sp-2] = ((int32_t) stack[sp-2]) >> stack[sp-1];
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Logical right shift
			case USHR:
				NEED_STACK(2);
				stack[sp-2] = stack[sp-2] >> stack[sp-1];
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Rotate left
			case ROTL:
				NEED_STACK(2);
				stack[sp-2] = _gen_rotl(stack[sp-2], stack[sp-1]);
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Logical right shift of 1 position
			case ROTR:
				NEED_STACK(2);
				stack[sp-2] = _gen_rotr(stack[sp-2], stack[sp-1]);
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Bitwise OR of the first two values on the stack
			case OR:
				NEED_STACK(2);
				stack[sp-2] |= stack[sp-1];
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Bitwise AND of the first two values on the stack
			case AND:
				NEED_STACK(2);
				stack[sp-2] &= stack[sp-1];
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Bitwise XOR of the first two values on the stack
			case XOR:
				NEED_STACK(2);
				stack[sp-2] ^= stack[sp-1];
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Bitwise not
			case NOT:
				NEED_STACK(1);
				stack[sp-1] = ~(stack[sp-1]);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;
/*
			//Find the first occurrence of a pattern inside a buffer in data memory. All parameters must be on the stack.
			case DSTRSRC:
				NEED_STACK(3);
				DATAMEM_CHECK(stack[sp-3]);
				j = 0;
				for (i = 0; i < stack[sp-2]; i++)
					if (stack[sp-1] == datamem[stack[sp-3]] && !(j))
						{
							stack[sp-3] = (uint32_t) i;
							j = 1;
						}
				sp-=2;
				pc++;
				break;


			//Find the first occurrence of a pattern inside a buffer in data memory. All parameters must be on the stack.
			case PSTRSRC:
				NEED_STACK(3);
				DATAMEM_CHECK(stack[sp-3]);
				j = 0;
				for (i = 0; i < stack[sp-2]; i++)
					if (stack[sp-1] == datamem[stack[sp-3]] && !(j))
						{
							stack[sp-3] = (uint32_t) i;
							j = 1;
						}
				sp-=2;
				pc++;
				break;
*/
//-------------------------------- ARITMHETIC INSTRUCTIONS ------------------------------------------


			// Addition of the two top items of the stack without overflow control
			case ADD:
				NEED_STACK(2);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-2]= %d, stack[SP-1]= %d, result= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2],stack[sp-2] + stack[sp-1]);
#endif
				stack[sp-2] = (int32_t) (stack[sp-2] + stack[sp-1]);
				sp--;
				pc++;
				break;

			// Addition of the two top items of the stack considered as signed 32bit int with overflow control
			case ADDSOV:
				NEED_STACK(2);
				if ((SIGN((int32_t) (stack[sp-1])) == SIGN((int32_t) (stack[sp-2]))) && (SIGN((int32_t) stack[sp-1] + (int32_t) stack[sp-2]) != SIGN((int32_t) stack[sp-1])))
					overflow = 1;
				stack[sp-2] += (int32_t) stack[sp-1];
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Addition of the two top items of the stack considered as unsigned 32bit int with overflow control
			case ADDUOV:
				NEED_STACK(2);
				if ((stack[sp-2] += stack[sp-1]) <= stack[sp-1])
					overflow=1;
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Subtraction the first value on the stack to the second one without overflow control
			case SUB:
				NEED_STACK(2);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[sp-2]= %d, stack[sp-1]= %d, result= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-2],stack[sp-1],stack[sp-2]-stack[sp-1]);
#endif
				stack[sp-2] -= stack[sp-1];
				sp--;
				pc++;
				break;

			// Subtraction the first value on the stack to the second one considered as signed 32bit int with overflow control
			case SUBSOV:
				//TODO if overflow=1;
				NEED_STACK(2);
				stack[sp-2] -= (int32_t) stack[sp-1];
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Subtraction the first value on the stack to the second one considered as unsigned 32bit int with overflow control
			case SUBUOV:
				NEED_STACK(2);
				if (stack[sp-1] > stack[sp-2])
					overflow=1;
				stack[sp-2] -= stack[sp-1];
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, First value= %d, Second value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1], stack[sp]);
#endif
				pc++;
				break;

			case MOD:
				NEED_STACK(2);
				stack[sp-2] %= stack[sp-1];
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, First value= %d, Second value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1], stack[sp]);
#endif
				pc++;
				break;

			//Negate the value of the element at the top of the stack
			case NEG:
				NEED_STACK(1);
				stack[sp-1] = - (int32_t) stack[sp-1];
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Multiplication of the first two values on the stack
			case IMUL:
				NEED_STACK(2);
				stack[sp-2] *= stack[sp-1];
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Multiplication of the first two values on the stack considered as signed 32 bit
			case IMULSOV:
				//TODO if overflow=1;
				NEED_STACK(2);
				stack[sp-2] *= (int32_t) stack[sp-1];
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;


			// Multiplication of the first two values on the stack considered as unsigned 32 bit
			case IMULUOV:
				//TODO if overflow=1;
				NEED_STACK(1);
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;


			// Increment the first element on the stack
			case IINC_1:
				NEED_STACK(1);
				(stack[sp-1])++;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Increment the second element on the stack
			case IINC_2:
				NEED_STACK(2);
				(stack[sp-2])++;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Increment the third element on the stack
			case IINC_3:
				NEED_STACK(3);
				(stack[sp-3])++;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Increment the fourth element on the stack
			case IINC_4:
				NEED_STACK(4);
				(stack[sp-4])++;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Decrement the first element on the stack
			case IDEC_1:
				NEED_STACK(1);
				(stack[sp-1])--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Decrement the second element on the stack
			case IDEC_2:
				NEED_STACK(2);
				(stack[sp-2])--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Dencrement the third element on the stack
			case IDEC_3:
				NEED_STACK(3);
				(stack[sp-3])--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Decrement the fourth element on the stack
			case IDEC_4:
				NEED_STACK(4);
				(stack[sp-4])--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

//--------------------------PACKET SCAN INSTRUCTIONS---------------------------------------------------------------


			case PSCANB:
				NEED_STACK(2);
				PKTMEM_CHECK(stack[sp-2] ,  1 );
				stack[sp-2] = do_pscanb((**exbuf).PacketBuffer, stack[sp-2],(uint8_t)stack[sp-1], (**exbuf).PacketLen);
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			case PSCANW:
				NEED_STACK(2);
				PKTMEM_CHECK(stack[sp-2] ,  1 );
				stack[sp-2] = do_pscanw((**exbuf).PacketBuffer, stack[sp-2], (uint16_t)stack[sp-1], (**exbuf).PacketLen);
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;
			case PSCANDW:
				NEED_STACK(2);
				PKTMEM_CHECK(stack[sp-2] ,  1 );
				stack[sp-2] = do_pscandw((**exbuf).PacketBuffer, stack[sp-2], stack[sp-1], (**exbuf).PacketLen);
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

//--------------------------FLOW CONTROL INSTRUCTIONS---------------------------------------------------------------

			// Branch if the first element on the stack is equal to the second one
			case JCMPEQ:
				NEED_STACK(2);
				//printf("fa la jcmpeq tra %d e%d\n",stack[sp-1],stack[sp-2]);
				if (stack[sp-1] == stack[sp-2])
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Result= equal",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Result= not equal",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				}
				sp-=2;
				pc+=5;

				break;

			// Branch if the first element on the stack is not equal to the second one
			case JCMPNEQ:
				NEED_STACK(2);
				if (stack[sp-1] != stack[sp-2])
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Result= not equal",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Result= equal",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				}
				sp-=2;
				pc+=5;

				break;

			// Branch if the second element on the stack is greater than the first one, considering both elements as unsigned int
			case JCMPG:
				NEED_STACK(2);
				if (stack[sp-1] < stack[sp-2])
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif

					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
				}
				sp-=2;
				pc+=5;
				break;

			// Branch if the second element on the stack is greater than the first one, considering both elements as signed int
			case JCMPG_S:
				NEED_STACK(2);
				if (((int32_t) stack[sp-1]) < ((int32_t) stack[sp-2]))
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
				}
				sp-=2;
				pc+=5;
				break;

			// Branch if the second element on the stack is equal to or greater than the first one, considering both elements as unsigned int
			case JCMPGE:
				NEED_STACK(2);
				if (stack[sp-2] >= stack[sp-1])
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
				}
				sp-=2;
				pc+=5;
				break;

			// Branch if the second element on the stack is equal to or greater than the first one, considering both elements as signed int
			case JCMPGE_S:
				NEED_STACK(2);
				if (((int32_t) stack[sp-1]) <= ((int32_t) stack[sp-2]))
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
				}

				sp-=2;
				pc+=5;
				break;

			// Branch if the second element on the stack is lower than the first one, considering both elements as unsigned int
			case JCMPL:
				NEED_STACK(2);
				if (stack[sp-1] > stack[sp-2])
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
				}
				sp-=2;
				pc+=5;

				break;

			// Branch if the second element on the stack is greater than the first one, considering both elements as signed int
			case JCMPL_S:
				NEED_STACK(2);
				if (((int32_t) stack[sp-1]) > ((int32_t) stack[sp-2]))
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
				}

				sp-=2;
				pc+=5;
				break;

			// Branch if the second element on the stack is equal to or lower than the first one, considering both elements as unsigned int
			case JCMPLE:
				NEED_STACK(2);
				if (stack[sp-1] >= stack[sp-2])
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
				}

				sp-=2;
				pc+=5;
				break;

			// Branch if the second element on the stack is equal to or lower than the first one, considering both elements as signed int
			case JCMPLE_S:
				NEED_STACK(2);
				if ((int32_t) stack[sp-1] >= (int32_t) stack[sp-2])
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
				
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
				}

				sp-=2;
				pc+=5;
				break;

			// Branch if the first element on the stack is equal to 0
			case JEQ:
				NEED_STACK(1);
				if (stack[sp-1] == 0)
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				}
				sp--;
				pc+=5;
				break;

			// Branch if the first element on the stack is not equal to 0
			case JNE:
				NEED_STACK(1);
				if (stack[sp-1] != 0)
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, stack[SP-1]= %d, stack[SP-2]= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1],stack[sp-2]);
#endif
				}

				sp--;
				pc+=5;
				break;

			// Unconditional branch
			case JUMP:
				NEED_STACK(0);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				pc += pr_buf[pc] + 1;
				JUMPCHECK(pc);
				break;

			// Unconditional branch (wide index)
			case JUMPW:
				NEED_STACK(0);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				pc += *(int32_t*)&pr_buf[pc] + 4;
				JUMPCHECK(pc);
				break;

			// Switch - This instruction provides for multiple branch possibilities
			case SWITCH:
				NEED_STACK(1);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp,stack[sp-1]);
#endif
				pc++;
				gen_do_switch(pr_buf, &pc, stack[sp-1]);
				JUMPCHECK(pc);
				sp--;
				break;

			// Call - Call a subroutine with an 8 bit offset
			case CALL:
				NEED_STACK(0);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				pc+=pr_buf[pc];
				JUMPCHECK(pc);
				break;

			// Ret - return from subroutine
			case RET:
				NEED_STACK(0);
				if (sp != 0)
				{
					errorprintf(__FILE__, __FUNCTION__, __LINE__, "The stack pointer is not zero at return time (sp = %d)\n", sp);
					ret = nvmFAILURE;
				}
				else
					ret = nvmSUCCESS;

				if (INIT_SEGMENT && locals)
					free (locals);
				loop = 0;

#ifdef RTE_PROFILE_COUNTERS
				if (dopktsend == 0 && inithandler == 0)
					HandlerState->ProfCounters->TicksEnd= nbProfilerGetTime();
#endif

#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				return ret;
				break;

//--------------------------------- COMPARISON INSTRUCTIONS ---------------------------------------------

			// Comparison of the first two items on the stack
			case CMP:
				NEED_STACK(2);
				if (stack[sp-1] == stack[sp-2])
					stack[sp-2] = (int32_t) 0;
				else if (stack[sp-1] > stack[sp-2])
					stack[sp-2] = (int32_t) 1;
				else
					stack[sp-2] = (uint32_t) (-1);
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Signed comparison of the first two items on the stack
			case CMP_S:
				NEED_STACK(2);
				if (((int32_t) stack[sp-1]) == ((int32_t) stack[sp-2]))
					stack[sp-2] = (int32_t) 0;
				else if (((int32_t) stack[sp-1]) > ((int32_t) stack[sp-2]))
					stack[sp-2] = (int32_t) 1;
				else
					stack[sp-2] = (uint32_t)(-1);
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Masked comparison of the first two items on the stack
			case MCMP:
				NEED_STACK(3);
				utemp1 = stack[sp-1] & stack[sp-3];
				utemp2 = stack[sp-2] & stack[sp-3];
				if (utemp1 == utemp2)
					stack[sp-3] = (int32_t) 0;
				else if (utemp1 > utemp2)
					stack[sp-3] = (int32_t) 1;
				else
					stack[sp-3] = (uint32_t)(-1);
				sp -= 2;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

//----------------------------------------PATTERN MATCHING INSTRUCTIONS------------------------------------------

			// Comparison between two fields in the packet buffer. Branch if equal
			case JFLDEQ:
				HANDLERS_ONLY;
				NEED_STACK(3);

				PKTMEM_CHECK(stack[sp-2] ,  stack[sp-1] );
				PKTMEM_CHECK(stack[sp-3] ,  stack[sp-1] );

				if (memcmp(&xbuffer[stack[sp-3]], &xbuffer[stack[sp-2]], stack[sp-1]) == 0)
				{
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				sp -= 3;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc +=5;
				break;

			// Comparison between two fields in the packet buffer. Branch if not equal
			case JFLDNEQ:
				HANDLERS_ONLY;
				NEED_STACK(3);

				PKTMEM_CHECK(stack[sp-2] ,  stack[sp-1] );
				PKTMEM_CHECK(stack[sp-3] ,  stack[sp-1] );

				if (memcmp(&xbuffer[stack[sp-3]], &xbuffer[stack[sp-2]], stack[sp-1]) != 0)
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				}

				sp -= 3;
				pc +=5;
				break;

			// Comparison between two fields in the packet buffer. Branch if the first i lexicographically less than the second
			case JFLDLT:
				HANDLERS_ONLY;
				NEED_STACK(3);

				PKTMEM_CHECK(stack[sp-2] ,  stack[sp-1] );
				PKTMEM_CHECK(stack[sp-3] ,  stack[sp-1] );

				if (memcmp(&xbuffer[stack[sp-3]], &xbuffer[stack[sp-2]], stack[sp-1]) < 0)
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				}
				sp -= 3;
				pc +=5;
				break;

			// Comparison between two fields in the packet buffer. Branch if the first i lexicographically greater than the second
			case JFLDGT:
				HANDLERS_ONLY;
				NEED_STACK(3);

				PKTMEM_CHECK(stack[sp-2] ,  stack[sp-1] );
				PKTMEM_CHECK(stack[sp-3] ,  stack[sp-1] );

				if (memcmp(&xbuffer[stack[sp-3]], &xbuffer[stack[sp-2]], stack[sp-1]) > 0)
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
					pc += *(int32_t*)&pr_buf[pc+1];
					JUMPCHECK(pc + 5);
				}
				else
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
						nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				}
				sp -= 3;
				pc +=5;

				break;



//--------------------------- DATA TRANSFER INSTRUCTIONS -------------------------------

			//Push an exchange buffer to a port
			case SNDPKT:
				HANDLERS_ONLY;
				NEED_STACK(0);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, Port=%d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, *(uint32_t *) &pr_buf[pc]);
#endif
				pc++;
				ret = nvmSUCCESS;

#ifdef RTE_PROFILE_COUNTERS
				dopktsend=1;
  				HandlerState->ProfCounters->TicksEnd= nbProfilerGetTime();
				HandlerState->ProfCounters->NumFwdPkts++;
#endif

// This code of CODE_PROFILING option should be commented because you need to
// evaluate only the cost of the netvm program, and you don't want to call the handler
#ifndef CODE_PROFILING
				ctd_port = *(uint32_t *) &pr_buf[pc];
				ret = nvmNetPacket_Send(*exbuf,ctd_port,HandlerState);
#endif			
				// In case you send the packet out, you do not execute any more instruction.
				// But... does it make sense?
				return ret;
				break;

			// Send a copy of the exchange buffer to a port
			case DSNDPKT:
				HANDLERS_ONLY;
				NEED_STACK(0);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				nvmNetPacketSendDup (*exbuf,*(uint32_t *) &pr_buf[pc] , HandlerState);
				pc+=4;
				break;

			// Pull an exchange buffer from a port
			case RCVPKT:
				HANDLERS_ONLY;
				NEED_STACK(0);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				ctdPort = *(uint32_t *) &pr_buf[pc];
				nvmNetPacket_Recive(exbuf,ctdPort,HandlerState);

				xbuffer = (**exbuf).PacketBuffer;
				xbufinfo = (**exbuf).InfoData;

				pktlen = (**exbuf).PacketLen;
				infolen = (**exbuf).InfoLen;

				pc+=4;
				break;

			// Copy a memory buffer from data memory to Exchange Buffer
			case DPMCPY:
				HANDLERS_ONLY;
				NEED_STACK(3);

				PKTMEM_CHECK(stack[sp-2] ,  stack[sp-1] );
				DATAMEM_CHECK(stack[sp-3] ,  stack[sp-1]);

				PROF_PKT_WRITE(stack[sp-2] ,  stack[sp-1], NULL );
				PROF_DATA_READ(stack[sp-3] ,  stack[sp-1] );

				for (i = 0; i < stack[sp-1]; i++)
					xbuffer[stack[sp-2] + i] = datamem[stack[sp-3] + i];

				sp-=3;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Copy a memory buffer from Exchange Buffer to data memory
			case PDMCPY:
				HANDLERS_ONLY;
				NEED_STACK(3);

				DATAMEM_CHECK(stack[sp-2] ,  stack[sp-1]);
				PKTMEM_CHECK(stack[sp-3] ,  stack[sp-1] );

				PROF_DATA_WRITE(stack[sp-2] ,  stack[sp-1], 0 );
				PROF_PKT_READ(stack[sp-3] ,  stack[sp-1] );


				for (i = 0; i < stack[sp-1]; i++)
					datamem[stack[sp-2] + i] = xbuffer[stack[sp-3] + i];
				sp-=3;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Copy a memory buffer from data memory to shared memory
			case DSMCPY:
				NEED_STACK(3);

				SHAREDMEM_CHECK(stack[sp-2] ,  stack[sp-1]);
				DATAMEM_CHECK(stack[sp-3] ,  stack[sp-1]);

				PROF_DATA_READ(stack[sp-3] ,  stack[sp-1] );

				for (i = 0; i < stack[sp-1]; i++)
					sharedmem[stack[sp-2] + i] = datamem[stack[sp-3] + i];
				sp-=3;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Copy a memory buffer from shared memory to data memory
			case SDMCPY:
				NEED_STACK(3);

				DATAMEM_CHECK(stack[sp-2] ,  stack[sp-1]);
				SHAREDMEM_CHECK(stack[sp-3] ,  stack[sp-1]);

				PROF_DATA_WRITE(stack[sp-2] ,  stack[sp-1], 0 );

				for (i = 0; i < stack[sp-1]; i++)
					datamem[stack[sp-2] + i] = sharedmem[stack[sp-3] + i];
				sp-=3;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Copy a memory buffer from shared memory to Exchange Buffer
			case SPMCPY:
				HANDLERS_ONLY;
				NEED_STACK(3);

				SHAREDMEM_CHECK(stack[sp-3] ,  stack[sp-1]);
				PKTMEM_CHECK(stack[sp-2] ,  stack[sp-1] );

				PROF_PKT_WRITE(stack[sp-2] ,  stack[sp-1], NULL );

				for (i = 0; i < stack[sp-1]; i++)
					xbuffer[stack[sp-2] + i] = sharedmem[stack[sp-3] + i];
				sp-=3;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Copy a memory buffer from Exchange Buffer to shared memory
			case PSMCPY:
				HANDLERS_ONLY;
				NEED_STACK(3);

				SHAREDMEM_CHECK(stack[sp-2] ,  stack[sp-1]);
				PKTMEM_CHECK(stack[sp-3] ,  stack[sp-1] );

				PROF_PKT_READ(stack[sp-3] ,  stack[sp-1] );

				for (i = 0; i < stack[sp-1]; i++)
					sharedmem[stack[sp-2] + i] = xbuffer[stack[sp-3] + i];
				sp-=3;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Create a new exchange buffer and puts it in the Connection Buffer
			case CRTEXBUF:
				HANDLERS_ONLY;
				NEED_STACK(1);
				*exbuf= arch_GetExbuf(HandlerState->PEState->RTEnv);
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Delete the exchange buffer
			case DELEXBUF:
				HANDLERS_ONLY;
				NEED_STACK(0);
				arch_ReleaseExbuf(HandlerState->PEState->RTEnv, *exbuf);
				//TODO ho messo exbuf a null ma non ne sono affatto sicuro
				*exbuf=NULL;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;


//------------------------------MISCELLANEOUS nvmOPCODES------------------------------------------

			//Nop - no operationsroutine with a wide range
			case NOP:
				NEED_STACK(0);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Exit from a program returning an integer value
			case EXIT:
				NEED_STACK(0);
				exit(*(int32_t *)&pr_buf[pc+1]);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				break;

			//Debug this instruction only
			case BRKPOINT:
				NEED_STACK(0);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

//------------------------------------MACROINSTRUCTIONS-------------------------------------------

//-----------------------------------BIT MANIPULATION MACROINSTRUCTIONS---------------------------

			// Find the first bit set in the top value of the stack
			case FINDBITSET:
				NEED_STACK(1);
				i = 31;
				if (stack[sp-1] != 0)
					{
						while ((stack[sp-1] & (1 << i)) == 0)
							i--;
						stack[sp-1] = i;
					}
				else
					stack[sp-1] = 0xffffffff;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Find the first bit set in the top values of the stack, with mask
			case MFINDBITSET:
				NEED_STACK(2);
				i = 31;
				if ((stack[sp-1] & stack[sp-2]) != 0)
				{
					while (((stack[sp-1] & stack[sp-2]) & (1 << i)) == 0)
						i--;
					stack[sp-2] = i;
					sp--;
				}
				else
				{
					stack[sp-2] = 0xffffffff;
					sp--;
				}
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Find the first bit set in the top values of the stack
			case XFINDBITSET:
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				NEED_STACK(pr_buf[pc]);
				for (j = 0; j < pr_buf[pc+1]; j++)
				{
					i = 31;
					if (stack[sp-1-j] != 0)
						{
							while ((stack[sp-1-j] & (1 << i)) == 0)
								i--;
							stack[sp-1-j] = (uint32_t)i;
						}
					else
						stack[sp-1] = 0xffffffff;
				}
				pc++;
				break;


			// Find the first bit set in the top values of the stack, with mask
			case XMFINDBITSET:
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				NEED_STACK(pr_buf[pc]);
				utemp1 = stack[sp-pr_buf[pc]];
				for (j = 0; j < pr_buf[pc]; j++)
				{
					i = 31;
					if ((stack[sp-pr_buf[pc]+j] & utemp1) != 0)
						{
							while (( stack[sp-pr_buf[pc]+j] & utemp1 & (1 << i)) == 0)
								i--;
							stack[sp-pr_buf[pc]+j-1] = (uint32_t) i;
						}
					else
						stack[sp-pr_buf[pc]+j-1] = 0xffffffff;
				}
				sp--;
				pc++;
				break;

			// Count the consecutive number of 0 starting from the MSB
			case CLZ:
				NEED_STACK(1);
				if (stack[sp-1] == 0)
					stack[sp-1] = 32;
				else
				{
					i=31;
					utemp1=0;
					while (((stack[sp-1] | (_gen_rotl(0xfffffffe,i))) != 0xffffffff) & (i > 0))		//TODO: "& i > 0?!?"
						{
							i--;
							utemp1++;
						}
					stack[sp-1] = utemp1;
				}
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Set the bit specified at the top of the stack from the second value at the top of the stack
			case SETBIT:
				NEED_STACK(2);
				if (stack[sp-1] < 32)
				stack[sp-2] |= _gen_rotl(1, stack[sp-1]);
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Invert the bit specified at hte top of the stack from the second value at the top of the stack
			case FLIPBIT:
				NEED_STACK(2);
				if (stack[sp-1] < 32)
				{
					if ((stack[sp-2] | (_gen_rotl(0xfffffffe, stack[sp-1]))) == 0xffffffff)
						stack[sp-2] &= (_gen_rotl(0xfffffffe, stack[sp-1]));
					else
						stack[sp-2] |= (_gen_rotl (1, stack[sp-1]));
				}
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Clear the bit specified at hte top of the stack from the second value at the top of the stack
			case CLEARBIT:
				NEED_STACK(2);
				if (stack[sp-1] < 32)
				stack[sp-2] &= _gen_rotl(0xfffffffe, stack[sp-1]);
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Test if the top value of the stack bit of the second top value of the stack and push its value on the stack
			case TESTBIT:
				NEED_STACK(2);
				if (stack[sp-1] < 32)
				{
					if ((stack[sp-2] | (_gen_rotl(0xfffffffe, stack[sp-1]))) == 0xffffffff)
						stack[sp-2] = 1;
					else
						stack[sp-2] = 0;
				}
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			//Test if the top value of the stack bit of the second top value of the stack
			//and push its negated value on the stack
			case TESTNBIT:
				NEED_STACK(2);
				if (stack[sp-1] < 32)
				{
					if ((stack[sp-2] | (_gen_rotl(0xfffffffe, stack[sp-1]))) != 0xffffffff)
						stack[sp-2] = 1;
					else
						stack[sp-2] = 0;
				}
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

//---------------------------------------- Info field manipulation instructions ----------------------------------------

			// Store a signed 8bit int from the stack to the Exchange Buffer info field after conversion from a 32bit int
			case IBSTR:
				HANDLERS_ONLY;
				NEED_STACK(2);

				PROF_INFO_WRITE(stack[sp-1],1, (uint8_t)stack[sp-2]);
				INFOMEM_CHECK(stack[sp-1] ,  1 );

				xbufinfo[stack[sp-1]] = (uint8_t) stack[sp-2];
				sp -= 2;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Store a signed 16bit int from the stack to the Exchange Buffer info field after conversion from a 32bit int
			case ISSTR:
				HANDLERS_ONLY;
				NEED_STACK(2);

				PROF_INFO_WRITE(stack[sp-1],2, stack[sp-2]);
				INFOMEM_CHECK(stack[sp-1] ,  2 );

				*(int16_t *)&xbufinfo[stack[sp-1]] = (stack[sp-2]);
				sp -= 2;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Value= %d, Position= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp],stack[sp+1]);
#endif
				pc++;
				break;

			// Store a 32it int from the stack to the Exchange Buffer info field
			case IISTR:
				HANDLERS_ONLY;
				NEED_STACK(2);

				PROF_INFO_WRITE(stack[sp-1],4, stack[sp-2]);
				INFOMEM_CHECK(stack[sp-1] ,  4 );

				*(int32_t *)&xbufinfo[stack[sp-1]] = (stack[sp-2]);
				sp -= 2;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Load an unsigned 8bit int from the Exchange Buffer info field onto the stack after conversion to a 32bit int
			case ISBLD:
				HANDLERS_ONLY;
				NEED_STACK(1);

				PROF_INFO_READ(stack[sp-1],1);
				INFOMEM_CHECK(stack[sp-1] ,  1 );

				stack[sp-1] = (uint32_t)xbufinfo[stack[sp-1]];
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %u",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Load an unsigned 16bit int from the Exchange Buffer info field onto the stack after conversion to a 32bit int
			case ISSLD:
				HANDLERS_ONLY;
				NEED_STACK(1);

				PROF_INFO_READ(stack[sp-1],2);
				INFOMEM_CHECK(stack[sp-1] ,  2 );

				stack[sp-1] = (uint32_t)(*(uint16_t *)&xbufinfo[stack[sp-1]]);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				break;

			// Load a signed 8bit int from the Exchange Buffer info field onto the stack after conversion to a 32bit int
			case ISSBLD:
				HANDLERS_ONLY;
				NEED_STACK(1);

				INFOMEM_CHECK(stack[sp-1] ,  1 );

				stack[sp-1] = (int32_t)xbufinfo[stack[sp-1]];
				PROF_INFO_READ(stack[sp-1],1);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Load a signed 16bit int from the Exchange Buffer info field onto the stack after conversion to a 32bit int
			case ISSSLD:
				HANDLERS_ONLY;
				NEED_STACK(1);

				PROF_INFO_READ(stack[sp-1],2);
				INFOMEM_CHECK(stack[sp-1] ,  2 );

				stack[sp-1] = (int32_t)(*(int16_t *)&xbufinfo[stack[sp-1]]);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

			// Load a 32bit int from the Exchange Buffer info field onto the stack
			case ISSILD:
				HANDLERS_ONLY;
				NEED_STACK(1);

				PROF_INFO_READ(stack[sp-1],4);
				INFOMEM_CHECK(stack[sp-1] ,  4 );

				stack[sp-1] = (uint32_t)(*(int32_t *)&xbufinfo[stack[sp-1]]);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp-1]);
#endif
				pc++;
				break;

//------------------------------------------- Locals management instructions -------------------------------------------

			//load an indexed local on the top of the stack
			case LOCLD:
				NEED_STACK(0);
				pc++;
				utemp1 = *(int32_t *)&pr_buf[pc];
				LOCALS_CHECK(utemp1);
				stack[sp] = locals[utemp1];
				pc+=4;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Loaded value= %d, Local= %d",
					nvmOpCodeTable[pr_buf[pc-5]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp], utemp1);
#endif
				sp++;
				break;

			//stores in the indexed local the value on the top of the stack
			case LOCST:
				NEED_STACK(1);
				pc++;
				utemp1 = *(int32_t *)&pr_buf[pc];
				LOCALS_CHECK(utemp1);
				locals[utemp1] = stack[sp-1];
				sp--;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Stored value= %d, Local= %d",
					nvmOpCodeTable[pr_buf[pc-1]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, stack[sp], utemp1);
#endif
				pc+=4;
				break;

			case HASH:
				pc++;
				utemp1 = pr_buf[pc];
				NEED_STACK(utemp1);
				sp -= utemp1;
				utemp2 = nvmHash((uint8_t*)&stack[sp], utemp1 * 4);
				stack[sp] = utemp2;
				sp++;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Num stack values= %d, Hash= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, utemp1, utemp2);
#endif
				pc++;
				break;

			case HASH32:
				NEED_STACK(1);
				utemp1 = nvmHash((uint8_t*)&stack[sp - 1], 4);
				stack[sp-1] = utemp1;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Hash= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, utemp1);
#endif
				pc++;
				break;

//------------------------------------ Instructions for interaction with coprocessors ----------------------------------
			case COPINIT:
				NEED_STACK(0);
				pc++;
				utemp1 = *(uint32_t *) &(pr_buf[pc]);		// Coprocessor ID
				utemp2 = *(uint32_t *) &(pr_buf[pc + 4]);	// Offset into initedmem
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Coprocessor= %u, Offset= %u",
					nvmOpCodeTable[pr_buf[pc-1]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, utemp1, utemp2);
#endif
				COPROCESSOR_CHECK(utemp1, 0);			// We suppose every coprocessor has at least 1 register
				stack[sp] = (HandlerState->PEState->CoprocTable)[utemp1].init (&(HandlerState->PEState->CoprocTable)[utemp1], initedmem + utemp2);
				sp++;
				pc += 8;
				break;

			case COPIN:
				pc++;
				utemp1 = *(uint32_t *) &(pr_buf[pc]);		// Coprocessor ID
				utemp2 = *(uint32_t *) &(pr_buf[pc + 4]);	// Coprocessor register
				COPROCESSOR_CHECK(utemp1, utemp2);
				(HandlerState->PEState->CoprocTable)[utemp1].read (&(HandlerState->PEState->CoprocTable)[utemp1], utemp2, &utemp3);		// Read from coprocessor
				stack[sp] = utemp3;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Coprocessor= %u, Register= %u, Value= %u",
					nvmOpCodeTable[pr_buf[pc-1]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, utemp1, utemp2, utemp3);
#endif
				sp++;
				pc += 8;
				break;

			case COPOUT:
				NEED_STACK(1);
				pc++;
				utemp1 = *(uint32_t *) &(pr_buf[pc]);		// Coprocessor ID
				utemp2 = *(uint32_t *) &(pr_buf[pc + 4]);	// Coprocessor register
				COPROCESSOR_CHECK(utemp1, utemp2);
				utemp3 = stack[sp - 1];
				(HandlerState->PEState -> CoprocTable)[utemp1].write (&(HandlerState->PEState -> CoprocTable)[utemp1], utemp2, &utemp3);		// Write to coprocessor
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Coprocessor= %u, Register= %u, Value= %u",
					nvmOpCodeTable[pr_buf[pc-1]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, utemp1, utemp2, utemp3);
#endif
				sp--;
				pc += 8;
				break;

			case COPRUN:
				pc++;
				utemp1 = *(uint32_t *) &(pr_buf[pc]);		// Coprocessor ID
				utemp2 = *(uint32_t *) &(pr_buf[pc + 4]);	// Coprocessor operation
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Coprocessor= %u, Operation= %u",
					nvmOpCodeTable[pr_buf[pc-1]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, utemp1, utemp2);

#endif
				COPROCESSOR_CHECK(utemp1, 0);
				(HandlerState->PEState -> CoprocTable)[utemp1].invoke (&(HandlerState->PEState ->CoprocTable)[utemp1], utemp2);
				pc += 8;
				break;

			case COPPKTOUT:
				HANDLERS_ONLY;
				pc++;
				utemp1 = *(uint32_t *) &(pr_buf[pc]);		// Coprocessor ID
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Coprocessor= %u",
					nvmOpCodeTable[pr_buf[pc-1]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, utemp1);
#endif
				// 2nd arg momentarily unused
				COPROCESSOR_CHECK(utemp1, 0);
				(HandlerState->PEState -> CoprocTable)[utemp1].xbuf = *exbuf;
				pc += 8;
				break;

			case INFOCLR:
				HANDLERS_ONLY;
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d",
					nvmOpCodeTable[pr_buf[pc]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				pc++;
				MMSET(xbufinfo,0,infolen);
				break;



//-------------------------------------- Instructions available only in the INIT segment ------------------------------------
			case SSMSIZE:
				INIT_SEGMENT_ONLY;
				NEED_STACK(0);
				pc++;
				utemp1 = *(uint32_t *) &pr_buf[pc];
				if ( HandlerState->PEState->ShdMem == NULL && HandlerState->PEState->ShdMem->Size == 0)
				{
					/* PE wants to use shared memory, and it is the first one making such a request.
					   So, allocate the memory. */
					if (utemp1 > 0)
					{
						sharedmem_size = utemp1;
						sharedmem = (uint8_t *) malloc (utemp1);
						if (sharedmem == NULL)
						{
							errorprintf(__FILE__, __FUNCTION__, __LINE__, "%s: Error in shared memory allocation\n", HandlerState->Handler->OwnerPE->Name);
							return nvmFAILURE;
						}
					}
					else
					{
						sharedmem = NULL;
						sharedmem_size = 0;
					}
					HandlerState->PEState->ShdMem = (nvmMemDescriptor *)sharedmem;
					HandlerState->PEState->ShdMem->Size = sharedmem_size;
					//HandlerState->PEState -> sml = sharedmem_size;
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Allocated memory= %u, Pointer= %u",
						nvmOpCodeTable[pr_buf[pc-1]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, sharedmem_size, sharedmem);
#endif
				}
				else
				{
					if (utemp1 == HandlerState->PEState->ShdMem->Size)
					{
						/* PE wants to use shared memory, and this has already been allocated by a previous
						   PE. The requested size matches the allocated size. Just init needed variables. */
						sharedmem = (uint8_t *) HandlerState->PEState->ShdMem;
						sharedmem_size = HandlerState->PEState->ShdMem->Size;
#ifdef ENABLE_NETVM_LOGGING
						logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Already allocated memory= %u",
							nvmOpCodeTable[pr_buf[pc-1]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp, utemp1);
#endif
					}
					else
					{
						/* PE wants to use shared memory, and this has already been allocated. Although, the
						   requested size differs with what has been allocated. Return an error. */
#ifdef ENABLE_NETVM_LOGGING
						logdata(LOG_NETIL_INTERPRETER, "%s; Handler= %d, Pidx= %d, SP= %d, Failure in allocating memory",
							nvmOpCodeTable[pr_buf[pc-1]].CodeName, HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
						errorprintf(__FILE__, __FUNCTION__, __LINE__, "%s: Requested shared memory size differs with previously allocated size.\n", HandlerState->Handler->OwnerPE->Name);
						return nvmFAILURE;
					}
				}
				pc+=4;	/* This is always needed */
				break;

			default:
				//If an erroneous instruction is found simply ignore it
				assert(1);
				errorprintf(__FILE__, __FUNCTION__, __LINE__, "nvm %s: Unknown Opcode: %x error, doing nothing\n",HandlerState->Handler->OwnerPE->Name, pr_buf[pc]);
#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_NETIL_INTERPRETER, "Instruction not valid; Handler= %d, Pidx= %d, SP= %d",
					HandlerState->Handler->OwnerPE->Name, pidx, sp);
#endif
				return nvmFAILURE;
				break;

		} /* End of switch() */
	} /* End of while() */

        // suppress gcc warning
        (void) overflow;

	return (ret);
}
