#!/usr/bin/python

"""
   Topology:  ap1<---->sta1<---->sta2
"""

from mininet.wifi import station
from mininet.net import Mininet
from mininet.node import  OVSKernelSwitch, Controller
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink


def topology():
    "Create a network."
    net = Mininet( wirelessRadios=8, controller=Controller, link=TCLink, switch=OVSKernelSwitch )
    #wirelessRadios = number of ap + number of STAs and its interfaces

    print "*** Creating nodes"
    h1 = net.addHost ( 'h1' )
    h2 = net.addHost ( 'h2' )
    h3 = net.addHost ( 'h3' )
    ap1 = net.addBaseStation( 'ap1', ssid="ssid_1", mode="g", channel="5" )
    sta1 = net.addStation( 'sta1', ip="192.168.0.100" )
    sta2 = net.addStation( 'sta2' )
    c0 = net.addController('c0', controller=Controller)

    print "*** Associating..."
    net.addLink(h1, h2)
    net.addLink(ap1, sta1) #Automaticaly add interface wlan0 in sta1
    net.addLink(ap1, sta2) #Automaticaly add interface wlan0 in sta1
    net.addLink(h3, h2)
    net.addLink(h1, sta2)
    net.addLink(sta2, h3)
    net.addLink(sta2, sta1)

    print "*** Starting network"
    net.build()
    c0.start()
    ap1.start( [c0] )

    print "*** Adding new Interfaces to STAs"
    station.addWlan(sta1) # add wlan1 in sta1
    station.addWlan(sta1) # add wlan2 (unused) in sta1
    station.addWlan(sta2) # add wlan1 in sta2
    station.addWlan(sta2) # add wlan2 (unused) in sta2
    station.addWlan(sta2) # add wlan3 (unused) in sta2

    print "***Creating adhoc network"
    net.addHoc(sta1, 'adhoc1', 'g', interface='wlan1')
    net.addHoc(sta2, 'adhoc1', 'g', interface='wlan1')
    sta1.cmd('ifconfig sta1-wlan1 192.168.10.1')
    sta2.cmd('ifconfig sta2-wlan1 192.168.10.2')

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()

