#!/usr/bin/python

"""
Script created by VND - Visual Network Description (SDN version)
"""
from mininet.net import Mininet
from mininet.node import Controller, RemoteController, OVSKernelSwitch, UserSwitch
from mininet.cli import CLI
from mininet.link import Intf
from mininet.log import setLogLevel
from mininet.link import Link, TCLink

def topology():
    "Create a network."
    net = Mininet( controller=RemoteController, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    h1 = net.addHost( 'h1', mac='00:00:00:00:00:01', ip='192.168.122.11/24' )
    h2 = net.addHost( 'h2', mac='00:00:00:00:00:02', ip='192.168.122.12/24' )
    h3 = net.addHost( 'h3', mac='00:00:00:00:00:03', ip='192.168.122.13/24' )
    h4 = net.addHost( 'h4', mac='00:00:00:00:00:04', ip='192.168.122.14/24' )
    s1 = net.addSwitch( 's1', listenPort=6673, mac='00:00:00:00:00:05' )
    s2 = net.addSwitch( 's2', listenPort=6674, mac='00:00:00:00:00:06' )
    s3 = net.addSwitch( 's3', listenPort=6675, mac='00:00:00:00:00:07' )
    s4 = net.addSwitch( 's4', listenPort=6676, mac='00:00:00:00:00:08' )
    s5 = net.addSwitch( 's5', listenPort=6677, mac='00:00:00:00:00:09' )
    s6 = net.addSwitch( 's6', listenPort=6678, mac='00:00:00:00:00:10' )

    ha1 = net.addHost( 'ha1', mac='00:00:00:00:00:20', ip='172.160.47/16' )
    ha2 = net.addHost( 'ha2', mac='00:00:00:00:00:21', ip='172.17.1.2/16' )
    hb1 = net.addHost( 'hb1', mac='00:00:00:00:00:22', ip='172.17.1.3/16' )
    hb2 = net.addHost( 'hb2', mac='00:00:00:00:00:23', ip='172.17.1.4/16' )

#    c0 = net.addController( 'c0', controller=RemoteController, ip='172.17.0.3', port=6633 )
 #   c0 = net.addController( 'c0', controller=RemoteController, ip='172.16.0.47', port=6633 )
    c0 = net.addController('c0', controller=Controller, port=6633 )

    print "*** Creating links"
    net.addLink(s6, hb2, 3, 0,bw=100)
    net.addLink(s6, hb1, 2, 0,bw=100)
    net.addLink(s5, ha2, 3, 0,bw=100)
    net.addLink(ha1, s5, 0, 2,bw=100)
    net.addLink(s6, s2, 1, 5,bw=100,delay='5ms')
    net.addLink(s2, s5, 4, 1,bw=100,delay='5ms')
    net.addLink(s4, h4, 2, 0,bw=100)
    net.addLink(s3, s4, 3, 1,bw=100,delay='5ms')
    net.addLink(s3, h3, 2, 0,bw=100)
    net.addLink(s2, s3, 3, 1,bw=100,delay='5ms')
    net.addLink(s2, h2, 2, 0,bw=100)
    net.addLink(s1, s2, 2, 1,bw=100,delay='5ms')
    net.addLink(s1, h1, 1, 0,bw=100)

    print "*** Starting network"
    net.build()
    c0.start()
    s1.start( [c0] )
    s2.start( [c0] )
    s3.start( [c0] )
    s4.start( [c0] )
    s5.start( [c0] )
    s6.start( [c0] )

    s1.cmd('ovs-vsctl add-port s1 eth1')

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()


