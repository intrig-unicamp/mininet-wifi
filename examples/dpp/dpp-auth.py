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
import subprocess


def onboard_device(configurator,configurator_clicmd, sta, sta_clicmd, mac_addr, ap_dpp_configurator_id, dpp_configurator_key, passwd,ssid) :

    #print("sta Key = " + sta_dpp_configurator_key + "\n")
    # Generate QR code for the device. Store the qr code id returned by the command.
    bootstrapping_info_id = sta.cmd( sta_clicmd + " dpp_bootstrap_gen  type=qrcode mac=" + mac_addr  + " key=" + dpp_configurator_key).split('\n')[1]
    print("bootstrapping_info_id " + bootstrapping_info_id)
    #Get QR Code of device using the bootstrap info id.
    bootstrapping_uri = "'" + sta.cmd(sta_clicmd + " dpp_bootstrap_get_uri " + str(bootstrapping_info_id)).split('\n')[1] + "'"
    #print("bootstrapping_uri " + bootstrapping_uri)
    #Make device listen to DPP request (The central frequency of channel 1 is 2412) in case if enrollee is a client device.
    sta.cmdPrint(sta_clicmd + " dpp_listen " + str(2412) )
    time.sleep(3)


    #On Configurator side.  Enter the QR Code in the Configurator.
    bootstrapping_info_id = configurator.cmd(configurator_clicmd + " dpp_qr_code " + bootstrapping_uri).split("\n")[1]
    info("bootstrapping_info_id " + str(bootstrapping_info_id) + "\n")
    #Send provisioning request to enrollee. (conf is ap-dpp if enrollee is an AP. conf is sta-dpp if enrollee is a client)
    print("ap_dpp_configurator_id " + ap_dpp_configurator_id)
    result = configurator.cmdPrint(configurator_clicmd + " dpp_auth_init peer={} conf=sta-psk ssid={} psk={} configurator={}".format(bootstrapping_info_id,ssid,passwd, ap_dpp_configurator_id ))
    print(">>>>> dpp_auth_init result " + str(result) + "\n")

    sta.cmd(sta_clicmd + " save_config")
    sta.cmdPrint(sta_clicmd + " reconfigure")
 




def topology():
    "Create a network."
    cwd = os.getcwd()
    net = Mininet_wifi()
    ssid = "012a"
    passwd = '12345678'
    cmd = ["wpa_passphrase", ssid, passwd]

    p = subprocess.Popen(cmd,shell=False, stdin= subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    res,err = p.communicate()
    lines = res.split("\n")

    for line in lines:
        print line
        if "psk" in line:
            start = line.strip().find("psk=")
            if start == 0:
                psk=line[5:]

    print("Psk = " + psk)

    info("*** Creating nodes\n")
    # Sta1 is the configurator.
    #                      'update_config=1\n'
    #                      'dpp_config_processing=2',

    ap1 = net.addAccessPoint('ap1', ssid=ssid, mode="g", channel="1",
                             passwd=passwd, encrypt='wpa2',
                             failMode="standalone", datapath='user')


    sta1 = net.addStation('sta1', ssid=ssid, passwd=passwd, encrypt='wpa2', 
           wpasup_flags='-dd -f /tmp/debug1.txt > /tmp/foo.txt',
           wpasup_globals='ctrl_interface=/var/run/wpa_supplicant1\n'
                          'ctrl_interface_group=0\n')

   


    sta2 = net.addStation('sta2',
           wpasup_flags='-dd -f /tmp/debug2.txt',
           wpasup_globals= 'ctrl_interface=/var/run/wpa_supplicant2\n'
                           'ctrl_interface_group=0\n'
                           'update_config=1\n'
                           'pmf=1\n'
                           'dpp_config_processing=2',

           encrypt='wpa2')


    """
    #                           'ieee80211w=1,'
    ap1 = net.addAccessPoint('ap1', 
                            hostapd_flags='-dd > /tmp/hostapd.txt',
                            mode="g", channel="1", 
                            failMode="standalone", datapath='user', 
                            ssid="test",
                            config= \
                                'wpa=2,'
                                'wpa_key_mgmt=WPA-PSK,'
                                'wpa_pairwise=CCMP,'
                                'wpa_passphrase="very secret passphrase",'
                                'logger_syslog=-1,'
                                'logger_syslog_level=0'
                                , isolate_clients=True)
   """


    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    info("*** Associating Stations\n")
    net.addLink(sta1, ap1)
    net.addLink(sta2, ap1)

    sta1.setMAC("00:00:00:00:00:81","sta1-wlan0")
    sta2.setMAC("00:00:00:00:00:82","sta2-wlan0")

    info("*** Starting network\n")
    net.build()
    ap1.start([])

    info("*** Adding openflow wireless rule : \n ")
    # For wireless isolation hack. Put a normal flow in there so stations
    # can ping each other
    ap1.cmd('ovs-ofctl add-flow ap1 "priority=10,actions=in_port,normal"')


    cli_cmd = "wpa_cli -p /var/run/wpa_supplicant1 "
    #cli_cmd = "hostapd_cli "
    configurator = sta1
    configurator.cmdPrint( cli_cmd+" log_level debug")
    dpp_configurator_id = configurator.cmdPrint( cli_cmd + ' dpp_configurator_add ').split('\n')[1]
    dpp_configurator_key = configurator.cmdPrint(cli_cmd + " dpp_configurator_get_key " + str(1)).split('\n')[1]
    configurator.cmdPrint( cli_cmd + " dpp_configurator_sign conf=sta-psk psk={} ssid={} configurator={}".format(psk,ssid,str(dpp_configurator_id)) )
    time.sleep(3)

    onboard_device(sta1,cli_cmd,sta2,"wpa_cli -p /var/run/wpa_supplicant2","00:00:00:00:00:82", dpp_configurator_id,dpp_configurator_key,psk,ssid)

    #cmd = "/usr/bin/wireshark"+ " -k"+ " -i ap1-wlan1"
    #ap1.cmdPrint(cmd + "&")



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
