#!/usr/bin/python

'This example shows how to create multiple interfaces in stations'

from __future__ import print_function
from mininet.node import OVSKernelSwitch, Controller
from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.link import adhoc


def topology():
    "Create a network."
    net = Mininet_wifi(controller=Controller, switch=OVSKernelSwitch)

    info("*** Creating nodes\n")
    sta1 = net.addStation('sta1', wlans=3)  # 3 wlan added
    sta2 = net.addStation('sta2')  # 1 wlan added
    ap1 = net.addAccessPoint('ap1', ssid='ssid_1', mode='g', channel='5')
    c0 = net.addController('c0', controller=Controller)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    info("*** Associating...\n")
    net.addLink(ap1, sta1)
    net.addLink(sta1, cls=adhoc, ssid='adhoc1')
    net.addLink(sta2, cls=adhoc, ssid='adhoc1')

    info("*** Starting network\n")
    net.build()
    c0.start()
    ap1.start([c0])

    info("*** Addressing...\n")
    sta1.setIP('192.168.10.1/24', intf="sta1-wlan1")
    sta2.setIP('192.168.10.2/24', intf="sta2-wlan0")

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
