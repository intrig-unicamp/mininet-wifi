#!/usr/bin/python

'This example creates a simple network topology with 1 AP and 2 stations'

from mininet.log import setLogLevel, info
from mininet.wifi.cli import CLI_wifi
from mininet.sixLoWPAN.net import Mininet_sixLoWPAN


def topology():
    "Create a network."
    net = Mininet_sixLoWPAN()

    info("*** Creating nodes\n")
    sta1 = net.add6LoWPAN('sta1')
    sta2 = net.add6LoWPAN('sta2')

    info("*** Configuring wifi nodes\n")
    net.configureSixLoWPANNodes()

    info("*** Associating Stations\n")
    net.addLink(sta1, link='6LoWPAN', panid='0xbeef')
    net.addLink(sta2, link='6LoWPAN', panid='0xbeef')

    info("*** Starting network\n")
    net.build()

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
