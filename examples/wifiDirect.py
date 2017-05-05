#!/usr/bin/python

'Example for WiFi Direct'

from mininet.net import Mininet
from mininet.node import  Controller
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink

def topology():
    "Create a network."
    net = Mininet(controller=Controller, link=TCLink)

    print "*** Creating nodes"
    sta1 = net.addStation('sta1', ip='10.0.0.1/8')
    sta2 = net.addStation('sta2', ip='10.0.0.2/8')

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Starting WiFi Direct"
    net.wifiDirect(sta1)
    net.wifiDirect(sta2)

    print "*** Starting network"
    net.build()

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel('info')
    topology()
