#!/usr/bin/python

"""
Script created by VND - Visual Network Description (SDN version)
"""
from mininet.net import Mininet
from mininet.node import Controller, RemoteController, OVSKernelSwitch, UserSwitch, OVBaseStation
from mininet.cli import CLI
from mininet.link import Intf
from mininet.log import setLogLevel
from mininet.link import Link, TCLink

def topology():
    "Create a network."
    net = Mininet( wirelessRadios=4, controller=RemoteController, link=TCLink, switch=OVBaseStation )
#    net = Mininet( wirelessRadios=4, controller=RemoteController, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    h1 = net.addStation( 'h1' )
    h2 = net.addStation( 'h2' )
    bs3 = net.addBaseStation( 'bs3' )
    c0 = net.addController('c0', controller=Controller, port=6633 )

    net.addLink(h1, bs3)
    net.addLink(h2, bs3)

    print "*** Starting network"
    net.build()
    c0.start()
    bs3.start( [c0] )


    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()


