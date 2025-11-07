#!/usr/bin/env python

"""
This example is implemented using celestrak
"""

import os

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi
from mn_wifi.link import wmediumd
from mn_wifi.wmediumdConnector import interference


def topology():
    "Create a network."
    net = Mininet_wifi(link=wmediumd, wmediumd_mode=interference)

    info("*** Creating nodes\n")
    net.addSatellite('sat1', catnr='25544') # ISS
    net.addSatellite('sat2', catnr='48274') # Starlink
    net.addSatellite('sat3', catnr='56393') # Starlink
    net.addSatellite('sat4', catnr='29108') # CALIPSO
    net.addSatellite('sat5', catnr='44874') # CHEOPS
    net.addSatellite('sat6', catnr='41860') # GALILEO
    net.addSatellite('sat7', catnr='42063') # SENTINEL-2B
    net.addSatellite('sat8', catnr='43013') # NOAA-20
    net.addSatellite('sat9', catnr='25994') # TERRA
    net.addSatellite('sat10', catnr='27424') # AQUA

    info("*** Configuring propagation model\n")
    net.setPropagationModel(model="logDistance", exp=1)

    info("*** Configuring nodes\n")
    net.configureNodes()

    path = os.path.dirname(os.path.abspath(__file__))
    nodes = net.satellites + net.aps
    net.telemetry(nodes=nodes, data_type='position', image='{}/map.png'.format(path),
                  min_x=-180, max_x=180, min_y=-90, max_y=90)

    info("*** Starting network\n")
    net.start()

    info("*** Configure Satellites\n")
    net.configureSatellites()

    info("*** Running CLI\n")
    CLI(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
