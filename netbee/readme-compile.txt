/**************************************************************************/
/*                      How to compile NetBee                             */
/**************************************************************************/

This section is useful only for the programmers that want to modify and 
recompile NetBee sources. 


1. Compiling NetBee on Windows
==============================

NetBee sources must be unpacked on the hard disk; in order to keep the 
settings of the project files, the folder structure must be the following:

(root)--+
        |
        +-- netbee   (the root folder of the NetBee library)
        |
        +-- xerces-c (XML library: include, library and DLL files)
        |
        +-- wpdpack  (WinPcap Developer's Pack: include, library and DLL 
		             files)
	
NetBee source code must be unpacked in the the netbee folder.

The Xerces library is used for managing XML files; the WinPcap library 
provides some low-level support for packet capture and such.

WinPcap warning: please note that you must install WinPcap on your system 
because the WinPcap Developer's Pack contains only headers and libraries 
required to compile NetBee, but it does not contain the run-time DLLs 
required to execute the software.


1.1 Required tools
------------------

In order to compile NetBee in Windows, you need the following tools:

* Microsoft Visual Studio .NET 2010
	The compiler, of course. 
	Please note that most of the code should compile with Visual Studio 
	>= 2005; we recommend version 2010 because of the better support to the
	C++ STL.

* Cmake	(www.cmake.org)	
	The building environment is created by CMake; hence you need this tool
	in order to create the project files (Visual Studio makefiles). 
	Note: you must install Cmake separately from Cygwin, since that version
	does not support the generation of project files for Visual Studio.

* CygWin	
	Some standard UNIX tools are used for some specific functions (scripts,
	e.g. for creating the source tarball, compiling grammar filesïfiles, 
	etc.). Although in principle you can avoid using CygWin, if you want to
	compile everything on Windows you must have this package installed.
	Please be careful that the following packages (in addition to default 
	ones) gets installed:
		* archive/zip
		* devel/bison
		* devel/doxygen
		* devel/flex


1.2 Required libraries
----------------------
In addition to the above mentioned tools, you need the following libraries:

* Xerces-C version greather than 3.0, 32 bits.
    (http://xml.apache.org/xerces-c/download.cgi)	
    Please note that multiple downloads are available, which depends on the
	C compiler you are using (e.g., Viual Studio 10 vs 9). 
	You have to extract all the content of the Xerces tarball 
	(e.g. xerces-c-3.1.1-x86-windows-vc-10.0.zip in Windows) in the xerces-c 
	folder, and copy the dynamic link libraries (e.g. xerces-c_3_1_1.dll)
	in your netbee/bin folder. Please note that Xerces < 3.0.0 should work,
	although you may have to change some makefiles.

* WinPcap Binaries (http://www.winpcap.org/install/default.htm)
	While WinPcap binaries are not needed for comping the NetBee library,
	they are needed when you will execute any software based on NetBee.
	WinPcap (version >= 4.1) must be downloaded from the WinPcap website
	and installed according to the accompanying instructions.

* WinPcap Developer's Pack (http://www.winpcap.org/devel.htm)
	This package is needed to compile the NetBee library, as the WinPcap
	binaries (the previous item) do not include the header files needed
	to compile the NetBee library.
	You have to extract all the content of the WinPcap Developer's pack 
	(e.g. WpdPack_4_1.zip) in the WpdPack folder.
	The Developer's pack myst have the same version of WinPcap.


1.3 Compiling NetBee
--------------------
You are now ready to compile NetBee on Windows.
In order to accomplish this task, you need to complete the following steps:

1) 	Prepare your environment.
	The first step is to add the CygWin and Cmake binary folders to your 
	executable path. You can either:
	
		* 	add these folders to your default path (start menu - 
			control panel - system - advanced - environment variables - 
			system variables - path)

		*	(or) add these folders to Visual Studio executable path 
			(Tools - Options - Project and Solutions - VC++ Directories)
			(please note that this option has been deprecated in Visual 
			Studio 2010, so better to go with the previous option)

2)	Create the project files.
	Then, you need to open a command shell, move into the NetBee root 
	folder, and type the command 

		cmake . (i.e., 'cmake [space] [dot]')

	in the following folders:
	
		* src
		* samples
		* tools

	Cmake will create the building environment for your system, and you 
	will find a Visual Studio solution file in each one of these folders.

