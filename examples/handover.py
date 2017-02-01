#!/usr/bin/python

"""
Handover example.
"""

from mininet.net import Mininet
from mininet.node import Controller, OVSKernelAP
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel

def topology():

    "Create a network."
    net = Mininet( controller=Controller, link=TCLink, accessPoint=OVSKernelAP )

    print "*** Creating nodes"
    sta1 = net.addStation( 'sta1', mac='00:00:00:00:00:02', ip='10.0.0.2/8' )
    sta2 = net.addStation( 'sta2', mac='00:00:00:00:00:03', ip='10.0.0.3/8' )
    ap1 = net.addAccessPoint( 'ap1', ssid= 'ssid-ap1', mode= 'g', channel= '1', position='15,30,0' )
    ap2 = net.addAccessPoint( 'ap2', ssid= 'ssid-ap2', mode= 'g', channel= '6', position='55,30,0' )
    c1 = net.addController( 'c1', controller=Controller )

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

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
    net.plotGraph(max_x=100, max_y=100)

    net.startMobility(startTime=0)
    net.mobility(sta1, 'start', time=1, position='10,30,0')
    net.mobility(sta2, 'start', time=2, position='10,40,0')
    net.mobility(sta1, 'stop', time=40, position='60,30,0')
    net.mobility(sta2, 'stop', time=40, position='25,40,0')
    net.stopMobility(stopTime=40)

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
