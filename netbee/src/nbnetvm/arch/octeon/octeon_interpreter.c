/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





/** @file octeon_interpreter.c
 * \brief This file contains the functions that implement the interpreter of the bytecode.
 *
 */

//
// FULVIO 4/02/2011
// WARNING
// This file should be pretty equivalent to the standard generic interpreter,
// but I'm pretty sure that there are some differencies. I believe this file
// has not been updated from long time, so please beware if you use
// the code here (I suggest to have a look at the generic interpreter and
// check that all the opcodes are fine)
//

#include "octeon_interpreter.h"
#include "../../opcodes.h"
#include <stdlib.h>
#include "assert.h"
#include <sys/timeb.h>		//needed for the implementation of the TSTAMP opcode
#include "arch.h"
#include "cvmx.h"
#include "cvmx-config.h"
#include "cvmx-pow.h"

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


void gen_do_switch(uint8_t  *pr_buf, int32_t *pc, uint32_t value)
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
		ERR1("Instruction %x can only be used in the INIT segment\n", pr_buf[pc]); \
		return nvmFAILURE; \
	}

///< Macro to be used in instructions that can only appear in the PUSH or PULL handler of a PE
#define HANDLERS_ONLY \
	if (INIT_SEGMENT) { \
		ERR1("Instruction %x can only be used in the PUSH or PULL handler\n", pr_buf[pc]); \
		return nvmFAILURE; \
	}

///< Macro to perform bounds checking on the stack
//#ifdef BOUNDS_CHECK		/* On UNIX platforms this can be set during configure */
#define NEED_STACK(n) \
	if (sp < 0) { \
		ERR1("Stack index is negative: %d\n", sp); \
		return nvmFAILURE; \
	} else if (sp >= stacksize && stacksize > 0) { \
		ERR2("Stack index (%d) exceeds stack size (%d)\n", sp, stacksize); \
		return nvmFAILURE; \
	} else if (sp < (n)) { \
		ERR2("Instruction needs %d stack element(s) but stack only contains %d\n", n, sp); \
		return nvmFAILURE; \
	}

#define LOCALS_CHECK(n) \
	if ((n) >= localsnum) { \
		ERR2("Trying to access local %d, but only %d local(s) allocated\n", n, localsnum); \
		return nvmFAILURE; \
	}

#define DATAMEM_CHECK(n) \
	if ((n) < 0) { \
		ERR1("Trying to access data memory with a negative offset (%d)\n", n); \
		return nvmFAILURE; \
	} else if (!HandlerState->PEState->DataMem || (n) >= HandlerState->PEState->DataMem->Size) { \
		ERR2("Trying to access data memory with too big an offset (%d > %d)\n", n, HandlerState->PEState->DataMem->Size); \
		return nvmFAILURE; \
	}

#define SHAREDMEM_CHECK(n) \
	if ((n) < 0) { \
		ERR1("Trying to access shared memory with a negative offset (%d)\n", n); \
		return nvmFAILURE; \
	} else if (sharedmem_size == 0) { \
		ERR0("Shared memory not initialised\n"); \
		return nvmFAILURE; \
	} else if ((n) >= sharedmem_size) { \
		ERR2("Trying to access shared memory with too big an offset (%d > %d)\n", n, sharedmem_size); \
		return nvmFAILURE; \
	}

#define INITEDMEM_CHECK(n) \
	if ((n) < 0) { \
		ERR1("Trying to access initialised memory with a negative offset (%d)\n", n); \
		return nvmFAILURE; \
	} else if (initedmem_size == 0) { \
			ERR0("Initialised memory not present\n"); \
			return nvmFAILURE; \
	} else if ((n) >= initedmem_size) { \
			ERR2("Trying to access initialised memory with too big an offset (%d > %d)\n", n, initedmem_size); \
			return nvmFAILURE; \
	}
#define COPROCESSOR_CHECK(copro, reg) \
	if (copro > HandlerState->PEState-> NCoprocRefs) { \
		ERR4 ("%d/%d: No such coprocessor: %u (> %u)\n", HandlerState->Handler->OwnerPE->Name, pidx, copro, HandlerState->PEState ->NCoprocRefs); \
		return nvmFAILURE; \
	} else if (reg > (HandlerState->PEState -> CoprocTable)[copro]. n_regs) { \
		ERR5 ("%d/%d: No such register in coprocessor %u: %u (> %u)\n", HandlerState->Handler->OwnerPE->Name, pidx, copro, reg, (HandlerState->PEState ->CoprocTable)[copro] . n_regs); \
		return nvmFAILURE; \
	}


