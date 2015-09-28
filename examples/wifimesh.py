#!/usr/bin/python

"""
This example shows how to work in adhoc mode

sta1 <---> sta2 <---> sta3

"""
from mininet.net import Mininet
from mininet.cli import CLI
from mininet.log import setLogLevel

def topology():
    "Create a network."
    net = Mininet( )
    #wirelessRadios = Number of STAs + APs

    print "*** Creating nodes"
    sta1 = net.addStation( 'sta1' )
    sta2 = net.addStation( 'sta2' )
    sta3 = net.addStation( 'sta3' )

    print "*** Creating links"
    net.addMesh(sta1, 'meshNet') #node, ssid
    net.addMesh(sta2, 'meshNet') #node, ssid
    net.addMesh(sta3, 'meshNet') #node, ssid

    print "*** Starting network"
    net.build()

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()

