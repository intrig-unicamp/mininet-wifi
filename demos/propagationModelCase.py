#!/usr/bin/python

"""
Propagation Model Demo
"""

from mininet.net import Mininet
from mininet.node import  Controller, OVSKernelSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink
import time

def topology():
    "Create a network."
    net = Mininet( controller=Controller, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    ap1 = net.addAccessPoint( 'ap1', ssid="ssid_ap1", txpower=15, mode="g", channel=1, position="10,10,0" )
    sta1 = net.addStation( 'sta1', ip='192.168.0.1/24', txpower=15, position='10,10,0' )
    sta2 = net.addStation( 'sta2', ip='192.168.0.2/24', txpower=15, position='11.36,10,0' )

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Adding Link"
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap1)

    "*** Available propagation models: friisPropagationLossModel, twoRayGroundPropagationLossModel, logDistancePropagationLossModel ***"
    net.propagationModel('logDistancePropagationLossModel', exp=3, sL=1)
    #net.propagationModel('ITUPropagationLossModel', pL=50)
    #net.propagationModel('twoRayGroundPropagationLossModel')
    #net.propagationModel('friisPropagationLossModel', sL=2)

    print "*** Starting network"
    net.build()

    for i in range(1,8):
        x = 10+i*1.36
 	y = 10
	z = 0
        pos = '%s,%s,%s' % (x,y,z)
        sta1.moveStationTo(pos)
        print sta1.params['rssi'][0] 

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
