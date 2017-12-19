#!/usr/bin/python

"""This example shows how to work in adhoc mode

sta1 <---> sta2 <---> sta3"""
import sys

from mininet.net import Mininet
from mininet.cli import CLI
from mininet.log import setLogLevel


def topology(autoTxPower):
    "Create a network."
    net = Mininet(enable_wmediumd=True, enable_interference=True)

    print "*** Creating nodes"
    if autoTxPower:
        sta1 = net.addStation('sta1', position='10,10,0', range=100)
        sta2 = net.addStation('sta2', position='50,10,0', range=100)
        sta3 = net.addStation('sta3', position='90,10,0', range=100)
    else:
        sta1 = net.addStation('sta1', position='10,10,0')
        sta2 = net.addStation('sta2', position='50,10,0')
        sta3 = net.addStation('sta3', position='90,10,0')

    net.propagationModel(model="logDistance", exp=4.5)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Creating links"
    net.addHoc(sta1, ssid='adhocNet', mode='g', channel=5)
    net.addHoc(sta2, ssid='adhocNet', mode='g', channel=5)
    net.addHoc(sta3, ssid='adhocNet', mode='g', channel=5)

    print "*** Starting network"
    net.build()

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    autoTxPower = True if '-a' in sys.argv else False
    topology(autoTxPower)
