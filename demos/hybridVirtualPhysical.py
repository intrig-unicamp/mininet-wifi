#!/usr/bin/python

"""Code created to be presented with the paper titled:
   "Rich Experimentation through Hybrid Physical-Virtual Software-Defined Wireless Networking Emulation" 
   authors: Ramon dos Reis Fontes and Christian Esteve Rothenberg"""

from mininet.net import Mininet
from mininet.node import RemoteController,OVSKernelSwitch, Controller, UserSwitch
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.node import Node
from mininet.log import setLogLevel
import os
import time

def topology():

    "Create a network."
    net = Mininet( controller=RemoteController, link=TCLink, switch=UserSwitch )
    sta = []

    print "*** Creating nodes"
    for n in range(10):
	sta.append(n)
	sta[n] = net.addStation( 'sta%s' % (n+1), wlans=2, mac='00:00:00:00:00:%s' % (n+1), ip='192.168.0.%s/24' % (n+1) )
    phyap1 = net.addPhysicalBaseStation( 'phyap1', protocols='OpenFlow13', ssid= 'ap-ssid1', mode= 'g', channel= '1', position='50,115,0', wlan='wlan11' )
    ap2 = net.addBaseStation( 'ap2', protocols='OpenFlow13', ssid= 'ap-ssid2', mode= 'g', channel= '11', position='100,175,0' )
    ap3 = net.addBaseStation( 'ap3', protocols='OpenFlow13', ssid= 'ap-ssid3', mode= 'g', channel= '6', position='150,115,0' )
    ap4 = net.addBaseStation( 'ap4', protocols='OpenFlow13', ssid= 'ap-ssid4', mode= 'g', channel= '6', position='100,55,0' )
    sta11 = net.addStation( 'sta11', ip='10.0.0.111/8', position='120,200,0')
    c1 = net.addController( 'c1', controller=RemoteController, port=6653 )
    root = Node( 'root', inNamespace=False )

    print "*** Creating links"
    for station in sta:
        net.addMesh(station, ssid='meshNet')

    """uncomment to plot graph"""
    net.plotGraph(max_x=240, max_y=240)

    """Routing"""
    net.meshRouting('custom')

    """Seed"""
    net.seed(20)

    print "*** Associating and Creating links"
    net.addLink(phyap1, ap2)
    net.addLink(ap2, ap3)
    net.addLink(sta11, ap2)
    net.addLink(ap3, ap4)
    net.addLink(ap4, phyap1)
    link = net.addLink( root, ap3 )
    link.intf1.setIP( '10.254', 8 )

    """Adding datapath"""
    net.addOfDataPath('ap3', 'wlan0')

    print "*** Starting network"
    net.build()

    c1.start()
    phyap1.start( [c1] )
    ap2.start( [c1] )
    ap3.start( [c1] )
    ap4.start( [c1] )

    time.sleep(2)
    """output=all,flood"""
    ap3.cmd('dpctl unix:/tmp/ap3 meter-mod cmd=add,flags=1,meter=1 drop:rate=100')
    ap3.cmd('dpctl unix:/tmp/ap3 flow-mod table=0,cmd=add in_port=4,eth_type=0x800,ip_dst=10.0.0.100, meter:1 apply:output=flood')

    startNAT( root )
    sta11.cmd('ip route add default via 10.0.0.254')
    sta11.cmd('pushd /homt/alpha; python3 -m http.server 80 &')

    ip = 201
    for station in sta:
        station.cmd('ifconfig %s-wlan1 10.0.0.%s/8' % (station, ip))
        station.cmd('ip route add default via 10.0.0.254')
        ip+=1

    "*** Available models: RandomWalk, TruncatedLevyWalk, RandomDirection, RandomWayPoint, GaussMarkov, ReferencePoint, TimeVariantCommunity ***"
    net.startMobility(startTime=0, model='RandomWalk', max_x=200, max_y=220, min_v=0.1, max_v=0.2)

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

def startNAT( root, inetIntf='wlan0', subnet='10.0/8', localIntf = None ):
    """Start NAT/forwarding between Mininet and external network
    root: node to access iptables from
    inetIntf: interface for internet access
    subnet: Mininet subnet (default 10.0/8)"""

    # Identify the interface connecting to the mininet network
    if localIntf == None:
        localIntf =  root.defaultIntf()
 
    # Flush any currently active rules
    root.cmd( 'iptables -F' )
    root.cmd( 'iptables -t nat -F' )

    # Create default entries for unmatched traffic
    root.cmd( 'iptables -P INPUT ACCEPT' )
    root.cmd( 'iptables -P OUTPUT ACCEPT' )
    root.cmd( 'iptables -P FORWARD DROP' )

    # Configure NAT
    root.cmd( 'iptables -I FORWARD -i', localIntf, '-d', subnet, '-j DROP' )
    root.cmd( 'iptables -A FORWARD -i', localIntf, '-s', subnet, '-j ACCEPT' )
    root.cmd( 'iptables -A FORWARD -i', inetIntf, '-d', subnet, '-j ACCEPT' )
    root.cmdPrint( 'iptables -t nat -A POSTROUTING -o ', inetIntf, '-j MASQUERADE' )

    # Instruct the kernel to perform forwarding
    root.cmd( 'sysctl net.ipv4.ip_forward=1' )

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
