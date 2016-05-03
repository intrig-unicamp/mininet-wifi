#!/usr/bin/python

"""Code created to be presented with the paper titled:
   "Rich Experimentation through Hybrid Physical-Virtual Software-Defined Wireless Networking Emulation" 
   authors: Ramon dos Reis Fontes and Christian Esteve Rothenberg"""

from mininet.net import Mininet
from mininet.node import RemoteController,OVSKernelSwitch, Controller, UserSwitch
from mininet.link import TCLink
from mininet.cli import CLI
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
	sta[n] = net.addStation( 'sta%s' % (n+1), wlans=2, mac='00:00:00:00:00:%s' % (n+1), ip='192.168.0.%s/8' % (n+1) )
    phyap1 = net.addPhysicalBaseStation( 'phyap1', protocols='OpenFlow13', ssid= 'ap-ssid1', mode= 'g', channel= '1', position='50,115,0', wlan='wlan0' )
    ap2 = net.addBaseStation( 'ap2', protocols='OpenFlow13', ssid= 'ap-ssid2', mode= 'g', channel= '11', position='100,175,0' )
    ap3 = net.addBaseStation( 'ap3', protocols='OpenFlow13', ssid= 'ap-ssid3', mode= 'g', channel= '6', position='150,115,0' )
    ap4 = net.addBaseStation( 'ap4', protocols='OpenFlow13', ssid= 'ap-ssid4', mode= 'g', channel= '6', position='100,55,0' )
    sta11 = net.addStation( 'sta11', ip='10.0.2.111', position='120,200,0')
    c1 = net.addController( 'c1', controller=RemoteController, port=6653 )


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

    """Adding datapath"""
    net.addOfDataPath('ap3', 'enp0s3')

    print "*** Starting network"
    net.build()

    c1.start()
    phyap1.start( [c1] )
    ap2.start( [c1] )
    ap3.start( [c1] )
    ap4.start( [c1] )

    time.sleep(2)
    ap3.cmd('dpctl unix:/tmp/ap3 meter-mod cmd=add,flags=1,meter=1 drop:rate=1000')
    #ap3.cmd('dpctl unix:/tmp/ap3 meter-mod cmd=add,flags=1,meter=2 drop:rate=2000')
    #ap3.cmd('dpctl unix:/tmp/ap3 flow-mod table=0,cmd=add in_port=2,eth_type=0x800, meter:1 apply:output=4')
    #ap3.cmd('dpctl unix:/tmp/ap3 flow-mod table=0,cmd=add in_port=4, meter:1 apply:output=flood')
    """output=all,flood"""
    ap3.cmd('dpctl unix:/tmp/ap3 flow-mod table=0,cmd=add in_port=4,eth_type=0x800,ip_dst=10.0.2.111, meter:1 apply:output=flood')
    #ap3.cmd('dpctl unix:/tmp/ap3 flow-mod table=0,cmd=add in_port=4,eth_type=0x800,ip_dst=10.0.2.25, meter:2 apply:output=flood')

    sta11.cmd('ip route add default via 10.0.2.2')
    sta11.cmd('pushd /homt/alpha; python3 -m http.server 80 &')

    ip = 101
    for station in sta:
        station.cmd('ifconfig %s-wlan1 10.0.2.%s' % (station, ip))
        station.cmd('ip route add default via 10.0.2.2')

        station.cmd('iptables -A FORWARD -i %s-wlan1 -o %s-mp0 -j ACCEPT' % (station,station))
        station.cmd('iptables -A FORWARD -i %s-mp0 -o %s-wlan1 -m state --state ESTABLISHED,RELATED -j ACCEPT' % (station, station))
        station.cmd('iptables -t nat -A POSTROUTING -o %s-mp0 -j MASQUERADE' % station)
        station.cmd('sysctl -w net.ipv4.ip_forward=1')

        if str(station) == 'sta11':
            station.cmd('route add -net 192.168.0.0/24 dev %s-wlan1' % station)

        if str(station) == 'sta11':
            for n in sta:
                station.pexec('route add -net 192.168.0.0/24 gw 192.168.0.%s' % ip)
        ip+=1

    "*** Available models: RandomWalk, TruncatedLevyWalk, RandomDirection, RandomWayPoint, GaussMarkov, ReferencePoint, TimeVariantCommunity ***"
    net.startMobility(startTime=0, model='RandomWalk', max_x=200, max_y=220, min_v=0.1, max_v=0.2)

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
