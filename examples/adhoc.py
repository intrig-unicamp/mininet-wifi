#!/usr/bin/python

"""This example shows how to work in adhoc mode

sta1 <---> sta2 <---> sta3"""

import sys

from mininet.log import setLogLevel, info
from mn_wifi.link import wmediumd, adhoc
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.wmediumdConnector import interference


def topology(args):
    "Create a network."
    net = Mininet_wifi(link=wmediumd, wmediumd_mode=interference)

    info("*** Creating nodes\n")
    if '-a' in args:
        sta1 = net.addStation('sta1', position='10,10,0', range=100)
        sta2 = net.addStation('sta2', position='50,10,0', range=100)
        sta3 = net.addStation('sta3', position='90,10,0', range=100)
    else:
        sta1 = net.addStation('sta1', ipv6='fe80::1',
                              position='10,10,0')
        sta2 = net.addStation('sta2', ipv6='fe80::2',
                              position='50,10,0')
        sta3 = net.addStation('sta3', ipv6='fe80::3',
                              position='90,10,0')

    net.setPropagationModel(model="logDistance", exp=4)

    #net.plotGraph(max_x=300, max_y=300)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    info("*** Creating links\n")
    # MANET routing protocols supported by proto: babel, batman and olsr
    if 'babel' in args or 'batman' in args or 'olsr' in args:
        proto = args[1]
        net.addLink(sta1, cls=adhoc, intf='sta1-wlan0',
                    ssid='adhocNet', proto=proto,
                    mode='g', channel=5, ht_cap='HT40+')
        net.addLink(sta2, cls=adhoc, intf='sta2-wlan0',
                    ssid='adhocNet', proto=proto,
                    mode='g', channel=5)
        net.addLink(sta3, cls=adhoc, intf='sta3-wlan0',
                    ssid='adhocNet', proto=proto,
                    mode='g', channel=5, ht_cap='HT40+')
    else:
        net.addLink(sta1, cls=adhoc, intf='sta1-wlan0',
                    ssid='adhocNet',
                    mode='g', channel=5, ht_cap='HT40+')
        net.addLink(sta2, cls=adhoc, intf='sta2-wlan0',
                    ssid='adhocNet', mode='g', channel=5)
        net.addLink(sta3, cls=adhoc, intf='sta3-wlan0',
                    ssid='adhocNet', mode='g', channel=5,
                    ht_cap='HT40+')

    info("*** Starting network\n")
    net.build()

    info("*** Addressing...\n")
    #sta1.setIPv6('2001::1/64', intf="sta1-wlan0")
    #sta2.setIPv6('2001::2/64', intf="sta2-wlan0")
    #sta3.setIPv6('2001::3/64', intf="sta3-wlan0")

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology(sys.argv)
