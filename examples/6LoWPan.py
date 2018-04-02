#!/usr/bin/python

'This example creates a simple network topology with 3 nodes'

from mininet.log import setLogLevel, info
from mininet.wifi.cli import CLI_wifi
from mininet.wifi.net import Mininet_wifi


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
    net.add6LoWPANLink(node1, panid='0xbeef')
    net.add6LoWPANLink(node2, panid='0xbeef')
    net.add6LoWPANLink(node3, panid='0xbeef')

    info("*** Starting network\n")
    net.build()

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
