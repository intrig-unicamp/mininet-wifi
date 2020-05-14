#!/usr/bin/python

import os

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi
from mn_wifi.bmv2 import Bmv2AP


def topology():
    'Create a network.'
    net = Mininet_wifi()

    info('*** Adding stations/hosts\n')
    h1 = net.addHost('h1', ip='10.0.0.1', mac="00:00:00:00:00:01")
    h2 = net.addHost('h2', ip='10.0.0.2', mac="00:00:00:00:00:02")
    sta1 = net.addStation('sta1', ip='10.0.0.3', mac="00:00:00:00:00:03")
    sta2 = net.addStation('sta2', ip='10.0.0.4', mac="00:00:00:00:00:04")

    info('*** Adding P4APs\n')
    path = os.path.dirname(os.path.abspath(__file__))
    json_file = path + '/ap-runtime.json'
    config1 = path + '/commands_ap1.txt'
    config2 = path + '/commands_ap2.txt'
    ap1 = net.addAccessPoint('ap1', json=json_file, loglevel='info',
                             pktdump=False, switch_config=config1, cls=Bmv2AP)
    ap2 = net.addAccessPoint('ap2', json=json_file, loglevel='info',
                             pktdump=False, switch_config=config2, cls=Bmv2AP)

    net.configureWifiNodes()

    info('*** Creating links\n')
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap2)
    net.addLink(h1, ap1)
    net.addLink(h2, ap2)

    info('*** Starting network\n')
    net.start()
    net.staticArp()

    info('*** Running CLI\n')
    CLI(net)

    info('*** Stopping network\n')
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
