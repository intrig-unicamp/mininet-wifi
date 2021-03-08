#!/usr/bin/python

"""
    Really simple and minimal testing module for Man-in-the-Middle inside a v2g communication

    To install RiseV2G and all the necessary for the MitM run: util/install.sh -G.
    It will:
    - Create the directory /usr/share/.miniV2G/RiseV2G and copy in it all the RiseV2G files:
        > RiseV2G jar files;
        > RiseV2G config files;
        > V2GDecoder jar file;
    - Make sure java, xterm, curl, parasite6 is installed and install it otherwise.
"""

from mn_wifi.net import Mininet_wifi
from mininet.node import NullController
from mininet.cli import CLI
from mininet.log import setLogLevel, info

from mn_wifi.v2g import EV, SE, MitMOVSSwitch, MitMNode
from time import sleep

def v2gNet():

    "Create a network without an EV and SE connected via a switch."

    start_on_load = False

    net = Mininet_wifi()

    info( '*** Adding controller\n' )
    net.addController( 'c0', NullController )
    # no controller, the switch will take care

    info( '*** Adding hosts: SE, EV and mim\n' )
    se1 = net.addHost("se1", SE )
    ev1 = net.addHost("ev1", EV )
    mim = net.addHost("mim", MitMNode)

    info( '*** Adding MiM switch\n' )

    # the reason why we used a switch is to create custom flows
    s1 = net.addSwitch( 's1', MitMOVSSwitch)

    info( '*** Creating links\n' )
    net.addLink( se1, s1 )
    net.addLink( ev1, s1 )
    net.addLink( mim, s1 )

    sleep(1)  # IMPORTANT! Give a second to the net to complete the setup (otherwise crashes are possible)

    info( '*** Starting network\n')
    net.start()

    info( '*** Running CLI\n' )
    info( '*** CAREFUL: In the example, with mim.start_server(dos_attack_1=True) the server can perform a DoS attack on EV (protocol version)\n')
    info( '*** CAREFUL: In the example, with mim.start_server(dos_attack_2=True) the server can perform a DoS attack on EV (removing charge options)\n')
    info( '*** For manual usage:\n' )
    info( '     - With `py ev1.charge(in_xterm=True)` the EV will start charging in the linked SE (manual by default).\n' )
    info( '*** Started automatically:\n' )
    info( '     - With `py se1.startCharge()` the SE will wait for charging EVs.\n' )
    info( '     - Inside `xterm mim` the MiM can start the server with `python2 v2g_server_socket.py`. The SE must be started.\n' )

    s1.add_mim_flows(se1, ev1, mim)

    mim.start_decoder(in_xterm=False)
    mim.start_spoof(se1, ev1)
    # this generates an entry in the table of source with macaddr of mim (but we don't care)

    info( '*** Starting charge on the SE.\n' )
    sleep(1)
    net.terms += [ se1.startCharge() ]

    net.terms += [ mim.start_server() ]
    # you could also start the server configured to act as a DoS attack
    # net.terms += [ mim.start_server(dos_attack_1=True) ]
    # net.terms += [ mim.start_server(dos_attack_2=True) ]

    # tested connection (IPv4)
    # # mim: nc -l -p 20000
    # # se1: nc 10.0.0.2 20000

    # tested connection (IPv6)
    # # mim: ncat -6 -l -p 20000
    # # se1: ncat -6 -C ev1ipv6%se1-eth0 20000
    # # ev1: ncat -6 -C se1ipv6%ev1-eth0 20000

    # tested mim (IPv6)
    # # se1: ncat -6 -l -p 20
    # # mim: ncat -6 -l -p 200 -c "ncat -6 se1ipv6%mim-eth0 20"
    # # ev1: ncat -6 se1ipv6%ev1-eth0 200

    if start_on_load == True:
        info( '*** EV is charging.\n' )
        sleep( 1 ) # IMPORTANT! Give a second to the net to complete the setup (otherwise crashes are possible)
        net.terms += [ ev1.charge( in_xterm=True ) ]

    CLI( net )

    info( '*** Stopping network' )
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    v2gNet()
