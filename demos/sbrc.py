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
    staList = []

    print "*** Creating nodes"
    for n in range(10):
	staList.append(n)
	staList[n] = net.addStation( 'sta%s' % (n+1), wlans=2, mac='00:00:00:00:00:%s' % (n+1), ip='192.168.0.%s/24' % (n+1) )
    phyap1 = net.addPhysicalBaseStation( 'phyap1', ssid= 'SBRC16-MininetWiFi', mode= 'g', channel= '1', position='50,115,0', phywlan='wlan11' )
    sta11 = net.addStation( 'sta11', ip='10.0.0.111/8', position='120,200,0')
    ap2 = net.addAccessPoint( 'ap2', ssid= 'ap2', mode= 'g', channel= '11', position='100,175,0' )
    ap3 = net.addAccessPoint( 'ap3', ssid= 'ap3', mode= 'g', channel= '6', position='150,50,0' )
    ap4 = net.addAccessPoint( 'ap4', ssid= 'ap4', mode= 'g', channel= '1', position='175,150,0' )
    c1 = net.addController( 'c1', controller=Controller, port=6653 )
    root = Node( 'root', inNamespace=False )

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    """uncomment to plot graph"""
    net.plotGraph(max_x=220, max_y=220)

    """Routing"""
    net.meshRouting('custom')

    """Seed"""
    net.seed(20)

    print "*** Associating and Creating links"
    for sta in staList:
        net.addMesh(sta, ssid='meshNet')
    net.addLink(phyap1, ap2)
    net.addLink(ap2, ap3)
    net.addLink(ap3, ap4)

    print "*** Starting network"
    net.build()
    c1.start()
    phyap1.start( [c1] )
    ap2.start( [c1] )
    ap3.start( [c1] )
    ap4.start( [c1] )

    ip = 201
    for sta in staList:
        sta.setIP('10.0.0.%s/8' % ip, intf="%s-wlan1" % sta)
        ip+=1

    "*** Available models: RandomWalk, TruncatedLevyWalk, RandomDirection, RandomWayPoint, GaussMarkov, ReferencePoint, TimeVariantCommunity ***"
    net.startMobility(startTime=0, model='RandomWalk', max_x=220, max_y=220, min_v=0.1, max_v=0.2)

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
