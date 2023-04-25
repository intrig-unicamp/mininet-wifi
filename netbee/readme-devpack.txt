NetBee Developer's Pack Readme
================================================================================

The NetBee Developer's Pack (nbDevPack) contains all the following files:

- 'bin' folder: runtime libraries of the NetBee library (i.e. DLLs), plus all 
  the pre-compiled samples, plus all the NetBee-based tools (such a nbeedump).
  In addition, it includes also some NetPDL files, some with a few protocols,
  other with a more complete set of protocols.

- 'include' folder: all the header files required to create a program based on 
  NetBee. For most of the programs, you can just include "nbee.h", which will 
  include all the remaining headers. If you wanto to link agains some specific 
  module of NetBee, you have to choise the right header file.

- 'lib' folder: all the libraries, which are required to link your program with 
  NetBee.

- 'samples' folder: all the NetBee samples. A set of project files for Microsoft 
  Visual Studio are provided. In any case, the CMakeFiles present in these folder 
  allow you to create the makefiles from scratch. This is the only option you 
  have in case you are working on Unix.

- 'tools' folder: all the tools that comes with NetBee. A set of project files 
  for Microsoft Visual Studio are provided. In any case, the CMakeFiles present
  in these folder allow you to create the makefiles from scratch. This is the 
  only option you have in case you are working on Unix.


In case of trouble, please check at the online documentation.
Then, if the  problem persists, please contact the Netgroup at Politecnico di 
Torino, Italy.
