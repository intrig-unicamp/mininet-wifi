#!/usr/bin/python

'This example runs stations in AP mode'

from mininet.net import Mininet
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.link import TCLink
import sys

net = Mininet(link=TCLink, enable_wmediumd=True, enable_interference=True)

def topology(mobility):

	"Create a network."
	if mobility:
		sta1 = net.addStation('sta1', mac='00:00:00:00:00:01', ip='10.0.0.1/8')
	else:
		sta1 = net.addStation('sta1', mac='00:00:00:00:00:01', ip='10.0.0.1/8', position='20,60,0')
	net.addStation('sta2', mac='00:00:00:00:00:02', ip='10.0.0.2/8', position='80,60,0')
	ap1 = net.addStation('ap1', type='ap', mac='02:00:00:00:01:00', ip='10.0.0.11/8', ssid="apadhoc-ssid1", mode="g", channel="1", position='40,60,0')
	ap2 = net.addStation('ap2', type='ap', mac='02:00:00:00:02:00', ip='10.0.0.12/8', ssid="apadhoc-ssid2", mode="g", channel="6", position='70,60,0')

	net.propagationModel("logDistancePropagationLossModel", exp=4.5)

	print "*** Configuring wifi nodes"
	net.configureWifiNodes()

	print "*** Adding Link"
	net.addLink(ap1, ap2)  # wired connection

	net.plotGraph(max_x=120, max_y=120)

	if mobility:
		net.startMobility(time=1)
		net.mobility(sta1, 'start', time=1, position='20.0,60.0,0.0')
		net.mobility(sta1, 'stop', time=5, position='90.0,60.0,0.0')
		net.stopMobility(time=6)
	else:
		pass

	print "*** Starting network"
	net.build()

	print "*** Running CLI"
	CLI(net)

	print "*** Stopping network"
	net.stop()

if __name__ == '__main__':
	setLogLevel('info')
	mobility = True if '-m' in sys.argv else False
	topology(mobility)
