# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(makeallsamples)

# nbee samples
#ADD_SUBDIRECTORY(nbee/interfacelist)
#ADD_SUBDIRECTORY(nbee/nbnetvmpcap)
#ADD_SUBDIRECTORY(nbee/nbnetvmpull)
#ADD_SUBDIRECTORY(nbee/nbnetvmpush)
ADD_SUBDIRECTORY(nbee/netpdlutils)
ADD_SUBDIRECTORY(nbee/packetcapture)
ADD_SUBDIRECTORY(nbee/packetdecoder)
ADD_SUBDIRECTORY(nbee/pdmlfieldextractor)
ADD_SUBDIRECTORY(nbee/pdmlreader)
ADD_SUBDIRECTORY(nbee/psmlreader)
ADD_SUBDIRECTORY(nbee/netpdl-download)
ADD_SUBDIRECTORY(nbee/fieldextractor)

# NetVM Samples
ADD_SUBDIRECTORY(nbnetvm/netvmcompiler)
ADD_SUBDIRECTORY(nbnetvm/netvmexec)

# NetPFL compiler samples
ADD_SUBDIRECTORY(nbpflcompiler/filtercompiler)

# NetPDL protocol database samples
ADD_SUBDIRECTORY(nbprotodb/protodbdump)

# Sockutils samples
ADD_SUBDIRECTORY(nbsockutils/sockutilstest)
