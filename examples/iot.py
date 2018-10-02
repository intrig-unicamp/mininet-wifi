#!/usr/bin/python

'This example show how to enable wifi and 6lowpan together'


from mininet.node import Controller
from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.sixLoWPAN.link import sixLoWPANLink


def topology():
    "Create a network."
    net = Mininet_wifi(controller=Controller)

    info("*** Creating nodes\n")
    sta1 = net.addStation('sta1', sixlowpan=1, wpan_ip='2001::1/64')
    sta2 = net.addStation('sta2', sixlowpan=1, wpan_ip='2001::2/64')
    ap1 = net.addAccessPoint('ap1', ssid="simplewifi",
                             mode="g", channel="5", failMode='standalone')

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    info("*** Associating Stations\n")
    net.addLink(sta1, cls=sixLoWPANLink, panid='0xbeef')
    net.addLink(sta2, cls=sixLoWPANLink, panid='0xbeef')
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
