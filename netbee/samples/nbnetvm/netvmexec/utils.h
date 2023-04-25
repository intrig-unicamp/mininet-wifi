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



#include <time.h>
#ifdef WIN32
#include <stdint.h>
#include <WinSock2.h>
#else
#include <sys/time.h>
#endif
#include <pcap.h>		// we use pcap for capturing packets from file


#ifdef WIN32
typedef uint64_t suseconds_t;
#endif

#define USECS_PER_SEC	1000000

// Parameters used in this sample
typedef struct _ConfigParams
{
	pcap_t* in_descr;
	pcap_dumper_t* dump_handle;
	int count;
	int usejit;
	int backend;
	int creationFlag;
	int dumpBackends;
	int useInfo;
	int opt_level;
	int jitflags;
	int dobcheck;
	char *outfile;
	char *infile;
	char *ifname;
	char *netilfile;
	int n_pkt;
#ifdef ENABLE_MULTICORE
	int ncores;			// stores the number of children to spawn 
#endif
} ConfigParams_t;


typedef struct {
	int running;
	struct timeval start;
	struct timeval end;
} my_timer;


void my_timer_start (my_timer *t);

double my_timer_elapsed (my_timer *t);

double my_timer_stop (my_timer *t);

#define log(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");
#define debug(...)
#define log_close(...) fprintf(stderr, "closing log"); fprintf(stderr, "\n");
