#!/usr/bin/python

"""Sample file for SUMO

***Requirements***:

sumo 1.1.0
sumo-gui"""

from mininet.node import Controller
from mininet.log import setLogLevel, info
from mn_wifi.telemetry import telemetry
from mn_wifi.node import UserAP
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.sumo.runner import sumo
from mn_wifi.link import wmediumd, mesh
from mn_wifi.wmediumdConnector import interference


def topology():

    "Create a network."
    net = Mininet_wifi(controller=Controller, accessPoint=UserAP,
                       link=wmediumd, wmediumd_mode=interference)

    info("*** Creating nodes\n")
    cars = []
    for id in range(0, 10):
        cars.append(net.addCar('car%s' % (id+1), wlans=2, encrypt='wpa2,'))

    e1 = net.addAccessPoint('e1', ssid='vanet-ssid', mac='00:00:00:11:00:01',
                            mode='g', channel='1', passwd='123456789a',
                            encrypt='wpa2', position='2600,3400,0')
    e2 = net.addAccessPoint('e2', ssid='vanet-ssid', mac='00:00:00:11:00:02',
                            mode='g', channel='6', passwd='123456789a',
                            encrypt='wpa2', position='2800,3400,0')
    e3 = net.addAccessPoint('e3', ssid='vanet-ssid', mac='00:00:00:11:00:03',
                            mode='g', channel='11', passwd='123456789a',
                            encrypt='wpa2', position='3000,3400,0')
    e4 = net.addAccessPoint('e4', ssid='vanet-ssid', mac='00:00:00:11:00:04',
                            mode='g', channel='1', passwd='123456789a',
                            encrypt='wpa2', position='2600,3200,0')
    e5 = net.addAccessPoint('e5', ssid='vanet-ssid', mac='00:00:00:11:00:05',
                            mode='g', channel='6', passwd='123456789a',
                            encrypt='wpa2', position='2800,3200,0')
    e6 = net.addAccessPoint('e6', ssid='vanet-ssid', mac='00:00:00:11:00:06',
                            mode='g', channel='11', passwd='123456789a',
                            encrypt='wpa2', position='3000,3200,0')
    c1 = net.addController('c1')

    info("*** Configuring Propagation Model\n")
    net.setPropagationModel(model="logDistance", exp=3.5)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    net.addLink(e1, e2)
    net.addLink(e2, e3)
    net.addLink(e3, e4)
    net.addLink(e4, e5)
    net.addLink(e5, e6)
    for car in cars:
        net.addLink(car, intf=car.params['wlan'][1],
                    cls=mesh, ssid='mesh-ssid', channel=5)

    net.useExternalProgram(program=sumo, port=8813,
                           config_file='map.sumocfg')

    info("*** Starting network\n")
    net.build()
    c1.start()
    e1.start([c1])
    e2.start([c1])
    e3.start([c1])
    e4.start([c1])
    e5.start([c1])
    e6.start([c1])

    for car in cars:
        car.setIP('192.168.0.%s/24' % (int(cars.index(car))+1),
                  intf='%s-wlan0' % car)
        car.setIP('192.168.1.%s/24' % (int(cars.index(car))+1),
                  intf='%s-mp1' % car)

    # Track the position of the nodes
    nodes = net.cars + net.aps
    telemetry(nodes, data_type='position',
              min_x=2500, min_y=2500,
              max_x=4000, max_y=4000)

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
