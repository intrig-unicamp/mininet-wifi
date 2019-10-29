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

    # On enrolee side
    # Generate QR code for the device. Store the qr code id returned by the command.
    info("enrollee: generate QR code for device\n")
    bootstrapping_info_id = sta.cmdPrint( sta_clicmd + " dpp_bootstrap_gen  type=qrcode mac=" + mac_addr  + " key=" + dpp_configurator_key).split('\n')[1]
    #Get QR Code of device using the bootstrap info id.
    info("enrollee: get the qr code using the returned bootstrap_info_id\n")
    bootstrapping_uri = "'" + sta.cmdPrint(sta_clicmd + " dpp_bootstrap_get_uri " + str(bootstrapping_info_id)).split('\n')[1] + "'"
    info("enrollee: listen for dpp provisioning request\n")
    #Make device listen to DPP request (The central frequency of channel 1 is 2412) in case if enrollee is a client device.
    sta.cmdPrint(sta_clicmd + " dpp_listen " + str(2412) )
    time.sleep(3)

    info("Configurator:  Enter the sta QR Code in the Configurator.\n")
    bootstrapping_info_id = configurator.cmdPrint(configurator_clicmd + " dpp_qr_code " + bootstrapping_uri).split("\n")[1]
    info("Configurator: Send provisioning request to enrollee. (conf is ap-dpp if enrollee is an AP. conf is sta-dpp if enrollee is a client)\n")
    result = configurator.cmdPrint(configurator_clicmd + ' dpp_auth_init peer={} conf=sta-psk ssid={} psk={} configurator={}'.format(bootstrapping_info_id,ssid,passwd, ap_dpp_configurator_id ))

    info("Enrollee: save the config file\n")
    sta.cmdPrint(sta_clicmd + " save_config")
    info("Enrollee:  reload the config file\n")
    sta.cmdPrint(sta_clicmd + " reconfigure")

def topology():
    "Create a network."
    cwd = os.getcwd()
    net = Mininet_wifi()
    # Note - there is a bug in the DPP code. This only works for short ssids
    ssid = "012a"
    passwd = '12345678'
    cmd = ["wpa_passphrase", ssid, passwd]

    p = subprocess.Popen(cmd,shell=False, stdin= subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    res,err = p.communicate()
    lines = res.split("\n")

    for line in lines:
        if "psk" in line:
            start = line.strip().find("psk=")
            if start == 0:
                psk=line[5:]

    info("Psk = " + psk + "\n")

    info("*** Creating nodes\n")

    ap1 = net.addAccessPoint('ap1', ssid=ssid, mode="g", channel="1",
                             hostapd_flags='-dd > /tmp/hostapd.txt',
                             passwd=passwd, encrypt='wpa2',
                             failMode="standalone", datapath='user')
    
    # sta1 is the configurator
    sta1 = net.addStation('sta1', ssid=ssid, passwd=passwd, encrypt='wpa2', 
           wpasup_flags='-dd -f /tmp/debug1.txt',
           wpasup_globals='ctrl_interface=/var/run/wpa_supplicant1\n'
                          'ctrl_interface_group=0\n')

    # sta2 is the enrolee.  Note that it does not have the 
    # PSK and it does not know the access point. 
    # It will be configured by the configurator
    sta2 = net.addStation('sta2',
           wpasup_flags='-dd -f /tmp/debug2.txt',
           wpasup_globals= 'ctrl_interface=/var/run/wpa_supplicant2\n'
                           'ctrl_interface_group=0\n'
                           'update_config=1\n'
                           'pmf=1\n'
                           'dpp_config_processing=2',

           encrypt='wpa2')

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
    ap1.cmdPrint('ovs-ofctl add-flow ap1 "priority=10,actions=in_port,normal"')

    cli_cmd = "wpa_cli -p /var/run/wpa_supplicant1 "
    configurator = sta1
    configurator.cmdPrint( cli_cmd+" log_level debug")
    info("Configurator: add a configurator object\n")
    dpp_configurator_id = configurator.cmdPrint( cli_cmd + ' dpp_configurator_add ').split('\n')[1]
    info("Configurator: get the configurator key\n")
    dpp_configurator_key = configurator.cmdPrint(cli_cmd + " dpp_configurator_get_key " + str(1)).split('\n')[1]
    info("Configurator: self sign the configurator\n")
    configurator.cmdPrint( cli_cmd + " dpp_configurator_sign conf=sta-psk psk={} ssid={} configurator={}".format(psk,ssid,str(dpp_configurator_id)) )
    f = open("sta2_0.staconf","r")
    lines = f.read()
    print("******************************\n")
    print("sta2 staconf BEFORE CONFIGURATION\n")
    print(lines)
    print("******************************")
    f.close()
    
    time.sleep(3)

    onboard_device(sta1,cli_cmd,sta2,"wpa_cli -p /var/run/wpa_supplicant2","00:00:00:00:00:82", 
                   dpp_configurator_id,dpp_configurator_key,psk,ssid.encode('hex'))

    time.sleep(3)

    f = open("sta2_0.staconf","r")
    lines = f.read()
    print("******************************\n")
    print("sta2 staconf AFTER CONFIGURATION\n")
    print(lines)
    print("******************************")
    f.close()

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
