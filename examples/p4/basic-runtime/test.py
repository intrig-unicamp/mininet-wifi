#!/usr/bin/python

import os

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi
from mn_wifi.bmv2 import P4RuntimeAP

def topology():
    'Create a network.'
    net = Mininet_wifi()

    info('*** Adding stations/hosts\n')
    sta1 = net.addStation('sta1', ip='10.0.0.2', mac='00:00:00:00:00:02')
    h1 = net.addHost('h1', ip='10.0.0.3', mac='00:00:00:00:00:03')

    info('*** Adding P4AP\n')
    ap1 = net.addAccessPoint('ap1',  cls=P4RuntimeAP, json_path='build/basic.json', runtime_json_path='runtime/ap1-runtime.json', 
			     log_console = True, log_dir = os.path.abspath('logs'), log_file = 'ap1.log', pcap_dump = os.path.abspath('pcaps'))

    net.configureWifiNodes()


    info('*** Creating links\n')
    net.addLink(sta1, ap1)
    net.addLink(h1, ap1)


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
