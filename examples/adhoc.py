#!/usr/bin/python

"""This example shows how to work in adhoc mode

sta1 <---> sta2 <---> sta3"""

from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.net import Mininet
from mininet.wifiPropagationModels import powerForRangeByPropagationModel


def topology():
    "Create a network."
    net = Mininet(enable_wmediumd=True, enable_interference=True)

    print "*** Creating nodes"
    sta1 = net.addStation('sta1', position='10,10,0')
    sta2 = net.addStation('sta2', position='50,10,0')
    sta3 = net.addStation('sta3', position='90,10,0')


    net.propagationModel("logDistancePropagationLossModel", exp=4)

    # calc for sta1, the others should be the same
    power_to_distance = powerForRangeByPropagationModel().logDistancePropagationLossModel(node=sta1, wlan=0,
                                                                                          distance=50)

    sta1.setTxPower(sta1.params['wlan'][0], power_to_distance)
    sta2.setTxPower(sta2.params['wlan'][0], power_to_distance)
    sta3.setTxPower(sta3.params['wlan'][0], power_to_distance)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Creating links"
    net.addHoc(sta1, ssid='adhocNet', mode='g', channel=5)
    net.addHoc(sta2, ssid='adhocNet', mode='g', channel=5)
    net.addHoc(sta3, ssid='adhocNet', mode='g', channel=5)

    print "*** Plot Graph"
    net.plotGraph(max_x=100, max_y=100)

    print "*** Starting network"
    net.build()

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
