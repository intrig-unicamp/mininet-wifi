#!/usr/bin/env bash

# Mininet install script for Ubuntu (and Debian Wheezy+)
# Brandon Heller (brandonh@stanford.edu)
# Modified by Ramon Fontes (ramonreisfontes@gmail.com)
# Modified by Luca Attanasio and Denis Donadel for the V2G part (2020)

# Fail on error
set -e

# Fail on unset var usage
set -o nounset

# Get directory containing mininet folder
MININET_DIR="$( cd -P "$( dirname "${BASH_SOURCE[0]}" )/../.." && pwd -P )"

# Set up build directory, which by default is the working directory
#  unless the working directory is a subdirectory of mininet,
#  in which case we use the directory containing mininet
BUILD_DIR="$(pwd -P)"
case $BUILD_DIR in
  $MININET_DIR/*) BUILD_DIR=$MININET_DIR;; # currect directory is a subdirectory
  *) BUILD_DIR=$BUILD_DIR;;
esac


# Attempt to identify Linux release

DIST=Unknown
RELEASE=Unknown
CODENAME=Unknown
ARCH=`uname -m`
if [ "$ARCH" = "x86_64" ]; then ARCH="amd64"; fi
if [ "$ARCH" = "i686" ]; then ARCH="i386"; fi

test -e /etc/debian_version && DIST="Debian"
grep Ubuntu /etc/lsb-release &> /dev/null && DIST="Ubuntu"
if [ "$DIST" = "Ubuntu" ] || [ "$DIST" = "Debian" ]; then
    install='sudo apt-get -y install'
    remove='sudo apt-get -y remove'
    pkginst='sudo dpkg -i'
    update='sudo apt-get'
    # Prereqs for this script
    if ! which lsb_release &> /dev/null; then
        $install lsb-release
    fi
fi
test -e /etc/fedora-release && DIST="Fedora"
if [ "$DIST" = "Fedora" ]; then
    install='sudo yum -y install'
    remove='sudo yum -y erase'
    pkginst='sudo rpm -ivh'
    update='sudo yum'
    # Prereqs for this script
    if ! which lsb_release &> /dev/null; then
        $install redhat-lsb-core
    fi
fi

# NOT RECOMMENDED TO INSTALL ANYTHING OTHER THAN: -g
test -e /etc/arch-release && DIST="Arch Linux"
if [ "$DIST" = "Arch Linux" ]; then
    install='sudo pacman -S --noconfirm'
    remove='sudo pacman -Rs --noconfirm'
    update='pacman -Syu'
    # Prereqs for this script
    if ! which lsb_release &> /dev/null; then
		$install lsb-release
    fi
fi

if which lsb_release &> /dev/null; then
    DIST=`lsb_release -is`
    RELEASE=`lsb_release -rs`
    CODENAME=`lsb_release -cs`
fi
echo "Detected Linux distribution: $DIST $RELEASE $CODENAME $ARCH"

# Kernel params

KERNEL_NAME=`uname -r`
KERNEL_HEADERS=kernel-headers-${KERNEL_NAME}

if ! echo $DIST | egrep 'Ubuntu|Debian|Fedora'; then
    echo "Install.sh currently only supports Ubuntu, Debian and Fedora."
    exit 1
fi

# More distribution info
DIST_LC=`echo $DIST | tr [A-Z] [a-z]` # as lower case


# Determine whether version $1 >= version $2
# usage: if version_ge 1.20 1.2.3; then echo "true!"; fi
function version_ge {
    # sort -V sorts by *version number*
    latest=`printf "$1\n$2" | sort -V | tail -1`
    # If $1 is latest version, then $1 >= $2
    [ "$1" == "$latest" ]
}

# Attempt to identify Python version
PYTHON=${PYTHON:-python}
PRINTVERSION='import sys; print(sys.version_info)'
PYTHON_VERSION=unknown
for python in $PYTHON python2 python3; do
    if $python -c "$PRINTVERSION" |& grep 'major=2'; then
        PYTHON=$python; PYTHON_VERSION=2; PYPKG=python
        break
    elif $python -c "$PRINTVERSION" |& grep 'major=3'; then
        PYTHON=$python; PYTHON_VERSION=3; PYPKG=python3
        break
    fi
done
if [ "$PYTHON_VERSION" == unknown ]; then
    echo "Can't find a working python command ('$PYTHON' doesn't work.)"
    echo "You may wish to export PYTHON or install a working 'python'."
    exit 1
fi
echo "Detected Python (${PYTHON}) version ${PYTHON_VERSION}"


DRIVERS_DIR=/lib/modules/${KERNEL_NAME}/kernel/drivers/net

OF13_SWITCH_REV=${OF13_SWITCH_REV:-""}

function kernel {
    echo "Install Mininet-compatible kernel if necessary"
    $update update
    if ! $install linux-image-$KERNEL_NAME; then
        echo "Could not install linux-image-$KERNEL_NAME"
        echo "Skipping - assuming installed kernel is OK."
    fi
}

# Install Mininet deps
function mn_deps {
    echo "Installing Mininet dependencies"
    if [ "$DIST" = "Fedora" -o "$DIST" = "RedHatEnterpriseServer" ]; then
        $install gcc make socat psmisc xterm openssh-clients iperf \
            iproute telnet python-setuptools libcgroup-tools \
            ethtool help2man pyflakes pylint python-pep8 python-pexpect
    elif [ "$DIST" = "SUSE LINUX"  ]; then
        $install gcc make socat psmisc xterm openssh iperf \
          iproute telnet ${PYPKG}-setuptools libcgroup-tools \
          ethtool help2man ${PYPKG}-pyflakes python3-pylint \
                            python-pep8 ${PYPKG}-pexpect ${PYPKG}-tk
    else  # Debian/Ubuntu
        $install gcc make socat psmisc xterm ssh iperf telnet \
                 ethtool help2man pep8 \
                 ${PYPKG}-setuptools ${PYPKG}-pexpect ${PYPKG}-tk

        if [ "$PYTHON_VERSION" == 3 ]; then
            sudo pip3 install --upgrade pip
            sudo pip3 install --upgrade pyflakes
            sudo pip3 install --upgrade pylint
        else
            sudo pip install --upgrade pip
            sudo pip install --upgrade pyflakes
            sudo pip install --upgrade pylint
            sudo pip install matplotlib==2.1.1 --ignore-installed six
        fi


        $install iproute2 || $install iproute
        $install cgroup-tools || $install cgroup-bin
    fi

    echo "Installing Mininet core"
    pushd $MININET_DIR/miniV2G
    if [ -d mininet ]; then
      echo "Removing mininet dir..."
      rm -r mininet
    fi

    sudo git clone --depth=1 https://github.com/mininet/mininet.git
    pushd $MININET_DIR/miniV2G/mininet
    sudo PYTHON=${PYTHON} make install
    popd
    echo "Installing MiniV2G e Mininet-WiFi core"
    pushd $MININET_DIR/miniV2G
    sudo PYTHON=${PYTHON} make install
    popd
}

# Install P4 deps
function p4_deps {
    echo "Installing P4 dependencies"
    pushd $BUILD_DIR/miniV2G/
    P4_DIR="p4-dependencies"
    [[ -d $P4_DIR ]] && echo $P4_DIR already exists, aborting && exit
    mkdir $P4_DIR
    pushd $BUILD_DIR/miniV2G/p4-dependencies
    git clone https://github.com/jafingerhut/p4-guide
    pushd $BUILD_DIR/miniV2G/p4-dependencies/p4-guide

    if [ "$DIST" = "Ubuntu" ] && [ "$RELEASE" = "20.04" ]; then
        git reset --hard 1fa500a
        patch -p0 < $MININET_DIR/miniV2G/util/p4-patches/p4-guide-v3-without-mininet.patch
        sudo ./bin/install-p4dev-v3.sh |& tee log.txt
    else
        git reset --hard ef0f4e1
        patch -p0 < $MININET_DIR/miniV2G/util/p4-patches/p4-guide-without-mininet.patch
        sudo ./bin/install-p4dev-v2.sh |& tee log.txt
    fi
}

# Install miniV2G deps
function wifi_deps {
    echo "Installing MiniV2G dependencies"
    $install wireless-tools rfkill ${PYPKG}-numpy pkg-config \
             libnl-3-dev libnl-genl-3-dev libssl-dev make libevent-dev patch \
             libdbus-1-dev ${PYPKG}-psutil ${PYPKG}-pip

    if [ "$DIST" = "Ubuntu" ] && [ "$RELEASE" = "14.04" ]; then
        sudo pip install --upgrade pip
        sudo pip install matplotlib==2.1.1 --ignore-installed six
    else
        if [ "$PYTHON_VERSION" == 3 ]; then
            $install python3-matplotlib
        else
            sudo pip install --upgrade pip
            sudo pip install matplotlib==2.1.1 --ignore-installed six
        fi
        $install net-tools
    fi

    pushd $MININET_DIR/miniV2G
    git submodule update --init --recursive
    pushd $MININET_DIR/miniV2G/hostap
    if [ "$DIST" = "Ubuntu" ] && [ "$RELEASE" =  "14.04" ]; then
        git reset --hard 2c129a1
        patch -p0 < $MININET_DIR/miniV2G/util/hostap-patches/config-1404.patch
    else
        patch -p0 < $MININET_DIR/miniV2G/util/hostap-patches/config.patch
    fi
    pushd $MININET_DIR/miniV2G/hostap/hostapd
    cp defconfig .config
    sudo make && make install
    pushd $MININET_DIR/miniV2G/hostap/wpa_supplicant
    cp defconfig .config
    sudo make && make install
    pushd $MININET_DIR/miniV2G/
    if [ -d iw ]; then
      echo "Removing iw..."
      rm -r iw
    fi
    git clone --depth=1 https://git.kernel.org/pub/scm/linux/kernel/git/jberg/iw.git
    pushd $MININET_DIR/miniV2G/iw
    sudo make && make install
    cd $BUILD_DIR
    if [ -d mac80211_hwsim_mgmt ]; then
      echo "Removing mac80211_hwsim_mgmt..."
      rm -r mac80211_hwsim_mgmt
    fi
    git clone --depth=1 https://github.com/ramonfontes/mac80211_hwsim_mgmt.git
    pushd $BUILD_DIR/mac80211_hwsim_mgmt
    sudo make install
}

function babeld {
    echo "Installing babeld..."

    cd $BUILD_DIR/miniV2G
    if [ -d babeld ]; then
          echo "Removing babeld..."
          rm -r babeld
        fi
    git clone --depth=1 https://github.com/jech/babeld
    cd $BUILD_DIR/miniV2G/babeld
    make
    sudo make install
}

function olsrd {
    echo "Installing olsrd..."
    $install bison flex
    
    cd $BUILD_DIR/miniV2G
    if [ -d olsrd ]; then
          echo "Removing olsrd..."
          rm -r olsrd
        fi
    git clone --depth=1 https://github.com/OLSR/olsrd
    cd $BUILD_DIR/miniV2G/olsrd
    make
    sudo make install
}

function olsrdv2 {
    echo "Installing olsrdv2..."
    $install liblua5.1-0-dev libjson-c-dev

    cd $BUILD_DIR/miniV2G
    if [ -d libubox ]; then
          echo "Removing libubox..."
          rm -r libubox
    fi
    git clone --depth=1 git://git.openwrt.org/project/libubox.git
    cd $BUILD_DIR/miniV2G/libubox
    mkdir build
    cd build
    cmake ..
    make
    sudo make install

    cd $BUILD_DIR/miniV2G
    if [ -d ubus ]; then
          echo "Removing ubus..."
          rm -r ubus
    fi
    git clone --depth=1 git://git.openwrt.org/project/ubus.git
    cd $BUILD_DIR/miniV2G/ubus
    mkdir build
    cd build
    cmake ..
    make
    sudo make install

    cd $BUILD_DIR/miniV2G
    if [ -d uci ]; then
          echo "Removing uci..."
          rm -r uci
    fi
    git clone --depth=1 git://git.openwrt.org/project/uci.git
    cd $BUILD_DIR/miniV2G/uci
    mkdir build
    cd build
    cmake ..
    make
    sudo make install

    cd $BUILD_DIR/miniV2G
    if [ -d OONF ]; then
          echo "Removing olsrdv2..."
          rm -r OONF
    fi
    git clone https://github.com/OLSR/OONF
    cd $BUILD_DIR/miniV2G/OONF/build
    cmake ..
    make
    sudo make install
}

function batman {
    echo "Installing B.A.T.M.A.N..."
    echo "Installing batmand..."
    cd $BUILD_DIR/miniV2G
    if [ -d batmand ]; then
          echo "Removing batmand..."
          rm -r batmand
    fi
    git clone https://github.com/open-mesh-mirror/batmand --depth=1
    cd batmand
    make
    sudo make install

    echo "Installing batman-adv..."
    cd $BUILD_DIR/miniV2G
    if [ -d batman-adv ]; then
          echo "Removing batman-adv..."
          rm -r batman-adv
    fi
    git clone https://github.com/open-mesh-mirror/batman-adv --depth=1
    cd batman-adv
    make
    sudo make install

    echo "Installing batctl..."
    cd $BUILD_DIR/miniV2G
    if [ -d batctl ]; then
          echo "Removing batctl..."
          rm -r batctl
    fi
    git clone https://github.com/open-mesh-mirror/batctl --depth=1
    cd batctl
    make
    sudo make install
}


# Install Mininet developer dependencies
function mn_dev {
    echo "Installing Mininet developer dependencies"
    $install doxygen doxypy texlive-fonts-recommended
    if ! $install doxygen-latex; then
        echo "doxygen-latex not needed"
    fi
}

# The following will cause a full OF install, covering:
# -user switch
# The instructions below are an abbreviated version from
# http://www.openflowswitch.org/wk/index.php/Debian_Install
function of {
    echo "Installing OpenFlow reference implementation..."
    cd $BUILD_DIR
    $install autoconf automake libtool make gcc patch
    if [ "$DIST" = "Fedora" ]; then
        $install git pkgconfig glibc-devel
    else
        $install git-core autotools-dev pkg-config libc6-dev
    fi
    git clone --depth=1 https://github.com/mininet/openflow
    cd $BUILD_DIR/openflow

    # Patch controller to handle more than 16 switches
    patch -p1 < $MININET_DIR/miniV2G/util/openflow-patches/controller.patch

    # Resume the install:
    ./boot.sh
    ./configure
    make
    sudo make install
    cd $BUILD_DIR
}

function of13 {
    echo "Installing OpenFlow 1.3 soft switch implementation..."
    cd $BUILD_DIR/
    $install  git-core autoconf automake autotools-dev pkg-config \
        make gcc g++ libtool libc6-dev cmake libpcap-dev libxerces-c3-dev  \
        unzip libpcre3-dev flex bison libboost-dev

    if [ ! -d "ofsoftswitch13" ]; then
        if [[ -n "$OF13_SWITCH_REV" ]]; then
            git clone https://github.com/CPqD/ofsoftswitch13.git
            cd ofsoftswitch13
            git checkout ${OF13_SWITCH_REV}
            cd ..
        else
            git clone --depth=1 https://github.com/CPqD/ofsoftswitch13.git
        fi
    fi

    # Install netbee
    NBEEDIR="netbee"
    git clone https://github.com/netgroup-polito/netbee.git
    cd ${NBEEDIR}/src
    cmake .
    make

    cd $BUILD_DIR/
    sudo cp ${NBEEDIR}/bin/libn*.so /usr/local/lib
    sudo ldconfig
    sudo cp -R ${NBEEDIR}/include/ /usr/

    # Resume the install:
    cd $BUILD_DIR/ofsoftswitch13
    ./boot.sh
    ./configure
    make
    sudo make install
    cd $BUILD_DIR
}

# Install Open vSwitch

function ovs {
    echo "Installing Open vSwitch..."

    if [ "$DIST" = "Fedora" -o "$DIST" = "RedHatEnterpriseServer" ]; then
        $install openvswitch openvswitch-controller
        return
    fi

    if [ "$DIST" = "Ubuntu" ] && ! version_ge $RELEASE 14.04; then
        # Older Ubuntu versions need openvswitch-datapath/-dkms
        # Manually installing openvswitch-datapath may be necessary
        # for manually built kernel .debs using Debian's defective kernel
        # packaging, which doesn't yield usable headers.
        if ! dpkg --get-selections | grep openvswitch-datapath; then
            # If you've already installed a datapath, assume you
            # know what you're doing and don't need dkms datapath.
            # Otherwise, install it.
            $install openvswitch-datapath-dkms
        fi
    fi

    $install openvswitch-switch
    OVSC=""
    if $install openvswitch-controller; then
        OVSC="openvswitch-controller"
    else
        echo "Attempting to install openvswitch-testcontroller"
        if $install openvswitch-testcontroller; then
            OVSC="openvswitch-testcontroller"
        else
            echo "Failed - skipping openvswitch-testcontroller"
        fi
    fi
    if [ "$OVSC" ]; then
        # Switch can run on its own, but
        # Mininet should control the controller
        # This appears to only be an issue on Ubuntu/Debian
        if sudo service $OVSC stop; then
            echo "Stopped running controller"
        fi
        if [ -e /etc/init.d/$OVSC ]; then
            sudo update-rc.d $OVSC disable
        fi
    fi

    # openvswitch-common requires python2 on Ubuntu < 20.04
    # if we install python2 and the current python version is 3
    # python2 will replace python3
    # that's why we try to revert python to python3
    if [ "$PYTHON_VERSION" == 3 ] && version_ge "$RELEASE" 20.04; then
        sudo rm /usr/bin/python
        sudo ln -s /usr/bin/python3 /usr/bin/python
    fi
}

function remove_ovs {
    pkgs=`dpkg --get-selections | grep openvswitch | awk '{ print $1;}'`
    echo "Removing existing Open vSwitch packages:"
    echo $pkgs
    if ! $remove $pkgs; then
        echo "Not all packages removed correctly"
    fi
    # For some reason this doesn't happen
    if scripts=`ls /etc/init.d/*openvswitch* 2>/dev/null`; then
        echo $scripts
        for s in $scripts; do
            s=$(basename $s)
            echo SCRIPT $s
            sudo service $s stop
            sudo rm -f /etc/init.d/$s
            sudo update-rc.d -f $s remove
        done
    fi
    echo "Done removing OVS"
}

# Install OFtest
function oftest {
    echo "Installing oftest..."

    # Install deps:
    $install tcpdump python-scapy

    # Install oftest:
    cd $BUILD_DIR/
    git clone https://github.com/floodlight/oftest.git
}

# Install cbench
function cbench {
    echo "Installing cbench..."

    if [ "$DIST" = "Fedora" ]; then
        $install net-snmp-devel libpcap-devel libconfig-devel
    else
        $install libsnmp-dev libpcap-dev libconfig-dev
    fi
    cd $BUILD_DIR/
    git clone --depth=1 https://github.com/mininet/oflops
    cd oflops
    sh boot.sh || true # possible error in autoreconf, so run twice
    sh boot.sh
    ./configure --with-openflow-src-dir=$BUILD_DIR/openflow
    make
    sudo make install || true # make install fails; force past this
}

# Script to copy built OVS kernel module to where modprobe will
# find them automatically.  Removes the need to keep an environment variable
# for insmod usage, and works nicely with multiple kernel versions.
#
# The downside is that after each recompilation of OVS you'll need to
# re-run this script.  If you're using only one kernel version, then it may be
# a good idea to use a symbolic link in place of the copy below.
function modprobe {
    echo "Setting up modprobe for OVS kmod..."
    set +o nounset
    if [ -z "$OVS_KMODS" ]; then
      echo "OVS_KMODS not set. Aborting."
    else
      sudo cp $OVS_KMODS $DRIVERS_DIR
      sudo depmod -a ${KERNEL_NAME}
    fi
    set -o nounset
}

# Script for installing wmediumd from git to /usr/bin/wmediumd
function wmediumd {
    echo "Installing wmediumd sources into $BUILD_DIR/wmediumd"
    cd $BUILD_DIR
    if [ -d wmediumd ]; then
      echo "Removing wmediumd..."
      rm -r wmediumd
    fi
    $install git make libevent-dev libconfig-dev libnl-3-dev libnl-genl-3-dev
    git clone --depth=1 -b mininet-wifi https://github.com/ramonfontes/wmediumd.git
    pushd $BUILD_DIR/wmediumd
    sudo make install
    popd
}


# Script for installing wpan tools
function wpan_tools {
    echo "Installing wpan tools sources"
    cd $BUILD_DIR
    if [ -d wpan-tools ]; then
      echo "Removing wpan-tools..."
      rm -r wpan-tools
    fi
    git clone --depth=1 https://github.com/linux-wpan/wpan-tools
    pushd $BUILD_DIR/wpan-tools
    sudo ./autogen.sh
    sudo ./configure
    sudo make
    sudo make install
    popd
}

function all {
    if [ "$DIST" = "Fedora" ]; then
        printf "\nFedora 18+ support (still work in progress):\n"
        printf " * Fedora 18+ has kernel 3.10 RPMS in the updates repositories\n"
        printf " * Fedora 18+ has openvswitch 1.10 RPMS in the updates repositories\n"
        printf " * the install.sh script options [-bfnpvw] should work.\n"
        printf " * for a basic setup just try:\n"
        printf "       install.sh -fnpv\n\n"
        exit 3
    fi
    echo "Installing all packages except for -eix (doxypy)..."
    kernel
    mn_deps
    # Skip mn_dev (doxypy/texlive/fonts/etc.) because it's huge
    # mn_dev
    of
    ovs
    wifi_deps
    p4_deps
    oftest
    cbench
    wmediumd
    babeld
    olsrd
    olsrdv2
    batman
    wpan_tools
    v2g
    mim
    echo "Enjoy MiniV2Gwifi!"
}

# Restore disk space and remove sensitive files before shipping a VM.
function vm_clean {
    echo "Cleaning VM..."
    sudo apt-get clean
    sudo apt-get autoremove
    sudo rm -rf /tmp/*
    sudo rm -rf openvswitch*.tar.gz

    # Remove sensistive files
    history -c  # note this won't work if you have multiple bash sessions
    rm -f ~/.bash_history  # need to clear in memory and remove on disk
    rm -f ~/.ssh/id_rsa* ~/.ssh/known_hosts
    sudo rm -f ~/.ssh/authorized_keys*

    # Remove Mininet files
    #sudo rm -f /lib/modules/python2.5/site-packages/*
    #sudo rm -f /usr/bin/mnexec

    # Clear optional dev script for SSH keychain load on boot
    rm -f ~/.bash_profile

    # Clear git changes
    git config --global user.name "None"
    git config --global user.email "None"

    # Note: you can shrink the .vmdk in vmware using
    # vmware-vdiskmanager -k *.vmdk
    echo "Zeroing out disk blocks for efficient compaction..."
    time sudo dd if=/dev/zero of=/tmp/zero bs=1M || true
    sync ; sleep 1 ; sync ; sudo rm -f /tmp/zero

}

function v2g {
    echo "Installing v2g..."

    if ! which curl; then
	echo "Installing curl..."
	$install curl
    fi

    # Download latest compiled release available on official repo
    LINK1=$(curl -s https://api.github.com/repos/V2GClarity/RISE-V2G/releases\
	| grep "browser_download_url.*.jar" \
	| grep -Po '(?<="browser_download_url": ")[^"]*' | sed -sn 1p); \
    LINK2=$(curl -s https://api.github.com/repos/V2GClarity/RISE-V2G/releases\
	| grep "browser_download_url.*.jar" \
	| grep -Po '(?<="browser_download_url": ")[^"]*' | sed -sn 2p); \
    echo "Downloading $LINK1 and $LINK2"; (cd $MININET_DIR/miniV2G/util/RiseV2G && sudo curl -L -O $LINK1); (cd $MININET_DIR/miniV2G/util/RiseV2G && sudo curl -L -O $LINK2);

    # Copy latest jar files to local directory
    sudo rm -rf /usr/share/.miniV2G/RiseV2G # remove folder to support updating
    sudo mkdir -p /usr/share/.miniV2G/RiseV2G
    # get latest version
    LATEST_SECC=$(ls $MININET_DIR/miniV2G/util/RiseV2G/*-secc-*.jar | tail -1)
    LATEST_EVCC=$(ls $MININET_DIR/miniV2G/util/RiseV2G/*-evcc-*.jar | tail -1)
    sudo cp $LATEST_SECC /usr/share/.miniV2G/RiseV2G
    sudo cp $LATEST_EVCC /usr/share/.miniV2G/RiseV2G
    sudo cp $MININET_DIR/miniV2G/util/RiseV2G/*.properties /usr/share/.miniV2G/RiseV2G
    # copy files for TLS key generation
    sudo cp -r $MININET_DIR/miniV2G/util/RiseV2G/RISE-V2G-Certificates /usr/share/.miniV2G/RiseV2G

    if ! which java; then
        echo "Installing java..."
        if [ "$DIST" = "Ubuntu" ] || [ "$DIST" = "Debian" ]; then
            $install default-jdk
        # Fedora, Oracle Linux, Red Hat Enterprise Linux, etc.
        elif [ "$DIST" = "Fedora" -o "$DIST" = "RedHatEnterpriseServer" ]; then
            $install java-1.8.0-openjdk
        elif [ "$DIST" = "Arch" ]; then
            $install java-openjdk
        else
            echo "Please, Install Java. It is required to run v2g."
        fi
    fi

    if ! which xhost; then
        echo "Installing xhost..."
        if [ "$DIST" = "Arch" ]; then
            $install xorg-xhost
	elif [ "$DIST" = "Ubuntu" ] || [ "$DIST" = "Debian" ]; then
	    $install x11-xserver-utils
        else
            echo "Please, Install xhost. It is required to run term.py."
        fi
    fi

    if ! which xterm; then
        echo "Installing xterm..."
        $install xterm
    fi
}

function mim {
    echo "Installing Man-in-the-Middle supports for v2g..."

    if ! command -v parasite6 &> /dev/null
    then
      echo "Cloning, making and installing parasite6..."
      $install libpcap-dev libssl-dev libnetfilter-queue-dev
      git clone https://github.com/vanhauser-thc/thc-ipv6
      cd thc-ipv6
      sudo make all
      sudo make install
      cd ..
      echo "Cleaning the mess..."
      rm -rf thc-ipv6
    else
      echo "parasite6 already installed."
    fi

    echo "Downloading V2Gdecoder..."
    (cd $MININET_DIR/miniV2G/util/RiseV2G && sudo curl -L -O https://github.com/FlUxIuS/V2Gdecoder/releases/download/v1/V2Gdecoder.jar)
    sudo mv -f $MININET_DIR/miniV2G/util/RiseV2G/V2Gdecoder.jar /usr/share/.miniV2G/

}

function usage {
    printf '\nUsage: %s [-abBcdEfgGhklmnOpPrStvVxy03]\n\n' $(basename $0) >&2

    printf 'This install script attempts to install useful packages\n' >&2
    printf 'for Mininet. It should (hopefully) work on Ubuntu 11.10+\n' >&2
    printf 'If you run into trouble, try\n' >&2
    printf 'installing one thing at a time, and looking at the \n' >&2
    printf 'specific installation function in this script.\n\n' >&2

    printf 'options:\n' >&2
    printf -- ' -a: (default) install (A)ll packages - good luck!\n' >&2
    printf -- ' -b: install controller (B)enchmark (oflops)\n' >&2
    printf -- ' -B: install B.A.T.M.A.N\n' >&2
    printf -- ' -d: (D)elete some sensitive files from a VM image\n' >&2
    printf -- ' -e: install Mininet d(E)veloper dependencies\n' >&2
    printf -- ' -E: install babeld\n' >&2
    printf -- ' -f: install Open(F)low\n' >&2
    printf -- ' -g: install RiseV2(g) and V2G support\n' >&2
    printf -- ' -G: install RiseV2(g), V2G support and dependencies for MitM\n' >&2
    printf -- ' -h: print this (H)elp message\n' >&2
    printf -- ' -k: install new (K)ernel\n' >&2
    printf -- ' -l: insta(L)l wmediumd\n' >&2
    printf -- ' -m: install Open vSwitch kernel (M)odule from source dir\n' >&2
    printf -- ' -n: install Mini(N)et dependencies + core files\n' >&2
    printf -- ' -o: install olsrdv2\n' >&2
    printf -- ' -O: install olsrd\n' >&2
    printf -- ' -p: install P4 dependencies\n' >&2
    printf -- ' -r: remove existing Open vSwitch packages\n' >&2
    printf -- ' -s <dir>: place dependency (S)ource/build trees in <dir>\n' >&2
    printf -- ' -v: install Open (V)switch\n' >&2
    printf -- ' -V <version>: install a particular version of Open (V)switch on Ubuntu\n' >&2
    printf -- ' -W: install MiniV2G and Mininet-WiFi dependencies\n' >&2
    printf -- ' -0: (default) -0[fx] installs OpenFlow 1.0 versions\n' >&2
    printf -- ' -3: -3[fx] installs OpenFlow 1.3 versions\n' >&2
    printf -- ' -6: install wpan tools\n' >&2
    exit 2
}

OF_VERSION=1.0

if [ $# -eq 0 ]
then
    all
else
    while getopts 'abBdeEfgGhiklmnoOPrSsvWx036' OPTION
    do
      case $OPTION in
      a)    all;;
      b)    cbench;;
      B)    batman;;
      d)    vm_clean;;
      e)    mn_dev;;
      E)    babeld;;
      f)    case $OF_VERSION in
            1.0) of;;
            1.3) of13;;
            *)  echo "Invalid OpenFlow version $OF_VERSION";;
            esac;;
      g)    v2g;;
      G)    v2g
            mim;;
      h)    usage;;
      k)    kernel;;
      l)    wmediumd;;
      m)    modprobe;;
      n)    mn_deps;;
      o)    olsrdv2;;
      O)    olsrd;;
      P)    p4_deps;;
      r)    remove_ovs;;
      s)    mkdir -p $OPTARG; # ensure the directory is created
            BUILD_DIR="$( cd -P "$OPTARG" && pwd )"; # get the full path
            echo "Dependency installation directory: $BUILD_DIR";;
      v)    ovs;;
      W)    wifi_deps;;
      0)    OF_VERSION=1.0;;
      3)    OF_VERSION=1.3;;
      6)    wpan_tools;;
      ?)    usage;;
      esac
    done
    shift $(($OPTIND - 1))
fi
