/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


/** @file netvm_bytecode.h
 * \brief This file contains prototypes, definitions and data structures related to the bytecode of a NetPE
 *
 */


#include <stdio.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup netvm_bytecode
	\{

	This section describes the format of a NetVM binary bytecode image, e.g. a file that contains a program
	for the virtual machine.
	The general structure is derived from the Windows Portable Executable (PE) binary format used also for
	CLR assemblies. However, it is more simple because many features of the original format are not needed.
	On the other hand, the binary format that we propose has some extensions not present in PE to handle
	aspects specifically related to the NetVM architecture.

	<h3>Relative Virtual Addresses</h3>
	Several fields in NetVM executables are specified in terms of Relative Virtual Addresses (RVA). An RVA
	is simply the offset of some item, relative to where the file is memory-mapped. For example, let's say
	the loader maps a PE file into memory starting at address 0x10000 in the virtual address space. If a certain
	table in the image starts at address 0x10464, then the table's RVA is 0x464. To convert an RVA into a usable
	pointer, simply add the RVA to the base address of the module.

	<h3>Executable image structure</h3>
	A NetVM executable is made up of an header, a section table and one or more sections. The binary file contains
	no space or padding between consecutive pieces of information; everything is aligned on byte boundaries: this
	helps keep files small. Data are encoded in little-endian (Intel) byte order.

	The structure of the header is defined in the nvmByteCodeImageHeader structure.
	After the signature, four bytes representing the 32 bit integer value 0x45444F4C (the string "LODE"), there
	is a image header and an optional image header. While the image header is mandatory, the optional image header
	may be absent.

	The structure of the image header is defined in the nvmByteCodeImageStruct structure.
	The structure of the optional image header is defined in the nvmByteCodeImageStructOpt structure.

	<h3>Sections</h3>
	The chunk after the image header contains the section table, an array of section headers. Its size is
	determined by the NumberOfSections field in the image Standard Header (nvmByteCodeImageStruct). Every section
	header describes a section of the executable, and has the format defined in the nvmByteCodeSectionHeader
	structure.

	After the section table, the file contains the bodies of the various sections. There are six section types:
	- init segment code section (type=BC_INIT_SCN)
	- push segment code section (type=BC_PUSH_SCN)
	- pull segment code section (type=BC_PULL_SCN)
	- port table section (type=BC_PORT_SCN)
	- constant pool (type=BC_INITIALIZED_DATA_SCN): a constant pool can be used	to store constant values like static
	variables or string literals FULVIO PER GIANLUCA - LORIS ANCORA DA FARE : credo che questa cosa sia un pelino da chiarire
	GV 07/15/2004 Non ho la piu' pallida idea di cosa sia, il formato file era stato creato da loris, sulla base del
	formato PE di Microsoft, in modo che fosse molto espandibile. Dopodiche' io non ci ho mai messo mano, non l'ho mai studiato, e non ho
	bene idea di come sia esattamente.
	FR 07/21/04: e' possibile chiedere a Loris?
	- exception table section: BC_ETABLE_SCN

	FULVIO PER GIANLUCA - LORIS ANCORA DA FARE : questi valori "type", menzionati poc'anzi, dove sono utilizzati? Esiste da qualche
	parte nel bytecode una variabile intera che tiene questa codifica?
	GV 07/15/2004 Vale il discorso di prima.

	Each section's bytecode can be accessed using the nvmByteCodeSectionHeader.PointerToRawData field of the
	corresponding section header as a base. The byte at the offset <code>i</code> from the beginning of the
	section can be found using the following relative addressing method:

	<pre>    addr = binary + section-&gt;PointerToRawData + i</pre>

	where:
	- <code>binary</code> is a pointer to the <code>nvmIMAGE_HEADER</code> struct
	- <code>section</code> is a pointer to the <code>_BC_SECTION_HEADER</code> struct of the section of interest
	- <code>i</code> is the displacement of the byte in the section's bytecode

	The binary format of each section depends on its kind, so the sections containing executable code are
	different from the section containing the port-table.

	<h3>Executable Code Sections</h3>
	The sections relative to an executable code segment (init, push, pull) always begin with two 32 bit integer
	parameters: the first represent the maximum operand's stack size on which the code segment will rely,
	while the second represent the locals area size, i.e. the size of the local variables pool that will
	be used by the code.
	Such parameters are set in a NetIL program with the assembler directives .maxstacksize and .locals.

    After those eight bytes the executable code begins.

    Let's make an example. The .init segment section begins at the RVA
	<code>initSection-&gt;PointerToRawData</code>.
	At that offset we find the <em>.maxstacksize</em> parameter, after four bytes we find the <em>.locals</em> parameter,
	after other four bytes, at the RVA <code>fileHdr-&gt;AddressOfInitEntryPoint</code>,
	we find the init segment's executable code, which length is given by the value
	<code>initSection-&gt;SizeOfRawData - 8</code>.

	<h3>Port Table Section</h3>
	The meaninig of the data found in a .ports segment section is totally different. This section contains the
	port table, an array that makes the association <code>port[index] = type</code> for each port connected
	to the NetPE. The first 32 bit integer at the beginning of the section is the number n of ports connected to the NetPE.
	The 32 bit integers that follow represent the types of the n ports.

	The port types defined in the file <code>netvmstructs.h</code> are:</p>

	<ul>
		<li>PUSH_INPUT_PORT</li>
		<li>PUSH_OUTPUT_PORT</li>
		<li>PULL_INPUT_PORT</li>
		<li>PULL_OUTPUT_PORT</li>
	</ul>

	Let's suppose that a NetPE has three connections: a push input port, a push output port and a pull output port,
	respectively named as <em>in1</em>, <em>out1</em> and <em>out2</em>. The .ports segment in the NetIL assembler
	source file will look like this:

	<pre>
	  segment .ports
	    push_input  in1
	    push_output out1
	    pull_output out2
	  ends
	</pre>

	The .ports section of such NetPE will begin with the 32 bit integer 0x00000003 (number of ports) and three
	32 bit integers will follow, the first containing the value defined as PUSH_INPUT_PORT, the second with the value
	PUSH_OUTPUT_PORT and the third with the value PULL_OUTPUT_PORT.

	<h3>Exception Table Section (currently not implemented)</h3>
	The exception table section, if present, contains the exception table of the executable, i.e. an array of
	pointers to the various exception handlers inside the code section. This allows to avoid manual initialization
	of the virtualprocessor’s exception table, often tedious and space consuming (not only in
	the binary but also in the sources), during the init phase.
*/


//Characteristics
#define BC_INIT_SCN				0x00000001	///< Section Code: Section is relative to an init segment.
#define BC_PUSH_SCN				0x00000002	///< Section Code: Section is relative to a push segment.
#define BC_PULL_SCN				0x00000003	///< Section Code: Section is relative to a pull segment.
#define BC_CODE_SCN				0x00000010	///< Section Code: Section contains actual code
#define BC_INSN_LINES_SCN		0x00000020	///< Section Code: Section contains bytecode to source line mappings
#define BC_INITIALIZED_DATA_SCN	0x00000040	///< Section Code: Section contains a constant pool.
#define BC_METADATA_SCN			0x00000080	///< Section Code: Section contains general PE properties.
#define BC_ETABLE_SCN			0x00000100	///< Section Code: Section contains the exception table.
#define BC_PORT_SCN				0x00000200	///< Section Code: Section contains the port table.

//Allignment of structure to one byte
#ifdef WIN32
	#pragma pack(push, ORIGINAL_PACKSIZE)
	#pragma pack(push, 1)
#else
	#pragma pack(push, 1)
	#endif

/*!
	\brief Structure keeping standard header of bytecode.
*/
typedef struct nvmByteCodeImageStruct
{
	uint16_t	NumberOfSections;		//!< Number of sections in this byte code.
	uint32_t	TimeDateStamp;			//!< The time in which the linker or compiler created this image, in UTC.
	uint16_t	SizeOfOptionalHeader;		//!< The size of an optional header that can follow this structure. '0' if no optional header is present.
	uint32_t	AddressOfInitEntryPoint;	//!< RVA of the Init function. This function must always be present.
	uint32_t	AddressOfPushEntryPoint;	//!< RVA of the Push function. This function must always be present.
	uint32_t	AddressOfPullEntryPoint;	//!< RVA the Pull function. This function must always be present.

} nvmByteCodeImageStruct;


/*!
	\brief Structure keeping optional header of bytecode.
*/
typedef struct nvmByteCodeImageStructOpt
{
	uint8_t    MajorLinkerVersion;				//!< The major version of the compiler/linker that produced this image.
	uint8_t    MinorLinkerVersion;				//!< The minor version of the compiler/linker that produced this image.
	uint8_t	MajorVirtualMachineVersion;		//!< The minimum major version of the NetVM needed to launch this executable.
	uint8_t	MinorVirtualMachineVersion;		//!< The minimum minor version of the NetVM needed to launch this executable.
	uint16_t	MajorImageVersion;				//!< A user-definable field, that allows the user to have different versions of an executable.
	uint16_t	MinorImageVersion;				//!< A user-definable field, that allows the user to have different versions of an executable.
	uint32_t	CheckSum;						//!< CRC checksum of the file. '0' if no checksum is provided.
	uint8_t    Compressed;						//!< '1' if the binary is compressed, '0' otherwise.
	uint32_t	DefaultDataMemSize;				//!< Data memory size required by the program. This value is used in case the data memory size is not set during the init phase.
	uint32_t	DefaultExchangeBufSize;			//!< Size of the exchange buffer required by the program. This value is used in case the exchange buffer size is not set during the init phase.
	uint32_t	DefaultStackSize;				//!< Stack size required by the program. This value is used in case the stack size is not set during the init phase (.maxstacksize directive).
	uint32_t	DefaultPortTableSize;			//!< FULVIO PER GIANLUCA: scrivere la documentazione di questo membro

} nvmByteCodeImageStructOpt;


/*!
  \brief Structure keeping general header of bytecode.
*/
typedef struct nvmByteCodeImageHeader
{
    uint32_t					Signature;			//!< Signature used to check the consistency of the bytecode; the right value is "LODE".
    nvmByteCodeImageStruct		FileHeader;			//!< Standard header of a bytecode.
    nvmByteCodeImageStructOpt	OptionalHeader;		//!< Optional header of the bytecode containing additional information; rigth now the interpreter ignores it.

} nvmByteCodeImageHeader;


/*!
  \brief Structure Keeping header of each section of the bytecode.
*/
typedef struct nvmByteCodeSectionHeader
{
	int8_t		Name[16];				//!< A NULL terminated string that names the section, maximum 8 characters. Typical values are '.init', '.push', '.pull', '.ports'.
	uint32_t	PointerToRawData;		//!< RVA of the (byte)code referred to this section.
	uint32_t	SizeOfRawData;			//!< Size (in bytes) of this section .
	uint32_t	PointerToRelocations;	//!< Not yet supported. In OBJs, this is the file-based offset to the relocation information for this section. The relocation information for each OBJ section immediately follows the raw data for that section.
	uint16_t	NumberOfRelocations;	//!< Not yet supported. //! \todo FULVIO PER GIANLUCA: a che serve a fare? GV 07/15/2004 Il formato simil PE estendibile. Non ne ho la piu' pallida idea.
	uint32_t	SectionFlag;			//!< Characteristics of the section, specifies the type of section.

} nvmByteCodeSectionHeader;


/*!
	\brief Main structure that contains the NetVM bytecode.
	In this structure there is a complete list of all the sections of the bytecode, these sections are respectively:
	- <b>Init section</b>: this section contains the code that is executed when a NetPE starts for the first time
	- <b>Push section</b>: this section contains the code that is executed when a NetPE receives a Push signal
	- <b>Pull section</b>: this section contains the code that is executed when a NetPE receives a Pull signal
	- <b>Port section</b>: this section contains a list of all the ports belonging to a NetPE
 */
struct _nvmByteCode
{
	FILE						*TargetFile;		//!< Descriptor of the file where the bytecode has been taken from
	nvmByteCodeImageHeader		*Hdr;				//!< Reference to the header of the bytecode
	nvmByteCodeSectionHeader	*SectionsTable;		//!< List of the headers sections of the bytecode; for further detail see the related documentation
	char						*Sections;			//!< List of the sections of the bytecode
	uint32_t					SizeOfSections;		//!< Total size of all the data stored in the various sections of the bytecode

};


#ifdef WIN32
	#pragma pack(pop, ORIGINAL_PACKSIZE)
#else
	#pragma pack(pop)
#endif

/* End of Doxygen group */
/** \} */

#ifdef __cplusplus
}
#endif

