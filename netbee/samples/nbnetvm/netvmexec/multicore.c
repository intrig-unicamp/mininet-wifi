/*
 * Copyright (c) 2002 - 2011
 * NetGroup, Politecnico di Torino (Italy)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following condition 
 * is met:
 * 
 * Neither the name of the Politecnico di Torino nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */


#ifdef ENABLE_MULTICORE

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sched.h>
#include "multicore.h"
#include "utils.h"
#include <nbnetvm.h>


volatile sig_atomic_t netvm_termination = 0;
int n_child_processing_over = 0;
extern ConfigParams_t ConfigParams;


/* handler called on SIGCHLD. Used to know if all children have finished */
void nvm_termination_hndlr(int sig)
{
int status;

	debug("Received a termination signal, value of counter: %d", n_child_processing_over);

	while (waitpid(-1, &status, WNOHANG) > 0)
	{
		if(++n_child_processing_over == ConfigParams.ncores)
		{
			netvm_termination = 1;
		}
	}
}

/* register handlers */
void setup_handlers()
{
	signal(SIGCHLD, nvm_termination_hndlr);
}


/* Copy the whole capture into the shared memory */
void copy_capture(struct shmstruct *ptr)
{
u_int32_t cap_len;
int r;
struct pcap_pkthdr *pkthdr;
const u_char *packet;
pkt_handle *hndl;
uint counter= 0;

	while ((r = pcap_next_ex (ConfigParams.in_descr, &pkthdr, &packet)) == 1)
	{
		if (counter >= QUEUE_SIZE)
		{
			error("Too many packet for queue");
			exit(-2);
		}
		
		hndl = &ptr->queues[counter];
		cap_len = pkthdr->caplen;
		hndl->cap_len = cap_len;

		memcpy(hndl->pkt_data, packet, cap_len);

		counter++;
	}
	/* insert a fake trailing packet to signal workers that capture is over */
	log("Inserting last packet into queue");
		
	hndl = &ptr->queues[counter];
	hndl->cap_len = 0;
}


/*
	The user may provide a callback function that should be registered on an output application interface.
	This function is called when an exchange buffer is sent toward an output socket of the NetVM application.
*/
int32_t ApplicationCallbackMC(nvmExchangeBuffer *xbuffer)
{
	/* struct pcap_pkthdr pkthdr; */
	/* u_int8_t *packet; */
	/* u_int16_t ip_src_offs = 0; */
	/* u_int16_t ip_src_len = 0; */
	/* u_int16_t ip_dst_offs = 0; */
	/* u_int16_t ip_dst_len = 0; */
	
	/* u_int8_t *ipsource = NULL; */
	/* u_int8_t *ipdest = NULL; */

	/* ip_src_offs = *(u_int16_t*)&xbuffer->InfoData[0]; */
	/* ip_src_len = *(u_int16_t*)&xbuffer->InfoData[2]; */
	/* ip_dst_offs = *(u_int16_t*)&xbuffer->InfoData[4]; */
	/* ip_dst_len = *(u_int16_t*)&xbuffer->InfoData[6]; */
	
	/* // Retrieve ip source and destination addresses */
	/* ipsource = &xbuffer->PacketBuffer[ip_src_offs]; */
	/* ipdest = &xbuffer->PacketBuffer[ip_dst_offs]; */

	/* printf("Packet %u\n", ConfigParams.n_pkt++); */
	/* printf("ip.src: offs = %u, len = %u, value = ", ip_src_offs, ip_src_len); */

	/* if (ip_src_len == 0) */
	/* 	printf("null\n"); */
	/* else */
	/* 	printf("%d.%d.%d.%d\n", ipsource[0], ipsource[1], ipsource[2], ipsource[3]); */
		
	/* printf("ip.dst: offs = %u, len = %u, value = ", ip_dst_offs, ip_dst_len); */

	/* if (ip_src_len == 0) */
	/* 	printf("null\n"); */
	/* else */
	/* 	printf("%d.%d.%d.%d\n\n",  ipdest[0], ipdest[1], ipdest[2], ipdest[3]);	 */
	return nvmSUCCESS;
}


/* function called by children after netvm setup. It reads packets
 * from the queue in the shared memory and writes them to the netvm */
