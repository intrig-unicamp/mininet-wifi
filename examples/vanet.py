#!/usr/bin/python

'Simple idea around Vehicular Ad Hoc Networks - VANETs'

import os
import random

from mininet.net import Mininet
from mininet.node import Controller, OVSKernelSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel

def topology():

    "Create a network."
    net = Mininet(controller=Controller, switch=OVSKernelSwitch,
                  enable_wmediumd=True, enable_interference=True)

    print "*** Creating nodes"
    cars = []
    for x in range(0, 10):
        cars.append(x)
    for x in range(0, 10):
        min_ = random.randint(1, 4)
        max_ = random.randint(11, 30)
        cars[x] = net.addCar('car%s' % (x + 1), wlans=1,
                             ip='10.0.0.%s/8'% (x + 1), min_speed=min_,
                             max_speed=max_, range=50)

    rsu11 = net.addAccessPoint('RSU11', ssid='RSU11', mode='g',
                               channel='1')
    rsu12 = net.addAccessPoint('RSU12', ssid='RSU12', mode='g',
                               channel='6')
    rsu13 = net.addAccessPoint('RSU13', ssid='RSU13', mode='g',
                               channel='11')
    rsu14 = net.addAccessPoint('RSU14', ssid='RSU14', mode='g',
                               channel='11')
    c1 = net.addController('c1', controller=Controller)

    print "*** Configuring Propagation Model"
    net.propagationModel(model="logDistance", exp=4.5)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Associating and Creating links"
    net.addLink(rsu11, rsu12)
    net.addLink(rsu11, rsu13)
    net.addLink(rsu11, rsu14)

    net.plotGraph(max_x=500, max_y=500)

    net.roads(10)

    net.startMobility(time=0)

    print "*** Starting network"
    net.build()
    c1.start()
    rsu11.start([c1])
    rsu12.start([c1])
    rsu13.start([c1])
    rsu14.start([c1])

    i = 201
    for sw in net.carsSW:
        sw.start([c1])
        os.system('ip addr add 10.0.0.%s dev %s' % (i, sw))
        i += 1

    i = 1
    j = 2
    k = 1
    for car in cars:
        car.setIP('192.168.0.%s/24' % k, intf='%s-wlan0' % car)
        car.setIP('192.168.1.%s/24' % i, intf='%s-eth1' % car)
        car.cmd('ip route add 10.0.0.0/8 via 192.168.1.%s' % j)
        i += 2
        j += 2
        k += 1

    i = 1
    j = 2
    for carsta in net.carsSTA:
        carsta.setIP('10.0.0.%s/24' % i, intf='%s-mp0' % carsta)
        carsta.setIP('192.168.1.%s/24' % j, intf='%s-eth2' % carsta)
        #May be confuse, but it allows ping to the name instead of ip addr
        carsta.setIP('10.0.0.%s/24' % i, intf='%s-wlan0' % carsta)
        carsta.cmd('echo 1 > /proc/sys/net/ipv4/ip_forward')
        i += 1
        j += 2

    for carsta1 in net.carsSTA:
        i = 1
        j = 1
        for carsta2 in net.carsSTA:
            if carsta1 != carsta2:
                carsta1.cmd('route add -host 192.168.1.%s gw 10.0.0.%s' % (j, i))
            i += 1
            j += 2

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
