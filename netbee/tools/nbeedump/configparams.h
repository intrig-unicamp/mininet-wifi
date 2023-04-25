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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>


// Global variables for configuration

struct _Backend
{
	int		Id;
	bool	Optimization;
	bool	Inline;
};


typedef struct _Backend Backend_t;

struct _ConfigParams
{
	const char*	NetPDLFileName;
	bool        ValidateNetPDL;
	const char*	CaptureFileName;
	int         CaptureFileNumber;	// In case multiple input capture files exists, let's keep the number of the current capture
	pcap_dumper_t *PcapDumpFile;
	const char*	SaveFileName;
	const char*	FilterString;
	char        AdapterName[1024];
	int         PromiscuousMode;
	u_long      NPackets;
	int			SnapLen;
	u_char		DoNotPrintNetworkNames;
	bool		QuietMode;
	int			RotateFiles;
	u_int		DebugLevel;
	u_char		DumpCode;
	const char *	DumpCodeFilename;
#ifdef	_DEBUG
	const char *	DumpLIRCodeFilename;
	const char *	DumpHIRCodeFilename;
	const char *	DumpProtoGraphFilename;
	const char *	DumpFilterAutomatonFilename;
	const char *	DumpNoCodeGraphFilename;
	const char *	DumpNetILGraphFilename;
	const char *	DumpLIRNoOptGraphFilename;
	const char *	DumpLIRGraphFilename;
#endif
	u_char		StopAfterDumpCode;
	bool		UseJit;
	u_int32_t	NBackends;
	Backend_t	Backends[2];
};
typedef struct _ConfigParams ConfigParams_t;

// Prototypes
void Usage();
int ParseCommandLine(int argc, char *argv[]);
