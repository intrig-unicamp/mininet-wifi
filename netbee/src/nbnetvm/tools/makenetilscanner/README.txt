makenetilscanner
this tool is used in order to generate the grammar file used for 
creating the lexical scanner for the NetIL assembly language through Flex.
The process is the following:
makenetilscanner reads a "template" scanner description containing the defitions
of all the NetIL assembler tokens and merges it with the tokens relative to
the opcodes of the assembler that it reads from the nbnetvm/src/opcodes.def
file. The result is a complete description of the NetIL lexical scanner, which 
can be processed by flex in order to generate the code implementing it. Finally
the generated scanner is compiled and linked into the netvm library.

nbnetvm/src/assembler/scanner-template.l       nbnetvm/src/opcodes.def
                                   |              |
                                   v              v
                                 +------------------+
                                 | makenetilscanner |
                                 +------------------+
                                          |
                                          v
                            nbnetvm/src/assembler/scanner.l
                                          |
                                          v
                                 +------------------+
                                 |       FLEX       |
                                 +------------------+
                                          |
                                          v
                            nbnetvm/src/assembler/scanner.c
                            
The usage of the tool is the following:

# makenetilscanner opcodes.def scanner-template.l > scanner.l
