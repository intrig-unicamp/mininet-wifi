#!/usr/bin/python

"""
This example shows how to use the wmediumd connector to prevent mac80211_hwsim stations reaching each other

The standard case should be covered in wmediumd_ibss_dynamic.py

author: Patrick Grosse (patrick.grosse@uni-muenster.de)
"""

from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.net import Mininet
from mininet.wmediumdConnector import WmediumdStarter, WmediumdSNRLink, WmediumdIntfRef


def topology():
    """Create a network. sta1 <--> sta2 <--> sta3"""

    print "*** Network creation"
    net = Mininet()

    print "*** Configure wmediumd"
    sta1wlan0 = WmediumdIntfRef('sta1', 'sta1-wlan0', '02:00:00:00:00:00')
    sta2wlan0 = WmediumdIntfRef('sta2', 'sta2-wlan0', '02:00:00:00:01:00')
    sta3wlan0 = WmediumdIntfRef('sta3', 'sta3-wlan0', '02:00:00:00:02:00')

    intfrefs = [sta1wlan0, sta2wlan0, sta3wlan0]
    links = [
        WmediumdSNRLink(sta1wlan0, sta2wlan0, 15),
        WmediumdSNRLink(sta2wlan0, sta1wlan0, 15),
        WmediumdSNRLink(sta2wlan0, sta3wlan0, 15),
        WmediumdSNRLink(sta3wlan0, sta2wlan0, 15)]
    WmediumdStarter.initialize(intfrefs, links)

    print "*** Creating nodes"
    sta1 = net.addStation('sta1')
    sta2 = net.addStation('sta2')
    sta3 = net.addStation('sta3')

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
