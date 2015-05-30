#!/usr/bin/python

"""
Working yet..
"""

from mininet.net import Mininet
from mininet.node import  Controller, OVSKernelSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink

"""
         ap1.
         /    .
        /       .
h1----s1        sta1
       \       .
        \    .
        ap2.

"""

def topology():
    "Create a network."
    net = Mininet( wirelessRadios=4, controller=Controller, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    h1 = net.addHost( 'h1', ip='192.168.0.10' )
    ap1 = net.addBaseStation( 'ap1', ssid="ssid_1", mode="g", channel="5" )
    ap2 = net.addBaseStation( 'ap2', ssid="ssid_2" )
    sta1 = net.addStation( 'sta1', ip="192.168.0.100" )
    s1 = net.addSwitch( 's1' )

    c0 = net.addController('c0', controller=Controller, ip='127.0.0.1', port=6633 )

    print "*** Adding Link"
    net.addLink(h1, s1)
    net.addLink(ap1, s1)
    net.addLink(ap2, s1)
    net.addLink(ap1, sta1)
    net.addLink(sta1, ap2)

    print "*** Starting network"
    net.build()
    c0.start()
    s1.start( [c0] )
    ap1.start( [c0] )
    ap2.start( [c0] )
    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
