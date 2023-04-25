
#define OCTEON_MODEL OCTEON_CN58XX_PASS2

#include "cvmx-config.h"
#include "cvmx.h"
#include "cvmx-fpa.h"
#include "cvmx-pko.h"
#include "cvmx-dfa.h"
#include "cvmx-pow.h"
#include "cvmx-fpa.h"
#include "cvmx-wqe.h"
#include "cvmx-packet.h"
#include "cvmx-asm.h"
#include "cvmx-bootmem.h"
#include "cvmx-helper.h"
#include "nbnetvm.h"


#define NUM_PACKET_BUFFERS  4096
#define NUM_WORK_BUFFERS    NUM_PACKET_BUFFERS
#define NUM_DFA_BUFFERS 512


#define net2hostl(a) ( \
	(((a) & 0x000000FF) << 24) + \
	(((a) & 0x0000FF00) << 8) + \
	(((a) & 0x00FF0000) >> 8) + \
	((a) >> 24) \
	)
#define net2hosts(a) ( \
	(((a) & 0x00FF) << 8) + \
	((a) >> 8) \
	)


nvmHandlerFunction *nvmStartFirstPE;


int n=0;
u_int64_t totalStringmatching, totalLookup;
u_int64_t StringMatchingCallsNum;


void octeon_print_PE_Statics()
{
}


int32_t appmain(int argc, char *argv[])
{
	totalStringmatching = 0;
	int npacchetti = 1;
	nvmExchangeBuffer exbuf;
	u_int64_t prima,dopo;

        if (argc > 1)
	{
		npacchetti = atoi(argv[1]);
	}

	cvmx_user_app_init();

	if(cvmx_helper_initialize_fpa (NUM_PACKET_BUFFERS, NUM_WORK_BUFFERS, CVMX_PKO_MAX_OUTPUT_QUEUES*64, 0, 0)!=0) {
		printf("ERROR initializing fpa");
        return 1;
    }

    void *dfa_buffers;

	printf("Allocating dfa buffers\n");
	dfa_buffers = cvmx_bootmem_alloc_named(CVMX_FPA_DFA_POOL_SIZE * NUM_DFA_BUFFERS, 128,"dfa");
	if (dfa_buffers == NULL) {
	    printf("Could not allocate dfa_buffers.\n");
	    printf("TEST FAILED\n");
	    return 1;
	}
	printf("initializing fpa pool for dfa\n");
	cvmx_fpa_setup_pool(CVMX_FPA_DFA_POOL, "dfa", dfa_buffers, CVMX_FPA_DFA_POOL_SIZE, NUM_DFA_BUFFERS);

	printf("Initializing the DFA\n");
	cvmx_dfa_initialize();

void *info_buffers;

	info_buffers = cvmx_bootmem_alloc_named(CVMX_NVM_OBJ_POOL_SIZE * NUM_PACKET_BUFFERS, 128, "info");
	if (info_buffers == NULL) {
	    printf("Could not allocate info_buffers.\n");
	    printf("TEST FAILED\n");
	    return 1;
	}
	cvmx_fpa_setup_pool(CVMX_NVM_OBJ_POOL, "info", info_buffers, CVMX_NVM_OBJ_POOL_SIZE, NUM_PACKET_BUFFERS);

int result = cvmx_helper_initialize_packet_io_global();
	
	//loadGraphIntoLLM();

	printf("Initialization done. Waiting for %d packets to be filtered.\n", npacchetti);
	cvmx_wqe_t *work;
	nvmExchangeBuffer *ptr= &exbuf;
	int i=0,k=0;
	nvmExecuteInit();
	
	while(i < npacchetti)
	{
		work = cvmx_pow_work_request_sync(CVMX_POW_NO_WAIT);

		if(!work) 
			continue;
		i++;
		//exbuf.PacketBuffer = work->packet_ptr.ptr;
		exbuf.PacketBuffer = (u_int8_t *)cvmx_phys_to_ptr(work->packet_ptr.s.addr);
		exbuf.PacketLen = work->len;
	 	exbuf.InfoData= cvmx_fpa_alloc	(CVMX_NVM_OBJ_POOL); 
	 	memset(cvmx_phys_to_ptr(exbuf.InfoData), 0, CVMX_NVM_OBJ_POOL_SIZE);//64 provvisorio meglio mettere la dim di tutto il pool 
		exbuf.ArchData = work;	
		nvmStartFirstPE(&ptr,0,NULL);
	//istruzioni per profiling
	//CVMX_RDHWR(prima, 2);
	//quello che devi misurare
	//CVMX_RDHWR(dopo, 2);
	//printf("ticks filtro: %llu", dopo - prima);
	}
	printf("\n===============================================================\n");
	octeon_print_PE_Statics();
	printf("Ticks spent in copro stringmatching: %llu\n", totalStringmatching);
	printf("Calls of copro stringmatching: %llu\n", StringMatchingCallsNum);

	//printf("Ticks spent in copro lookup: %llu", totalLookup);
	cvmx_bootmem_free_named("dfa");
	cvmx_bootmem_free_named("info");
	return 0;
}


