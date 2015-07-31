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
    net = Mininet( wirelessRadios=3, waitTime=10 )
    #wirelessRadios = Number of STAs + APs
    #waitTime = Time (sec) necessary to connect STAs in Ad-hoc mode (it depends of each machine)

    print "*** Creating nodes"
    sta1 = net.addStation( 'sta1' )
    sta2 = net.addStation( 'sta2' )
    sta3 = net.addStation( 'sta3' )

    print "*** Creating links"
    net.addHoc(sta1, 'adhocNet', 'g') #node, ssid, mode
    net.addHoc(sta2, 'adhocNet', 'g') #node, ssid, mode
    net.addHoc(sta3, 'adhocNet', 'g') #node, ssid, mode

    print "*** Starting network"
    net.build()

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()

