#!/usr/bin/python

'This example creates a simple network topology with 1 AP and 2 stations'

import sys

from mininet.node import Controller
from mininet.log import setLogLevel, info
from mn_wifi.node import OVSKernelAP
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi


def topology(isVirtual):
    "Create a network."
    net = Mininet_wifi(controller=Controller, accessPoint=OVSKernelAP)

    info("*** Creating nodes\n")
    if isVirtual:
        sta1 = net.addStation('sta1', nvif=2)
    else:
        sta1 = net.addStation('sta1')
    sta2 = net.addStation('sta2')
    if isVirtual:
        ap1 = net.addAccessPoint('ap1', ssid="simplewifi",
                                 mode="g", channel="5")
    else:
        # isolate_clientes: Client isolation can be used to prevent low-level
        # bridging of frames between associated stations in the BSS.
        # By default, this bridging is allowed.
        # OpenFlow rules are required to allow communication among nodes
        ap1 = net.addAccessPoint('ap1', ssid="simplewifi",
                                 isolate_clientes=True, mode="g", channel="5")
    c0 = net.addController('c0', controller=Controller, ip='127.0.0.1',
                           port=6633)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    info("*** Associating Stations\n")
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap1)

    info("*** Starting network\n")
    net.build()
    c0.start()
    ap1.start([c0])

    if not isVirtual:
        ap1.cmd('ovs-ofctl add-flow ap1 "priority=0,arp,in_port=1,'
                'actions=output:in_port,normal"')
        ap1.cmd('ovs-ofctl add-flow ap1 "priority=0,icmp,in_port=1,'
                'actions=output:in_port,normal"')
        ap1.cmd('ovs-ofctl add-flow ap1 "priority=0,udp,in_port=1,'
                'actions=output:in_port,normal"')
        ap1.cmd('ovs-ofctl add-flow ap1 "priority=0,tcp,in_port=1,'
                'actions=output:in_port,normal"')

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    isVirtual = True if '-v' in sys.argv else False
    topology(isVirtual)