void nvmSendOut(nvmExchangeBuffer **exbuf, int out_port, void *dummy)
{
cvmx_wqe_t * work = (cvmx_wqe_t *) (*exbuf)->ArchData;
int64_t port;
cvmx_pko_command_word0_t pko_command;
 cvmx_buf_ptr_t  packet_ptr;

	/* Build a PKO pointer to this packet */
	pko_command.u64 = 0;
	port = work->ipprt;
	int queue = cvmx_pko_get_base_queue(port);
	//printf("in invia pacchetto prima della prepare\n");
	cvmx_pko_send_packet_prepare(port, queue, 1);
	/* Build a PKO pointer to this packet */

	// TODO controllare se serve a qualcosa pko_command.s.dontfree = 1;
	if (work->word2.s.bufs == 0)
	{
		/* Packet is entirely in the WQE. Give the WQE to PKO and have it free it */
		pko_command.s.total_bytes = work->len;
		pko_command.s.segs = 1;
		pko_command.s.dontfree = 1;
		packet_ptr.u64 = 0;
		packet_ptr.s.pool = CVMX_FPA_WQE_POOL;
		packet_ptr.s.size = CVMX_FPA_WQE_POOL_SIZE;
		packet_ptr.s.addr = cvmx_ptr_to_phys(work->packet_data);
		if (cvmx_likely(!work->word2.s.not_IP))
		{
			if (work->word2.s.is_v6)
				packet_ptr.s.addr += 2;
			else
				packet_ptr.s.addr += 6;
		}
	}
	else
	{
		pko_command.s.total_bytes = work->len;
		pko_command.s.segs = work->word2.s.bufs;
		packet_ptr = work->packet_ptr;
	}
	if (cvmx_pko_send_packet_finish(port, queue, pko_command, packet_ptr, 1))
	{
		printf("Failed to send packet using cvmx_pko_send_packet2\n");
	}
	else
		// In fact, packet number is wrong; if some packet do not satify the filter,
		// they do not arrive here and are not counted. So, this counter has no
		// relationships with the ordinal number of packets we're sending to the Octeon board
		printf("Sent packet num %d on core %ld\n", ++n, cvmx_get_core_num());

	cvmx_helper_free_packet_data(work);
	cvmx_fpa_free(work, CVMX_FPA_WQE_POOL, 0);
	cvmx_fpa_free((*exbuf)->InfoData, CVMX_NVM_OBJ_POOL, 0);
}


void nvmReleaseWork(nvmExchangeBuffer **exbuf, int port, void *dummy)
{
cvmx_wqe_t * work = (cvmx_wqe_t *) (*exbuf)->ArchData;

	cvmx_helper_free_packet_data(work);
	cvmx_fpa_free((*exbuf)->InfoData, CVMX_NVM_OBJ_POOL, 0);
	cvmx_fpa_free(work, CVMX_FPA_WQE_POOL, 0);
}


void nvmDropPacket(nvmExchangeBuffer **exbuf, uint32_t port, void *dummy)
{
	nvmReleaseWork(exbuf, port, dummy);
}

