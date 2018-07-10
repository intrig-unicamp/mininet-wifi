#!/usr/bin/python

"""This example shows how to work in adhoc mode

sta1 <---> sta2 <---> sta3"""

import sys

from mininet.log import setLogLevel, info
from mn_wifi.link import wmediumd, adhoc
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.wmediumdConnector import interference


def topology(autoTxPower):
    "Create a network."
    net = Mininet_wifi(link=wmediumd, wmediumd_mode=interference)

    info("*** Creating nodes\n")
    if autoTxPower:
        sta1 = net.addStation('sta1', position='10,10,0', range=100)
        sta2 = net.addStation('sta2', position='50,10,0', range=100)
        sta3 = net.addStation('sta3', position='90,10,0', range=100)
    else:
        sta1 = net.addStation('sta1', position='10,10,0')
        sta2 = net.addStation('sta2', position='50,10,0')
        sta3 = net.addStation('sta3', position='90,10,0')

    net.propagationModel(model="logDistance", exp=4)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    info("*** Creating links\n")
    net.addLink(sta1, cls=adhoc, ssid='adhocNet', mode='g', channel=5)
    net.addLink(sta2, cls=adhoc, ssid='adhocNet', mode='g', channel=5)
    net.addLink(sta3, cls=adhoc, ssid='adhocNet', mode='g', channel=5)

    info("*** Starting network\n")
    net.build()

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    autoTxPower = True if '-a' in sys.argv else False
    topology(autoTxPower)
