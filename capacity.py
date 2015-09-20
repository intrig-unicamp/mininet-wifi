#!/usr/bin/python

"""                                                          
                                                                sta5 
                                                                 v
                                                                .
                                                               . 
                                                              .
                                                             v
Capacity Sharing - h1<---->s1<---->ap1< o o o >sta1< . . . >sta2< . . . >sta3
                           v                                 v
                            \                                 .
                             \                                 .
                              \                                 . 
                               V                                 V
                              ap2< . . . >sta6< . . . >sta7     sta4
"""
 
import subprocess
import time
import os
 
from mininet.wifi import station
from mininet.net import Mininet
from mininet.node import  RemoteController, OVSKernelSwitch, Controller
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink
"from mininet.node import Node" 
 
def topology():
    "Create a network."
    net = Mininet( wirelessRadios=12, controller=Controller, link=TCLink, switch=OVSKernelSwitch )
 
    print "*** Creating nodes"
    s1 = net.addSwitch( 's1', listenPort=6673 )
    h1 = net.addHost( 'h1', ip='192.168.0.1/24' )
    ap1 = net.addBaseStation( 'ap1', listenPort=6674, ssid="ssid_1", mode="g", channel="5" )
    ap2 = net.addBaseStation( 'ap2', listenPort=6675, ssid="ssid_2", mode="g", channel="1" )
    sta1 = net.addStation( 'sta1', ip="192.168.0.100/24" )
    sta2 = net.addStation( 'sta2' )
    sta3 = net.addStation( 'sta3' )
    sta4 = net.addStation( 'sta4' )
    sta5 = net.addStation( 'sta5' )
    sta6 = net.addStation( 'sta6', ip="192.168.0.200/24" )
    sta7 = net.addStation( 'sta7' ) 
    c0 = net.addController( 'c0', controller=Controller )

    print "*** Adding Link"
    net.addLink( ap1, sta1 )
    net.addLink( ap1, s1 )
    net.addLink( s1, h1 )
    net.addLink( ap2, s1 )
    net.addLink( ap2, sta6 )

    print "*** Starting network"
    net.build( )
    c0.start( )
    ap1.start( [c0] )
    ap2.start( [c0] )
    s1.start( [c0] )
   

    print "*** Adding new Interface to STA2"
    station.addWlan( sta1 )
    print "*** Adding new Interface to STA3/STA4/STA5"
    station.addWlan( sta2 )
    station.addWlan( sta3 )
    station.addWlan( sta4 )
    station.addWlan( sta5 )
    station.addWlan( sta7 )
    print "*** Adding new Interface to STA7"
    station.addWlan( sta6 )


    print  "***Creating adhoc1 network"
    net.addHoc( sta1, 'adhoc1', 'g', interface='wlan1' ) #Sta1 already has wlan0
    net.addHoc( sta2, 'adhoc1', 'g', interface='wlan0' )

    
  
    print "***Creating adhoc2 network"
    net.addHoc( sta2, 'adhoc2', 'g', interface='wlan1' ) #Sta2 already has wlan0
    net.addHoc( sta3, 'adhoc2', 'g', interface='wlan0' )
    net.addHoc( sta4, 'adhoc2', 'g', interface='wlan0' )
    net.addHoc( sta5, 'adhoc2', 'g', interface='wlan0' )


    
    print  "***Creating adhoc3 network"
    net.addHoc( sta6, 'adhoc3', 'g', interface='wlan1' ) #Sta6 already has wlan0
    net.addHoc( sta7, 'adhoc3', 'g', interface='wlan0' )


    print  "***Routing"
    h1.cmd('route add -net 192.168.10.0/24 gw 192.168.0.100')
    h1.cmd('route add -net 192.168.20.0/24 gw 192.168.0.100')
    h1.cmd('route add -net 10.10.0.0/24 gw 192.168.0.200')
    sta1.cmd('ifconfig sta1-wlan1 192.168.10.1/24')
    sta2.cmd('ifconfig sta2-wlan0 192.168.10.2/24')
    sta2.cmd('ifconfig sta2-wlan1 192.168.20.1/24')
    sta3.cmd('ifconfig sta3-wlan0 192.168.20.2/24')
    sta4.cmd('ifconfig sta4-wlan0 192.168.20.3/24')
    sta5.cmd('ifconfig sta5-wlan0 192.168.20.4/24')
    sta6.cmd('ifconfig sta6-wlan1 10.10.0.1/24')
    sta7.cmd('ifconfig sta7-wlan0 10.10.0.100/24')
    sta1.cmd('route add -net 192.168.20.0/24 gw 192.168.10.2 dev sta1-wlan1')
    sta1.cmd('route add -net 10.10.0.0/24 gw 192.168.0.200 dev sta1-wlan0')
    sta2.cmd('route add -net 0.0.0.0 gw 192.168.10.1 dev sta2-wlan0')
    sta3.cmd('route add -net 0.0.0.0 gw 192.168.20.1 dev sta3-wlan0')
    sta4.cmd('route add -net 0.0.0.0 gw 192.168.20.1 dev sta4-wlan0')
    sta5.cmd('route add -net 0.0.0.0 gw 192.168.20.1 dev sta5-wlan0')
    sta6.cmd('route add -net 0.0.0.0 gw 192.168.0.1 dev sta6-wlan0')
    sta7.cmd('route add -net 0.0.0.0 gw 10.10.0.1 dev sta7-wlan0')
 

    print "*** Running CLI"
    CLI( net )
 
    print "*** Stopping network"
    net.stop()
 
if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