3)	Compile all the projects.
	To compile everything, you need just to open each solution that has been
	created in the previous step (they should be called "makeallXXX.sln") 
	with Microsoft Visual Studio.
	Then, from Visual Studio, you need to select the "Build" menu, then the 
	"Rebuild Solution" command.	The compilation should go smoothly, and you 
	will end up having all the binaries in the correct place.
	Please compile the NetBee library in the same order used to create the 
	makefiles (i.e., sources first, then samples and tools).

4)	Download a valid NetPDL database.	
	You need to download a valid NetPDL protocol database from the 
	http://www.nbee.org website for having NetBee to work.
	So, move into the bin folder and launch the "downloadnetpdldb" 
	executable (which has been created in the previous compilation step).
	This tool will connect to the Internet, download the most updated NetPDL
	database, and save it in the current folder.
	Alternatively, you can use the NetPDL database provided with the netbee
	library. Details on how to use this database can be found in the file
	"languages/misc/netpdl_creator_readme.txt".

5)	Creating Docs and Source Pack.
	At this point you may need to create the documentation and such, e.g.,
	in order to have a look at the documentation of the classes implemented
	by NetBee.
	For this, a set of bash scripts files are located in the root folder;
	please refer to the information contained in these files for compiling 
	the documentation and creating the source pack.
	On Windows, you can launch those scripts from a CygWin shell.




2. Compiling NetBee on Unix-like systems
========================================

Currently we compiled NetBee on Linux Ubuntu 12.04, Mac OS X Lion and 
FreeBSD 8.3.
Since NetBee is cross platform, we do not expect problems in compiling the 
library on other platforms.

**Required tools**

