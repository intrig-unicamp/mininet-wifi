#!/usr/bin/python

'This example shows how to create wireless link between two APs'

from mininet.net import Mininet
from mininet.node import Controller, OVSKernelAP
from mininet.cli import CLI
from mininet.log import setLogLevel


def topology():
    "Create a network."
    net = Mininet(controller=Controller, accessPoint=OVSKernelAP,
                  enable_wmediumd=True, enable_interference=True)

    print "*** Creating nodes"
    sta1 = net.addStation('sta1', mac='00:00:00:00:00:11')
    sta2 = net.addStation('sta2', mac='00:00:00:00:00:12')
    ap1 = net.addAccessPoint('ap1', wlans=2, ssid='ssid1,', position='10,10,0')
    ap2 = net.addAccessPoint('ap2', wlans=2, ssid='ssid2,', position='30,10,0')
    c0 = net.addController('c0', controller=Controller, ip='127.0.0.1',
                           port=6633)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Associating Stations"
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap2)
    net.addMesh(ap1, intf='ap1-wlan2', ssid='mesh-ssid')
    net.addMesh(ap2, intf='ap2-wlan2', ssid='mesh-ssid')

    print "*** Starting network"
    net.build()
    c0.start()
    ap1.start([c0])
    ap2.start([c0])

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
