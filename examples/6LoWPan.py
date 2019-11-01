#!/usr/bin/python

'This example creates a simple network topology with 3 nodes'

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.sixLoWPAN.link import sixLoWPAN


def topology():
    "Create a network."
    net = Mininet_wifi(iot_module='fakelb')
    # iot_module: fakelb or mac802154_hwsim
    # mac802154_hwsim is only supported from kernel 4.18

    info("*** Creating nodes\n")
    net.addSensor('sensor1', ip6='2001::1/64', panid='0xbeef')
    net.addSensor('sensor2', ip6='2001::2/64', panid='0xbeef')
    net.addSensor('sensor3', ip6='2001::3/64', panid='0xbeef')

    info("*** Configuring nodes\n")
    net.configureWifiNodes()

    info("*** Starting network\n")
    net.build()

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