int32_t octRT_Execute_Handlers(nvmExchangeBuffer **exbuf, uint32_t port, nvmHandlerState *HandlerState)
{
	uint8_t i, j, loop, overflow;
	uint32_t stack_top;
	uint8_t *xbuffer, *xbufinfo, *datamem, *sharedmem, *initedmem;
	int32_t ret = nvmFAILURE;
	uint32_t utemp1, utemp2, utemp3, pidx, sharedmem_size, initedmem_size;
	char erbuf[250];

	uint32_t *stack = NULL;
	uint32_t *locals = NULL;
	uint32_t localsnum = 0;
	uint32_t stacksize = 0;

	//uint8_t *pr_buf;
	uint8_t *pr_buf;
	uint32_t pc, sp;
	uint32_t ctd_port;
	uint32_t ctdPort;
	uint32_t dopktsend=0;
	uint32_t inithandler=0;

#ifdef WIN32
	struct _timeb timebuffer;
#else
	struct timeb timebuffer;
#endif

	overflow = 0;
	sp = 0;
	pc = 0;
	
	//entrypoint of the bytecode
	pr_buf = HandlerState->Handler->ByteCode;

	if (HandlerState->PEState->DataMem)
	{
		datamem = HandlerState->PEState->DataMem->Base;
	}
	sharedmem = HandlerState->PEState->ShdMem->Base;
	if (HandlerState->PEState->InitedMem)
	{
		initedmem = HandlerState->PEState->InitedMem->Base;
		initedmem_size = HandlerState->PEState->InitedMem->Size;
	}
	else
	{
		initedmem = NULL;
		initedmem_size = 0;
	}

	if ( HandlerState->NLocals + HandlerState->StackSize > 0)
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
			else {
				return nvmFAILURE;
			}
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
		inithandler=1;
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
    		cvmx_wqe_t * work = cvmx_pow_work_request_sync(CVMX_POW_NO_WAIT);
			if (work == NULL)
				printf("il work è nullo\n");
			printf("il work non è nullo\n");
				
			//(**exbuf).PacketBuffer= (uint8_t *)work->packet_ptr.ptr;	
				
			xbuffer = (**exbuf).PacketBuffer;
			xbufinfo = (**exbuf).InfoData;
		}
		else
		{
			/* Pull */
			xbuffer = NULL;
			xbufinfo = NULL;
		}

		pidx = port;

		/* Push the called port ID on the top of the stack */
		stack[sp] = pidx;
		sp++;
	}

	NETVM_ASSERT (pr_buf != NULL, "pr_buf is NULL!");

	loop = 1;
	while (loop)
	{
#if 0
		uint8_t* insn = &pr_buf[pc];
		uint32_t line = 0;
		char insnBuff[256];
		printf("executing instruction at pc: %u ==> %s\n", pc, nvmDumpInsn(insnBuff, 256, &insn, &line));
#endif
		if (sp > 0)
			stack_top = stack[sp-1];
		switch (pr_buf[pc])
		{
//-------------------------------STACK MANAGEMENT INSTRUCTIONS---------------------------------------------------------

			// Push a constant value onto the stack
			case PUSH:
				NEED_STACK(0);
				pc++;
				stack[sp] =  *(int32_t *)&pr_buf[pc];
				printf("fa la push di %d\n",stack[sp]);
				sp++;
				pc+=4;
				break;

			// Push two constant values onto the stack
			  case PUSH2:
				NEED_STACK(0);
				pc++;
				stack[sp] = *(int32_t *)&pr_buf[pc];
				pc+=4;
				stack[sp+1] = *(int32_t *)&pr_buf[pc];
				sp += 2;
				pc+=4;
				break;

			// Remove the top of the stack
			case POP:
				NEED_STACK(1);
				sp--;
				pc++;
				break;

			// Remove the I top values of the stack
			case POP_I:
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
				pc++;
				break;

			// Push the constant 2 onto the stack
			case CONST_2:
				NEED_STACK(0);
				stack[sp] = 2;
				sp++;
				pc++;
				break;

			// Push the constant 0 onto the stack
			case CONST_0:
				NEED_STACK(0);
				stack[sp] = 0;
				sp++;
				pc++;
				break;
			// Push the constant -1 onto the stack
			case CONST__1:
				NEED_STACK(0);
				stack[sp] = (uint32_t)(-1);
				sp++;
				pc++;
				break;

			//Store the time stamp into the stack
			case TSTAMP_S:
				NEED_STACK(0);
				#ifdef WIN32
				_ftime(&timebuffer);
				#else
				ftime(&timebuffer);
				#endif
				stack[sp] = (uint32_t)(timebuffer.time);
				sp += 1;
				pc++;
				break;

			//Store the timestamp (nanoseconds) into the stack
			case TSTAMP_US:
				NEED_STACK(0);
				#ifdef WIN32
				_ftime(&timebuffer);
				#else
				ftime(&timebuffer);
				#endif
				stack[sp] = (uint32_t)(timebuffer.millitm * 1000000);
				sp += 1;
				pc++;
				break;

			case SLEEP:
				NEED_STACK(1);
#ifdef WIN32
				_sleep(stack[sp-1]);
#else
				usleep(stack[sp-1] * 1000);
#endif
				sp--;
				pc++;
				break;

			// Duplication of the top operand stack value
			case DUP:
				NEED_STACK(1);
				stack[sp] = stack[sp-1];
				sp++;
				pc++;
				break;

			//Swap the top two operands stack values
			case SWAP:
				NEED_STACK(2);
				utemp1 = stack[sp-1];
				stack[sp-1] = stack[sp-2];
				stack[sp-2] = utemp1;
				pc++;
				break;

			//Change the mode between little endian and big endian
			case IESWAP:
				NEED_STACK(1);
				stack[sp-1] = nvm_ntohl(stack[sp-1]);
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
				pc++;
				break;

//------------------------------- LOAD AND STORE INSTRUCTIONS:----------------------------------

			// Load a signed 8bit int from the Exchange Buffer segment onto the stack after conversion to a 32bit int
			case PBLDS:
				HANDLERS_ONLY;
				NEED_STACK(1);
				stack[sp-1] = (int32_t)xbuffer[stack[sp-1]];
				pc++;
				break;

			// Load an unsigned 8bit int from the Exchange Buffer segment onto the stack after conversion to a 32bit int
			case PBLDU:
				HANDLERS_ONLY;
				NEED_STACK(1);
				stack[sp-1] = (uint32_t)xbuffer[stack[sp-1]];
				pc++;
				break;

			// Load a signed 16bit int from the Exchange Buffer segment onto the stack after conversion to a 32bit int
			case PSLDS:
				HANDLERS_ONLY;
				NEED_STACK(1);
				stack[sp-1] = (int32_t)(nvm_ntohs(*(int16_t *)&xbuffer[stack[sp-1]]));
				pc++;
				break;

			// Load an unsigned 16bit int from the Exchange Buffer segment onto the stack after conversion to a 32bit int
			case PSLDU:
				HANDLERS_ONLY;
				NEED_STACK(1);
				stack[sp-1] = (uint32_t)(nvm_ntohs(*(uint16_t *)&xbuffer[stack[sp-1]]));
				pc++;
				break;

			// Load a 32bit int from the Exchange Buffer segment onto the stack
			case PILD:
				HANDLERS_ONLY;
				NEED_STACK(1);
				stack[sp-1] = (uint32_t)(nvm_ntohl(*(uint32_t *)&xbuffer[stack[sp-1]]));
				pc++;
				break;

			// Load a signed 8bit int from the data memory segment onto the stack after conversion to a 32bit int
			case DBLDS:
				NEED_STACK(1);
				DATAMEM_CHECK(stack[sp-1]);
				stack[sp-1] = (int32_t)datamem[stack[sp-1]];
				pc++;
				break;

			// Load an unsigned 8bit int from the data memory segment onto the stack after conversion to a 32bit int
			case DBLDU:
				NEED_STACK(1);
				DATAMEM_CHECK(stack[sp-1]);
				stack[sp-1] = (uint32_t)datamem[stack[sp-1]];
				pc++;
				break;

			// Load a signed 16bit int from the data memory segment onto the stack after conversion to a 32bit int
			case DSLDS:
				NEED_STACK(1);
				DATAMEM_CHECK(stack[sp-1]);
				stack[sp-1] = (int32_t)nvm_ntohs(*(int16_t *)&datamem[stack[sp-1]]);
				pc++;
				break;

			// Load an unsigned 16bit int from the data memory segment onto the stack after conversion to a 32bit int
			case DSLDU:
				NEED_STACK(1);
				DATAMEM_CHECK(stack[sp-1]);
				stack[sp-1] = (uint32_t)nvm_ntohs(*(uint16_t *)&datamem[stack[sp-1]]);
				pc++;
				break;

			// Load a 32bit int from the data segment onto the stack
			case DILD:
				NEED_STACK(1);
				DATAMEM_CHECK(stack[sp-1]);
				stack[sp-1] = (uint32_t)nvm_ntohl(*(uint32_t *)&datamem[stack[sp-1]]);
				pc++;
				break;

			// Load a signed 8bit int from the shared memory segment onto the stack after conversion to a 32bit int
			case SBLDS:
				NEED_STACK(1);
				SHAREDMEM_CHECK(stack[sp-1]);
				stack[sp-1] = (int32_t)sharedmem[stack[sp-1]];
				pc++;
				break;

			// Load an unsigned 8bit int from the shared memory segment onto the stack after conversion to a 32bit int
			case SBLDU:
				NEED_STACK(1);
				SHAREDMEM_CHECK(stack[sp-1]);
				stack[sp-1] = (uint32_t)sharedmem[stack[sp-1]];
				pc++;
				break;

			// Load a signed 16bit int from the shared memory segment onto the stack after conversion to a 32bit int
			case SSLDS:
				NEED_STACK(1);
				SHAREDMEM_CHECK(stack[sp-1]);
				stack[sp-1] = (int32_t)nvm_ntohs(*(int16_t *)&sharedmem[stack[sp-1]]);
				pc++;
				break;

			// Load an unsigned 16bit int from the shared memory segment onto the stack after conversion to a 32bit int
			case SSLDU:
				NEED_STACK(1);
				SHAREDMEM_CHECK(stack[sp-1]);
				stack[sp-1] = (uint32_t)nvm_ntohs(*(uint16_t *)&sharedmem[stack[sp-1]]);
				pc++;
				break;

			// Load a 32bit int from the shared memory segment onto the stack
			case SILD:
				NEED_STACK(1);
				SHAREDMEM_CHECK(stack[sp-1]);
				stack[sp-1] = (uint32_t)nvm_ntohl(*(uint32_t *)&sharedmem[stack[sp-1]]);
				pc++;
				break;

			// Load a byte from the Exchange Buffer, masks it with the 0x0f constant and multiplies it for 4.
			// Can be used to calculate the lenght of a IPv4 header
			case BPLOAD_IH:
				HANDLERS_ONLY;
				NEED_STACK(1);
				stack[sp-1] = (uint32_t)(((xbuffer[0] + stack[sp-1]) & 0x0f) << 2);
				pc++;
				break;


			// Store a signed 8bit int from the stack to data memory after conversion from a 32 bit int
			case DBSTR:
				NEED_STACK(2);
				DATAMEM_CHECK(stack[sp-1]);
				datamem[stack[sp-1]] = (int8_t) stack[sp-2];
				sp -= 2;
				pc++;
				break;

			// Store a signed 16bit int from the stack to data memory after conversion from a 32bit int
			case DSSTR:
				NEED_STACK(2);
				DATAMEM_CHECK(stack[sp-1]);
				*(int16_t *)&datamem[stack[sp-1]] = nvm_ntohs(stack[sp-2]);
				sp -= 2;
				pc++;
				break;

			// Store a 32bit int from the stack to data memory
			case DISTR:
				NEED_STACK (2);
				DATAMEM_CHECK(stack[sp-1]);
				*(int32_t *)&datamem[stack[sp-1]] = nvm_ntohl(stack[sp-2]);
				sp -= 2;
				pc++;
				break;

			// Store a signed 8bit int from the stack to shared memory after conversion from a 32bit int
			case SBSTR:
				NEED_STACK(2);
				SHAREDMEM_CHECK(stack[sp-1]);
				sharedmem[stack[sp-1]] = (int8_t) stack[sp-2];
				sp -= 2;
				pc++;
				break;

			// Store a signed 16bit int from the stack to shared memory after conversion from a 32bit int
			case SSSTR:
				NEED_STACK(2);
				SHAREDMEM_CHECK(stack[sp-1]);
				*(int16_t *)&sharedmem[stack[sp-1]] = nvm_ntohs(stack[sp-2]);
				sp -= 2;
				pc++;
				break;

			// Store a 32bit int from the stack to shared memory
			case SISTR:
				NEED_STACK(2);
				SHAREDMEM_CHECK(stack[sp-1]);
				*(int32_t *)&sharedmem[stack[sp-1]] = nvm_ntohl(stack[sp-2]);
				sp -= 2;
				pc++;
				break;

			// Store an unsigned 8bit int from the stack to Exchange Buffer after conversion from a 32bit int
			case PBSTR:
				HANDLERS_ONLY;
				NEED_STACK(2);
				if (xbuffer != NULL)
					xbuffer[stack[sp-1]] = (uint8_t) stack[sp-2];
				sp -= 2;
				pc++;
				break;

			// Store an unsigned 16bit int from the stack to Exchange Buffer after conversion from a 32bit int
			case PSSTR:
				HANDLERS_ONLY;
				NEED_STACK(2);
				*(uint16_t *)&xbuffer[stack[sp-1]] = nvm_ntohs(stack[sp-2]);
				sp -= 2;
				pc++;
				break;

			// Store an unsigned 16bit int from the stack to Exchange Buffer after conversion from a 32bit int
			case PISTR:
				HANDLERS_ONLY;
				NEED_STACK(2);
				*(uint32_t *)&xbuffer[stack[sp-1]] = nvm_ntohl(stack[sp-2]);
				sp -= 2;
				pc++;
				break;

			//Puts a random integer at the top of the stack
			case IRND:
				NEED_STACK(0);
				stack[sp] = (uint32_t) rand();
				sp++;
				pc++;
				break;

//------------------------------- BIT MANIPULATION INSTRUCTIONS:----------------------------------

			//Arithmetical left shift
			case SHL:
				NEED_STACK(2);
				stack[sp-2] = ((int32_t) stack[sp-2]) << stack[sp-1];
				sp--;
				pc++;
				break;

			//Arithmetical right shift
			case SHR:
				NEED_STACK(2);
				stack[sp-2] = ((int32_t) stack[sp-2]) >> stack[sp-1];
				sp--;
				pc++;
				break;

			//Logical right shift
			case USHR:
				NEED_STACK(2);
				stack[sp-2] = stack[sp-2] >> stack[sp-1];
				sp--;
				pc++;
				break;

			//Rotate left
			case ROTL:
				NEED_STACK(2);
				stack[sp-2] = _gen_rotl(stack[sp-2], stack[sp-1]);
				sp--;
				pc++;
				break;

			//Logical right shift of 1 position
			case ROTR:
				NEED_STACK(2);
				stack[sp-2] = _gen_rotr(stack[sp-2], stack[sp-1]);
				sp--;
				pc++;
				break;

			// Bitwise OR of the first two values on the stack
			case OR:
				NEED_STACK(2);
				stack[sp-2] |= stack[sp-1];
				sp--;
				pc++;
				break;

			// Bitwise AND of the first two values on the stack
			case AND:
				NEED_STACK(2);
				stack[sp-2] &= stack[sp-1];
				sp--;
				pc++;
				break;

			// Bitwise XOR of the first two values on the stack
			case XOR:
				NEED_STACK(2);
				stack[sp-2] ^= stack[sp-1];
				sp--;
				pc++;
				break;

			// Bitwise not
			case NOT:
				NEED_STACK(1);
				stack[sp-1] = ~(stack[sp-1]);
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
				pc++;
				break;

			// Addition of the two top items of the stack considered as unsigned 32bit int with overflow control
			case ADDUOV:
				NEED_STACK(2);
				if ((stack[sp-2] += stack[sp-1]) <= stack[sp-1])
					overflow=1;
				sp--;
				pc++;
				break;

			// Subtraction the first value on the stack to the second one without overflow control
			case SUB:
				NEED_STACK(2);
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
				pc++;
				break;

			// Subtraction the first value on the stack to the second one considered as unsigned 32bit int with overflow control
			case SUBUOV:
				NEED_STACK(2);
				if (stack[sp-1] > stack[sp-2])
					overflow=1;
				stack[sp-2] -= stack[sp-1];
				sp--;
				pc++;
				break;

			case MOD:
				NEED_STACK(2);
				stack[sp-2] %= stack[sp-1];
				sp--;
				pc++;
				break;
				break;
			//Negate the value of the element at the top of the stack
			case NEG:
				NEED_STACK(1);
				stack[sp-1] = - (int32_t) stack[sp-1];
				pc++;
				break;

			// Multiplication of the first two values on the stack
			case IMUL:
				NEED_STACK(2);
				stack[sp-2] *= stack[sp-1];
				sp--;
				pc++;
				break;

			// Multiplication of the first two values on the stack considered as signed 32 bit
			case IMULSOV:
				//TODO if overflow=1;
				NEED_STACK(2);
				stack[sp-2] *= (int32_t) stack[sp-1];
				sp--;
				pc++;
				break;


			// Multiplication of the first two values on the stack considered as unsigned 32 bit
			case IMULUOV:
				//TODO if overflow=1;
				NEED_STACK(1);
				sp--;
				pc++;
				break;


			// Increment the first element on the stack
			case IINC_1:
				NEED_STACK(1);
				(stack[sp-1])++;
				pc++;
				break;

			// Increment the second element on the stack
			case IINC_2:
				NEED_STACK(2);
				(stack[sp-2])++;
				pc++;
				break;

			// Increment the third element on the stack
			case IINC_3:
				NEED_STACK(3);
				(stack[sp-3])++;
				pc++;
				break;

			// Increment the fourth element on the stack
			case IINC_4:
				NEED_STACK(4);
				(stack[sp-4])++;
				pc++;
				break;

			// Decrement the first element on the stack
			case IDEC_1:
				NEED_STACK(1);
				(stack[sp-1])--;
				pc++;
				break;

			// Decrement the second element on the stack
			case IDEC_2:
				NEED_STACK(2);
				(stack[sp-2])--;
				pc++;
				break;

			// Dencrement the third element on the stack
			case IDEC_3:
				NEED_STACK(3);
				(stack[sp-3])--;
				pc++;
				break;

			// Decrement the fourth element on the stack
			case IDEC_4:
				NEED_STACK(4);
				(stack[sp-4])--;
				pc++;
				break;

//--------------------------FLOW CONTROL INSTRUCTIONS---------------------------------------------------------------

			// Branch if the first element on the stack is equal to the second one
			case JCMPEQ:
				NEED_STACK(2);
				if (stack[sp-1] == stack[sp-2])
				{
					pc += *(int32_t*)&pr_buf[pc+1];
				}

				sp-=2;
				pc+=5;
				break;

			// Branch if the first element on the stack is not equal to the second one
			case JCMPNEQ:
				NEED_STACK(2);
				if (stack[sp-1] != stack[sp-2])
				{
					pc += *(int32_t*)&pr_buf[pc+1];
				}
				sp-=2;
				pc+=5;
				break;

			// Branch if the second element on the stack is greater than the first one, considering both elements as unsigned int
			case JCMPG:
				NEED_STACK(2);
				if (stack[sp-1] < stack[sp-2])
					pc += *(int32_t*)&pr_buf[pc+1];
				sp-=2;
				pc+=5;
				break;

			// Branch if the second element on the stack is greater than the first one, considering both elements as signed int
			case JCMPG_S:
				NEED_STACK(2);
				if (((int32_t) stack[sp-1]) < ((int32_t) stack[sp-2]))
					pc += *(int32_t*)&pr_buf[pc+1];
				sp-=2;
				pc+=5;
				break;

			// Branch if the second element on the stack is equal to or greater than the first one, considering both elements as unsigned int
			case JCMPGE:
				NEED_STACK(2);
				if (stack[sp-2] >= stack[sp-1])
					pc += *(int32_t*)&pr_buf[pc+1];
				sp-=2;
				pc+=5;
				break;

			// Branch if the second element on the stack is equal to or greater than the first one, considering both elements as signed int
			case JCMPGE_S:
				NEED_STACK(2);
				if (((int32_t) stack[sp-1]) <= ((int32_t) stack[sp-2]))
					pc += *(int32_t*)&pr_buf[pc+1];
				sp-=2;
				pc+=5;
				break;

			// Branch if the second element on the stack is lower than the first one, considering both elements as unsigned int
			case JCMPL:
				NEED_STACK(2);
				if (stack[sp-1] > stack[sp-2])
				{
					pc += *(int32_t*)&pr_buf[pc+1];
				}
				sp-=2;
				pc+=5;
				break;

			// Branch if the second element on the stack is greater than the first one, considering both elements as signed int
			case JCMPL_S:
				NEED_STACK(2);
				if (((int32_t) stack[sp-1]) > ((int32_t) stack[sp-2]))
					pc += *(int32_t*)&pr_buf[pc+1];
				sp-=2;
				pc+=5;
				break;

			// Branch if the second element on the stack is equal to or lower than the first one, considering both elements as unsigned int
			case JCMPLE:
				NEED_STACK(2);
				if (stack[sp-1] >= stack[sp-2])
					pc += *(int32_t*)&pr_buf[pc+1];
				sp-=2;
				pc+=5;
				break;

			// Branch if the second element on the stack is equal to or lower than the first one, considering both elements as signed int
			case JCMPLE_S:
				NEED_STACK(2);
				if ((int32_t) stack[sp-1] >= (int32_t) stack[sp-2])
					pc += *(int32_t*)&pr_buf[pc+1];
				sp-=2;
				pc+=5;
				break;

			// Branch if the first element on the stack is equal to 0
			case JEQ:
				NEED_STACK(1);
				if (stack[sp-1] == 0)
					pc += *(int32_t*)&pr_buf[pc+1];
				sp--;
				pc+=5;
				break;

			// Branch if the first element on the stack is not equal to 0
			case JNE:
				NEED_STACK(1);
				if (stack[sp-1] != 0)
					pc += *(int32_t*)&pr_buf[pc+1];
				sp--;
				pc+=5;
				break;

			// Unconditional branch
			case JUMP:
				NEED_STACK(0);
				pc++;
				pc += pr_buf[pc] + 1;
				break;

			// Unconditional branch (wide index)
			case JUMPW:
				NEED_STACK(0);
				pc++;
				pc += *(int32_t*)&pr_buf[pc] + 4;
				break;

			//Switch - This instruction provides for multiple branch possibilities
			case SWITCH:
				NEED_STACK(1);
				pc++;
				gen_do_switch(pr_buf, &pc, stack[sp-1]);
				sp--;
				break;

			//Call - Call a subroutine with an 8 bit offset
			case CALL:
				NEED_STACK(0);
				pc++;
				pc+=pr_buf[pc];
				break;

			//Ret - return from subroutine
			case RET:
				printf("fa la ret\n");
				NEED_STACK(0);
				if (sp != 0) {
					ERR1("The stack pointer is not zero at return time (sp = %d)\n", sp);
					ret = nvmFAILURE;
				} else
					ret = nvmSUCCESS;
				if (INIT_SEGMENT && locals)
					free (locals);
				loop = 0;
				#ifdef RTE_PROFILE_COUNTERS
					if(dopktsend == 0 && inithandler == 0)
					{
						//HandlerState->ProfCounters->numPktDiscarded++;
  						//octRT_SetTime_Now(DISCARDED, HandlerState);
					}
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
				pc++;
				break;

//----------------------------------------PATTERN MATCHING INSTRUCTIONS------------------------------------------

			// Comparison between two fields in the packet buffer. Branch if equal
			case JFLDEQ:
				HANDLERS_ONLY;
				NEED_STACK(3);
				if (memcmp(&xbuffer[stack[sp-3]], &xbuffer[stack[sp-2]], stack[sp-1]) == 0)
					pc += *(int32_t*)&pr_buf[pc+1];
				sp -= 3;
				pc +=5;
				break;

			// Comparison between two fields in the packet buffer. Branch if not equal
			case JFLDNEQ:
				HANDLERS_ONLY;
				NEED_STACK(3);
				if (memcmp(&xbuffer[stack[sp-3]], &xbuffer[stack[sp-2]], stack[sp-1]) != 0)
					pc += *(int32_t*)&pr_buf[pc+1];
				sp -= 3;
				pc +=5;
				break;

			// Comparison between two fields in the packet buffer. Branch if the first i lexicographically less than the second
			case JFLDLT:
				HANDLERS_ONLY;
				NEED_STACK(3);
				if (memcmp(&xbuffer[stack[sp-3]], &xbuffer[stack[sp-2]], stack[sp-1]) < 0)
					pc += *(int32_t*)&pr_buf[pc+1];
				sp -= 3;
				pc +=5;
				break;

			// Comparison between two fields in the packet buffer. Branch if the first i lexicographically greater than the second
			case JFLDGT:
				HANDLERS_ONLY;
				NEED_STACK(3);
				if (memcmp(&xbuffer[stack[sp-3]], &xbuffer[stack[sp-2]], stack[sp-1]) > 0)
					pc += *(int32_t*)&pr_buf[pc+1];
				sp -= 3;
				pc +=5;
				break;

		

//--------------------------- DATA TRANSFER INSTRUCTIONS -------------------------------

			//Push an exchange buffer to a port
			case SNDPKT:
				HANDLERS_ONLY;
				NEED_STACK(0);
				pc++;
				dopktsend=1;

#ifdef RTE_PROFILE_COUNTERS
  	//	arch_SetTime_Now(FORWARDED, HandlerState);
	//	HandlerState->ProfCounters->numPktOut++;
	#endif
				
				ctd_port = *(uint32_t *) &pr_buf[pc];
				printf("fa la send\n");
				nvmNetPacket_Send(*exbuf,ctd_port,HandlerState);
				pc+=4;
				break;

			//Send a copy of the exchange buffer to a port
			case DSNDPKT:
				HANDLERS_ONLY;
				NEED_STACK(0);
				pc++;
				nvmNetPacketSendDup (*exbuf,*(uint32_t *) &pr_buf[pc] , HandlerState);
				pc+=4;
				break;

			//Pull an exchange buffer from a port
			case RCVPKT:
				HANDLERS_ONLY;
				NEED_STACK(0);
				pc++;
				ctdPort = *(uint32_t *) &pr_buf[pc];
				nvmNetPacket_Recive(exbuf,ctdPort,HandlerState);

				xbuffer = (**exbuf).PacketBuffer;
				xbufinfo = (**exbuf).InfoData;
				pc+=4;
				break;

			//Copy a memory buffer from data memory to Exchange Buffer
			case DPMCPY:
				HANDLERS_ONLY;
				NEED_STACK(3);
				DATAMEM_CHECK(stack[sp-3]);
				DATAMEM_CHECK(stack[sp-3] + stack[sp-1] - 1);
				for (i = 0; i < stack[sp-1]; i++)
					xbuffer[stack[sp-2] + i] = datamem[stack[sp-3] + i];
				sp-=3;
				pc++;
				break;

			//Copy a memory buffer from Exchange Buffer to data memory
			case PDMCPY:
				HANDLERS_ONLY;
				NEED_STACK(3);
				DATAMEM_CHECK(stack[sp-2]);
				DATAMEM_CHECK(stack[sp-2] + stack[sp-1] - 1);
				for (i = 0; i < stack[sp-1]; i++)
					datamem[stack[sp-2] + i] = xbuffer[stack[sp-3] + i];
				sp-=3;
				pc++;
				break;

			//Copy a memory buffer from data memory to shared memory
			case DSMCPY:
				NEED_STACK(3);
				SHAREDMEM_CHECK(stack[sp-2]);
				SHAREDMEM_CHECK(stack[sp-2] + stack[sp-1] - 1);
				DATAMEM_CHECK(stack[sp-3]);
				DATAMEM_CHECK(stack[sp-3] + stack[sp-1] - 1);
				for (i = 0; i < stack[sp-1]; i++)
					sharedmem[stack[sp-2] + i] = datamem[stack[sp-3] + i];
				sp-=3;
				pc++;
				break;

			//Copy a memory buffer from shared memory to data memory
			case SDMCPY:
				NEED_STACK(3);
				DATAMEM_CHECK(stack[sp-2]);
				DATAMEM_CHECK(stack[sp-2] + stack[sp-1] - 1);
				SHAREDMEM_CHECK(stack[sp-3]);
				SHAREDMEM_CHECK(stack[sp-3] + stack[sp-1] - 1);
				for (i = 0; i < stack[sp-1]; i++)
					datamem[stack[sp-2] + i] = sharedmem[stack[sp-3] + i];
				sp-=3;
				pc++;
				break;

			//Copy a memory buffer from shared memory to Exchange Buffer
			case SPMCPY:
				HANDLERS_ONLY;
				NEED_STACK(3);
				SHAREDMEM_CHECK(stack[sp-3]);
				SHAREDMEM_CHECK(stack[sp-3] + stack[sp-1] - 1);
				for (i = 0; i < stack[sp-1]; i++)
					xbuffer[stack[sp-2] + i] = sharedmem[stack[sp-3] + i];
				sp-=3;
				pc++;
				break;

			//Copy a memory buffer from Exchange Buffer to shared memory
			case PSMCPY:
				HANDLERS_ONLY;
				NEED_STACK(3);
				SHAREDMEM_CHECK(stack[sp-2]);
				SHAREDMEM_CHECK(stack[sp-2] + stack[sp-1] - 1);
				for (i = 0; i < stack[sp-1]; i++)
					sharedmem[stack[sp-2] + i] = xbuffer[stack[sp-3] + i];
				sp-=3;
				pc++;
				break;

			//Create a new exchange buffer and puts it in the Connection Buffer
			case CRTEXBUF:
				HANDLERS_ONLY;
				NEED_STACK(1);
				*exbuf=octRT_GetExbuf(HandlerState->PEState->RTEnv);
				sp--;
				pc++;
				break;
			//Delete the exchange buffer
			case DELEXBUF:
				HANDLERS_ONLY;
				NEED_STACK(0);
				octRT_ReleaseExbuf(HandlerState->PEState->RTEnv,*exbuf);
				*exbuf=NULL;
				//TODO ho messo exbuf a null ma non ne sono affatto sicuro
				pc++;
				break;


//------------------------------MISCELLANEOUS nvmOPCODES------------------------------------------

			//Nop - no operationsroutine with a wide range
			case NOP:
				NEED_STACK(0);
				pc++;
				break;

			//Exit from a program returning an integer value
			case EXIT:
				NEED_STACK(0);
				exit(*(int32_t *)&pr_buf[pc+1]);
				break;

			//Debug this instruction only
			case BRKPOINT:
				NEED_STACK(0);
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
				pc++;
				break;

			// Find the first bit set in the top values of the stack
			case XFINDBITSET:
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
				pc++;
				break;

			// Set the bit specified at the top of the stack from the second value at the top of the stack
			case SETBIT:
				NEED_STACK(2);
				if (stack[sp-1] < 32)
				stack[sp-2] |= _gen_rotl(1, stack[sp-1]);
				sp--;
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
				pc++;
				break;

			//Clear the bit specified at hte top of the stack from the second value at the top of the stack
			case CLEARBIT:
				NEED_STACK(2);
				if (stack[sp-1] < 32)
				stack[sp-2] &= _gen_rotl(0xfffffffe, stack[sp-1]);
				sp--;
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
				pc++;
				break;

//---------------------------------------- Info field manipulation instructions ----------------------------------------

			// Store a signed 8bit int from the stack to the Exchange Buffer info field after conversion from a 32bit int
			case IBSTR:
				HANDLERS_ONLY;
				NEED_STACK(2);
				xbufinfo[stack[sp-1]] = (uint8_t) stack[sp-2];
				sp -= 2;
				pc++;
				break;

			// Store a signed 16bit int from the stack to the Exchange Buffer info field after conversion from a 32bit int
			case ISSTR:
				HANDLERS_ONLY;
				NEED_STACK(2);
				*(int16_t *)&xbufinfo[stack[sp-1]] = nvm_ntohs(stack[sp-2]);
				sp -= 2;
				pc++;
				break;

			// Store a 32it int from the stack to the Exchange Buffer info field
			case IISTR:
				HANDLERS_ONLY;
				NEED_STACK(2);
				*(int32_t *)&xbufinfo[stack[sp-1]] = nvm_ntohl(stack[sp-2]);
				sp -= 2;
				pc++;
				break;

			// Load an unsigned 8bit int from the Exchange Buffer info field onto the stack after conversion to a 32bit int
			case ISBLD:
				HANDLERS_ONLY;
				NEED_STACK(1);
				stack[sp-1] = (uint32_t)xbufinfo[stack[sp-1]];
				pc++;
				break;

			// Load an unsigned 16bit int from the Exchange Buffer info field onto the stack after conversion to a 32bit int
			case ISSLD:
				HANDLERS_ONLY;
				NEED_STACK(1);
				stack[sp-1] = (uint32_t)nvm_ntohs(*(uint16_t *)&xbufinfo[stack[sp-1]]);
				pc++;
				break;

			// Load a signed 8bit int from the Exchange Buffer info field onto the stack after conversion to a 32bit int
			case ISSBLD:
				HANDLERS_ONLY;
				NEED_STACK(1);
				stack[sp-1] = (int32_t)xbufinfo[stack[sp-1]];
				pc++;
				break;

			// Load a signed 16bit int from the Exchange Buffer info field onto the stack after conversion to a 32bit int
			case ISSSLD:
				HANDLERS_ONLY;
				NEED_STACK(1);
				stack[sp-1] = (int32_t)nvm_ntohs(*(int16_t *)&xbufinfo[stack[sp-1]]);
				pc++;
				break;

			// Load a 32bit int from the Exchange Buffer info field onto the stack
			case ISSILD:
				HANDLERS_ONLY;
				NEED_STACK(1);
				stack[sp-1] = (uint32_t)nvm_ntohl(*(int32_t *)&xbufinfo[stack[sp-1]]);
				pc++;
				break;

//------------------------------------------- Locals management instructions -------------------------------------------

			//load an indexed local on the top of the stack
			case LOCLD:
				NEED_STACK(0);
				pc++;
				utemp1 = pr_buf[pc];
				LOCALS_CHECK(utemp1);
				stack[sp] = (uint32_t) locals[utemp1];
				pc+=4;
				sp++;
				break;

			//stores in the indexed local the value on the top of the stack
			case LOCST:
				NEED_STACK(1);
				pc++;
				utemp1 = pr_buf[pc];
				LOCALS_CHECK(utemp1);
				locals[utemp1] = stack[sp-1];
				sp--;
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
				pc++;
				break;

			case HASH32:
				NEED_STACK(1);
				pc++;
				utemp2 = nvmHash((uint8_t*)&stack[sp - 1], 4);
				stack[sp-1] = utemp2;
				break;

//------------------------------------ Instructions for interaction with coprocessors ----------------------------------
			case COPINIT:
				NEED_STACK(0);
				pc++;
				utemp1 = *(uint32_t *) &(pr_buf[pc]);		// Coprocessor ID
				utemp2 = *(uint32_t *) &(pr_buf[pc + 4]);	// Offset into initedmem
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
				(HandlerState->PEState -> CoprocTable)[utemp1]. write (&(HandlerState->PEState -> CoprocTable)[utemp1], utemp2, &utemp3);		// Write to coprocessor
				sp--;
				pc += 8;
				break;

			case COPRUN:
				pc++;
				utemp1 = *(uint32_t *) &(pr_buf[pc]);		// Coprocessor ID
				utemp2 = *(uint32_t *) &(pr_buf[pc + 4]);	// Coprocessor operation
				COPROCESSOR_CHECK(utemp1, 0);
				(HandlerState->PEState -> CoprocTable)[utemp1]. invoke (&(HandlerState->PEState ->CoprocTable)[utemp1], utemp2);
				pc += 8;
				break;

			case COPPKTOUT:
				pc++;
				utemp1 = *(uint32_t *) &(pr_buf[pc]);		// Coprocessor ID
				// 2nd arg momentarily unused
				COPROCESSOR_CHECK(utemp1, 0);
				(HandlerState->PEState -> CoprocTable)[utemp1] . xbuf = *exbuf;
				pc += 8;
				break;

//-------------------------------------- Instructions exclusive to the INIT segment ------------------------------------
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
							ERR1("%s: Error in shared memory allocation\n", HandlerState->Handler->OwnerPE->Name);
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
				}
				else if (utemp1 == HandlerState->PEState->ShdMem->Size)
				{
					/* PE wants to use shared memory, and this has already been allocated by a previous
					   PE. The requested size matches the allocated size. Just init needed variables. */
					sharedmem = (uint8_t *) HandlerState->PEState->ShdMem;
					sharedmem_size = HandlerState->PEState->ShdMem->Size;
				}
				else
				{
					/* PE wants to use shared memory, and this has already been allocated. Although, the
					   requested size differs with what has been allocated. Return an error. */
					ERR1("%s: Requested shared memory size differs with previously allocated size.\n", HandlerState->Handler->OwnerPE->Name);
					return nvmFAILURE;
				}
				pc+=4;	/* This is always needed */
				break;

			default:
				//If an erroneous instruction is found simply ignore it
				assert(1);
				ERR2("nvm %s: Unknown Opcode: %x error, doing nothing\n",HandlerState->Handler->OwnerPE->Name, pr_buf[pc]);
				return nvmFAILURE;
				break;

		} /* End of switch() */
	} /* End of while() */

	return (ret);
}
