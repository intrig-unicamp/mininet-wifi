#!/usr/bin/python

'This example shows how to work with authentication'

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi
import os
from os import path


def topology():
    "Create a network."
    cwd = os.getcwd()
    net = Mininet_wifi()

    config = 'key_mgmt=WPA-EAP,' \
    'scan_ssid=1,' \
    'identity="mranga@nist.gov",' \
    'eap=TLS,' \
    'ssid="simplewifi",' \
    'ca_cert="{}/examples/eap-tls/CA/ca.crt",' \
    'client_cert="{}/examples/eap-tls/CA/client.crt",' \
    'private_key="{}/examples/eap-tls/CA/client.key"'.format(cwd, cwd, cwd)

    info("*** Creating nodes\n")
    sta1 = net.addStation('sta1', wpasup_flags='-dd -f /tmp/debug1.txt',
                          wpasup_globals='eapol_version=2', encrypt='wpa2', config=config)

    sta2 = net.addStation('sta2', wpasup_flags='-dd -f /tmp/debug2.txt',
                          wpasup_globals='eapol_version=2', encrypt='wpa2', config=config)

    ap1 = net.addAccessPoint('ap1', 
                            ssid="simplewifi", hostapd_flags='-dd > /tmp/hostapd.txt',
                            mode="g", channel="1", encrypt='wpa2', failMode="standalone",
                            datapath='user', client_isolation=True,
                            config='eap_server=1,'
                                   'ieee8021x=1,'
                                   'wpa=2,'
                                   'eap_message=howdy,'
                                   'eapol_version=2,'
                                   'wpa_key_mgmt=WPA-EAP,'
                                   'logger_syslog=-1,'
                                   'logger_syslog_level=0,'
                                   'ca_cert={}/examples/eap-tls/CA/ca.crt,'
                                   'server_cert={}/examples/eap-tls/CA/server.crt,'
                                   'private_key={}/examples/eap-tls/CA/server.key,'
                                   'eap_user_file={}/examples/eap-tls/eap_users'
                             .format(cwd, cwd, cwd, cwd))

    info("*** Configuring nodes\n")
    net.configureNodes()

    info("*** Associating Stations\n")
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap1)

    info("*** Starting network\n")
    net.build()
    ap1.start([])

    info("*** Adding openflow wireless rule : ")
    # For wireless isolation hack. Put a normal flow in there so stations
    # can ping each other
    ap1.cmd('ovs-ofctl add-flow ap1 "priority=10,actions=in_port,normal"')

    info("\n*** Try the following at the CLI \n")
    info("sta1 ping sta2 \n")
    info("/tmp/debug*.txt and /tmp/hostapd.txt contain logs \n")
    info("cat /var/log/syslog | grep hostapd shows you if the authentication succeeded\n")
    CLI(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology()