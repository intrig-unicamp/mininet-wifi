#!/usr/bin/python

'This example creates a simple network topology with 3 nodes'

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.sixLoWPAN.link import sixLoWPANLink


def topology():
    "Create a network."
    net = Mininet_wifi()

    info("*** Creating nodes\n")
    node1 = net.add6LoWPAN('node1', ip='2001::1/64')
    node2 = net.add6LoWPAN('node2', ip='2001::2/64')
    node3 = net.add6LoWPAN('node3', ip='2001::3/64')

    info("*** Configuring nodes\n")
    net.configureWifiNodes()

    info("*** Associating Nodes\n")
    net.addLink(node1, cls=sixLoWPANLink, panid='0xbeef')
    net.addLink(node2, cls=sixLoWPANLink, panid='0xbeef')
    net.addLink(node3, cls=sixLoWPANLink, panid='0xbeef')

    info("*** Starting network\n")
    net.build()

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
