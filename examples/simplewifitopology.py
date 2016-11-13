#!/usr/bin/python

"""
This example shows how to work with WiFi on Mininet.
"""

from mininet.net import Mininet
from mininet.node import  Controller, OVSKernelSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink

def topology():
    "Create a network."
    net = Mininet( controller=Controller, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    sta1 = net.addStation( 'sta1' )
    sta2 = net.addStation( 'sta2' )
    ap1 = net.addAccessPoint( 'ap1', ssid="simplewifi", mode="g", channel="5" )
    c0 = net.addController('c0', controller=Controller, ip='127.0.0.1', port=6633 )

    print "*** Associating Stations"
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap1)

    print "*** Starting network"
    net.build()
    c0.start()
    ap1.start( [c0] )

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()


