#!/usr/bin/python

"""Code created to be presented with the paper titled:
   "Rich Experimentation through Hybrid Physical-Virtual Software-Defined Wireless Networking Emulation" 
   authors: Ramon dos Reis Fontes and Christian Esteve Rothenberg"""

from mininet.net import Mininet
from mininet.node import RemoteController,OVSKernelSwitch, Controller
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.node import Node
from mininet.log import setLogLevel
import os
import time

def topology():

    "Create a network."
    net = Mininet( controller=RemoteController, link=TCLink, switch=OVSKernelSwitch )
    sta = []

    print "*** Creating nodes"
    for n in range(10):
	sta.append(n)
	sta[n] = net.addStation( 'sta%s' % (n+1), wlans=2, mac='00:00:00:00:00:%s' % (n+1), ip='192.168.0.%s/24' % (n+1) )
    phyap1 = net.addPhysicalBaseStation( 'phyap1', ssid= 'ap-ssid1', mode= 'g', channel= '1', position='50,115,0', wlan='wlan11' )
    ap2 = net.addBaseStation( 'ap2', ssid= 'ap-ssid2', mode= 'g', channel= '11', position='100,175,0' )
    c1 = net.addController( 'c1', controller=Controller, port=6653 )
    root = Node( 'root', inNamespace=False )

    print "*** Creating links"
    for station in sta:
        net.addMesh(station, ssid='meshNet')

    """uncomment to plot graph"""
    net.plotGraph(max_x=240, max_y=240)

    """Routing"""
    net.meshRouting('custom')

    """Seed"""
    net.seed(20)

    print "*** Associating and Creating links"
    net.addLink(phyap1, ap2)

    print "*** Starting network"
    net.build()

    c1.start()
    phyap1.start( [c1] )
    ap2.start( [c1] )

    ip = 201
    for station in sta:
        station.cmd('ifconfig %s-wlan1 10.0.0.%s/8' % (station, ip))
        ip+=1

    "*** Available models: RandomWalk, TruncatedLevyWalk, RandomDirection, RandomWayPoint, GaussMarkov, ReferencePoint, TimeVariantCommunity ***"
    net.startMobility(startTime=0, model='RandomWalk', max_x=200, max_y=220, min_v=0.1, max_v=0.2)

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
