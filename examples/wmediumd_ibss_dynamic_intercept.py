#!/usr/bin/python

"""
This example shows how to use the wmediumd connector to prevent mac80211_hwsim stations reaching each other

This is the standard example of using wmediumd with Mininet-Wifi

author: Patrick Grosse (patrick.grosse@uni-muenster.de)
"""

from mininet.net import Mininet
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.wmediumdConnector import WmediumdConn, DynamicWmediumdIntfRef, WmediumdLink


def topology():
    """Create a network. sta1 <--> sta2 <--> sta3"""

    print "*** Network creation"
    net = Mininet()

    print "*** Creating nodes"
    sta1 = net.addStation('sta1')
    sta2 = net.addStation('sta2')
    sta3 = net.addStation('sta3')

    print "*** Configure wmediumd"
    # This should be done right after the station has been initialized
    sta1wlan0 = DynamicWmediumdIntfRef(sta1)
    sta2wlan0 = DynamicWmediumdIntfRef(sta2)
    sta3wlan0 = DynamicWmediumdIntfRef(sta3)

    intfrefs = [sta1wlan0, sta2wlan0, sta3wlan0]
    links = [
        WmediumdLink(sta1wlan0, sta2wlan0, 15),
        WmediumdLink(sta2wlan0, sta1wlan0, 15),
        WmediumdLink(sta2wlan0, sta3wlan0, 15),
        WmediumdLink(sta3wlan0, sta2wlan0, 15)]
    WmediumdConn.set_wmediumd_data(intfrefs, links)

    WmediumdConn.connect_wmediumd_on_startup()

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