* Cmake	(www.cmake.org)
	The building process is managed through CMake. Therefore you need this tool in order to create 
	the project files. Please install this package using the tools provided by your Unix 
	distribution.
	Note: version 2.6 is required. You can check the version installed in your system by issuing the 
	command
	
		# cmake --version
		
	from your favourite shell. If your distribution does not provide the correct version through the 
	package manager, please download the tarball from the Cmake site and install it manually. 
	On Ubuntu Linux you can use apt-get:
		
		# sudo apt-get install cmake
	
	On Mac OS X you can get the Universal .dmg package from the Cmake site (www.cmake.org), or use 
	the Darwin Ports (http://darwinports.com/) tool:
		
		# sudo port install cmake

* GNU GCC	
	The compiler, of course. 
	Note: the NetBee build system has been tested with gcc version 4.1.2 (on Ubuntu Linux 7.04) and 
	with gcc version 4.0.1 (on Intel Mac OS X 10.4 Tiger)
	If missing on your Ubuntu Linux you can use apt-get:

		# sudo apt-get install g++

* Libpcap	
	The library used to capture traffic from your network interfaces and to parse capture files. 
	Although the binaries should be unstalled by default by the operating system, the system headers 
	may not.
	On Ubuntu Linux you can use apt-get to install the appropriate package (binaries and development
        files):
		
		# sudo apt-get install libpcap libpcap-dev
	
	On Mac OS X:
		
		# sudo port install libpcap
	
	Please note that latest netbee revisions require libpcap 0.9.8 for correctly compiling, so make 
	sure your system is up to date. For instance, on Ubuntu 7.04 the libpcap0.8 package contains the 
	version 0.9.5 of the library, while on Ubuntu 7.10 you will find the version 0.9.7 of libpcap.

* Xerces-C version greater than 3.0	
	This library provides the access to all XML files for parsing.
	In order to get it, you can use the package manager available in your Unix distribution.

	On Ubuntu Linux (version >= 9.04) you can use apt-get:
	
		# sudo apt-get install libxerces-c3.1 libxerces-c-dev
	
	On Mac OS X you can use the Darwin Ports (http://darwinports.com/) tool:
	
		# sudo port install xercesc3

* libpcre	
	This library is required for the regular expression support. Often it is already present on your 
	systen; in case it is missing, you can use the package manager available in your Unix 
	distribution.
	On Ubuntu Linux:
	
		# sudo apt-get install libpcre3 libpcre3-dev
	
	On Mac OS X libpcre should be already installed, anyway you can retrieve it through Darwin Ports
	(http://darwinports.com/):
	
		# sudo port install pcre

* libsqlite3 (optional)	
	This library is optional; it may be required if some compilation flags are turned on in order to 
	dump some data on a SQLite3 database. Often it is already present on your systen; in case it is 
	missing, you can use the package manager available in your Unix distribution.
	On Ubuntu Linux:
	
		# sudo apt-get install libsqlite3 libsqlite3-dev
	
	On Mac OS X you can retrieve it through Darwin Ports (http://darwinports.com/):
	
		# sudo port install sqlite3

* A recent version of flex and bison	
	These tools are needed in order to build the parsers of the several compilers included in the 
	NetBee library. You should install them using the package manager available in your Unix 
	distribution.
	On Ubuntu Linux you can use apt-get:
	
		# sudo apt-get install flex bison
	
	On Mac OS X you can use the Darwin Ports tool (http://darwinports.com/):
	
		# sudo port install flex
		# sudo port install bison
	
	Note: the NetBee build system has been tested with flex version 2.5, and bison version 2.3 on 
	both Ubuntu Linux 7.04 and Intel Mac OS X 10.4 Tiger

**Compiling NetBee**

1)	In order to compile the NetBee library and the tools you need first to create the build system 
	using cmake. So you need to open a shell, move into the "src" subfolder of the NetBee directory 
	and issue the "cmake ." command:
	
		# cd netbee/src
		# cmake .
	
	Then you can build the NetBee library by issuing the make command:
	
		# make
	
	The build process should proceed smoothly, although some warnings may be present (please don't 
	care about them). At the end you should find a set of libraries and of course the NetBee shared 
	library in the netbee/bin folder.

2) 	In order to compile the tools provided with NetBee you should follow the same previous steps into 
	the "netbee/tools" subfolder:
	
		# cd netbee/tools
		# cmake .
	
	Then you can build the tools by issuing the make command:
	
		# make
	
	At the end you should find all the tools (e.g., nbeedump, nbextractor, etc) in the "netbee/bin" 
	folder.

3) 	Then, you can compile all the samples using the same procedure. You have to move into the 
	"netbee/samples" subfolder:

		# cd netbee/samples
		# cmake .

	Then you can build the tools by issuing the make command:

		# make

	At the end you should find all the samples in the netbee/bin folder.

4) 	Finally, you will need to download a valid NetPDL database for protocol definitions. So, move 
	into the "bin" folder and launch the "downloadnetpdldb" executable (which has been compiled in 
	the previous step). It will connect to the Internet, download the most updated NetPDL database, 
	and save it in the current folder.
	Alternatively, you can use the NetPDL database provided with the netbee library. Details on how
	to use this database can be found in the file "languages/misc/netpdl_creator_readme.txt".

5) 	In addition, you may need to create the documentation and such. For this, a set of UNIX script 
	files are located in the root folder; please refer to the information contained in these files 
	for compiling the documentation and creating the source pack.

**Installing NetBee [Optional] **

	To install NetBee, open a shell, move into the netbee folder, and issue the
	"install.sh" command:

		# sudo ./insatall.sh
	
*****************************************************************************************************

***Possible problems***

**Cmake returns the "LINK : fatal error LNK1104: cannot open file 'user32.lib'"**

There is nothing we can do with this error. It is related to Cmake, not to our environment. A 
suggestion can be found here: 
		http://people.mech.kuleuven.be/~tdelaet/bfl_doc/installation_guide/node15.html.

However, what we have notices is that this error is more likely to happen if you have Visual Studio 
C++, instead of Visual Studio .NET 2005. So, in case you do not have Visual .NET, please install that 
compiler and see is now works.

