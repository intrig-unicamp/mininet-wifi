#!/bin/bash

function usage {
echo
echo "Usage: octeonfilter.sh [options]"
echo "Compiles a filter in netpdf syntax into an octeon executable"
echo
echo "Options:"
echo "-n,--netbee-root     root folder of the netbee library [default ../../../]"
echo "-f,--f-compiler      filtercompiler location [default \$nbeeroot/bin/filtercompiler]"
echo "-c,--c-compiler      netvmcompiler location [default \$nbeeroot/bin/netvmcompiler]"
echo "-p,--netpdl          netpdl file location [default \$nbeeroot/bin/netpdl-min.xml]"
echo "-F,--filter          netpfl filter [default 'ip']"
echo "-o,--netil-file      output netil code filename [filter.netil]"
echo "-P,--octeon-prefix   prefix for octeon assembler files [octeonfilter]"
echo "-C,--c-flags         flags to pass to the mips gcc [-g -O0]"
echo "-h,--help            print this message and exit"
echo
}


# default options
NETBEE_ROOT=../../..
FILTERCOMPILER=$NETBEE_ROOT/bin/filtercompiler
NETVMCOMPILER=$NETBEE_ROOT/bin/netvmcompiler
NETPDL=$NETBEE_ROOT/bin/netpdl-min.xml
NETPFLFILTER='ip'
NETIL_FILE=filter.netil
OCTEON_PREFIX=octeonfilter
OCTEON_CFLAGS="-g -O0"

# parse options
TEMP=$(getopt -o n:f:c:F:p:o:P:C:h \
-l netbee-root:,f-compiler:,n-compiler:,netpdl:,netil-file:,octeon-prefix:,c-flags:,help \
-n $0 -- "$@")

if [ $? != 0 ]; then
	usage
	exit 1
fi

if echo "$TEMP" | grep -- -h || echo "$TEMP" | grep -- -help; then
	usage
	exit 0
fi

eval set -- "$TEMP"

while true; do
	case "$1" in
         -n|--netbee-root) NETBEE_ROOT=$2; shift 2;;
         -f|--f-compiler) FILTERCOMPILER=$2; shift 2;;
         -c|--c-compiler) NETVMCOMPILER=$2; shift 2;;
         -F|--filter) NETPFLFILTER=$2; shift 2;;
         -p|--netpdl) NETPDL=$2; shift 2;;
         -o|--netil-file) NETIL_FILE=$2; shift 2;;
         -P|--octeon-prefix) OCTEON_PREFIX=$2; shift 2;;
         -C|--c-flags) OCTEON_CFLAGS-$2; shift 2;;
		 --) shift; break;;
		 *) echo "Error"; usage; exit 1;;
    esac
done

if [ ! -z "$@" ]; then
    NETPFLFILTER="$@"
fi

# Check for Octeon SDK environment variables
if [ ! $OCTEON_ROOT ]; then
	echo Error: OCTEON environment variables not defined.
	echo Please note that you need those variables also when building the NetVM
	echo library, otherwise the static library libnetvm.a is not created.
	echo 
	exit 1
fi

# Check for existing tools
if [ ! -x $FILTERCOMPILER ]; then
	echo Did you build NetBee samples?
	echo You must build filtercompiler first.
	exit 1
fi

if [ ! -x $NETVMCOMPILER ]; then
	echo Did you build NetBee samples?
	echo You must build netvmcompiler first.
	exit 1
fi

# Get the backendID for one of the octeon backends (the asm or the C one)
# and returns the first one available
OCTEON_BACKEND=`$NETVMCOMPILER -backends | awk '/octeon/ {print $1}' | awk 'NR==1'`
if [ -z $OCTEON_BACKEND ]; then
    echo Did you build the octeon backend in the libnbnetvm?
    echo You need to enable it before running this sample.
    exit 1
fi

# Clean
rm -rf *.push.s *.init.s octeon_precompile.c octeon_precompile.h

# Compile filter
if ! $FILTERCOMPILER -o $NETIL_FILE -netpdl $NETPDL "$NETPFLFILTER"; then
	echo Error: unable to compile netpfl filter to netil.
	exit 1
fi

# Compile netil
if ! $NETVMCOMPILER -b $OCTEON_BACKEND -p $OCTEON_PREFIX $NETIL_FILE; then
	echo Error: unable to compile netil to octeon native code.
	exit 1
fi


# Build Octeon executable
echo "Creating the final executable"
mips64-octeon-linux-gnu-gcc $OCTEON_CFLAGS \
	octeon_appmain.c octeon_precompile.c *.push.s *.init.s \
	-L$NETBEE_ROOT/lib/ -lnbnetvm -lcvmx \
	-I$OCTEON_ROOT/target/include/ \
	-I./config/ \
	-I$NETBEE_ROOT/include/ \
	-I$NETBEE_ROOT/src/nbnetvm/arch/octeon/config/ \
	-I$NETBEE_ROOT/src/nbnetvm/arch/octeon/coprocessors  \
	-mabi=64 -lrt -static -Wl,-T,$OCTEON_ROOT/target/lib/cvmx-shared-linux.ld \
	-o $OCTEON_PREFIX

echo
echo "Octeon executable is now created in file '" $OCTEON_PREFIX "'"
echo

