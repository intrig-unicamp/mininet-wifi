#!/usr/bin/python

'Example for WiFi Direct'

from mininet.net import Mininet
from mininet.node import  Controller
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink
import time

def topology():
    "Create a network."
    net = Mininet(controller=Controller, link=TCLink, enable_wmediumd=True, enable_interference=True)

    print "*** Creating nodes"
    sta1 = net.addStation('sta1', ip='10.0.0.1/8', position='10,10,0')
    sta2 = net.addStation('sta2', ip='10.0.0.2/8', position='40,40,0')

    print "*** Configuring Propagation Model"
    net.propagationModel("logDistancePropagationLossModel", exp=3.5)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Starting WiFi Direct"
    net.wifiDirect(sta1)
    net.wifiDirect(sta2)

    print "*** Starting network"
    net.build()

    "Plotting graph"
    net.plotGraph(max_x=200, max_y=200)

    sta1.cmd('wpa_cli -ista1-wlan0 p2p_find')
    sta2.cmd('wpa_cli -ista2-wlan0 p2p_find')
    sta2.cmd('wpa_cli -ista2-wlan0 p2p_peers')
    time.sleep(2)
    sta1.cmd('wpa_cli -ista1-wlan0 p2p_peers')
    time.sleep(2)
    pin = sta1.cmd('wpa_cli -ista1-wlan0 p2p_connect 02:00:00:00:01:00 pin auth')
    time.sleep(3)
    sta2.cmd('wpa_cli -ista2-wlan0 p2p_connect 02:00:00:00:00:00 %s' % pin)

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel('info')
    topology()
