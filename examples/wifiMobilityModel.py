#!/usr/bin/python

"""
Setting the position of Nodes (only for Stations and Access Points) and providing mobility using mobility models.

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
    h1 = net.addHost( 'h1', mac='00:00:00:00:00:01', ip='10.0.0.1/8' )
    sta1 = net.addStation( 'sta1', mac='00:00:00:00:00:02', ip='10.0.0.2/8' )
    sta2 = net.addStation( 'sta2', mac='00:00:00:00:00:03', ip='10.0.0.3/8' )
    ap1 = net.addBaseStation( 'ap1', ssid= 'new-ssid', mode= 'g', channel= '1', position='10,10,0' )
    c1 = net.addController( 'c1', controller=Controller )

    """uncomment to plot graph"""
    #net.plotGraph(max_x=60, max_y=60)

    print "*** Associating and Creating links"
    net.addLink(ap1, h1, 1, 0)
    net.addLink(ap1, sta1)
    net.addLink(ap1, sta2)
    
    print "*** Starting network"
    net.build()
    c1.start()
    ap1.start( [c1] )
    
    
    "*** Available models: RandomWalk, TruncatedLevyWalk, RandomDirection, RandomWaypoint, GaussMarkov ***"
    net.startMobility(0, model='RandomDirection', max_x=20, max_y=20, min_v=0.1, max_v=0.2)
   
    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
