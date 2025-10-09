#!/usr/bin/env python

"""This example is implemented using the FlightRadar24 API

Additional attributes:
node.registration
node.aircraft_code
node.icao_24bit
"""

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi
from mn_wifi.link import wmediumd
from mn_wifi.wmediumdConnector import interference


def topology():
    "Create a network."
    net = Mininet_wifi(link=wmediumd, wmediumd_mode=interference,
                       noise_th=-91, fading_cof=3)

    info("*** Creating nodes\n")
    net.addAircraft('aircraft1', ip='10.0.0.1/8')
    net.addAircraft('aircraft2', ip='10.0.0.2/8')
    net.addAircraft('aircraft3', ip='10.0.0.3/8')
    net.addAircraft('aircraft4', ip='10.0.0.4/8')
    decoder1 = net.addHost('decoder1', ip='10.0.0.5/8')
    ap1 = net.addAccessPoint('LIS1', ssid="ads-b", mode="g", channel="1",
                             position='-9.13, 38.73, 0', failMode='standalone')

    info("*** Configuring propagation model\n")
    net.setPropagationModel(model="logDistance", exp=1)

    info("*** Configuring nodes\n")
    net.configureNodes()

    info("*** Creating Link\n")
    net.addLink(decoder1, ap1)

    nodes = net.stations + net.aps
    net.telemetry(nodes=nodes, data_type='position',
                  min_x=-20, max_x=5, min_y=30, max_y=45)

    info("*** Starting network\n")
    net.start()

    info("*** Configure Aircrafts\n")
    net.configureAircrafts(38.715994, -9.128210, 500000) # lat, long, range

    info("*** Running CLI\n")
    CLI(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
