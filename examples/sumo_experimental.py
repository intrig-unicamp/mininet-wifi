#!/usr/bin/python

"""
   SUMO example
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
    car = []
    for x in range(0,20):
        car.append(x)
    for x in range(0,20):
        car[x] = net.addVehicle( 'car%s' % x, wlans=2, ip='10.0.0.%s/8' % x )
	
    bs1 = net.addBaseStation( 'BS1', ssid= 'new-ssid1', mode= 'g', channel= '1' )
    bs2 = net.addBaseStation( 'BS2', ssid= 'new-ssid2', mode= 'g', channel= '6' )
    bs3 = net.addBaseStation( 'BS3', ssid= 'new-ssid3', mode= 'g', channel= '11' )
    c1 = net.addController( 'c1', controller=Controller )

    print "*** Associating and Creating links"
    for x in range(0,20):
        net.addMesh(car[x], ssid='mesh')

    net.addLink(bs1, bs2)
    net.addLink(bs1, bs3)
       
    print "*** Starting network"
    net.build()
    c1.start()
    bs1.start( [c1] )
    bs2.start( [c1] )
    bs3.start( [c1] )
  
    """Available Options: sumo, sumo-gui"""
    #Put your sumocfg file in /mininet/sumo/data
    net.useExternalProgram('sumo-gui', config_file='map.sumocfg')

    """Routing"""
    #net.meshRouting('custom')


    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
