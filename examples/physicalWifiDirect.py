#!/usr/bin/python

'Example for WiFi Direct'

import os
from time import sleep

from mininet.log import setLogLevel, info
from mn_wifi.link import wmediumd, wifiDirectLink,\
    physicalWifiDirectLink
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.wmediumdConnector import interference


def topology():
    "Create a network."
    net = Mininet_wifi(link=wmediumd, wmediumd_mode=interference,
                       configureWiFiDirect=True, autoAssociation=False)

    info("*** Creating nodes\n")
    sta1 = net.addStation('sta1', ip='10.0.0.1/8', position='10,10,0',
                          inNamespace=False)
    sta2 = net.addStation('sta2', ip='10.0.0.2/8', position='20,20,0')

    info("*** Configuring Propagation Model\n")
    net.propagationModel(model="logDistance", exp=3.5)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    net.plotGraph(max_x=200, max_y=200)

    info("*** Starting WiFi Direct\n")
    intf='wlxf4f26d193319'
    net.addLink(sta1, cls=physicalWifiDirectLink, intf=intf)
    net.addLink(sta2, cls=wifiDirectLink)

    info("*** Starting network\n")
    net.build()

    sta1.cmd('wpa_cli -ista1-wlan0 p2p_find')
    sta2.cmd('wpa_cli -ista2-wlan0 p2p_find')
    sta2.cmd('wpa_cli -ista2-wlan0 p2p_peers')
    sleep(3)
    sta1.cmd('wpa_cli -ista1-wlan0 p2p_peers')
    sleep(3)
    pin = sta1.cmd('wpa_cli -ista1-wlan0 p2p_connect %s pin auth'
                   % sta2.params['mac'][1])
    sleep(3)
    sta2.cmd('wpa_cli -ista2-wlan0 p2p_connect %s %s'
             % (sta1.params['mac'][1], pin))

    # This is the interface/ip addr of the physical node
    os.system('ip addr add 10.0.0.3/8 dev %s' % intf)

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
