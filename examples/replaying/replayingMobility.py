#!/usr/bin/python

'Replaying Mobility'

from mininet.net import Mininet
from mininet.node import Controller, OVSAP
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.wifiReplaying import replayingMobility

def topology():

    "Create a network."
    net = Mininet(controller=Controller, link=TCLink, accessPoint=OVSAP,
                  enable_wmediumd=True, enable_interference=True)

    print "*** Creating nodes"
    sta1 = net.addStation('sta1', mac='00:00:00:00:00:02', ip='10.0.0.1/8', speed=4)
    sta2 = net.addStation('sta2', mac='00:00:00:00:00:03', ip='10.0.0.2/8', speed=6)
    sta3 = net.addStation('sta3', mac='00:00:00:00:00:04', ip='10.0.0.3/8', speed=3)
    sta4 = net.addStation('sta4', mac='00:00:00:00:00:05', ip='10.0.0.4/8', speed=3)
    ap1 = net.addAccessPoint('ap1', ssid='new-ssid', mode='g', channel='1',
                             position='45,45,0')
    c1 = net.addController('c1', controller=Controller)

    print "*** Configuring Propagation Model"
    net.propagationModel(model="logDistance", exp=4.5)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Creating links"
    net.addHoc(sta3, ssid='adhocNet')
    net.addHoc(sta4, ssid='adhocNet')

    print "*** Starting network"
    net.build()
    c1.start()
    ap1.start([c1])

    'ploting graph'
    #net.plotGraph(max_x=200, max_y=200)

    getTrace(sta1, 'examples/replaying/replayingMobility/node1.dat')
    getTrace(sta2, 'examples/replaying/replayingMobility/node2.dat')
    getTrace(sta3, 'examples/replaying/replayingMobility/node3.dat')
    getTrace(sta4, 'examples/replaying/replayingMobility/node4.dat')

    replayingMobility(net)

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()

def getTrace(sta, file_):

    file_ = open(file_, 'r')
    raw_data = file_.readlines()
    file_.close()

    sta.position = []

    for data in raw_data:
        line = data.split()
        x = line[0]  # First Column
        y = line[1]  # Second Column
        sta.position.append('%s,%s,0' % (x, y))


if __name__ == '__main__':
    setLogLevel('info')
    topology()
