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

#pragma once

#ifdef ENABLE_MULTICORE

#include <semaphore.h>
#include <pcap.h>

#define MAX_PROC 4
#define QUEUE_SIZE 800000
#define MAX_PKT_SIZE 1500
#define TMP_FNAME "/netvm_shm"

/* structure that holds packet informations (size and data) */
typedef struct
{
	u_int32_t cap_len;
	u_char pkt_data[MAX_PKT_SIZE];
} pkt_handle;

/* shared memory structure */
struct shmstruct
{
	sem_t nvmsetup_sem;			/* parent waits on this sem for all children to complete netvm setup */
	sem_t pstart_sem;			/* children wait on this sem before starting the processing (synchronizes children */
	sem_t log_sem;				/* assures that log dump is done sequantially (no mixed output) */
	pkt_handle queues[QUEUE_SIZE]; /* packet capture */
};


int child_n;

int go_multi_core(char* filename);

#endif // ENABLE_MULTICORE
