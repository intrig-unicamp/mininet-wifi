#!/usr/bin/python

""" Handover example supported by bgscan (Background scanning) and wmediumd.

ieee 802.11r can be enabled adding the parameters below:

ieee80211r='yes'
mobility_domain='a1b2'

e.g. ap1 = net.addAccessPoint('ap1', ..., ieee80211r='yes', mobility_domain='a1b2',...)"""

from mininet.net import Mininet
from mininet.node import Controller, UserAP
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel

def topology():

    "Create a network."
    net = Mininet(controller=Controller, link=TCLink, accessPoint=UserAP, enable_wmediumd=True, enable_interference=True)

    print "*** Creating nodes"
    net.addStation('sta1', position='15,20,0')
    ap1 = net.addAccessPoint('ap1', mac='00:00:00:00:00:01', range=70, ssid="handover", mode="g", channel="1", passwd='123456789a', encrypt='wpa2', position='15,30,0')
    ap2 = net.addAccessPoint('ap2', mac='00:00:00:00:00:02', range=70, ssid="handover", mode="g", channel="6", passwd='123456789a', encrypt='wpa2', position='55,30,0')
    ap3 = net.addAccessPoint('ap3', mac='00:00:00:00:00:03', range=70, ssid="handover", mode="g", channel="1", passwd='123456789a', encrypt='wpa2', position='100,100,0')
    c1 = net.addController('c1', controller=Controller)

    print "*** Configuring Propagation Model"
    net.propagationModel("logDistancePropagationLossModel", exp=3.5)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Creating links"
    net.addLink(ap1, ap2)
    net.addLink(ap2, ap3)

    print "*** Setting bgscan"
    net.setBgscan(signal=-45, s_inverval=5, l_interval=10)

    print "*** Starting network"
    net.build()
    c1.start()
    ap1.start([c1])
    ap2.start([c1])
    ap3.start([c1])

    """uncomment to plot graph"""
    net.plotGraph(max_x=150, max_y=150)

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel('info')
    topology()