void read_queue_loop(int child_id, nvmAppInterface *nvm_interface)
{
int fd, batch_counter = 0;
int index = 0;					/* index of ready data */
const struct pcap_pkthdr *pkthdr;
const u_char *packet;
struct shmstruct *ptr;
pkt_handle *handle;
my_timer _timer;
char errbuf[1500];
	
	log("child %d starting read_queue", child_id);
	/* first open shared memeory */
	fd = shm_open(TMP_FNAME, O_RDWR , S_IRWXU); /* open */
	if( (ptr = mmap(NULL, sizeof(struct shmstruct), PROT_READ | PROT_WRITE,
					MAP_SHARED, fd, 0)) == MAP_FAILED){
		log("child[%d] cannot open shared mem", child_id);
		exit(-1);
	};
	ftruncate(fd, sizeof(struct shmstruct)); /* resize */
	close(fd);								 /* close fd */

	log("Child %d finished setting up shared mem: ptr %lx", child_id, (long unsigned int) ptr);

	my_timer_start(&_timer);
	/* consumer */
	for ( ; ; ){

		handle = &ptr->queues[index];
		
		if(handle->cap_len <= 0){
			break;
		}
//		ptr->sdatas[child_id].bytes += handle->cap_len;
		if( nvmWriteAppInterface(nvm_interface, (u_int8_t *) handle->pkt_data,
								 handle->cap_len, NULL, errbuf) != nvmSUCCESS)
		{
			fprintf(stderr, "error writing packets to the NetVM application interface\n");
			exit(1);
		}
		index++;
	}
	my_timer_stop(&_timer);
	log("Child[%d] received termination packet", child_id);
	log("Processed %d packets in %f seconds, %f pps", index, my_timer_elapsed(&_timer),
		index/my_timer_elapsed(&_timer));

	exit (0);
}


/* called by each child, sets the netvm up */
int NetVMSetup(char* filename, struct shmstruct *ptr, int child_id)
{
	//NetVM structures
	nvmByteCode				*BytecodeHandle = NULL;	//Bytecode image
	nvmNetVM				*NetVM=NULL;			//NetVM application Object
	nvmNetPE				*NetPE1=NULL;			//NetPE
	nvmSocket				*SocketIn=NULL;			//Input Socket	
	nvmSocket				*SocketOut=NULL;		//Output Socket
	nvmRuntimeEnvironment	*RT=NULL;				//Runtime environment
	nvmAppInterface			*InInterf=NULL;			//Input Application Interface 
	nvmAppInterface			*OutInterf=NULL;		//Output Application Interface 
	nvmPhysInterfaceInfo	*info=NULL;				//List of physical interface descriptors
	char					errbuf[nvmERRBUF_SIZE]; //buffer for error messages


	//create a NetVM application for runtime execution
	NetVM = nvmCreateVM(nvmRUNTIME_COMPILEANDEXECUTE, errbuf);

	//create a bytecode image by assembling a NetIL program read from file
	BytecodeHandle = nvmAssembleNetILFromFile(filename, errbuf);

	if (BytecodeHandle == NULL)
	{
		fprintf(stderr, "Error assembling netil source code: %s\n", errbuf);
		return -1;
	}
	
	//create a PE inside NetVM using the bytecode image
	NetPE1 = nvmCreatePE(NetVM, BytecodeHandle, errbuf);
	if (NetPE1 == NULL)
	{
		fprintf(stderr, "Error Creating PE: %s\n", errbuf);
		return -1;
	}
	nvmSetPEFileName(NetPE1, filename);
	//create two netvm sockets inside NetVM
	SocketIn = nvmCreateSocket(NetVM, errbuf);
	SocketOut = nvmCreateSocket(NetVM, errbuf);

	//connect the netvm sockets to the input (i.e. 0) and output (i.e. 1) ports of the PE
	nvmConnectSocket2PE(NetVM, SocketIn, NetPE1, 0, errbuf);
	nvmConnectSocket2PE(NetVM, SocketOut, NetPE1, 1, errbuf);

	RT = nvmCreateRTEnv(NetVM, ConfigParams.creationFlag, errbuf);
	if (RT == NULL)
	{
		fprintf(stderr, "Error creating NetVM runtime environment\n");
		return -1;
	}

	// Create an output interface for receiving packets from the NetVM
	OutInterf = nvmCreateAppInterfacePushOUT(RT, ApplicationCallbackMC, errbuf);

	// Bind the output interface to the output socket
	nvmBindAppInterf2Socket(OutInterf, SocketOut);

	//create an application input inteface, since we manually inject packets into NetVM
	InInterf = nvmCreateAppInterfacePushIN(RT, errbuf);
		
	//bind the application interface to the NetVM input socket
	nvmBindAppInterf2Socket(InInterf, SocketIn);

	// Initialize the runtime environment for the execution
	if (nvmNetStart(NetVM, RT, ConfigParams.usejit, ConfigParams.jitflags, ConfigParams.opt_level, errbuf) != nvmSUCCESS)
	{
		fprintf(stderr, "Error Starting the Runtime Environment:\n%s\n",errbuf);
		exit(1);
	}

	/* signal parent termination of netvm setup */
	log("child[%d] signaling to parent that set up is finished", child_id);
	sem_post(&(ptr->nvmsetup_sem) );
	/* now wait for a signal from parent (ensures that all children
	 * start at the same time) */
	log("child[%d] waiting to start", child_id);
	sem_wait(&(ptr->pstart_sem) );
	log("child[id] Here we goooo", child_id);
	/* read queue */
	read_queue_loop(child_id, InInterf);

	// Destroy the whole runtime environment
	nvmDestroyRTEnv(RT);

	return (nvmSUCCESS);
}


