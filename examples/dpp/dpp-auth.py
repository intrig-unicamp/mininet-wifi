#!/usr/bin/python

'This example shows how to work with authentication'

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI_wifi
from mn_wifi.net import Mininet_wifi
import time
import os
import os.path
import os
from os import path




def topology():
    "Create a network."
    cwd = os.getcwd()
    net = Mininet_wifi()

    info("*** Creating nodes\n")
    sta1 = net.addStation('sta1',
           wpasup_flags='-dd  -f /tmp/debug1.txt',
           wpasup_globals='ctrl_interface=/var/run/wpa_supplicant1\n'
                          'ctrl_interface_group=0\n'
                          'update_config=1\n'
                          'pmf=2\n'
                          'dpp_config_processing=2',
           encrypt='wpa2')



    sta2 = net.addStation('sta2',
           wpasup_flags='-dd -f /tmp/debug2.txt',
           wpasup_globals= 'ctrl_interface=/var/run/wpa_supplicant2\n'
                           'ctrl_interface_group=0\n'
                           'update_config=1\n'
                           'pmf=2\n'
                           'dpp_config_processing=2',

           encrypt='wpa2')


    ap1 = net.addAccessPoint('ap1', 
                            hostapd_flags='-ddd > /tmp/hostapd.txt',
                            mode="g", channel="1", 
                            failMode="standalone", datapath='user', 
                            ssid="simplewifi",
                            config= \
                                'wpa=2,'
                                'wpa_key_mgmt=DPP,'
                                'ieee80211w=1,'
                                'wpa_pairwise=CCMP,'
                                'rsn_pairwise=CCMP'
                                , isolate_clients=True)


    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    info("*** Associating Stations\n")
    net.addLink(sta1, ap1)
    #net.addLink(sta2, ap1)

    sta1.setMAC("00:00:00:00:00:81","sta1-wlan0")

    info("*** Starting network\n")
    net.build()
    ap1.start([])

    info("*** Adding openflow wireless rule : \n ")
    # For wireless isolation hack. Put a normal flow in there so stations
    # can ping each other
    ap1.cmd('ovs-ofctl add-flow ap1 "priority=10,actions=in_port,normal"')

    cli_cmd = "hostapd_cli "
    ap_dpp_configurator_id = ap1.cmd( cli_cmd + ' dpp_configurator_add').split('\n')[1].encode("string-escape")
    ap_dpp_configurator_key = ap1.cmd(cli_cmd + " dpp_configurator_get_key " + str(1)).split('\n')[1]
    info("ap Key = " + ap_dpp_configurator_key + "\n")

    cli_cmd = "wpa_cli -p /var/run/wpa_supplicant1 "
    ap1.cmd(cli_cmd + " log_level debug")
    sta_dpp_configurator_id = ap1.cmd( cli_cmd + ' dpp_configurator_add').split('\n')[1].encode("string-escape")
    sta_dpp_configurator_key = ap1.cmd(cli_cmd + " dpp_configurator_get_key " + str(1)).split('\n')[1].encode("string-escape")
    print("sta Key = " + sta_dpp_configurator_key + "\n")
    # Generate QR code for the device. Store the qr code id returned by the command.
    bootstrapping_info_id = sta1.cmd( cli_cmd + " dpp_bootstrap_gen  type=qrcode mac=" + "00:00:00:00:00:81 " + " key=" + sta_dpp_configurator_key).split('\n')[1]
    #Get QR Code of device using the bootstrap info id.
    bootstrapping_uri = "'" + sta1.cmd(cli_cmd + " dpp_bootstrap_get_uri " + str(bootstrapping_info_id)).split('\n')[1] + "'"
    print("bootstrapping_uri " + bootstrapping_uri)
    #Make device listen to DPP request (The central frequency of channel 1 is 2412) in case if enrollee is a client device.
    sta1.cmdPrint(cli_cmd + " dpp_listen " + str(2412) )


    #On Configurator side.  Enter the QR Code in the Configurator.
    cli_cmd = "hostapd_cli "
    bootstrapping_info_id = ap1.cmd(cli_cmd + " dpp_qr_code " + bootstrapping_uri).split("\n")[1]
    info("bootstrapping_info_id " + str(bootstrapping_info_id) + "\n")
    #Send provisioning request to enrollee. (conf is ap-dpp if enrollee is an
    # AP. conf is sta-dpp if enrollee is a client)
    print("ap_dpp_configurator_id " + ap_dpp_configurator_id)
    result = ap1.cmdPrint(cli_cmd + " dpp_auth_init " +"peer=" +  bootstrapping_info_id + " conf=sta-dpp" + " configurator=" + str(ap_dpp_configurator_id) )
    info(">>>>> dpp_auth_init result " + str(result) + "\n")

    cli_cmd = "wpa_cli -p /var/run/wpa_supplicant1"
    sta1.cmd(cli_cmd + " save_config")
    




    info("\n*** Try the following at the CLI \n")
    info("sta1 ping sta2 \n")
    info("/tmp/debug*.txt and /tmp/hostapd.txt contain logs \n")
    info("cat /var/log/syslog | grep hostapd shows you if the authentication succeeded\n")
    CLI_wifi(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    if path.exists("/tmp/debug1.txt") :
        os.remove("/tmp/debug1.txt")
    if path.exists("/tmp/debug2.txt") :
        os.remove("/tmp/debug2.txt")
    if path.exists("/tmp/hostapd.txt") :
        os.remove("/tmp/hostapd.txt")
    setLogLevel('info')
    topology()
