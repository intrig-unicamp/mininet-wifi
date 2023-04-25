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

#include <sys/timeb.h>

#ifdef WIN32
#define ftime _ftime
#define timeb _timeb
#else
#define ftime ftime
#define timeb timeb
#endif

/*!
	\brief Small class to measure execution time.

	This class is provided as commodity for the samples that need to measure the time elapsed
	to accomplish some task.
	The measurement is not very precise, therefore the user should use this class only when
	the time to measure is far bigger than one second.
*/

class CMeasurement
{
public:
	CMeasurement() {};
	~CMeasurement() {};

	//! Start the measurement process
	void Start()
	{
		memset (&elapsed_time, 0, sizeof (struct timeb));
		ftime( &start );
	};

	//! End the measurement process and print (on screen) the elapsed time
	void EndAndPrint(char *InitString= NULL)
	{
		ftime( &finish ); diff= finish.millitm - start.millitm; 
		
		if (diff >= 0) elapsed_time.millitm+= diff;
		else elapsed_time.millitm+= (diff + 1000); 

		if (diff >= 0) elapsed_time.time= finish.time - start.time;
		else elapsed_time.time= finish.time - start.time - 1;

		if (InitString)
			printf( "\n%s: measured %d.%d seconds.\n", InitString, (int) elapsed_time.time, elapsed_time.millitm);
		else
			printf( "\nMeasured %d.%d seconds.\n", (int) elapsed_time.time, elapsed_time.millitm);
	};

private:
	struct timeb start, finish, elapsed_time;
	int diff;
};