/* main multicore function */
int go_multi_core(char* filename)
{
pid_t child_pids[MAX_PROC];
int i,fd;
struct shmstruct *ptr;
cpu_set_t mask;

	/* Setting up main process shared memory */
	shm_unlink(TMP_FNAME); 		/* delete previous file */
	fd = shm_open(TMP_FNAME, O_RDWR | O_CREAT | O_EXCL, S_IRWXU); /* open */
	if ( (ptr = mmap(NULL, sizeof(struct shmstruct), PROT_READ | PROT_WRITE,
					 MAP_SHARED, fd, 0)) == MAP_FAILED)
	{
		log("cannot map memory :(");
		exit (-1);
	};

	ftruncate(fd, sizeof(struct shmstruct)); /* resize */
	close(fd);								 /* close fd */

	log("Size of memory: %ldM, handle %ld, addr: %lx",
			 sizeof(struct shmstruct)/(1<<20), sizeof(pkt_handle), (long unsigned int) ptr);

	/* initialize sempaphores in shared memory */
	sem_init(&(ptr->pstart_sem), 1, 0);
	sem_init(&(ptr->nvmsetup_sem), 1, 0);
	sem_init(&(ptr->log_sem), 1, 1);

	/* setup some handlers to get children initialization end*/
	setup_handlers();
	log("starting copying capture");
	/* copy capture in memory */
	copy_capture(ptr);
	log("copied capture into memory");

	/* for each child */
	for (i = 0; i < ConfigParams.ncores; i++)
	{
		/* fork */
		log("Forking child %d", i);
		
		if ((child_pids[i] = fork()) >= 0)
		{
			if (child_pids[i] == 0)
			{
				/* kid */
				CPU_ZERO(&mask);
				CPU_SET(i, &mask);
				/* bind each child to a single core */
				if (sched_setaffinity(0, sizeof(mask), &mask) == -1)
				{
					log("setaffinity badly dead");
					exit (-1);
				}
				
				prctl(PR_SET_PDEATHSIG, SIGHUP); /* if parent dies, we die too */
				child_n = i;
				log("Child %d, pid %d", i, getpid());
				/* setup netmv */
				NetVMSetup(filename, ptr, i);
				break;
			}
			else
			{
				/* parent */
				continue;
			}
		};
	}

	/* log ("Freeing rule options"); */
	/* rule_free_options (sdata.rules); */

	/* wait children to finish netvm setup */
	for (i = 0 ; i < ConfigParams.ncores; i++)
	{
		log("%d processes finished setting up nvm", i);
		sem_wait(&(ptr->nvmsetup_sem) );
	}
	
	log("continuing");

	/* now i tell kids to start */
	for (i = 0; i < ConfigParams.ncores; i++)
	{
		sem_post(&(ptr->pstart_sem) );
	}

	/* finally wait packet processing */
	while(!netvm_termination)
		sleep(1);

	log_close();
	return 0;

}

#endif // ENABLE_MULTICORE

