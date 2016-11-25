#!/usr/bin/python

"""
Working yet..
"""
import subprocess
import os

from mininet.net import Mininet
from mininet.node import  RemoteController, OVSKernelSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink

"""
         ap1.
         /    .
        /       .
h1----s1        sta1
       \       .
        \    .
        ap2.

"""

def topology():
    "Create a network."
    net = Mininet( controller=RemoteController, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    ap1 = net.addAccessPoint( 'ap1', ssid="ssid_1", mode="g", channel="5" )
    ap2 = net.addAccessPoint( 'ap2', ssid="ssid_2", mode="g", channel="11" )
    sta3 = net.addStation( 'sta3', ip="192.168.0.100/24", wlans=2 )
    h4 = net.addHost( 'h4', ip="192.168.0.1/24", mac="00:00:00:00:00:04" )
    s5 = net.addSwitch( 's5' )
    c0 = net.addController('c0', controller=RemoteController, ip='127.0.0.1', port=6653 )

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Adding Link"
    net.addLink(h4, s5, bw=1000)
    net.addLink(ap1, s5, bw=1000)
    net.addLink(ap2, s5, bw=1000)
    net.addLink(ap1, sta3)
    net.addLink(sta3, ap2)

    print "*** Starting network"
    net.build()
    c0.start()
    s5.start( [c0] )
    ap1.start( [c0] )
    ap2.start( [c0] )

    sta3.cmd("ifconfig sta3-wlan1 192.168.1.100/24 up")
    h4.cmd("ifconfig h4-eth0:0 192.168.1.1/24")

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
