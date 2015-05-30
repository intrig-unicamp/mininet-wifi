#!/usr/bin/python

"""
This example shows how work with wireless and wired media
"""

from mininet.net import Mininet
from mininet.node import  Controller, OVSKernelSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink

def topology():
    "Create a network."
    net = Mininet( wirelessRadios=3, controller=Controller, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    ap1 = net.addBaseStation( 'ap1', ssid="simplewifi", mode="g", channel="5" )
    sta1 = net.addStation( 'sta1', ip='192.168.0.1/24' )
    sta2 = net.addStation( 'sta2', ip='192.168.0.2/24' )
    h3 = net.addHost( 'h3', ip='192.168.0.3/24' )
    h4 = net.addHost( 'h4', ip='192.168.0.4/24' )

    c0 = net.addController('c0', controller=Controller, ip='127.0.0.1', port=6633 )

    print "*** Adding Link"
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap1)
    net.addLink(h3, ap1)
    net.addLink(h4, ap1)

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


