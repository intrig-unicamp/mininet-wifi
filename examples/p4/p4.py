#!/usr/bin/python

"""
Veriy whether this set of apps is running (only for ONOS - Remote Controller):
onos> apps -s -a
*  10 org.onosproject.generaldeviceprovider 1.13.0.SNAPSHOT General Device Provider
*  11 org.onosproject.protocols.grpc       1.13.0.SNAPSHOT gRPC Protocol Subsystem
*  12 org.onosproject.protocols.p4runtime  1.13.0.SNAPSHOT P4Runtime Protocol Subsystem
*  13 org.onosproject.p4runtime            1.13.0.SNAPSHOT P4Runtime Provider
*  14 org.onosproject.drivers              1.13.0.SNAPSHOT Default Drivers
*  15 org.onosproject.drivers.p4runtime    1.13.0.SNAPSHOT P4Runtime Drivers
*  16 org.onosproject.pipelines.basic      1.13.0.SNAPSHOT Basic Pipelines
*  17 org.onosproject.pipelines.fabric     1.13.0.SNAPSHOT Fabric Pipeline
*  18 org.onosproject.drivers.mellanox     1.13.0.SNAPSHOT Mellanox Drivers
*  19 org.onosproject.hostprovider         1.13.0.SNAPSHOT Host Location Provider
*  20 org.onosproject.lldpprovider         1.13.0.SNAPSHOT LLDP Link Provider
*  41 org.onosproject.protocols.gnmi       1.13.0.SNAPSHOT gNMI Protocol Subsystem
*  70 org.onosproject.drivers.gnmi         1.13.0.SNAPSHOT gNMI Drivers
*  77 org.onosproject.drivers.bmv2         1.13.0.SNAPSHOT BMv2 Drivers
"""


import os
import sys

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi
from mn_wifi.bmv2 import ONOSBmv2AP
from mininet.node import RemoteController


def topology(remote_controller):
    'Create a network.'
    net = Mininet_wifi()

    info('*** Adding stations/hosts\n')
    h1 = net.addHost('h1', ip='10.0.0.1', mac="00:00:00:00:00:01")
    h2 = net.addHost('h2', ip='10.0.0.2', mac="00:00:00:00:00:02")
    sta1 = net.addStation('sta1', ip='10.0.0.3', mac="00:00:00:00:00:03")
    sta2 = net.addStation('sta2', ip='10.0.0.4', mac="00:00:00:00:00:04")

    args1 = dict()
    args2 = dict()
    if not remote_controller:
        path = os.path.dirname(os.path.abspath(__file__))
        json_file = path + '/ap-runtime.json'
        config1 = path + '/commands_ap1.txt'
        config2 = path + '/commands_ap2.txt'
        args1 = {'json': json_file, 'switch_config': config1}
        args2 = {'json': json_file, 'switch_config': config2}

    info('*** Adding P4APs\n')
    ap1 = net.addAccessPoint('ap1', cls=ONOSBmv2AP, **args1)
    ap2 = net.addAccessPoint('ap2', cls=ONOSBmv2AP, **args2)

    if remote_controller:
        info('*** Adding Controller\n')
        net.addController('c0', controller=RemoteController)

    net.configureWifiNodes()

    info('*** Creating links\n')
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap2)
    net.addLink(h1, ap1)
    net.addLink(h2, ap2)

    info('*** Starting network\n')
    net.start()
    if not remote_controller:
        net.staticArp()

    info('*** Running CLI\n')
    CLI(net)

    info('*** Stopping network\n')
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    remote_controller = True if '-r' in sys.argv else False
    topology(remote_controller)
