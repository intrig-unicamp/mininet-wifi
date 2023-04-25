makopcodetable
this tool is used in order to create a table containing a description (i.e a textual mnemonic and the number of expected arguments) for
every instruction opcode of the NetIL assembly language. In particular, since
the numerical values assigned to NetIL opcodes represent a sparse set of values 
in the interval 0..255, and the file opcodes.def contains their description
in an arbitrary order, the makeopcodetable tool scans the opcodes.def file and
creates an array of descriptions that can be indexed by using the opcodes' 
numerical values following the schema presented in the figure:

                  opcodetable:
 
 Opcode     Mnemonic      Number of    
 value        name        arguments
  0x00  -->  spload.8",  T_NO_ARG_INST
  0x01  -->  upload.8",  T_NO_ARG_INST
  0x02  -->  spload.16", T_NO_ARG_INST
  0x03  -->  upload.16", T_NO_ARG_INST
  0x04  -->  spload.32", T_NO_ARG_INST
  0x05  -->  smload.8",  T_NO_ARG_INST
  0x06  -->  umload.8",  T_NO_ARG_INST
  0x07  -->  smload.16", T_NO_ARG_INST
  0x08  -->  umload.16", T_NO_ARG_INST
  0x09  -->  umload.32", T_NO_ARG_INST
    .           .              .
    .           .              .
  0xC6  -->    "",             0
  0xC7  -->    "",             0
    .           .              .
    .           .              .
  0xFD  -->  copro.init", T_COPRO_INIT_INST
  0xFE  -->  copro.exbuf", T_2_COPRO_IO_ARG_INST
  0xFF  -->  exit", T_1INT_ARG_INST}

The usage of the tool is the following:

# makeopcodetable opcodes.def > opcodetable.def