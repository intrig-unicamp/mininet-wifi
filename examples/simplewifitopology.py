#!/usr/bin/python

"""
This example shows how to add work with WiFi on Mininet.
"""

from mininet.net import Mininet
from mininet.node import Controller, RemoteController, OVBaseStation
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink

def topology():
    "Create a network."
    net = Mininet( wirelessRadios=4, controller=RemoteController, link=TCLink, switch=OVBaseStation )

    print "*** Creating nodes"
    sta1 = net.addStation( 'sta1' )
    sta2 = net.addStation( 'sta2' )
    bs1 = net.addBaseStation( 'bs1' )

    c0 = net.addController('c0', controller=Controller, port=6633 )

    print "*** Adding Link"
    net.addLink(sta1, bs1)
    net.addLink(sta2, bs1)

    print "*** Starting network"
    net.build()
    c0.start()
    bs1.start( [c0] )


    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()


