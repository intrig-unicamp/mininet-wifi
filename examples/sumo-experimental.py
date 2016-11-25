#!/usr/bin/python

"""
                   (mesh)
         ..........................
         .                        .
         .                        .
     10.0.0.1/24             10.0.0.2/24
    (car1STA-mp0)            (car2STA-mp0)
       car1STA                  car2STA
    (car1STA-eth0)          (car2STA-eth0)
    192.168.1.2/24           192.168.1.4/24 
         |                        |
    (car1SW-eth1)            (car2SW-eth1)
       car1SW                   car2SW
    (car1SW-eth2)            (car2SW-eth2)
         |                        |
    (car1-eth0)               (car2-eth0)
   192.168.1.1/24            192.168.1.3/24
    	car1                     car2
    (car1-wlan0)              (car2-wlan0)
   192.168.0.1/24            192.168.0.3/24
         .                        .
         .                        .
         ...........RSU1............
                  (infra)
"""


from mininet.net import Mininet
from mininet.node import Controller, OVSKernelSwitch, UserSwitch, RemoteController
from mininet.link import TCLink
from mininet.cli import CLI
from mininet.log import setLogLevel
import os


class InbandController( RemoteController ):

    def checkListening( self ):
        "Overridden to do nothing."
        return

def topology():

    "Create a network."
    net = Mininet(controller=Controller, link=TCLink, switch=OVSKernelSwitch)

    print "*** Creating nodes"
    car = []
    stas = []
    for x in range(0, 10):
        car.append(x)
        stas.append(x)
    for x in range(0, 10):
        car[x] = net.addCar('car%s' % (x), wlans=1, ip='10.0.0.%s/8' % (x + 1), mode='b', max_x=40, max_y=40, range=1000)

    rsu10 = net.addAccessPoint('rsu10', ssid='rsu10', mode='g', channel='1', position='3500,2500,0', range=500)
    rsu11 = net.addAccessPoint('rsu11', ssid='rsu11', mode='g', channel='6', position='3000,2500,0', range=500)
    rsu12 = net.addAccessPoint('rsu12', ssid='rsu12', mode='g', channel='11', position='2500,3000,0', range=500)
    rsu13 = net.addAccessPoint('rsu13', ssid='rsu13', mode='g', channel='11', position='4000,3000,0', range=500)
    rsu14 = net.addAccessPoint('rsu14', ssid='rsu14', mode='g', channel='11', position='4500,3000,0', range=500)
    c1 = net.addController('c1', controller=Controller, ip='127.0.0.1', port=6633)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    net.meshRouting('custom') 

    net.addLink(rsu10,rsu11)
    net.addLink(rsu11,rsu12)
    net.addLink(rsu12,rsu13)
    net.addLink(rsu13,rsu14)

    """Available Options: sumo, sumo-gui"""
    #Put your sumocfg file in /mininet/sumo/data
    net.useExternalProgram('sumo-gui', config_file='map.sumocfg')

    print "*** Starting network"
    net.build()
    c1.start()
    rsu10.start([c1])
    rsu11.start([c1])
    rsu12.start([c1])
    rsu13.start([c1])
    rsu14.start([c1])
    i = 201
    for sw in net.vehicles:
        sw.start([c1])
        os.system('ifconfig %s 10.0.0.%s' % (sw, i))
        i+=1

    i = 1
    j = 2
    for c in car:
        c.cmd('ifconfig %s-wlan0 192.168.0.%s/24 up' % (c,i))
        c.cmd('ifconfig %s-eth0 192.168.1.%s/24 up' % (c,i))
        c.cmd('ip route add 10.0.0.0/8 via 192.168.1.%s' % j)
        i+=2
        j+=2
     
    i = 1
    j = 2
    for v in net.vehiclesSTA:
        v.cmd('ifconfig %s-eth0 192.168.1.%s/24 up' % (v, j))
        v.cmd('ifconfig %s-mp0 10.0.0.%s/24 up' % (v,i))
        v.cmd('echo 1 > /proc/sys/net/ipv4/ip_forward')
        i+=1    
        j+=2

  
    for v1 in net.vehiclesSTA:
        i = 1
        j = 1
        for v2 in net.vehiclesSTA:
            if v1 != v2: 
                v1.cmd('route add -host 192.168.1.%s gw 10.0.0.%s' % (j,i))
            i+=1
            j+=2

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel('info')
    topology()
