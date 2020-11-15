FROM debian:buster-slim

WORKDIR /mininet-wifi

# install required packages
RUN apt-get update

RUN apt-get install --yes --no-install-recommends apt-utils \
    git \
    make \
    pkg-config  \
    wireless-tools  \
    libdbus-1-dev  \
    libevent-dev  \
    libnl-3-dev \
    libnl-genl-3-dev  \
    libssl-dev  \
    patch  \
    rfkill  \
    python-setuptools \
    net-tools \
    iputils-ping \
    nano \
    curl \
    iptables \
    python-pip \
    python-dev \
    python-setuptools \
    build-essential \
    aptitude \
    openssh-client \
    sudo

COPY . .

RUN util/install.sh -Wlnfv

# in order to copy ssh key later
RUN mkdir ~/.ssh

ENTRYPOINT ["/mininet-wifi/util/docker/entrypoint.sh"]
