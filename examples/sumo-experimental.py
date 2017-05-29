#!/usr/bin/python

from mininet.net import Mininet
from mininet.node import Controller, OVSKernelAP, RemoteController
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel
import os


class InbandController(RemoteController):

    def checkListening(self):
        "Overridden to do nothing."
        return

def topology():

    "Create a network."
    net = Mininet(controller=Controller, link=TCLink, accessPoint=OVSKernelAP, enable_wmediumd=True, enable_interference=True)

    print "*** Creating nodes"
    car = []
    stas = []
    for x in range(0, 10):
        car.append(x)
        stas.append(x)
    for x in range(0, 10):
        car[x] = net.addCar('car%s' % (x), wlans=1, ip='10.0.0.%s/8' % (x + 1))

    e1 = net.addAccessPoint('e1', ssid='enodeb1', mac='00:00:00:11:00:01', mode='g', channel='1', position='3279.02,3736.27,0')
    e2 = net.addAccessPoint('e2', ssid='enodeb2', mac='00:00:00:11:00:02', mode='g', channel='6', position='2320.82,3565.75,0')
    e3 = net.addAccessPoint('e3', ssid='enodeb3', mac='00:00:00:11:00:03', mode='g', channel='11', position='2806.42,3395.22,0')
    e4 = net.addAccessPoint('e4', ssid='enodeb4', mac='00:00:00:11:00:04', mode='g', channel='1', position='3332.62,3253.92,0')
    e5 = net.addAccessPoint('e5', ssid='enodeb5', mac='00:00:00:11:00:05', mode='g', channel='6', position='2887.62,2935.61,0')
    e6 = net.addAccessPoint('e6', ssid='enodeb6', mac='00:00:00:11:00:06', mode='g', channel='11', position='2351.68,3083.40,0')
    c1 = net.addController('c1', controller=Controller, ip='127.0.0.1', port=6653)

    net.propagationModel("logDistancePropagationLossModel", exp=2.5)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    net.addLink(e1, e2)
    net.addLink(e2, e3)
    net.addLink(e3, e4)
    net.addLink(e4, e5)
    net.addLink(e5, e6)

    "Available Options: sumo, sumo-gui"
    net.useExternalProgram('sumo-gui', config_file='map.sumocfg')

    print "*** Starting network"
    net.build()
    c1.start()
    e1.start([c1])
    e2.start([c1])
    e3.start([c1])
    e4.start([c1])
    e5.start([c1])
    e6.start([c1])

    i = 201
    for sw in net.vehicles:
        sw.start([c1])
        os.system('ifconfig %s 10.0.0.%s' % (sw, i))
        i += 1

    i = 1
    j = 2
    for c in car:
        c.cmd('ifconfig %s-wlan0 192.168.0.%s/24 up' % (c, i))
        c.cmd('ifconfig %s-eth0 192.168.1.%s/24 up' % (c, i))
        c.cmd('ip route add 10.0.0.0/8 via 192.168.1.%s' % j)
        i += 2
        j += 2

    i = 1
    j = 2
    for v in net.vehiclesSTA:
        v.cmd('ifconfig %s-eth0 192.168.1.%s/24 up' % (v, j))
        v.cmd('ifconfig %s-mp0 10.0.0.%s/24 up' % (v, i))
        v.cmd('echo 1 > /proc/sys/net/ipv4/ip_forward')
        i += 1
        j += 2


    for v1 in net.vehiclesSTA:
        i = 1
        j = 1
        for v2 in net.vehiclesSTA:
            if v1 != v2:
                v1.cmd('route add -host 192.168.1.%s gw 10.0.0.%s' % (j, i))
            i += 1
            j += 2

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel('info')
    topology()
