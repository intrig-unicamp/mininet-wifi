#!/usr/bin/python

'Example for WiFi Direct'

from time import sleep

from mininet.log import setLogLevel, info
from mn_wifi.link import wmediumd, wifiDirectLink
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.wmediumdConnector import interference


def topology():
    "Create a network."
    net = Mininet_wifi(link=wmediumd, wmediumd_mode=interference,
                       configureWiFiDirect=True, autoAssociation=False)

    info("*** Creating nodes\n")
    sta1 = net.addStation('sta1', ip='10.0.0.1/8', position='10,10,0')
    sta2 = net.addStation('sta2', ip='10.0.0.2/8', position='20,20,0')

    info("*** Configuring Propagation Model\n")
    net.propagationModel(model="logDistance", exp=3.5)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    net.plotGraph(max_x=200, max_y=200)

    info("*** Starting WiFi Direct\n")
    net.addLink(sta1, cls=wifiDirectLink)
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
                   % sta2.params['mac'][0])
    sleep(3)
    sta2.cmd('wpa_cli -ista2-wlan0 p2p_connect %s %s'
             % (sta1.params['mac'][0], pin))

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
