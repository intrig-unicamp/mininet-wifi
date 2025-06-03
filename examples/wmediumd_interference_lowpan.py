#!/usr/bin/env python

"Setting the position of lowpan sensors with wmediumd to calculate the interference"

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi
from mn_wifi.sixLoWPAN.wmediumdConnector import interference
from mn_wifi.sixLoWPAN.link import LoWPAN, wmediumd_802154


def topology():
    "Create a network."
    net = Mininet_wifi(link=wmediumd_802154, wmediumd_802154_mode=interference,
                       noise_th=-91, fading_cof=3)

    info("*** Creating nodes\n")
    sensor1 = net.addSensor('sensor1', ip6='fe80::1/64', mac="02:00:00:00:00:00:00:00",
                            panid='0xbeef', position='10,0,0')
    sensor2 = net.addSensor('sensor2', ip6='fe80::2/64', mac="02:00:00:00:00:00:00:01",
                            panid='0xbeef', position='20,0,0')
    sensor3 = net.addSensor('sensor3', ip6='fe80::3/64', mac="02:00:00:00:00:00:00:02",
                            panid='0xbeef', position='100,0,0')

    info("*** Configuring Propagation Model\n")
    net.setPropagationModel(model="logDistance", exp=4)

    # NOTE: This code depends on the modified module available at
    # https://github.com/ramonfontes/wmediumd_802154/
    #net.setModule("ieee802154/mac802154_hwsim.ko")

    info("*** Configuring nodes\n")
    net.configureNodes()

    info("*** Adding links\n")
    net.addLink(sensor1, sensor2, cls=LoWPAN)
    net.addLink(sensor1, sensor3, cls=LoWPAN)

    #net.plotGraph(min_x=-50, min_y=-100, max_x=150, max_y=100)

    info("*** Starting network\n")
    net.build()

    info("*** Running CLI\n")
    CLI(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()