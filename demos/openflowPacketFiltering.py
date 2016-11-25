#!/usr/bin/python

"""
Author: Chih-Heng Ke, smallko@gmail.com

This code serves as alternative if you want to filter OpenFlow Packets when
transmitter and receives are associated to the same AP. 
"""

from mininet.net import Mininet
from mininet.node import Controller, RemoteController, OVSKernelSwitch, IVSSwitch, UserSwitch
from mininet.link import Link, TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel

def topology():

    "Create a network."
    net = Mininet( controller=RemoteController, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    ap1 = net.addAccessPoint( 'ap1', ssid= 'new-ssid', mode= 'g', channel= '1' )
    sta2 = net.addStation( 'sta2', wlans=1, mac='00:02:00:00:00:02', ip='10.0.0.2/8' )
    sta3 = net.addStation( 'sta3', wlans=1, mac='00:02:00:00:00:03', ip='10.0.0.3/8' )
    sta4 = net.addStation( 'sta4', wlans=1, mac='00:02:00:00:00:04', ip='10.0.0.4/8' )
    c5 = net.addController( 'c5', ip='127.0.0.1', port=6633 )
    h7 = net.addHost( 'h7', mac='00:00:00:00:00:07', ip='10.0.0.7/8' )

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()
    
    print "*** Creating links"
    net.addLink(sta4, ap1)
    net.addLink(sta3, ap1)
    net.addLink(sta2, ap1)
    net.addLink(ap1, h7)

    print "*** Starting network"
    net.build()
    c5.start()
    ap1.start( [c5] )
   
    ap1.cmd("iw dev ap1-wlan0 interface add vwlan1 type managed")   
    ap1.cmd("iw dev ap1-wlan0 interface add vwlan2 type managed")
    ap1.cmd("ifconfig vwlan1 hw ether 00:00:00:aa:bb:11")
    ap1.cmd("ifconfig vwlan2 hw ether 00:00:00:aa:bb:22")
    ap1.cmd("ifconfig vwlan1 up")
    ap1.cmd("ifconfig vwlan2 up")
    ap1.cmd("ovs-vsctl add-port ap1 vwlan1")
    ap1.cmd("ovs-vsctl add-port ap1 vwlan2")
    ap1.cmd("echo -e 'interface=vwlan1\ndriver=nl80211\nssid=vwlan1\nhw_mode=g\nchannel=1\nwme_enabled=1\nwmm_enabled=1' > vwlan1.conf")
    ap1.cmd("hostapd -B vwlan1.conf &")
    ap1.cmd("echo -e 'interface=vwlan2\ndriver=nl80211\nssid=vwlan2\nhw_mode=g\nchannel=1\nwme_enabled=1\nwmm_enabled=1' > vwlan2.conf")
    ap1.cmd("hostapd -B vwlan2.conf &")
  
    sta2.cmd("ifconfig sta2-wlan0 down")
    sta2.cmd("iwconfig sta2-wlan0 essid 'vwlan1'")
    sta2.cmd("ifconfig sta2-wlan0 up")

    sta3.cmd("ifconfig sta3-wlan0 down")
    sta3.cmd("iwconfig sta3-wlan0 essid 'vwlan2'")
    sta3.cmd("ifconfig sta3-wlan0 up")

    ### after the above setting, sta2( sta3, or sta4) can ping h7. sta2 can also ping sta3/sta4.

    #the following rule can block sta2 from communicating with other host or station
    ap1.cmd("ovs-ofctl add-flow ap1 priority=65535,ip,nw_dst=10.0.0.2,actions=drop")

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
