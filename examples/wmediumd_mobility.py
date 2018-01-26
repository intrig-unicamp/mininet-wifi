#!/usr/bin/python

"""
Setting the position of Nodes (only for Stations and Access Points)
and providing mobility using mobility models with wmediumd enabled."""

from mininet.node import Controller
from mininet.log import setLogLevel, info
from mininet.wifi.node import OVSKernelAP
from mininet.wifi.link import wmediumd
from mininet.wifi.cli import CLI_wifi
from mininet.wifi.net import Mininet_wifi


def topology():

    "Create a network."
    net = Mininet_wifi(controller=Controller, accessPoint=OVSKernelAP,
                  link=wmediumd)

    info("*** Creating nodes\n")
    ap1 = net.addAccessPoint('ap1', ssid='new-ssid', mode='a', channel='36',
                             position='150,150,0')
    net.addStation('sta1', mac='00:00:00:00:00:02', ip='10.0.0.2/8')
    net.addStation('sta2', mac='00:00:00:00:00:03', ip='10.0.0.3/8')
    c1 = net.addController('c1', controller=Controller)

    info("*** Configuring Propagation Model\n")
    net.propagationModel(model="logDistance", exp=3)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    net.plotGraph(max_x=300, max_y=300)

    net.seed(20)

    net.startMobility(time=0, model='RandomDirection', max_x=300, max_y=300,
                      min_v=0.5, max_v=0.8)

    info("*** Starting network\n")
    net.build()
    c1.start()
    ap1.start([c1])

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
