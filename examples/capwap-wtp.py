#!/usr/bin/python

"""Enabling CAPWAP_WTP
Please consider to follow up this repository:
https://github.com/ramonfontes/opencapwap-mininet-wifi """

from mininet.node import Controller
from mininet.log import setLogLevel, info
from mn_wifi.node import OVSKernelAP
from mn_wifi.cli import CLI_wifi
from mn_wifi.link import wmediumd
from mn_wifi.net import Mininet_wifi
from mn_wifi.wmediumdConnector import interference


def topology():
    "Create a network."
    net = Mininet_wifi(controller=Controller, accessPoint=OVSKernelAP,
                       driver='capwap_wtp', link=wmediumd,
                       wmediumd_mode=interference)

    info("*** Creating nodes\n")
    ap1 = net.addAccessPoint('ap1', ssid='new-ssid', mode='g', channel='1',
                             position='15,30,0')
    net.addStation('sta1', mac='00:00:00:00:00:02', ip='10.0.0.1/8',
                   position='10,20,0')
    net.addStation('sta2', mac='00:00:00:00:00:03', ip='10.0.0.2/8',
                   position='20,20,0')
    c1 = net.addController('c1', controller=Controller)

    info("*** Configuring Propagation Model\n")
    net.propagationModel(model="logDistance", exp=4)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    net.plotGraph(max_x=100, max_y=100)

    info("*** Starting network\n")
    net.build()
    c1.start()
    ap1.start([c1])

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('debug')
    topology()
