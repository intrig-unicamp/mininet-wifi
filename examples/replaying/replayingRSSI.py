#!/usr/bin/python

"Replaying RSSI"

from mininet.node import Controller,OVSKernelSwitch
from mininet.log import setLogLevel, info
from mn_wifi.replaying import replayingRSSI
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi


def topology():
    "Create a network."
    net = Mininet_wifi( controller=Controller, switch=OVSKernelSwitch )

    info("*** Creating nodes\n")
    sta1 = net.addStation( 'sta1', mac='00:00:00:00:00:02', ip='10.0.0.2/8' )
    sta2 = net.addStation( 'sta2', mac='00:00:00:00:00:03', ip='10.0.0.3/8' )
    ap1 = net.addAccessPoint( 'ap1', ssid='new-ssid', mode='g', channel='1',
                              position='50,50,0' )
    c1 = net.addController( 'c1', controller=Controller )

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    info("*** Adding Link\n")
    sta1.params['associatedTo'][0] = ap1
    sta2.params['associatedTo'][0] = ap1

    info("*** Starting network\n")
    net.build()
    c1.start()
    ap1.start( [c1] )

    net.plotGraph(max_x=100, max_y=100)

    getTrace(sta1, 'examples/replaying/replayingRSSI/node1_rssiData.dat')
    getTrace(sta2, 'examples/replaying/replayingRSSI/node2_rssiData.dat')

    replayingRSSI(net)

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()

def getTrace(sta, file):

    file = open(file, 'r')
    raw_data = file.readlines()
    file.close()

    sta.time = []
    sta.rssi = []

    for data in raw_data:
        line = data.split()
        sta.time.append(float(line[0])) #First Column = Time
        sta.rssi.append(float(line[1])) #Second Column = RSSI


if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()
