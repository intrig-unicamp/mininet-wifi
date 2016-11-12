#!/usr/bin/python

"""
   Veicular Ad Hoc Networks - VANETs

"""

from mininet.net import Mininet
from mininet.node import Controller,OVSKernelSwitch
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel
import os
import random

def topology():

    "Create a network."
    net = Mininet( controller=Controller, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    car = []
    stas = []
    for x in range(0, 10):
        car.append(x)
        stas.append(x)
    for x in range(0, 10):
        min = random.randint(1,10)
        max= random.randint(11,30)
        car[x] = net.addCar('car%s' % (x+1), wlans=1, ip='10.0.0.%s/8' % (x + 1), min_speed=min, max_speed=max, range=50)

    bs1 = net.addBaseStation( 'RSU1', ssid= 'RSU1', mode= 'g', channel= '1', range=100 )
    bs2 = net.addBaseStation( 'RSU2', ssid= 'RSU2', mode= 'g', channel= '6', range=100 )
    bs3 = net.addBaseStation( 'RSU3', ssid= 'RSU3', mode= 'g', channel= '11', range=100 )
    bs4 = net.addBaseStation( 'RSU4', ssid= 'RSU4', mode= 'g', channel= '11', range=100 )
    c1 = net.addController( 'c1', controller=Controller )

    net.meshRouting('custom') 

    print "*** Associating and Creating links"
    net.addLink(bs1, bs2)
    net.addLink(bs1, bs3)
    net.addLink(bs1, bs4)

    print "*** Starting network"
    net.build()
    c1.start()
    bs1.start( [c1] )
    bs2.start( [c1] )
    bs3.start( [c1] )
    bs4.start( [c1] )
    i = 201
    for sw in net.vehicles:
        sw.start([c1])
        os.system('ifconfig %s 10.0.0.%s' % (sw, i))
        i+=1

    """uncomment to plot graph"""
    net.plotGraph(max_x=500, max_y=500)

    """Number of Roads"""
    net.roads(20)

    """Start Mobility"""
    net.startMobility(startTime=0)

    i = 1
    j = 2
    k = 1
    for c in car:
        c.cmd('ifconfig %s-wlan0 192.168.0.%s/24 up' % (c,k))
        c.cmd('ifconfig %s-eth0 192.168.1.%s/24 up' % (c,i))
        c.cmd('ip route add 10.0.0.0/8 via 192.168.1.%s' % j)
        i+=2
        j+=2
        k+=1

    i = 1
    j = 2
    for v in net.vehiclesSTA:
        v.cmd('ifconfig %s-eth0 192.168.1.%s/24 up' % (v, j))
        v.cmd('ifconfig %s-mp0 10.0.0.%s/24 up' % (v,i))
        v.cmd('echo 1 > /proc/sys/net/ipv4/ip_forward')
        i+=1    
        j+=2

    for v1 in net.vehiclesSTA:
        i = 1
        j = 1
        for v2 in net.vehiclesSTA:
            if v1 != v2: 
                v1.cmd('route add -host 192.168.1.%s gw 10.0.0.%s' % (j,i))
            i+=1
            j+=2

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
