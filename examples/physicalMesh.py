#!/usr/bin/python

"This example shows how to work with physical mesh nodes"

import os

from mininet.log import setLogLevel, info
from mn_wifi.link import wmediumd, mesh, physicalMesh
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.wmediumdConnector import interference


def topology():
    "Create a network."
    net = Mininet_wifi(link=wmediumd, wmediumd_mode=interference)

    info("*** Creating nodes\n")
    sta1 = net.addStation('sta1', position='10,10,0', inNamespace=False)
    sta2 = net.addStation('sta2', position='50,10,0')
    sta3 = net.addStation('sta3', position='90,10,0')

    net.propagationModel(model="logDistance", exp=4)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    info("*** Creating links\n")
    # intf: physical interface
    net.addLink(sta1, cls=physicalMesh, intf='wlxf4f26d193319',
                ssid='meshNet', channel=5)
    net.addLink(sta2, cls=mesh, ssid='meshNet', channel=5)
    net.addLink(sta3, cls=mesh, ssid='meshNet', channel=5)

    net.plotGraph(max_x=100, max_y=100)

    info("*** Starting network\n")
    net.build()

    # This is the interface/ip addr of the physical node
    os.system('ip addr add 10.0.0.4/8 dev physta1-mp0')

    info("*** Running CLI\n")
    CLI_wifi(net)

    # Delete the interface created previously
    os.system('iw dev physta1-mp0 del')

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
