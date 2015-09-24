#!/usr/bin/python

"""
This example shows how to work with different APs
"""
from mininet.net import Mininet
from mininet.node import  Controller, OVSKernelSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink

def topology():
    "Create a network."
    net = Mininet( controller=Controller, link=TCLink, switch=OVSKernelSwitch )
    #wirelessRadios = Number of STAs + APs

    print "*** Creating nodes"
    sta1 = net.addStation( 'sta1', ip="192.168.0.1" )
    sta2 = net.addStation( 'sta2', ip="192.168.0.2" )
    sta3 = net.addStation( 'sta3', ip="192.168.0.3" )
    sta4 = net.addStation( 'sta4', ip="192.168.0.4" )
    ap1 = net.addBaseStation( 'ap1', ssid="ssid_1", mode="g", channel="1" )
    ap2 = net.addBaseStation( 'ap2', ssid="ssid_2", mode="b", channel="6" )
    c0 = net.addController('c0', controller=Controller, ip='127.0.0.1', port=6633 )

    print "*** Adding Link"
    net.addLink(ap1, ap2) #wired connection
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap1)
    net.addLink(sta3, ap2)
    net.addLink(sta4, ap2)

    print "*** Starting network"
    net.build()
    c0.start()
    ap1.start( [c0] )
    ap2.start( [c0] )

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
