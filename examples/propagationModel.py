#!/usr/bin/python

'This example show how to configure Propagation Models'

from mininet.net import Mininet
from mininet.node import Controller, OVSKernelAP
from mininet.cli import CLI
from mininet.log import setLogLevel


def topology():

    "Create a network."
    net = Mininet(controller=Controller, accessPoint=OVSKernelAP)

    print "*** Creating nodes"
    net.addStation('sta1', antennaHeight='1', antennaGain='5')
    net.addStation('sta2', antennaHeight='1', antennaGain='5')
    ap1 = net.addAccessPoint('ap1', ssid='new-ssid', equipmentModel='DI524',
                             mode='g', channel='1', position='50,50,0')
    c1 = net.addController('c1', controller=Controller)

    net.propagationModel(model="logDistance", exp=4)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    net.plotGraph(max_x=100, max_y=100)

    net.seed(1)

    net.startMobility(time=0, model='RandomWayPoint', max_x=100, max_y=100,
                      min_v=0.5, max_v=0.5)

    print "*** Starting network"
    net.build()
    c1.start()
    ap1.start([c1])

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
