#!/usr/bin/python

'This example creates a simple network topology with 1 AP and 2 stations'
import sys

from mininet.net import Mininet
from mininet.node import  Controller, OVSKernelAP
from mininet.cli import CLI
from mininet.log import setLogLevel


def topology(isVirtual):
    "Create a network."
    net = Mininet(controller=Controller, accessPoint=OVSKernelAP)

    print "*** Creating nodes"
    if isVirtual:
        sta1 = net.addStation('sta1', nvif=2)
    else:
        sta1 = net.addStation('sta1')
    sta2 = net.addStation('sta2')
    ap1 = net.addAccessPoint('ap1', ssid="simplewifi", mode="g", channel="5")
    c0 = net.addController('c0', controller=Controller, ip='127.0.0.1',
                           port=6633)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Associating Stations"
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap1)

    print "*** Starting network"
    net.build()
    c0.start()
    ap1.start([c0])

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    isVirtual = True if '-v' in sys.argv else False
    topology(isVirtual)
