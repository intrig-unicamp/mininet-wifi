#!/usr/bin/python

'Setting position of the nodes and enable sockets'

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.telemetry import telemetry


def topology():

    net = Mininet_wifi(set_socket_ip='127.0.0.1',
                       set_socket_port=12345)
    # set_socket_ip: localhost must be replaced by ip address
    # of the network interface of your system
    # The same must be done with socket_client.py

    info("*** Creating nodes\n")
    net.addStation('sta1', mac='00:00:00:00:00:02', ip='10.0.0.1/8',
                   position='30,60,0')
    net.addStation('sta2', mac='00:00:00:00:00:03', ip='10.0.0.2/8',
                   position='70,30,0')
    ap1 = net.addAccessPoint('ap1', ssid='new-ssid', mode='g', channel='1',
                             failMode="standalone", position='50,50,0')
    h1 = net.addHost('h1', ip='10.0.0.3/8')

    net.setPropagationModel(model="logDistance", exp=4.5)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    info("*** Creating links\n")
    net.addLink(ap1, h1)

    info("*** Starting network\n")
    net.addNAT().configDefault()
    net.build()
    ap1.start([])

    nodes = net.stations + net.aps
    telemetry(nodes, data_type='position',
              min_x=0, min_y=0,
              max_x=100, max_y=100)

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
