#!/usr/bin/python

"Replaying Bandwidth"

from mininet.node import Controller,OVSKernelSwitch
from mininet.link import TCLink
from mininet.log import setLogLevel, info
from mn_wifi.replaying import replayingBandwidth
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi


def topology():
    "Create a network."
    net = Mininet_wifi( controller=Controller, link=TCLink, switch=OVSKernelSwitch )

    info("*** Creating nodes\n")
    sta1 = net.addStation( 'sta1', mac='00:00:00:00:00:02', ip='10.0.0.2/8' )
    ap1 = net.addAccessPoint( 'ap1', ssid='new-ssid', mode='g', channel='1',
                              position='50,50,0' )
    c1 = net.addController( 'c1', controller=Controller )

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    info("*** adding Link\n")
    net.addLink(sta1, ap1)

    info("*** Starting network\n")
    net.build()
    c1.start()
    ap1.start( [c1] )

    net.plotGraph(max_x=100, max_y=100)

    getTrace(sta1, 'examples/replaying/replayingBandwidth/throughputData.dat')

    replayingBandwidth(net)

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()

def getTrace(sta, file):

    file = open(file, 'r')
    raw_data = file.readlines()
    file.close()

    sta.time = []
    sta.throughput = []

    for data in raw_data:
        line = data.split()
        sta.time.append(float(line[0])) #First Column = Time
        sta.throughput.append(float(line[1])) #Second Column = Throughput


if __name__ == '__main__':
    setLogLevel( 'info' )
    topology()