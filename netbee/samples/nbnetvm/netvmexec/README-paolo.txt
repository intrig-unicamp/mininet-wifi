* Compilation
  just #define MULTICORE. Then you can specify the number of desidered
  cores at runtime with -m flag
* How it works
  If a multicore run is specified, parent process sets up shared
  memory, the mechanism that is used to communicate with children, by
  copying capture file in it and initializing semaphores. Then the
  parent forks and wait for children to complete netvm initialization
  (and JIT compilation). After that sends a signal to children to
  start the processing. Children then read  packet queue in the
  shared memory, print some logs and die.
* added files
** multicore.* 
   Multicore application logic. Shared memory set-up, capture copying,
   children forks and synchronization functions are all here.
** globals.h 
   global variables
** common.h
   I moved some stuff here since they were common to multiple files
** log.h, misc.h, utils.*
   file required by the log mechanism
