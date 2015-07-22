#!/usr/bin/python

"""
Script created by VND - Visual Network Description (SDN version)
"""
from mininet.net import Mininet
from mininet.node import Controller, RemoteController, OVSKernelSwitch, IVSSwitch, UserSwitch
from mininet.link import Link, TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel

def topology():
    "Create a network."

    print "*** Creating nodes"
    net = Mininet( wirelessRadios=3, controller=Controller, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
#    h1 = net.addHost( 'h1')
    h1 = net.addHost( 'h1', mac='00:00:00:00:00:01', ip='10.0.0.1/8' )
    sta2 = net.addStation( 'sta2' )
#    sta2 = net.addStation( 'sta2', mac='00:00:00:00:00:02', ip='192.168.1.1' )
 #   sta3 = net.addStation( 'sta3', mac='00:00:00:00:00:13', ip='192.168.1.2' )
#    sta3 = net.addStation( 'sta3' )
    sta3 = net.addStation( 'sta3', ip='10.0.0.3' )
    ap3 = net.addBaseStation( 'ap3', ssid= 'new-ssid', mode= 'g', channel= '1' )
    c6 = net.addController( 'c6', controller=Controller )

    print "*** Creating links"
    net.addLink(ap3, h1, 1, 0)
    net.addLink(ap3, sta2)
    net.addLink(ap3, sta3)

    print "*** Starting network"
    net.build()
    c6.start()
    ap3.start( [c6] )

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()

