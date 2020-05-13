#!/usr/bin/python

"""
NOTE: you have to install wireless-regdb and CRDA
      please refer to https://mininet-wifi.github.io/80211p/
"""

from mininet.log import setLogLevel, info
from mn_wifi.link import wmediumd, ITSLink
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi
from mn_wifi.wmediumdConnector import interference


def topology():
    "Create a network."
    net = Mininet_wifi(link=wmediumd, wmediumd_mode=interference)

    info("*** Creating nodes\n")
    sta1 = net.addStation('sta1', ip='10.0.0.1/8', position='25,50,0')
    sta2 = net.addStation('sta2', ip='10.0.0.2/8', position='50,50,0')

    info("*** Configuring Propagation Model\n")
    net.setPropagationModel(model="logDistance", exp=4.5)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    net.plotGraph(max_x=100, max_y=100)

    info("*** Starting ITS Links\n")
    net.addLink(sta1, intf='sta1-wlan0', cls=ITSLink, channel='181')
    net.addLink(sta2, intf='sta2-wlan0', cls=ITSLink, channel='181')

    info("*** Starting network\n")
    net.build()

    info("*** Running CLI\n")
    CLI(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
