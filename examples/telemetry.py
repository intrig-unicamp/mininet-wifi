#!/usr/bin/python

'This uses telemetry() to enable a graph with live statistics'

from mininet.node import Controller
from mininet.log import setLogLevel, info
from mn_wifi.link import wmediumd
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.wmediumdConnector import interference
from mn_wifi.telemetry import telemetry


def topology():
    "Create a network."
    net = Mininet_wifi(controller=Controller, link=wmediumd,
                       wmediumd_mode=interference,
                       noise_threshold=-91, fading_coefficient=3)

    info("*** Creating nodes\n")
    ap1 = net.addAccessPoint('ap1', ssid='new-ssid', mode='a', channel='36',
                             position='15,30,0')
    net.addStation('sta1', mac='00:00:00:00:00:02', ip='10.0.0.1/8',
                   position='10,20,0')
    net.addStation('sta2', mac='00:00:00:00:00:03', ip='10.0.0.2/8',
                   position='20,50,0')
    net.addStation('sta3', mac='00:00:00:00:00:04', ip='10.0.0.3/8',
                   position='20,60,10')
    c1 = net.addController('c1')

    info("*** Configuring Propagation Model\n")
    net.setPropagationModel(model="logDistance", exp=4)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    # refer to mn_wifi/telemetry for more information
    nodes = net.stations
    telemetry(nodes, single=True, data_type='rssi')

    info("*** Starting network\n")
    net.build()
    c1.start()
    ap1.start([c1])

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
