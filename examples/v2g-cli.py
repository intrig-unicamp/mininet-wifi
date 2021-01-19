#!/usr/bin/python

""" 
    Really simple and minimal testing module for v2g

    To install RiseV2G on your local machine run: util/install.sh -g.
    It will:
    - Create the directory /usr/share/.miniV2G/RiseV2G and copy in it all the RiseV2G files (latest jar releases and two config files);
    - Make sure java, xterm, curl is installed and install it otherwise.
"""

from mn_wifi.net import Mininet_wifi
from mininet.node import Controller
from mininet.cli import CLI
from mininet.log import setLogLevel, info

from mn_wifi.v2g import EV, SE
from time import sleep

def v2gNet():

    "Create a network with a controller, EV and SE connected via a switch."

    start_on_load = False

    net = Mininet_wifi( controller=Controller )

    info( '*** Adding controller\n' )
    net.addController( 'c0' )

    info( '*** Adding hosts: SE and EV\n' )
    se1 = net.addHost("se1", SE )
    ev1 = net.addHost("ev1", EV )

    info( '*** Adding switch\n' )

    s1 = net.addSwitch( 's1' )

    info( '*** Creating links\n' )
    net.addLink( se1, s1 )
    net.addLink( ev1, s1 )

    sleep(1)  # IMPORTANT! Give a second to the net to complete the setup (otherwise crashes are possible)

    info( '*** Starting network\n')
    net.start()

    info( '*** Running CLI\n' )
    info( '*** BASIC USAGE:\n' )
    info( '     - With `py se1.startCharge()` the SE will wait for charging EVs.\n' )
    info( '     - With `py ev1.charge(in_xterm=True)` the EV will start charging in the linked SE.\n' )

    # Activate TLS
    # ev1.setTLS(True, True, se1)

    if start_on_load == True:
        info( '*** Starting charge on the SE.\n' )
        # TODO: move the sleep command to the SE and EV charge.
        sleep( 1 ) # IMPORTANT! Give a second to the net to complete the setup (otherwise crashes are possible)
        net.terms += [ se1.startCharge() ]
        info( '*** EV is charging.\n' )
        sleep( 1 )
        net.terms += [ ev1.charge( in_xterm=True ) ]
    
    CLI( net )

    info( '*** Stopping network' )
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    v2gNet()
