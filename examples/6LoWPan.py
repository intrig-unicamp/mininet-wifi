#!/usr/bin/python

import os

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi

"""This example creates a simple network topology with 3 nodes

       sensor1
      /       \
    /          \
sensor2      sensor3
"""


def topology():
    "Create a network."
    net = Mininet_wifi(iot_module='mac802154_hwsim')
    # iot_module: fakelb or mac802154_hwsim
    # mac802154_hwsim is supported from kernel 4.18

    info("*** Creating nodes\n")
    # There is no need to set the node position.
    # Signal range and position won't work as we expect
    # because there is no wmediumd-like code for mac802154_hwim yet.
    # However, you may want to add mobility and node position
    # and use wpan-hwsim for some purposes.
    net.addSensor('sensor1', ip6='2001::1/64', panid='0xbeef')
    net.addSensor('sensor2', ip6='2001::2/64', panid='0xbeef')
    net.addSensor('sensor3', ip6='2001::3/64', panid='0xbeef')

    info("*** Configuring nodes\n")
    net.configureWifiNodes()

    info("*** Starting network\n")
    net.build()

    info("*** Adding edges")
    # This is useful for routing
    # You may want to refer to https://github.com/linux-wpan/rpld
    # if you want to implement some custom routing
    os.system('wpan-hwsim edge add 0 1')  # sensor1 - sensor2
    os.system('wpan-hwsim edge add 1 0')  # sensor2 - sensor1
    os.system('wpan-hwsim edge add 0 2')  # sensor1 - sensor3
    os.system('wpan-hwsim edge add 2 0')  # sensor3 - sensor1

    info("*** Running CLI\n")
    CLI(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
