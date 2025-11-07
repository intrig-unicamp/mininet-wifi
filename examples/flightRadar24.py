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
    num_aircrafts = 20
    for i in range(1, num_aircrafts + 1):
        name = f'plane{i}'
        ip = f'10.0.0.{i}/8'
        net.addAircraft(name, ip=ip)
    decoder1 = net.addHost('decoder1', ip='10.0.0.5/8')
    ap1 = net.addAccessPoint('LIS1', ssid="ads-b", mode="g", channel="1",
                             position='-9.13, 38.73, 0', failMode='standalone')

    info("*** Configuring propagation model\n")
    net.setPropagationModel(model="logDistance", exp=1)

    info("*** Configuring nodes\n")
    net.configureNodes()

    info("*** Creating Link\n")
    net.addLink(decoder1, ap1)

    nodes = net.aircrafts + net.aps
    net.telemetry(nodes=nodes, data_type='position', icon_text_size=8,
                  icon='plane.png', icon_width=.8, icon_height=.8,
                  min_x=-20, max_x=0, min_y=30, max_y=45)

    info("*** Starting network\n")
    net.start()

    info("*** Configure Aircrafts\n")
    net.configureAircrafts(38.73, -9.13, 500000) # lat, long, range

    info("*** Running CLI\n")
    CLI(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
