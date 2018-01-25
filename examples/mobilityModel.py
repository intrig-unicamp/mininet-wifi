#!/usr/bin/python

'Setting the position of Nodes and providing mobility using mobility models'

from mininet.net import Mininet
from mininet.node import Controller
from mininet.log import setLogLevel, info
from mininet.wifi.node import OVSKernelAP
from mininet.wifi.cli import CLI_WiFi

def topology():
    "Create a network."
    net = Mininet(controller=Controller, accessPoint=OVSKernelAP)

    info("*** Creating nodes\n")
    net.addStation('sta1', mac='00:00:00:00:00:02', ip='10.0.0.2/8',
                   min_x=30, max_x=30, min_y=70, max_y=70, min_v=1, max_v=1)
    net.addStation('sta2', mac='00:00:00:00:00:03', ip='10.0.0.3/8',
                   min_x=30, max_x=30, min_y=70, max_y=70, min_v=5, max_v=5)
    ap1 = net.addAccessPoint('ap1', ssid='new-ssid', mode='g', channel='1',
                             position='50,50,0')
    c1 = net.addController('c1', controller=Controller)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    net.plotGraph(max_x=300, max_y=300)

    net.seed(20)

    net.startMobility(time=0, model='RandomDirection', max_x=100, max_y=100,
                      min_v=0.5, max_v=0.8)

    info("*** Starting network\n")
    net.build()
    c1.start()
    ap1.start([c1])

    info("*** Running CLI\n")
    CLI_WiFi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
