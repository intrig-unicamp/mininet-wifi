#!/usr/bin/python

"""
   Veicular Ad Hoc Networks - VANETs

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
    car1 = net.addVehicle( 'car1', wlans=2, mac='00:00:00:00:00:01', ip='10.0.0.1/8', min_speed=1, max_speed=5 )
    car2 = net.addVehicle( 'car2', wlans=2, mac='00:00:00:00:00:02', ip='10.0.0.2/8', min_speed=5, max_speed=10 )
    car3 = net.addVehicle( 'car3', mac='00:00:00:00:00:03', ip='10.0.0.3/8', min_speed=10, max_speed=15 )
    car4 = net.addVehicle( 'car4', mac='00:00:00:00:00:04', ip='10.0.0.4/8', min_speed=15, max_speed=20 )
    car5 = net.addVehicle( 'car5', mac='00:00:00:00:00:05', ip='10.0.0.5/8', min_speed=15, max_speed=20 )
    bs1 = net.addBaseStation( 'BS1', ssid= 'new-ssid1', mode= 'g', channel= '1' )
    bs2 = net.addBaseStation( 'BS2', ssid= 'new-ssid2', mode= 'g', channel= '6' )
    bs3 = net.addBaseStation( 'BS3', ssid= 'new-ssid3', mode= 'g', channel= '11' )
    c1 = net.addController( 'c1', controller=Controller )

    print "*** Associating and Creating links"
    net.addLink(bs1, car1)
    net.addLink(bs2, car2)
    net.addLink(bs1, bs2)
    net.addLink(bs1, bs3)
    net.addMesh(car1, ssid='mesh')
    net.addMesh(car2, ssid='mesh')
    net.addMesh(car3, ssid='mesh')
    net.addMesh(car4, ssid='mesh')
    net.addMesh(car5, ssid='mesh')
       
    print "*** Starting network"
    net.build()
    c1.start()
    bs1.start( [c1] )
    bs2.start( [c1] )
    bs3.start( [c1] )
    
    """uncomment to plot graph"""
    net.plotGraph(max_x=500, max_y=500)

    """Number of Roads"""
    net.roads(6)

    """Seed"""
    net.seed(20) 

    """Start Mobility"""
    net.startMobility(startTime=0)
   
    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
