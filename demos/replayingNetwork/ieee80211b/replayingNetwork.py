#!/usr/bin/python

"""
Replaying Network Conditions
"""

from mininet.net import Mininet
from mininet.node import Controller,OVSKernelSwitch
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.wifiReplaying import replayingNetworkBehavior

def topology():

    "Create a network."
    net = Mininet( controller=Controller, link=TCLink, switch=OVSKernelSwitch )

    print "*** Creating nodes"
    sta1 = net.addStation( 'sta1', mac='00:00:00:00:00:01', ip='192.168.0.1/24', position='47.28,50,0' )
    sta2 = net.addStation( 'sta2', mac='00:00:00:00:00:02', ip='192.168.0.2/24', position='54.08,50,0' )
    ap3 = net.addBaseStation( 'ap3', range=20, ssid='ap-ssid3', mode= 'b', channel= '1', position='50,50,0' )
    c4 = net.addController( 'c4', controller=Controller, port=6653 )

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Starting network"
    net.build()
    c4.start()
    ap3.start( [c4] )

    sta1.cmd('iw dev sta1-wlan0 interface add mon0 type monitor &')
    sta1.cmd('ifconfig mon0 up &')
    sta2.cmd('iw dev sta2-wlan0 interface add mon0 type monitor &')
    sta2.cmd('ifconfig mon0 up &')
    sta2.cmd('pushd /home/alpha/Downloads; python3 -m http.server 80 &')

    getTrace(sta1, 'clientTrace.txt')
    getTrace(sta2, 'serverTrace.txt')

    replayingNetworkBehavior.addNode(sta1)
    replayingNetworkBehavior.addNode(sta2)
    replayingNetworkBehavior(net)
   
    net.autoAssociation()

    print "*** Running CLI"
    CLI( net )

    print "*** Stopping network"
    net.stop()

def getTrace(sta, file):

    file = open(file, 'r')
    raw_data = file.readlines()
    file.close()

    sta.time = [] 
    sta.bw = [] 
    sta.loss = [] 
    sta.delay = [] 
    sta.latency = [] 
  
    for data in raw_data:
        line = data.split()
        sta.time.append(float(line[0])) #First Column = Time
        sta.bw.append(((float(line[1]))/1000000)/2) #Second Column = BW
        #sta.loss.append(1) #Second Column = LOSS
        sta.loss.append(float(line[2])) #second Column = LOSS
        sta.latency.append(float(line[3])) #Second Column = LATENCY
        sta.delay.append(float(line[4])) #Second Column = DELAY

if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
