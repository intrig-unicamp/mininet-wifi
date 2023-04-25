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


// For scanning the protocol, database, we have to include this file
#include <nbprotodb.h>

#define DEFAULT_NETPDL_DATABASE "./netpdl.xml"

const char *NetPDLFileName= DEFAULT_NETPDL_DATABASE;
int ValidateNetPDL;


void Usage()
{
char string[]= \
	"\nUsage:\n"	\
	"  protodbdump [NetPDLFile]\n\n"	\
	"Options:\n"	\
	" -netpdl FileName: name (and *path*) of the file containing the NetPDL\n"			\
	"     description. In case it is omitted, the NetPDL description embedded\n"		\
	"     within the NetBee library will be used.\n"									\
	" -validate: in case this flag is turned on, it validates the NetPDL file\n"		\
	"     against the XSD schema.\n"													\
	" -h: prints this help message.\n\n"												\
	"Description\n"																		\
	"============================================================================\n"	\
	"This program can be used to see how to interact with the NetPDL protocol\n"		\
	"  database in order to extract information related to the format of the\n"			\
	"  protocols contained into it.\n"													\
	"This program opens a NetPDL protocol database and prints the names of\n"			\
	"  all the protocols contained into it.\n"											\
	"In addiction, it prints the most important information related to the\n"			\
	"  *most important fields* within each protocol.\n";

	printf("%s", string);
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

		if (strcmp(argv[CurrentItem], "-validate") == 0)
		{
			ValidateNetPDL= true;
			CurrentItem++;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-h") == 0)
		{
			Usage();
			return 0;
		}

		printf("Error: parameter '%s' is not valid.\n", argv[CurrentItem]);
		return 0;
	}

	return 1;
}


void GetFieldType(struct _nbNetPDLElementBase *NetPDLElement, char *FieldString)
{
	switch(NetPDLElement->Type)
	{
		case nbNETPDL_IDEL_FIELD:
		{
		struct _nbNetPDLElementFieldBase *FieldElement;
		const char *FieldElementType;

			FieldElement= (struct _nbNetPDLElementFieldBase *) NetPDLElement;

			switch(FieldElement->FieldType)
			{
				case nbNETPDL_ID_FIELD_FIXED: FieldElementType= NETPDL_FIELD_ATTR_TYPE_FIXED; break;
				case nbNETPDL_ID_FIELD_BIT: FieldElementType= NETPDL_FIELD_ATTR_TYPE_BIT; break;
				case nbNETPDL_ID_FIELD_VARIABLE: FieldElementType= NETPDL_FIELD_ATTR_TYPE_VARIABLE; break;
				case nbNETPDL_ID_FIELD_TOKENENDED: FieldElementType= NETPDL_FIELD_ATTR_TYPE_TOKENENDED; break;
				case nbNETPDL_ID_FIELD_TOKENWRAPPED: FieldElementType= NETPDL_FIELD_ATTR_TYPE_TOKENWRAPPED; break;
				case nbNETPDL_ID_FIELD_LINE: FieldElementType= NETPDL_FIELD_ATTR_TYPE_LINE; break;
				case nbNETPDL_ID_FIELD_PADDING: FieldElementType= NETPDL_FIELD_ATTR_TYPE_PADDING; break;
				case nbNETPDL_ID_FIELD_PLUGIN: FieldElementType= NETPDL_FIELD_ATTR_TYPE_PLUGIN; break;
				default: FieldElementType= "Unknown field type"; break;
			}

			sprintf(FieldString, "\t<%s type=\"%s\" name=\"%s\" longname=\"%s\" .../>\n", nbNETPDL_EL_FIELDS, FieldElementType, FieldElement->Name, FieldElement->LongName);
		}; break;

		case nbNETPDL_IDEL_SWITCH: sprintf(FieldString, "\t<%s ...>\n", nbNETPDL_EL_SWITCH); break;
		case nbNETPDL_IDEL_LOOP: sprintf(FieldString, "\t<%s ...>\n", nbNETPDL_EL_LOOP); break;
		case nbNETPDL_IDEL_LOOPCTRL: sprintf(FieldString, "\t<%s ...>\n", nbNETPDL_EL_LOOPCTRL); break;
		case nbNETPDL_IDEL_IF: sprintf(FieldString, "\t<%s ...>\n", nbNETPDL_EL_IF); break;
		case nbNETPDL_IDEL_INCLUDEBLK: sprintf(FieldString, "\t<%s ...>\n", nbNETPDL_EL_INCLUDEBLK); break;
		case nbNETPDL_IDEL_BLOCK: sprintf(FieldString, "\t<%s ...>\n", nbNETPDL_EL_BLOCK); break;
		default: sprintf(FieldString, "\t<(Another NetPDL element)>\n", nbNETPDL_EL_BLOCK); break;
	}
}


int main(int argc, char *argv[])
{
char ErrBuf[2048];
struct _nbNetPDLDatabase *NetPDLProtoDB;

	if (ParseCommandLine(argc, argv) == 0)
		return 0;

	printf("\n\nLoading NetPDL protocol database...\n");

	// Get the class that contains the NetPDL protocol database
	if (ValidateNetPDL)
		NetPDLProtoDB= nbProtoDBXMLLoad(NetPDLFileName, nbPROTODB_FULL | nbPROTODB_VALIDATE, ErrBuf, sizeof(ErrBuf));
	else
		NetPDLProtoDB= nbProtoDBXMLLoad(NetPDLFileName, nbPROTODB_FULL, ErrBuf, sizeof(ErrBuf));

	if (NetPDLProtoDB == NULL)
	{
		printf("Error loading the NetPDL protocol Database: %s\n", ErrBuf);
		return 0;
	}
	printf("NetPDL Protocol database loaded.\n");
	printf("  Creator: %s\n", NetPDLProtoDB->Creator);
	printf("  Creation date: %s\n", NetPDLProtoDB->CreationDate);
	printf("  NetPDL version: %u.%u\n\n", (unsigned int) NetPDLProtoDB->VersionMajor, (unsigned int) NetPDLProtoDB->VersionMinor);


	// Scan the content of the protocol database, from the first to the last protocol
	for (unsigned int i= 0; i < NetPDLProtoDB->ProtoListNItems; i++)
	{
	struct _nbNetPDLElementBase *NetPDLElement;
	char FieldString[1024];

		NetPDLElement= NetPDLProtoDB->ProtoList[i]->FirstField;
		// If the protocol has fields, let's retrieve the structure associated to the first field

		printf("\nProtocol n. %d: %s (%s)\n", i, NetPDLProtoDB->ProtoList[i]->Name, NetPDLProtoDB->ProtoList[i]->LongName);

		if (NetPDLElement)
		{
			while (NetPDLElement != nbNETPDL_INVALID_ELEMENT)
			{
				GetFieldType(NetPDLElement, FieldString);
				printf("%s", FieldString);

				NetPDLElement= nbNETPDL_GET_ELEMENT(NetPDLProtoDB, NetPDLElement->NextSibling);
			}
		}
		else
			printf("\t(no fields associated to this protocol)\n");
	}

	nbProtoDBXMLCleanup();

	return 1;
}
