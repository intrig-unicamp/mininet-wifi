#!/usr/bin/python

"""This code illustrates how to enable IEEE 802.11p
   *** You have to install wireless-regdb and CRDA.
   *** Please refer to the user manual for further information
   *** Tested with 5805 Mhz"""

from mininet.log import setLogLevel, info
from mn_wifi.link import wmediumd, ITSLink
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
from mn_wifi.wmediumdConnector import interference


def topology():
    "Create a network."
    net = Mininet_wifi(link=wmediumd, wmediumd_mode=interference)

    info("*** Creating nodes\n")
    sta1 = net.addStation('sta1', ip='10.0.0.1/8',
                          position='10,10,0', channel='161')
    sta2 = net.addStation('sta2', ip='10.0.0.2/8',
                          position='20,20,0', channel='161')

    info("*** Configuring Propagation Model\n")
    net.setPropagationModel(model="logDistance", exp=3.5)

    net.setModule('./mac80211_hwsim.ko')

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    net.plotGraph(max_x=200, max_y=200)

    info("*** Starting WiFi Direct\n")
    net.addLink(sta1, cls=ITSLink)
    net.addLink(sta2, cls=ITSLink)

    info("*** Starting network\n")
    net.build()

    info("*** Running CLI\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
