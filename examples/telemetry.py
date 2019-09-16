#!/usr/bin/python

'This example creates a simple network topology with 1 AP and 2 stations'

import sys

from mininet.node import Controller
from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.telemetry import telemetry


def topology():
    "Create a network."
    net = Mininet_wifi(controller=Controller)

    info("*** Creating nodes\n")
    sta1 = net.addStation('sta1')
    sta2 = net.addStation('sta2')
    ap1 = net.addAccessPoint('ap1', ssid="simplewifi", mode="g", channel="5")
    ap2 = net.addAccessPoint('ap2', ssid="simplewifi1", mode="g", channel="1")
    c0 = net.addController('c0')

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    info("*** Associating Stations\n")
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap2)
    net.addLink(ap1, ap2)

    # refer to mn_wifi/telemetry for more information
    nodes = net.aps + net.stations
    telemetry(nodes, single=True)

    info("*** Starting network\n")
    net.build()
    c0.start()
    ap1.start([c0])
    ap2.start([c0])

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
