#!/usr/bin/python

"""
This example shows how to add work with WiFi on Mininet.
"""

from mininet.net import Mininet
from mininet.node import  Controller, OVSKernelSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink

def topology():
    "Create a network."
    net = Mininet( wirelessRadios=6, controller=Controller, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    ap1 = net.addBaseStation( 'ap1', ssid="ssid_1", mode="g", channel="5" )
 #   ap2 = net.addBaseStation( 'ap2', ssid="ssid_2" )
    sta1 = net.addStation( 'sta1', ip="192.168.0.100" )
    h1 = net.addHost( 'h1', ip="192.168.0.1" )
    s1 = net.addSwitch( 's1' )

    c0 = net.addController('c0', controller=Controller, ip='127.0.0.1', port=6633 )

    print "*** Adding Link"
    net.addLink(h1, s1)
    net.addLink(ap1, s1)
#    net.addLink(s1, ap2)
    net.addLink(ap1, sta1)
#    net.addLink(sta1, ap1)
#    net.addLink(ap2, sta1)

    print "*** Starting network"
    net.build()
    c0.start()
    s1.start( [c0] )
    ap1.start( [c0] )
  #  ap2.start( [c0] )
    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
