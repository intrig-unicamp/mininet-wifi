#!/usr/bin/python

"""
This example shows how to use the wmediumd connector to prevent mac80211_hwsim stations reaching each other

It starts the wmediumd and 'disables' the connection from sta1 to sta2

author: Patrick Grosse (patrick.grosse@uni-muenster.de)
"""

from mininet.log import setLogLevel
from mininet.net import Mininet
from mininet.wmediumdConnector import WmediumdConn, DynamicWmediumdIntfRef, WmediumdLink, WmediumdServerConn


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
    WmediumdConn.set_wmediumd_data(intfrefs, links, with_server=True)

    WmediumdConn.connect_wmediumd_on_startup()

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Creating links"
    net.addHoc(sta1, ssid='adNet')
    net.addHoc(sta2, ssid='adNet')
    net.addHoc(sta3, ssid='adNet')

    print "*** Starting network"
    net.start()
    print "\n\n\n"
    print "*** Pinging sta2"
    sta1.cmdPrint('ping -c 4 10.0.0.2')

    print "\n\n\n"
    print "*** Update wmediumd"
    WmediumdServerConn.connect()
    WmediumdServerConn.send_update(WmediumdLink(sta1wlan0, sta2wlan0, 0))

    print "\n\n\n"
    print "*** Pinging sta2 again"
    sta1.cmdPrint('ping -c 4 10.0.0.2')

    print "\n\n\n"

    print "*** Stopping wmediumd"
    WmediumdServerConn.disconnect()
    WmediumdConn.disconnect_wmediumd()

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
