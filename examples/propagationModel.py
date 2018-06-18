#!/usr/bin/python

'This example show how to configure Propagation Models'

from mininet.node import Controller
from mininet.log import setLogLevel, info
from mn_wifi.node import OVSKernelAP
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi


def topology():

    "Create a network."
    net = Mininet_wifi(controller=Controller, accessPoint=OVSKernelAP)

    info("*** Creating nodes\n")
    net.addStation('sta1', antennaHeight='1', antennaGain='5')
    net.addStation('sta2', antennaHeight='1', antennaGain='5')
    ap1 = net.addAccessPoint('ap1', ssid='new-ssid', equipmentModel='DI524',
                             mode='g', channel='1', position='50,50,0')
    c1 = net.addController('c1', controller=Controller)

    net.propagationModel(model="logDistance", exp=4)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    net.plotGraph(max_x=100, max_y=100)

    net.startMobility(time=0, model='RandomWayPoint', max_x=100, max_y=100,
                      min_v=0.5, max_v=0.5, seed=20)

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
