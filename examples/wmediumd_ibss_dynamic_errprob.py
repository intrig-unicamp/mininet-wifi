#!/usr/bin/python

"""
This example shows how to use the wmediumd connector to prevent mac80211_hwsim stations reaching each other

The standard case should be covered in wmediumd_ibss_dynamic.py

author: Patrick Grosse (patrick.grosse@uni-muenster.de)
"""

from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.net import Mininet
from mininet.wmediumdConnector import DynamicWmediumdIntfRef, WmediumdERRPROBLink, WmediumdStarter


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
        WmediumdERRPROBLink(sta1wlan0, sta2wlan0, 0.2),
        WmediumdERRPROBLink(sta2wlan0, sta1wlan0, 0.2),
        WmediumdERRPROBLink(sta2wlan0, sta3wlan0, 0.1),
        WmediumdERRPROBLink(sta3wlan0, sta2wlan0, 0.1)]
    WmediumdStarter.initialize(intfrefs, links, use_errprob=True)

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Start wmediumd"
    WmediumdStarter.start()

    print "*** Creating links"
    net.addHoc(sta1, ssid='adNet')
    net.addHoc(sta2, ssid='adNet')
    net.addHoc(sta3, ssid='adNet')

    print "*** Starting network"
    net.start()

    print "*** Running CLI"
    CLI(net)

    print "*** Stopping wmediumd"
    WmediumdStarter.stop()

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
