#!/usr/bin/python

'This example shows how to work with Radius Server'

from mininet.net import Mininet
from mininet.node import  Controller, UserAP
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink

def topology():
    "Create a network."
    net = Mininet( controller=Controller, link=TCLink, accessPoint=UserAP, enable_wmediumd=True, enable_interference=True )

    print "*** Creating nodes"
    sta1 = net.addStation( 'sta1', radius_passwd='sdnteam', encrypt='wpa2', radius_identity='joe', position='110,120,0' )
    sta2 = net.addStation( 'sta2', radius_passwd='hello', encrypt='wpa2', radius_identity='bob', position='200,100,0' )
    ap1 = net.addAccessPoint( 'ap1', ssid='simplewifi', authmode='8021x', mode='a', channel='36', encrypt='wpa2', position='150,100,0' )
    c0 = net.addController('c0', controller=Controller, ip='127.0.0.1', port=6633 )

    print "*** Configuring Propagation Model"
    net.propagationModel(model="logDistance", exp=3.5)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Associating Stations"
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap1)

    print "*** Building graph"
    net.plotGraph(max_x=300, max_y=300)

    print "*** Starting network"
    net.build()
    c0.start()
    ap1.start( [c0] )

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
