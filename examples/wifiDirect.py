#!/usr/bin/python

'Example for WiFi Direct'

from time import sleep

from mininet.net import Mininet
from mininet.cli import CLI
from mininet.log import setLogLevel


def topology():
    "Create a network."
    net = Mininet(enable_wmediumd=True, enable_interference=True,
                  configureWiFiDirect=True, disableAutoAssociation=True)

    print "*** Creating nodes"
    sta1 = net.addStation('sta1', ip='10.0.0.1/8', position='10,10,0')
    sta2 = net.addStation('sta2', ip='10.0.0.2/8', position='20,20,0')

    print "*** Configuring Propagation Model"
    net.propagationModel(model="logDistance", exp=3.5)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    net.plotGraph(max_x=200, max_y=200)

    print "*** Starting WiFi Direct"
    net.wifiDirect(sta1)
    net.wifiDirect(sta2)

    print "*** Starting network"
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

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
