dependencies(){
    echo "Installing Dependencies"

    sudo apt-get install -y cmake g++ git automake libtool libgc-dev bison flex libfl-dev libgmp-dev libboost-dev libboost-iostreams-dev libboost-graph-dev llvm pkg-config python3 python3-scapy python3-ply tcpdump graphviz golang libpcre3-dev libpcre3 curl mininet lsb-release
    #python3-ipaddr  now part of python3 as ipaddress
    sudo apt-get install -y texlive-full doxygen


    echo "Install basic dependencies"
    git clone https://github.com/p4lang/behavioral-model.git || echo "Repository already cloned."
    cd behavioral-model
    ./install_deps.sh
    cd ..

    echo "deb http://deb.debian.org/debian bookworm-backports main" | sudo tee /etc/apt/sources.list.d/bookworm-backports.list
    sudo apt update
}

install_protobuf(){
    apt install -y protobuf-compiler
    protoc --version
}

install_libyang(){
    git clone https://github.com/CESNET/libyang.git || echo "Repository already cloned."
    cd libyang
    git checkout v1.0.184
    mkdir build
    cd build
    cmake ..
    make
    sudo make install
    sudo ldconfig
    cd ../..
}

install_sysrepo(){
    git clone https://github.com/sysrepo/sysrepo.git || echo "Repository already cloned."
    cd sysrepo
    git checkout v1.4.70
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=Off -DCALL_TARGET_BINS_DIRECTLY=Off ..
    make
    sudo make install
    sudo ldconfig
    cd ../..
}

install_libgrpc++1() {
    wget http://ftp.es.debian.org/debian/pool/main/p/protobuf/libprotobuf23_3.12.4-1+deb11u1_amd64.deb    #Its for debian 11 but it works for debian 12
    sudo apt -y install ./libprotobuf23_3.12.4-1+deb11u1_amd64.deb 

    wget http://ftp.es.debian.org/debian/pool/main/a/abseil/libabsl20200923_0~20200923.3-2_amd64.deb
    sudo apt -y install ./libabsl20200923_0~20200923.3-2_amd64.deb

    wget http://security.debian.org/debian-security/pool/updates/main/o/openssl/libssl1.1_1.1.1w-0+deb11u2_amd64.deb
    sudo apt -y install ./libssl1.1_1.1.1w-0+deb11u2_amd64.deb

    wget http://ftp.es.debian.org/debian/pool/main/g/grpc/libgrpc10_1.30.2-3_amd64.deb
    sudo apt -y install ./libgrpc10_1.30.2-3_amd64.deb

    wget http://ftp.de.debian.org/debian/pool/main/g/grpc/libgrpc++1_1.30.2-3_amd64.deb
    sudo apt -y install ./libgrpc++1_1.30.2-3_amd64.deb
}

install_grpc_extras(){                      #To install PI
    sudo apt install -y libjudy-dev           
    sudo apt install -y libgrpc-dev          
    sudo apt install -y libgrpc++-dev      
    sudo apt install -y protobuf-compiler-grpc
}

install_PI(){
    git clone https://github.com/p4lang/PI
    cd PI
    git submodule update --init --recursive

    ./autogen.sh
    ./configure --with-proto
    make
    make check
    make install

    sudo ldconfig
    cd ..
}

install_behavioral_model(){
    #From Source, alrady downloaded: https://github.com/p4lang/behavioral-model.git

    cd behavioral-model
    sudo apt-get install -y automake cmake \
    libpcap-dev libboost-test-dev libboost-program-options-dev \
    libboost-system-dev libboost-filesystem-dev libboost-thread-dev \
    libevent-dev libssl-dev
    sudo ./install_deps.sh

    ./autogen.sh
    ./configure --enable-debugger --with-pi
    make
    sudo make install  # if you need to install bmv2

    sudo ldconfig
    make check

    cd ..
}

install_p4c(){
    #From source alternative
    git clone --recursive https://github.com/p4lang/p4c.git || echo "Repository already cloned."

    #dependencies
    sudo apt-get install cmake g++ git automake libtool libgc-dev bison flex \
    libfl-dev libboost-dev libboost-iostreams-dev \
    libboost-graph-dev llvm pkg-config python3 python3-pip \
    tcpdump

    cd p4c
    pip3 install --user -r requirements.txt
    mkdir build
    cd build
    cmake ..             #Lots of <optional arguments>
    make -j4
    #make -j4 check      #failed test: 2802 - ubpf/testdata/p4_16_samples/action_call_ubpf.p4 (Failed)

    sudo make install    #Optional

    cd ..
}


set -e  # Stop the script on any error
echo "127.0.0.1 mininet-wifi" >> /etc/hosts             #To enable the usage of sudo
echo "Starting setup"

dependencies
install_protobuf

sudo pip install grpcio
sudo pip install psutil

install_libyang
install_sysrepo

install_libgrpc++1
install_grpc_extras
install_PI

#Sometimes the script fails to install the next dependencies, but the process can be continued by running them manually and directly in the terminal
install_behavioral_model
install_p4c

sudo service openvswitch-switch start               #enable openvswitch
sudo apt install iputils-ping                       #install ping

echo "Setup complete"