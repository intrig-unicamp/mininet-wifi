#!/usr/bin/python

"""
Tracing Mobility
"""

from mininet.net import Mininet
from mininet.node import Controller,OVSKernelSwitch, RemoteController
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.wifiTracing import tracingMobility

def topology():

    "Create a network."
    net = Mininet( controller=RemoteController, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    sta1 = net.addStation( 'sta1', mac='00:00:00:00:00:02', ip='10.0.0.2/8', speed=1 )
    sta2 = net.addStation( 'sta2', mac='00:00:00:00:00:03', ip='10.0.0.3/8', speed=2 )
    ap1 = net.addBaseStation( 'ap1', ssid= 'new-ssid', mode= 'g', channel= '1', position='50,50,0' )
    c1 = net.addController( 'c1', controller=RemoteController )

    print "*** Starting network"
    net.build()
    c1.start()
    ap1.start( [c1] )

    """uncomment to plot graph"""
    net.plotGraph(max_x=100, max_y=100)

    for pos in range (0, 100):
        x = 10+pos
        y = 1+pos
        sta1.trackingPos.append('%s,%s,0' % (x, y))

    for pos in range (0, 100):
        x = 100-pos
        y = 90-pos
        sta2.trackingPos.append('%s,%s,0' % (x, y))

    tracingMobility()

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()

