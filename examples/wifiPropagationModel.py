#!/usr/bin/python

"""
   Example: Propagation Models
"""

from mininet.net import Mininet
from mininet.node import Controller,OVSKernelSwitch
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel

def topology():

    "Create a network."
    net = Mininet( controller=Controller, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    sta1 = net.addStation( 'sta1' )
    sta2 = net.addStation( 'sta2' )
    ap1 = net.addAccessPoint( 'ap1', ssid= 'new-ssid', equipmentModel='DI524', mode='g', channel='1', position='50,50,0' )
    c1 = net.addController( 'c1', controller=Controller )

    print "*** Associating and Creating links"
    net.addLink(ap1, sta1)
    net.addLink(ap1, sta2)
    
    print "*** Starting network"
    net.build()
    c1.start()
    ap1.start( [c1] )
    
    """uncomment to plot graph"""
    net.plotGraph(max_x=100, max_y=100)

    """Seed"""
    net.seed(1) 

    "*** Available propagation models: friisPropagationLossModel, twoRayGroundPropagationLossModel, logDistancePropagationLossModel ***"
    net.propagationModel('friisPropagationLossModel', sL=2)

    "*** Available mobility models: RandomWalk, TruncatedLevyWalk, RandomDirection, RandomWayPoint, GaussMarkov ***"
    net.startMobility(startTime=0, model='RandomWayPoint', max_x=100, max_y=100, min_v=0.5, max_v=0.5)
   
    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
