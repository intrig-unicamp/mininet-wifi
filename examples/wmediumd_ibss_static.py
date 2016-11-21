#!/usr/bin/python

"""
This example shows how to use the wmediumd connector to prevent mac80211_hwsim stations reaching each other

author: Patrick Grosse (patrick.grosse@uni-muenster.de)
"""

from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.net import Mininet
from mininet.wmediumdConnector import WmediumdConn, WmediumdLink, StaticWmediumdIntfRef


def topology():
    """Create a network. sta1 <--> sta2 <--> sta3"""

    print "*** Network creation"
    net = Mininet()

    print "*** Configure wmediumd"
    sta1wlan0 = StaticWmediumdIntfRef('sta1', 'sta1-wlan0', '02:00:00:00:00:00')
    sta2wlan0 = StaticWmediumdIntfRef('sta2', 'sta2-wlan0', '02:00:00:00:01:00')
    sta3wlan0 = StaticWmediumdIntfRef('sta3', 'sta3-wlan0', '02:00:00:00:02:00')

    intfrefs = [sta1wlan0, sta2wlan0, sta3wlan0]
    links = [
        WmediumdLink(sta1wlan0, sta2wlan0, 15),
        WmediumdLink(sta2wlan0, sta1wlan0, 15),
        WmediumdLink(sta2wlan0, sta3wlan0, 15),
        WmediumdLink(sta3wlan0, sta2wlan0, 15)]
    WmediumdConn.set_wmediumd_data(intfrefs, links)

    WmediumdConn.intercept_module_loading()

    print "*** Creating nodes"
    sta1 = net.addStation('sta1')
    sta2 = net.addStation('sta2')
    sta3 = net.addStation('sta3')

    print "*** Creating links"
    net.addHoc(sta1, ssid='adNet')
    net.addHoc(sta2, ssid='adNet')
    net.addHoc(sta3, ssid='adNet')

    print "*** Starting network"
    net.start()

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping wmediumd"
    WmediumdConn.disconnect_wmediumd()

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
