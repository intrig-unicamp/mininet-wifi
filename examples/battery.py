#!/usr/bin/env python

"""This example creates a simple network topology in which
   stations are equipped with batteries"""

import sys

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi
from mn_wifi.energy import Energy


def topology(args):
    "Create a network."
    net = Mininet_wifi(iot_module='mac802154_hwsim')

    info("*** Creating nodes\n")
    net.addSensor('sensor1', ip6='2001::1/64', voltage=3.7,
                  battery_capacity=0.000003, panid='0xbeef')
    net.addSensor('sensor2', ip6='2001::2/64', voltage=3.7,
                  battery_capacity=0.000002, panid='0xbeef')
    net.addSensor('sensor3', ip6='2001::3/64', voltage=3.7,
                  battery_capacity=0.000001, panid='0xbeef')

    info("*** Configuring nodes\n")
    net.configureNodes()

    if '-p' in args:
        net.plotEnergyMonitor(nodes=net.sensors, title="Battery Consumptions")

    info("*** Starting network\n")
    net.build()

    info("*** Measuring energy consumption\n")
    Energy(net.sensors)

    info("*** Running CLI\n")
    CLI(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology(sys.argv)
