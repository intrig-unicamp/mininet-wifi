#!/usr/bin/python

"""
Script created by VND - Visual Network Description (SDN version)
"""
from mininet.net import Mininet
from mininet.node import Controller, RemoteController, OVSKernelSwitch, UserSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import Link, TCLink

def topology():
    "Create a network."
    net = Mininet( wirelessRadios=3, controller=Controller, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    s1 = net.addBaseStation( 's1', ssid= 'nwsew-ssid', mode= 'g', channel= '1' )
    sta2 = net.addStation( 'sta2' )
    c4 = net.addController( 'c4', controller=Controller, ip='127.0.0.1', port=6633 )

    print "*** Creating links"
    net.addLink(s1, sta2)

    print "*** Starting network"
    net.build()
    c4.start()
    s1.start( [c4] )

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()

