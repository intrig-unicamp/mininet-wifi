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

#include <nbpflcompiler.h>
// If we want to use the NetPFL filter compiler module, we need to include
// also some primitives defined in the NetPDL protocol dababase module, which
// creates the protocol database (that is one of the input of the
// NetPFL compiler)
#include <nbprotodb.h>


#define DEFAULT_NETPDL_DATABASE "./netpdl.xml"
#define DEFAULT_INPUT_STRING "ip.src == 10.0.0.1"
#define DEFAULT_OUTPUT_FILE "stdout"
#define DEFAULT_AUTOMATON_FILE "automaton.dot"


const char *NetPDLFileName= DEFAULT_NETPDL_DATABASE;
const char *FilterString= DEFAULT_INPUT_STRING;
const char *OutputFilename= DEFAULT_OUTPUT_FILE;
const char *AutomatonFilename= DEFAULT_AUTOMATON_FILE;

FILE *OutputFile= stdout;
bool CompileOnly = false;
bool automaton = false;


void Usage()
{
char string[]= \
	"nUsage:\n"
	"  filtercompiler [options] [filterstring]\n\n"
	"Example:\n"
	"  filtercompiler -netpdl netpdl.xml \"ip.src == 10.0.0.1\"\n\n"
	"Options:\n"
	" -netpdl FileName: name (and *path*) of the file containing the NetPDL\n"
	"     description. In case it is omitted, the NetPDL description embedded\n"
	"     within the NetBee library will be used.\n"
	" -compileonly: in this case only the NetPFL syntax is checked, but the\n"
	"     NetIL code and the final state automaton are not actually generated.\n"
	"     It can be used to check if the first step of the compiler behaves\n"
	"     correctly and if the NetPFL string is correct. This option cannot\n"
	"     ne used at the same time of \"automaton\".\n"
	" -automaton: in this case the NetPFL syntax is checked and the automaton\n"
	"     related to the filter is built, but the NetIl code is not generated.\n"
	"     This option cannot be used at the same time of \"-compileonly\".\n"
	" -dump_netil Filename: write the netil code to \"FileName\" (default: stdout)\n"
	" -dump_fsa Filename.dot: write the final state atuomaton to \"Filename.dot\".\n"
	"     If this option is not specified, the automaton is written to \n"
	"     '"DEFAULT_AUTOMATON_FILE"'\n."
	" filterstring: a string containing the filter to be compiled, using the\n"
	"     NetPFL syntax. Default: we use the '" DEFAULT_INPUT_STRING "' filter.\n"
	" -h: prints this help message.\n\n"
	"Description\n"
	"============================================================================\n"
	"This program can be used to see how to interact with the NetPFLCompiler\n"
	"module. It prints on files the finite state automaton and the related legend,\n"
	"and to console the NetIL code generated from NetPFL filtering rule, e.g.\n"
	"'" DEFAULT_INPUT_STRING "', by exploiting the protocol format and encapuslation\n"
	"information provided in the given NetPDL protocol database.\n\n";

	fprintf(stderr, "%s", string);
}


