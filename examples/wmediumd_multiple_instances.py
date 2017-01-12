#!/usr/bin/python

"""
This example shows how to use the wmediumd connector to prevent mac80211_hwsim stations reaching each other

This example is capable of being run multiple times without interfering with the other instance.
The kernel mod mac80211_hwsim is started only once, as well as wmediumd. mac80211_hwsim is managed over
mac80211_hwsim_mgmt after it has been started. wmediumd includes a unix socket server which allows the management with
external tools like Mininet-WiFi.

author: Patrick Grosse (patrick.grosse@uni-muenster.de)
"""

from mininet.net import Mininet
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.wmediumdConnector import DynamicWmediumdIntfRef, WmediumdLink, WmediumdManager, WmediumdServerConn


def topology():
    """Create a network. sta1 <--> sta2 <--> sta3"""

    print "*** Connect wmediumd manager"
    WmediumdManager.connect()

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

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()

    print "*** Creating links"
    net.addHoc(sta1, ssid='adNet')
    net.addHoc(sta2, ssid='adNet')
    net.addHoc(sta3, ssid='adNet')

    print "*** Starting network"
    for intfref in intfrefs:
        WmediumdServerConn.register_interface(intfref.get_intf_mac())

    for link in links:
        WmediumdServerConn.update_link(link)

    net.start()

    print "*** Running CLI"
    CLI(net)

    print "*** Disconnecting wmediumd manager"
    WmediumdManager.disconnect()

    print "*** Stopping network"
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