**Cmake returns the "CMAKE_CXX_COMPILER-NOTFOUND error"**

This means that the user has not installed g++ or another C++ compiler that cmake knows about. On an 
Ubuntu system, it is easy to install gcc and not realize g++ did not come bundled with it.

**Some module fails to compile**

This can be due to the impossibility to generate the grammar files with Flex/Bison. In this case 
please make sure that Flex and Bison are properly installed (in Windows, they are part of the CygWin 
package) and are in your path.

**In DEBUG mode, I get tons of strange messages on screen**

This is due to the "libpcre" module, which creates tons of debug messages when compiled in DEBUG mode. 
This issue can be solved by compiling this module in RELEASE mode.

*****************************************************************************************************

***Enabling Native Backends***

The NetVM can geneate native code for several platforms (Intel x86, Intel x64, Cavium Octeon, 
Xelerated X11). The former two are supported through Just-In-Time techniques, while the latters are 
supported through Ahead-Of-Time code generation (i.e., the output must be further processed by the 
native compiler for that platform in order to get the actual code).

The first backend is the most widely tested, while the second is still work in progress. The latters 
are really experimental.

When you want to deal with the generation of native code, you may have to take into account the 
following steps:

	* Enable the compilation of the backend in the makefile: by default, only the NetVM interpreter 
	  is turned on. You can select the additional native backends you want by editing the 
	  "src/nbnetvm/CMakeLists.txt" file and turn the proper option on.
	  
	* Disable the Execution Disable bit on your Intel plaform: on latest operating systems (e.g., 
	  Linux), the Execution Disable bit available on the newest Intel hardware is turned on by 
	  default. This prevents memory pages that are marked as 'data' to be executed. However, this is
	  what we use to do in dynamic code gneration: we create dynamically the code in memory and then 
	  we execute it. The NetBee library on Linux has some additional code that is able to avoid this 
	  problem, but in general other operating systems (e.g. MacOS X) may not be able to execute the 
	  dynamically generated code. In case you experiment a crash each time you try to execute native
	  code (most tools and examples have the '-jit' switch, which turns the execution ot native code 
	  on), please check this on your platform and try disabling the Execute Disable bit in your 
	  operating system.


*****************************************************************************************************

***Preparing the run-time environment (Windows)***

This step is required only if you compile the NetBee library on Windows since you have to add some 
libreries to the NetBee binaries; this step is not required on UNIX since we suppose that those 
libraries are already present in your system.

Please note that headers files for those libraries are included in the NetBee source folder, in the
'contrib' folder.

In Windows, you have to add the following two libraries to the 'bin' folder:

	* pcre: library required to parse and execute regular expressions

	* sqlite3: library that is optionally required by some tools in order to dump data on a SQLite 3 
	database. This library is not required in a clean compilation of the NetBee, but it may be 
	requires when some optional compilation flags are turned on.

Binaries can be found in the proper folder under 'contrib'; please copy the required binary files 
into the NetBee 'bin' folder in order to prepare the execution environment for NetBee.

*****************************************************************************************************

***Creating the Developer's Pack***

The NetBee library should not used directly to create other programs. The expected behaviour is the 
following:

	* You compile the NetBee library form sources
	* You create the NetBee Developer's Pack (nbDevPack) out of compiled sources
	* You use the nbDevPack to create your software based on NetBee

This choice allows to create NetBee-based programs without the need to distribute NetBee sources. 
Therfore, the Developer's pack contains all the libraries and header files required to create a 
program based on NetBee. In order to create the NetBee Developer's pack, the NetBee sources contain a 
set of UNIX shell files with the proper set of commands in there. In case you have the NetBee sources 
and you want to create the Developer's Pack, please launch the UNIX shell scripts (also through a 
CygWin shell) and read the instructions contained in there.

Please be careful in creating the Developer's Pack out of the NetBee sources if you want to use the 
NetBee library in third-party programs, and compile your program against the developer's pack instead 
of against NetBee sources directly.