int ParseCommandLine(int argc, char *argv[])
{
int CurrentItem;
	
	CurrentItem= 1;

	while (CurrentItem < argc)
	{
		if (strcmp(argv[CurrentItem], "-netpdl") == 0)
		{
			NetPDLFileName= argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-dump_netil") == 0)
		{
			OutputFilename = argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}
		
		if (strcmp(argv[CurrentItem], "-dump_fsa") == 0)
		{
			//we have to do some checks
			char *current = argv[CurrentItem+1];
			int size = strlen(current);
			if(size<5)
			{
				printf("Error: parameter '%s' is not valid. It must end with '.dot'\n", current);
				return nbFAILURE;
			}
			int length = size - 4;
			char *sub = &current[size-4];
			
			if(strcmp(sub,".dot")!=0)
			{
				printf("Error: parameter '%s' is not valid. It must end with '.dot'\n", current);
				return nbFAILURE;
			}			
		
			AutomatonFilename = current;
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-compileonly") == 0)
		{
			if(automaton)
			{
				printf("Error: you cannot specify the options \"-automaton\" and \"-compileonly\" at the same time\n");
				return nbFAILURE;
			}
		
			CompileOnly= true;
			CurrentItem++;
			continue;
		}
		
		if (strcmp(argv[CurrentItem], "-automaton") == 0)
		{
			if(CompileOnly)
			{
				printf("Error: you cannot specify the options \"-automaton\" and \"-compileonly\" at the same time\n");
				return nbFAILURE;
			}
		
			automaton= true;
			CurrentItem++;
			continue;
		}


		if (strcmp(argv[CurrentItem], "-h") == 0)
		{
			Usage();
			return nbFAILURE;
		}

		// This should be the filter string
		if (argv[CurrentItem][0] != '-')
		{
			FilterString= argv[CurrentItem];
			CurrentItem++;
			continue;
		}

		printf("Error: parameter '%s' is not valid.\n", argv[CurrentItem]);
		return nbFAILURE;
	}

	return nbSUCCESS;
}




int main(int argc, char *argv[])
{
char ErrBuf[2048];
int RetVal;
char *GeneratedCode= NULL;
nbNetPFLCompiler *NetPFLCompiler;

	if (ParseCommandLine(argc, argv) == nbFAILURE)
		return nbFAILURE;

	//if it is needed, open the file for the netil code
	if (!CompileOnly && !automaton && strcmp(OutputFilename, DEFAULT_OUTPUT_FILE) != 0)
	{
		if ((OutputFile = fopen(OutputFilename, "w")) == NULL)
		{
			fprintf(stderr, "\nError opening the output file %s\n", OutputFilename);
			return 1;
		}

	}

	fprintf(stderr, "\n\nLoading NetPDL protocol database '%s'...\n", NetPDLFileName);

	// Get the object containing the NetPDL protocol database
	// This function belongs to the NetPDL Protocol Database module
	struct _nbNetPDLDatabase *NetPDLProtoDB= nbProtoDBXMLLoad(NetPDLFileName, nbPROTODB_FULL, ErrBuf, sizeof(ErrBuf));
	if (NetPDLProtoDB == NULL)
	{
		fprintf(stderr, "Error loading the NetPDL protocol Database: %s\n", ErrBuf);
		return 0;
	}

	fprintf(stderr, "NetPDL Protocol database loaded.\n");

	// Instantiates the NetPFL compiler using the previously loaded protocol database
	NetPFLCompiler= nbAllocateNetPFLCompiler(NetPDLProtoDB);

	// Initialize the compiler assuming we are filtering ethernet frames
	RetVal= NetPFLCompiler->NetPDLInit(nbNETPDL_LINK_LAYER_ETHERNET);
	
	if (RetVal == nbSUCCESS)
	{
		fprintf(stderr, "NetPFL Compiler successfully initialized\n");
	}
	else
	{
	int i=1;
	_nbNetPFLCompilerMessages *Message= NetPFLCompiler->GetCompMessageList();

		fprintf(stderr, "Unable to initialize the NetPFL compiler: %s\n\tError messages:\n", NetPFLCompiler->GetLastError());

		while (Message)
		{
			printf("\t%3d. %s\n", i, Message->MessageString);
			i++;
			Message= Message->Next;
		}
		return nbFAILURE;
	}

	fprintf(stderr, "Compiling filter '%s'...\n", FilterString);

	if (CompileOnly)
	{
		// Compiles only the filter, without actually generating the fsa and the NetIL code
		RetVal= NetPFLCompiler->CheckFilter(FilterString);
	}
	else if(automaton)
	{
		// Compiles teh filter and generates the fsa, without generateing the NetIL code
		RetVal= NetPFLCompiler->CreateAutomatonFromFilter(FilterString);
	}
	else
	{
		// Compiles the filter and generates NetIL code
		RetVal= NetPFLCompiler->CompileFilter(FilterString, &GeneratedCode);
	}
	
	if (RetVal == nbSUCCESS)
	{
		fprintf(stderr, "NetPFL filter successfully compiled\n");
		if(!CompileOnly)
		{
			NetPFLCompiler->PrintFSA(AutomatonFilename);
			printf("FSA printed in file: \"%s\"\n",AutomatonFilename);
			printf("Legend printed in file: \"legend.dot\"\n");
		}
	}
	else
	{
	int i=1;
	_nbNetPFLCompilerMessages *Message= NetPFLCompiler->GetCompMessageList();

		fprintf(stderr, "Unable to initialize the NetPFL compiler: %s\n\tError messages:\n", NetPFLCompiler->GetLastError());

		while (Message)
		{
			fprintf(stderr, "\t%3d. %s\n", i++, Message->MessageString);
			Message= Message->Next;
		}
		return nbFAILURE;
	}

	if (!CompileOnly && !automaton)
	{
		if (strcmp(OutputFilename, DEFAULT_OUTPUT_FILE) == 0)
			fprintf(stdout, "\nDumping the generated NetIL code:\n");
		fprintf(OutputFile, "%s", GeneratedCode);
		fclose(OutputFile);
	}
	nbDeallocateNetPFLCompiler(NetPFLCompiler);
	// This function belongs to the NetPDL Protocol Database module
	nbProtoDBXMLCleanup();
	return nbSUCCESS;
}
