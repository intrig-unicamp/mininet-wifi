#!/usr/bin/python

"""
Demo for VANETs. It still has to be customized.. 
"""

from mininet.net import Mininet
from mininet.node import Controller,OVSKernelSwitch, RemoteController
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.wmediumdConnector import WmediumdStarter, DynamicWmediumdIntfRef, WmediumdLink


class InbandController( RemoteController ):

    def checkListening( self ):
        "Overridden to do nothing."
        return

def topology():

    "Create a network."
    net = Mininet( controller=Controller, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    car = []
    stas = []
    for x in range(0, 4):
        car.append(x)
        stas.append(x)
    for x in range(0, 4):
        car[x] = net.addCar('car%s' % (x), wlans=2, ip='10.0.0.%s/8' % (x + 1), \
        mac='00:00:00:00:00:0%s' % x, mode='b', position='%d,%d,0' % ((150-(x*20)),(100-(x*20))) )

    rs1 = net.addAccessPoint( 'rs1', ssid='ssid1', mode= 'g', channel= '1', position='60,50,0' )
    rs2 = net.addAccessPoint( 'rs2', ssid='ssid2', mode= 'g', channel= '6', position='160,100,0' )
    bs3 = net.addAccessPoint( 'bs3', ssid='ssid3', mode= 'g', channel= '6', position='150,120,0', range=70 )
    c1 = net.addController( 'c1', controller=Controller )
    h1 = net.addHost ( 'h1' )
    s10 = net.addSwitch ( 's10' )

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Creating links"
    net.addLink(rs1, s10)
    net.addLink(rs2, s10)
    net.addLink(bs3, s10)
    net.addLink(s10, h1)
    net.addLink(bs3, car[0])
    net.addLink(rs2, car[0])
    net.addLink(rs1, car[3])

    print "*** Starting network"
    net.build()
    c1.start()
    rs1.start( [c1] )
    rs2.start( [c1] )
    bs3.start( [c1] )
    s10.start( [c1] )

    i = 201
    for sw in net.vehicles:
        sw.start([c1])

    i = 1
    j = 2
    for c in car:
        c.cmd('ifconfig %s-wlan0 192.168.0.%s/24 up' % (c,i))
        c.cmd('ifconfig %s-eth0 192.168.1.%s/24 up' % (c,i))
        c.cmd('ip route add 10.0.0.0/8 via 192.168.1.%s' % j)
        i+=2
        j+=2

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

    h1.cmd('ifconfig h1-eth0 200.0.10.2')
    h1.cmd('ifconfig h1-eth0:0 200.1.10.2')
    net.vehiclesSTA[3].cmd('ifconfig car3STA-eth1 200.0.10.1')
    net.vehiclesSTA[0].cmd('ifconfig car0STA-eth0 200.1.10.50')

    car[0].cmd('modprobe bonding mode=3')
    car[0].cmd('ip link add bond0 type bond')
    car[0].cmd('ip link set bond0 address 02:01:02:03:04:08')
    car[0].cmd('ip link set car0-eth0 down')
    car[0].cmd('ip link set car0-eth0 address 00:00:00:00:00:11')
    car[0].cmd('ip link set car0-eth0 master bond0')
    car[0].cmd('ip link set car0-wlan0 down')
    car[0].cmd('ip link set car0-wlan0 address 00:00:00:00:00:15')
    car[0].cmd('ip link set car0-wlan0 master bond0')
    car[0].cmd('ip link set car0-wlan1 down')
    car[0].cmd('ip link set car0-wlan1 address 00:00:00:00:00:13')
    car[0].cmd('ip link set car0-wlan1 master bond0')
    car[0].cmd('ip addr add 200.0.10.100/24 dev bond0')
    car[0].cmd('ip link set bond0 up')

    car[0].cmd('ifconfig bond0:0 200.1.10.100')
    car[3].cmd('ifconfig car3-wlan0 200.1.10.150')

    h1.cmd('ip route add 192.168.1.8 via 200.1.10.150')
    h1.cmd('ip route add 10.0.0.1 via 200.1.10.150')
    h1.cmd('ip route add 200.1.10.100 via 200.1.10.150')

    net.vehiclesSTA[3].cmd('ip route add 200.1.10.2 via 192.168.1.7')
    net.vehiclesSTA[3].cmd('ip route add 200.1.10.100 via 10.0.0.1')
    net.vehiclesSTA[0].cmd('ip route add 200.1.10.2 via 10.0.0.4')

    car[0].cmd('ip route add 10.0.0.4 via 200.1.10.50')
    car[0].cmd('ip route add 192.168.1.7 via 200.1.10.50')
    car[0].cmd('ip route add 200.1.10.2 via 200.1.10.50')
    car[3].cmd('ip route add 200.1.10.100 via 192.168.1.8')


    """uncomment to plot graph"""
    net.plotGraph(max_x=250, max_y=250)

    mp1 = net.vehiclesSTA[0]
    mp2 = net.vehiclesSTA[1]
    mp3 = net.vehiclesSTA[2]
    mp4 = net.vehiclesSTA[3]

    print "*** Configure wmediumd"
    sta1wlan0 = DynamicWmediumdIntfRef(mp1)
    sta2wlan0 = DynamicWmediumdIntfRef(mp2)
    sta3wlan0 = DynamicWmediumdIntfRef(mp3)
    sta4wlan0 = DynamicWmediumdIntfRef(mp4)

    intfrefs = [sta1wlan0, sta2wlan0, sta3wlan0, sta4wlan0]
    links = [
        WmediumdLink(sta1wlan0, sta2wlan0, 15),
        WmediumdLink(sta2wlan0, sta1wlan0, 15),
        WmediumdLink(sta2wlan0, sta3wlan0, 15),
        WmediumdLink(sta3wlan0, sta2wlan0, 15),
        WmediumdLink(sta3wlan0, sta4wlan0, 15),
        WmediumdLink(sta4wlan0, sta3wlan0, 15)]
    #WmediumdStarter.initialize(intfrefs, links)

    #WmediumdStarter.start()

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping wmediumd"
    #WmediumdStarter.stop()

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
