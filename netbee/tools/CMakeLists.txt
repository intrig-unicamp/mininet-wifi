# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.7)

PROJECT(makealltools)

IF(CMAKE_COMPILER_IS_GNUCXX)
        ADD_DEFINITIONS(-Wall -Wno-write-strings)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)
IF(CMAKE_COMPILER_IS_GNUCC)
        ADD_DEFINITIONS(-Wall -Wno-write-strings)
ENDIF(CMAKE_COMPILER_IS_GNUCC)

SET (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

##### Paper testing (e.g., pFSA - ToN)
OPTION(
  ENABLE_TEST_REPORTS
  "Enable various test reports for paper testing (e.g., pFSA - ToN)"
  OFF
)

IF(ENABLE_TEST_REPORTS)
#  SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ltcmalloc") Ivano: I commented this row because it did not compile
  ADD_DEFINITIONS(-DENABLE_TEST_REPORTS)
ENDIF(ENABLE_TEST_REPORTS)

#########################################

##### Code profiling by Google
OPTION(
  ENABLE_GOOGLE_CPU_PROFILER
  "Link against the Google Cpu Profiler performance tool library (it must be installed and available to the linker)"
  OFF
)

IF(ENABLE_GOOGLE_CPU_PROFILER)
  SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lprofiler")
ENDIF(ENABLE_GOOGLE_CPU_PROFILER)
#########################################

ADD_SUBDIRECTORY(nbeedump)
ADD_SUBDIRECTORY(nbsvchelper)
ADD_SUBDIRECTORY(nbsvcinstaller)
ADD_SUBDIRECTORY(nbextractor)

