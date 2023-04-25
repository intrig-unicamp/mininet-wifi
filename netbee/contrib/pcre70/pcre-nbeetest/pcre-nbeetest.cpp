/*
 * Copyright (c) 2002 - 2006
 * NetGroup, Politecnico di Torino (Italy)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Politecnico di Torino nor the names of its
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
#include <pcre.h>



int main(void)
{
//	char SearchString[]= "";
//	char SearchString[]= "GET / HTTP/1.1 HTTP/1.0";
//	char SearchString[]= "HTTP/1.1 200 OK\r\ncontent-type: x/mime";
//	char SearchString[]= "Header1\r\n\tlinea1.1\r\nHeader2\r\n\tlinea2.2\r\n";
//char SearchString[]= "pippo-;";
char SearchString[]= "\x04\x6D\xC4\x24\x7E\x5E";
//	char RegularExpression[]= "";
//	char RegularExpression[]= "HTTP";
//	char RegularExpression[]= "http/(0\\.9|1\\.0|1\\.1) [1-5][0-9][0-9]|post [\x09-\x0d -~]* http/[01]\\.[019]";
//	char RegularExpression[]= "\r\n[^\t ]";
//	char RegularExpression[]= "^\xe3.?.?.?.?[\x01\x02\x05\x14\x15\x16\x18\x19\x1a\x1b\x1c\x20\x21\x32\x33\x34\x35\x36\x38\x40\x41\x42\x43\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x60\x81\x82]";
//	char RegularExpression[]= "^\xE3.?.?.?.?[\x01\x02\x05\x14\x15\x16\x18\x19\x1a\x1b]";
//char RegularExpression[]= "\\w(?=;)";
char RegularExpression[]= "^\x04\x00[\x07\x05]\x00..\x00\x00[\x53\x44]\x53\x00[\x11\x12\x13]\x00";

pcre *PCRECompiledRegExp;
const char *PCREErrorPtr;
int PCREErrorOffset;
int RexExpReturnCode;
int MatchingOffset[30];
int i;

	PCRECompiledRegExp= pcre_compile(RegularExpression, PCRE_CASELESS, &PCREErrorPtr, &PCREErrorOffset, NULL);
	if (PCRECompiledRegExp == NULL)
	{
		printf("Regular expression compilation for string '%s' has failed at offset %d with error '%s'",
				RegularExpression, PCREErrorOffset, PCREErrorPtr);

		return 0;
	}

	RexExpReturnCode= pcre_exec(
		PCRECompiledRegExp,					// the compiled pattern
		NULL,								// no extra data - we didn't study the pattern
		SearchString,						// the subject string
		sizeof(SearchString),				// the length of the subject
		0,									// start at offset 0 in the subject
		0,									// default options
		MatchingOffset,						// output vector for substring information
		30);								// number of elements in the output vector

	for (i = 0; i < RexExpReturnCode; i++)
	{
		char *substring_start = SearchString + MatchingOffset[2*i];
		int substring_length = MatchingOffset[2*i+1] - MatchingOffset[2*i];
//			printf("%d (%d): %s\n", i, substring_length, substring_start);
		printf("%2d : %.*s\n", i, substring_length, substring_start);
	}

	if ((RexExpReturnCode >= 0) || (RexExpReturnCode == -1))
		return 0;
	else
		return -1;

    return 0;
}

