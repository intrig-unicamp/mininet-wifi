#!/usr/bin/python

"""This example shows how to enable 4-address
Warning: It works only when network manager is stopped"""

from mininet.net import Mininet
from mininet.node import Controller, OVSKernelAP
from mininet.cli import CLI
from mininet.log import setLogLevel


def topology():
    "Create a network."
    net = Mininet( controller=Controller, accessPoint=OVSKernelAP,
                   enable_wmediumd=True, enable_interference=True,
                   configure4addr=True, disableAutoAssociation=True )

    print "*** Creating nodes"
    ap1 = net.addAccessPoint( 'ap1', _4addr="ap", ssid="wds-ssid1",
                              mode="g", channel="1", position='30,30,0' )
    ap2 = net.addAccessPoint( 'ap2', _4addr="client", ssid="wds-ssid2",
                              mode="g", channel="1", position='40,60,0' )
    ap3 = net.addAccessPoint( 'ap3', _4addr="client", ssid="wds-ssid3",
                              mode="g", channel="1", position='50,30,0' )
    sta1 = net.addStation( 'sta1', ip="192.168.0.1/24", position='31,32,0' )
    sta2 = net.addStation( 'sta2', ip="192.168.0.2/24", position='32,34,0' )
    sta3 = net.addStation( 'sta3', ip="192.168.0.3/24", position='41,62,0' )
    sta4 = net.addStation( 'sta4', ip="192.168.0.4/24", position='42,64,0' )
    sta5 = net.addStation( 'sta5', ip="192.168.0.5/24", position='51,32,0' )
    sta6 = net.addStation( 'sta6', ip="192.168.0.6/24", position='52,34,0' )
    c0 = net.addController('c0', controller=Controller, ip='127.0.0.1',
                           port=6633)

    print "*** Configuring Propagation Model"
    net.propagationModel(model="logDistance", exp=4.5)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Adding Link"
    net.addLink(ap1, ap2, link='4addr')
    net.addLink(ap1, ap3, link='4addr')
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap1)
    net.addLink(sta3, ap2)
    net.addLink(sta4, ap2)
    net.addLink(sta5, ap3)
    net.addLink(sta6, ap3)

    net.plotGraph(max_x=100, max_y=100)

    print "*** Starting network"
    net.build()
    c0.start()
    ap1.start( [c0] )
    ap2.start( [c0] )
    ap3.start( [c0] )

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
