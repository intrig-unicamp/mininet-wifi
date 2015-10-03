#!/usr/bin/python

"""
Handover example.

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
    sta1 = net.addStation( 'sta1', mac='00:00:00:00:00:02', ip='10.0.0.2/8' )
    sta2 = net.addStation( 'sta2', mac='00:00:00:00:00:03', ip='10.0.0.3/8' )
    ap1 = net.addBaseStation( 'ap1', ssid= 'new-ssid1', mode= 'g', channel= '1', position='15,50,0' )
    ap2 = net.addBaseStation( 'ap2', ssid= 'new-ssid2', mode= 'g', channel= '6', position='25,30,0' )
    c1 = net.addController( 'c1', controller=Controller )

    print "*** Creating links"
    net.addLink(ap1, ap2)
    net.addLink(ap1, sta1)
    net.addLink(ap1, sta2)

    print "*** Starting network"
    net.build()
    c1.start()
    ap1.start( [c1] )
    ap2.start( [c1] )

    """uncomment to plot graph"""
    #net.plotGraph(max_x=100, max_y=100)

    net.startMobility(0)
    net.mobility('sta1', 'start', time=1, position='10.0,45.0,0.0')
    net.mobility('sta2', 'start', time=2, position='10.0,40.0,0.0')
    net.mobility('sta1', 'stop', time=12, position='15.0,10,0.0')
    net.mobility('sta2', 'stop', time=22, position='25.0,25,0.0')
    net.stopMobility(23)

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
