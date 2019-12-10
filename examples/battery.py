#!/usr/bin/python

"""This example creates a simple network topology in which
   stations are equipped with batteries"""

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi


def topology():
    "Create a network."
    net = Mininet_wifi()

    info("*** Creating nodes\n")
    sta1 = net.addStation('sta1', battery=100)
    sta2 = net.addStation('sta2', battery=100)
    ap1 = net.addAccessPoint('ap1', ssid="simplewifi", mode="g",
                             channel="5", failMode='standalone')

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    info("*** Associating Stations\n")
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap1)

    info("*** Starting network\n")
    net.build()
    ap1.start([])

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
