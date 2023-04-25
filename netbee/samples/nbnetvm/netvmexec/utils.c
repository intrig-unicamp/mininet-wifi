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


#include "utils.h"
#if (defined _WIN32) || (defined (_WIN64))
#include <sys/timeb.h>		// ftime()
#else
#include <sys/time.h>
#endif


// Windows does not have gettimeofday
#if (defined _WIN32) || (defined (_WIN64))

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
struct _timeb timebuffer;

	_ftime(&timebuffer);
	// There may be an overflow problem here, as we take an int64 and we store the result into a long 
	tv->tv_sec= (long) timebuffer.time;
	tv->tv_usec= timebuffer.millitm * 1000;
	return 1;
}

#endif


void my_timer_start (my_timer *t)
{
	gettimeofday (&(t->start), NULL);
	t -> running = 1;

	return;
}


double my_timer_elapsed (my_timer *t)
{
double out;
suseconds_t u;
struct timeval tv;

	if (t -> running)
		gettimeofday (&tv, NULL);
	else
		tv = t->end;

	out = tv.tv_sec - (t->start).tv_sec;
	if (tv.tv_usec >= (t->start).tv_usec)
	{
		u = tv.tv_usec - (t->start).tv_usec;
	}
	else
	{
		if (out > 0)
			out--;
		u = USECS_PER_SEC + tv.tv_usec - (t->start).tv_usec;
	}

	out += ((double) u / (double) USECS_PER_SEC);

	return (out);
}


double my_timer_stop (my_timer *t)
{
	gettimeofday (&(t->end), NULL);
	t -> running = 0;

	return (my_timer_elapsed (t));
}

