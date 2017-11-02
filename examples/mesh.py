#!/usr/bin/python

"""This example shows how to work in adhoc mode

It is a full mesh network

     .sta3.
    .      .
   .        .
sta1 ----- sta2"""

from mininet.net import Mininet
from mininet.cli import CLI
from mininet.log import setLogLevel

def topology():
    "Create a network."
    net = Mininet(enable_wmediumd=True, enable_interference=True)

    print "*** Creating nodes"
    sta1 = net.addStation('sta1', position='10,10,0')
    sta2 = net.addStation('sta2', position='50,10,0')
    sta3 = net.addStation('sta3', position='90,10,0')

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Creating links"
    net.addMesh(sta1, ssid='meshNet', channel=5)
    net.addMesh(sta2, ssid='meshNet', channel=5)
    net.addMesh(sta3, ssid='meshNet', channel=5)

    print "*** Starting network"
    net.build()

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
