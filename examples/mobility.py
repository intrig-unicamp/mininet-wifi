#!/usr/bin/python

'Setting the position of nodes and providing mobility'
import sys

from mininet.net import Mininet
from mininet.node import Controller, OVSKernelAP
from mininet.cli import CLI
from mininet.log import setLogLevel


def topology(coord):

    "Create a network."
    net = Mininet(controller=Controller, accessPoint=OVSKernelAP)

    print "*** Creating nodes"
    h1 = net.addHost('h1', mac='00:00:00:00:00:01', ip='10.0.0.1/8')
    sta1 = net.addStation('sta1', mac='00:00:00:00:00:02', ip='10.0.0.2/8')
    sta2 = net.addStation('sta2', mac='00:00:00:00:00:03', ip='10.0.0.3/8')
    ap1 = net.addAccessPoint('ap1', ssid='new-ssid', mode='g', channel='1',
                             position='45,40,0')
    c1 = net.addController('c1', controller=Controller)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Associating and Creating links"
    net.addLink(ap1, h1)

    net.plotGraph(max_x=200, max_y=200)

    if coord:
        sta1.coord = ['40.0,30.0,0.0', '31.0,10.0,0.0', '31.0,30.0,0.0']
        sta2.coord = ['40.0,40.0,0.0', '55.0,31.0,0.0', '55.0,81.0,0.0']

    net.startMobility(time=0, repetitions=1)
    if coord:
        net.mobility(sta1, 'start', time=1)
        net.mobility(sta2, 'start', time=2)
        net.mobility(sta1, 'stop', time=12)
        net.mobility(sta2, 'stop', time=22)
    else:
        net.mobility(sta1, 'start', time=1, position='40.0,30.0,0.0')
        net.mobility(sta2, 'start', time=2, position='40.0,40.0,0.0')
        net.mobility(sta1, 'stop', time=12, position='31.0,10.0,0.0')
        net.mobility(sta2, 'stop', time=22, position='55.0,31.0,0.0')
    net.stopMobility(time=23)

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
    coord = True if '-c' in sys.argv else False
    topology(coord)
