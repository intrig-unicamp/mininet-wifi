#!/usr/bin/python

"""
This example shows how to use the wmediumd connector to prevent mac80211_hwsim stations reaching each other
(with Mobility)

It starts wmediumd and 'disables' the connection from sta1 to sta2

authors: Patrick Grosse (patrick.grosse@uni-muenster.de)
         Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.net import Mininet
from mininet.wmediumdConnector import WmediumdStarter, DynamicWmediumdIntfRef, WmediumdSNRLink, WmediumdServerConn


def topology():
    """Create a network. sta1 <--> sta2 <--> sta3"""

    print "*** Network creation"
    net = Mininet()

    print "*** Creating nodes"
    sta1 = net.addStation('sta1', range=200)
    sta2 = net.addStation('sta2', range=200)
    sta3 = net.addStation('sta3', range=200)

    print "*** Configure wmediumd"
    # This should be done right after the station has been initialized
    sta1.wmediumdIface = DynamicWmediumdIntfRef(sta1)
    sta2.wmediumdIface = DynamicWmediumdIntfRef(sta2)
    sta3.wmediumdIface = DynamicWmediumdIntfRef(sta3)

    intfrefs = [sta1.wmediumdIface, sta2.wmediumdIface, sta3.wmediumdIface]
    links = [
        WmediumdSNRLink(sta1.wmediumdIface, sta2.wmediumdIface, 15),
        WmediumdSNRLink(sta2.wmediumdIface, sta1.wmediumdIface, 15),
        WmediumdSNRLink(sta2.wmediumdIface, sta3.wmediumdIface, 15),
        WmediumdSNRLink(sta3.wmediumdIface, sta2.wmediumdIface, 15)]
    WmediumdStarter.initialize(intfrefs, links, with_server=True)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Start wmediumd"
    WmediumdStarter.start()

    net.plotGraph(max_x=240, max_y=240)

    net.meshRouting('custom')

    net.seed(20)

    print "*** Creating links"
    net.addMesh(sta1, ssid='adNet')
    net.addMesh(sta2, ssid='adNet')
    net.addMesh(sta3, ssid='adNet')

    print "*** Starting network"
    net.start()
    print "\n\n\n"
    print "*** Pinging sta2"
    sta1.cmdPrint('ping -c 1 10.0.0.2')

    print "*** Setting up the mobility model"
    net.startMobility(startTime=0, model='RandomDirection', max_x=220, max_y=220, min_v=0.1, max_v=0.2)

    print "*** Update wmediumd"
    WmediumdServerConn.connect()

    CLI(net)

    print "*** Stopping network"
    net.stop()

    print "*** Stopping wmediumd"
    WmediumdServerConn.disconnect()
    WmediumdStarter.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
