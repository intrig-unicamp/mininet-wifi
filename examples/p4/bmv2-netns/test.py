#!/usr/bin/python

import os

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi
from mn_wifi.bmv2 import P4RuntimeAP


class MininetWifi_bmv2InNetns(Mininet_wifi):

     def configureControlNetwork(self):
        """Configure control network."""
        info( '*** Configuring the intf control - controller\n' )
        for controller in self.controllers:
            info( controller.name + ' ')
            controller.cmd('ip link set dev lo up')
            info( '\n' )
        
        info( '*** Configuring the intf control for %s APs\n' % len( self.aps ) )
        for ap in self.aps:
            info( ap.name + ' ')
            ap.cmd( 'ip link set dev lo up' )
    


def topology():
    'Create a network.'
    net = MininetWifi_bmv2InNetns(inNamespace = True)

    info('*** Adding stations/hosts\n')
    sta1 = net.addStation('sta1', ip='10.0.0.1', mac='00:00:00:00:00:01')
    h1 = net.addHost('h1', ip='10.0.0.2', mac='00:00:00:00:00:02')
    sta2 = net.addStation('sta2', ip='10.0.0.3', mac='00:00:00:00:00:03')
    h2 = net.addHost('h2', ip='10.0.0.4', mac='00:00:00:00:00:04')

    info('*** Adding P4RuntimeAP\n')
    ap1 = net.addAccessPoint('ap1',  cls=P4RuntimeAP, json_path='build/basic.json', runtime_json_path='runtime/ap1-runtime.json', 
			     log_console = True, log_dir = os.path.abspath('logs'), log_file = 'ap1.log', pcap_dump = os.path.abspath('pcaps'))

    ap2 = net.addAccessPoint('ap2',  cls=P4RuntimeAP, json_path='build/basic.json', runtime_json_path='runtime/ap2-runtime.json', 
			     log_console = True, log_dir = os.path.abspath('logs'), log_file = 'ap2.log', pcap_dump = os.path.abspath('pcaps'))
    

    net.configureWifiNodes()


    info('*** Creating links\n')
    net.addLink(sta1, ap1)
    net.addLink(h1, ap1)
    net.addLink(ap1, ap2)
    net.addLink(sta2, ap2)
    net.addLink(h2, ap2)


    info('*** Starting network\n')
    net.start()
    net.staticArp()

    info('*** Running CLI\n')
    CLI(net)

    info('*** Stopping network\n')
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()
