#!/usr/bin/python

'This example shows how to create multiple interfaces in stations'

from mininet.net import Mininet
from mininet.node import OVSKernelSwitch, Controller
from mininet.cli import CLI
from mininet.log import setLogLevel


def topology():
    "Create a network."
    net = Mininet(controller=Controller, switch=OVSKernelSwitch)

    print "*** Creating nodes"
    sta1 = net.addStation('sta1', wlans=3)  # 3 wlan added
    sta2 = net.addStation('sta2')  # 1 wlan added
    ap1 = net.addAccessPoint('ap1', ssid='ssid_1', mode='g', channel='5')
    c0 = net.addController('c0', controller=Controller)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Associating..."
    net.addLink(ap1, sta1)
    net.addHoc(sta1, ssid='adhoc1')
    net.addHoc(sta2, ssid='adhoc1')

    print "*** Starting network"
    net.build()
    c0.start()
    ap1.start([c0])

    print "***Addressing..."
    sta1.setIP('192.168.10.1/24', intf="sta1-wlan1")
    sta2.setIP('192.168.10.2/24', intf="sta2-wlan0")

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
