#!/usr/bin/python

"""Enabling CAPWAP_WTP
Please consider to follow up this repository:
https://github.com/ramonfontes/opencapwap-mininet-wifi 
"""

from mininet.net import Mininet
from mininet.node import Controller, OVSKernelAP
from mininet.cli import CLI
from mininet.log import setLogLevel


def topology():

    "Create a network."
    net = Mininet(controller=Controller, accessPoint=OVSKernelAP,
                  driver='capwap_wtp', enable_wmediumd=True,
                  enable_interference=True)

    print "*** Creating nodes"
    ap1 = net.addAccessPoint('ap1', ssid='new-ssid', mode='g', channel='1',
                             position='15,30,0')
    net.addStation('sta1', mac='00:00:00:00:00:02', ip='10.0.0.1/8',
                   position='10,20,0')
    net.addStation('sta2', mac='00:00:00:00:00:03', ip='10.0.0.2/8',
                   position='20,20,0')
    c1 = net.addController('c1', controller=Controller)

    print "*** Configuring Propagation Model"
    net.propagationModel(model="logDistance", exp=4)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    net.plotGraph(max_x=100, max_y=100)

    print "*** Starting network"
    net.build()
    c1.start()
    ap1.start([c1])

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel('debug')
    topology()
