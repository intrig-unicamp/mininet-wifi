#!/usr/bin/python

import os

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi
from mn_wifi.bmv2 import ONOSBmv2Switch
from mininet.term import makeTerm, cleanUpScreens
from mininet.node import RemoteController


def topology():
    'Create a network.'
    net = Mininet_wifi()

    info('*** Adding stations/hosts\n')
    h1 = net.addHost('h1', ip='10.0.0.1', mac="00:00:00:00:00:01")
    sta1 = net.addStation('sta1', ip='10.0.0.2', mac="00:00:00:00:00:02")

    path = os.path.dirname(os.path.abspath(__file__))
    json_file = path + '/handover.json'
    config1 = path + '/commands_s1.txt'

    info('*** Adding APs\n')
    ap1 = net.addAccessPoint('ap1', failMode='standalone', mac="00:00:00:00:00:10",
                             ssid='ap1', position='35,50,0')
    ap2 = net.addAccessPoint('ap2', failMode='standalone', mac="00:00:00:00:00:11",
                             ssid='ap2', position='105,50,0')

    info('*** Adding P4Switch\n')
    s1 = net.addSwitch('s1', cls=ONOSBmv2Switch)

    net.addController('c0', controller=RemoteController)

    info("*** Configuring propagation model\n")
    net.setPropagationModel(model="logDistance", exp=4)

    info('*** Configuring WiFi Nodes\n')
    net.configureWifiNodes()

    info('*** Creating links\n')
    net.addLink(s1, ap1)
    net.addLink(s1, ap2)
    net.addLink(s1, h1)

    net.plotGraph(max_x=120, max_y=120)

    info('*** Configuring Mobility\n')
    net.startMobility(time=0)
    net.mobility(sta1, 'start', time=1, position='10,30,0')
    net.mobility(sta1, 'stop', time=20, position='120,30,0')
    net.stopMobility(time=20)

    info('*** Starting network\n')
    net.start()
    #net.staticArp()

    ap1.cmd('iw dev ap1-wlan1 interface add mon1 type monitor')
    ap2.cmd('iw dev ap2-wlan1 interface add mon2 type monitor')
    ap1.cmd('ip link set mon1 up')
    ap2.cmd('ip link set mon2 up')

    #makeTerm(ap1, cmd="bash -c 'cd %s && python sniffer.py mon1 0;'" % path)
    #makeTerm(ap2, cmd="bash -c 'cd %s && python sniffer.py mon2 1;'" % path)

    '''
    s1 simple_switch_CLI --thrift-port 50001 <<< "table_delete MyIngress.ipv4_lpm 1"
    s1 simple_switch_CLI --thrift-port 50001 <<< "table_add MyIngress.ipv4_lpm ipv4_forward 10.0.0.2 => 00:00:00:00:00:02 2"
    s1 table_modify MyIngress.ipv4_lpm ipv4_forward 1 00:00:00:00:00:02 2
    '''

    info('*** Running CLI\n')
    CLI(net)

    info('*** Stopping network\n')
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
