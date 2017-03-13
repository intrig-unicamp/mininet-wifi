#!/usr/bin/python

"""
This example shows how to create wireless link between two APs.
"""

from mininet.net import Mininet
from mininet.node import Controller, OVSKernelAP
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink

def topology():
    "Create a network."
    net = Mininet( controller=Controller, link=TCLink, accessPoint=OVSKernelAP )

    print "*** Creating nodes"
    h1 = net.addHost('h1', mac='00:00:00:00:00:11')
    h2 = net.addHost('h2', mac='00:00:00:00:00:12')
    ap1 = net.addWirelessMeshAP( 'ap1' )
    ap2 = net.addWirelessMeshAP( 'ap2' )
    c0 = net.addController('c0', controller=Controller, ip='127.0.0.1', port=6653 )

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Associating Stations"
    net.addLink(h1, ap1)
    net.addLink(h2, ap2)
    net.addMesh(ap1, ssid='mesh-ssid')
    net.addMesh(ap2, ssid='mesh-ssid')

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
