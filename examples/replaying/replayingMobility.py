#!/usr/bin/python

'Replaying Mobility'
import os

from mininet.node import Controller
from mininet.log import setLogLevel, info
from mn_wifi.replaying import replayingMobility
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.link import wmediumd, adhoc
from mn_wifi.wmediumdConnector import interference


def topology():
    "Create a network."
    net = Mininet_wifi(controller=Controller, link=wmediumd,
                       wmediumd_mode=interference)

    info("*** Creating nodes\n")
    sta1 = net.addStation('sta1', mac='00:00:00:00:00:02',
                          ip='10.0.0.1/8', speed=4)
    sta2 = net.addStation('sta2', mac='00:00:00:00:00:03',
                          ip='10.0.0.2/8', speed=6)
    sta3 = net.addStation('sta3', mac='00:00:00:00:00:04',
                          ip='10.0.0.3/8', speed=3)
    sta4 = net.addStation('sta4', mac='00:00:00:00:00:05',
                          ip='10.0.0.4/8', speed=3)
    ap1 = net.addAccessPoint('ap1', ssid='new-ssid',
                             mode='g', channel='1',
                             position='45,45,0')
    c1 = net.addController('c1', controller=Controller)

    info("*** Configuring Propagation Model\n")
    net.setPropagationModel(model="logDistance", exp=4.5)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    info("*** Creating links\n")
    net.addLink(sta3, cls=adhoc, intf='sta3-wlan0', ssid='adhocNet')
    net.addLink(sta4, cls=adhoc, intf='sta4-wlan0', ssid='adhocNet')

    path = os.path.dirname(os.path.abspath(__file__))
    getTrace(sta1, '%s/replayingMobility/node1.dat' % path, net)
    getTrace(sta2, '%s/replayingMobility/node2.dat' % path, net)
    getTrace(sta3, '%s/replayingMobility/node3.dat' % path, net)
    getTrace(sta4, '%s/replayingMobility/node4.dat' % path, net)

    'ploting graph'
    net.plotGraph(max_x=200, max_y=200)

    info("*** Starting network\n")
    net.build()
    c1.start()
    ap1.start([c1])

    replayingMobility(net)

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()

def getTrace(sta, file_, net):

    net.isReplaying = True
    file_ = open(file_, 'r')
    raw_data = file_.readlines()
    file_.close()

    sta.position = []
    pos = '-1000,0,0'
    sta.params['position'] = [float(x) for x in pos.split(',')]

    for data in raw_data:
        line = data.split()
        x = line[0]  # First Column
        y = line[1]  # Second Column
        pos = float(x),float(y),0.0
        sta.position.append(pos)


if __name__ == '__main__':
    setLogLevel('info')
    topology()
