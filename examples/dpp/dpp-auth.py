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
import socket
import sys
import json

import binascii


def get_hex_key_from_der_key(derfile):
    with open(derfile, mode="rb") as file:
        fileContent = file.read()
    hex_str = binascii.b2a_hex(fileContent)
    return hex_str


def onboard_device(configurator,configurator_clicmd, sta, sta_clicmd, mac_addr,  passwd,ssid, cacertpath) :

    # On enrolee side
    # Generate QR code for the device. Store the qr code id returned by the command.
    #  The private key of the certificate.
    dpp_configurator_key="3077020101042026F1181A112402FC90A286B299997A146B5C311C15753211DF9927641702CC58A00A06082A8648CE3D030107A14403420004883D409D7FDED9CBD4800991D615E4F559FCCC0B347D6AFF561FF5EFB60FD94B4E96D46235D0EA99B07B14C032AAD3404EB41DF630215ACDD2C5778FC81A2B57"

    dpp_configurator_id = sta.cmdPrint( sta_clicmd + ' dpp_configurator_add key=' + dpp_configurator_key + " curve=prime256v1").split('\n')[1]
    info("enrollee: generate QR code for device public key = {}\n".format(dpp_configurator_key))
    bootstrapping_info_id = sta.cmd( sta_clicmd + " dpp_bootstrap_gen  type=qrcode mac=" + mac_addr  + " key=" + dpp_configurator_key ).split('\n')[1]
    #Get QR Code of device using the bootstrap info id.
    info("enrollee: get the qr code using the returned bootstrap_info_id\n")
    bootstrapping_uri = "'" + sta.cmd(sta_clicmd + " dpp_bootstrap_get_uri " + str(bootstrapping_info_id)).split('\n')[1] + "'"
    info("enrollee : show the dpp bootstrap info\n")
    sta.cmdPrint(sta_clicmd + " dpp_bootstrap_info " + bootstrapping_info_id)
    info("bootstrapping_uri = " + bootstrapping_uri + "\n")
    info("enrollee: listen for dpp provisioning request\n")
    #Make device listen to DPP request (The central frequency of channel 1 is 2412) in case if enrollee is a client device.
    sta.cmd(sta_clicmd + " dpp_listen " + str(2412) )
    time.sleep(3)

    info("Configurator:  Enter the sta QR Code in the Configurator.\n")
    bootstrapping_info_id = configurator.cmdPrint(configurator_clicmd + " dpp_qr_code " + bootstrapping_uri).split("\n")[1]
    info("Configurator: Send provisioning request to enrollee. (conf is ap-dpp if enrollee is an AP. conf is sta-dpp if enrollee is a client). configurator_id = {} \n".format(dpp_configurator_id))
    result = configurator.cmd(configurator_clicmd + ' dpp_auth_init peer={} conf=sta-psk ssid={} psk={} configurator={} cacert={}'.format(bootstrapping_info_id,ssid,passwd, dpp_configurator_id ,cacertpath))
    time.sleep(3)
    result = configurator.cmd(configurator_clicmd + " dpp_config_status id=" + str(bootstrapping_info_id)).split("\n")[1]
    print("dpp_config_status returned " + str(result))
    jsonval = json.loads(result)
    mud_url = jsonval["config_status"]["mud_url"]
    idevid = jsonval["config_status"]["idevid"]
    print("idevid "+ idevid)
    print ("mud_url " + mud_url)
	

    info("Enrollee: save the config file\n")
    sta.cmd(sta_clicmd + " save_config")
    info("Enrollee:  reload the config file\n")
    sta.cmd(sta_clicmd + " reconfigure")

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
    cwd = os.path.realpath(os.getcwd())


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
                           'dpp_mud_url=https://www.nist.local/nistmud1\n'
                           'dpp_idevid={}/DevID50/DevIDCredentials/IDevID50.cert.pem\n'
                           'dpp_name=MyDpp\n'
                           'dpp_config_processing=2'.format(cwd),

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
    ap1.cmd('ovs-ofctl add-flow ap1 "priority=10,actions=in_port,normal"')
    #sta1.cmdPrint("python sockserver.py&")

    
    # on the configurator 

    cli_cmd = "wpa_cli -p /var/run/wpa_supplicant1 "
    configurator = sta1
    dpp_version = configurator.cmd( cli_cmd + " get_capability dpp").split('\n')[1]
    print("dpp_version is " + dpp_version)
    if dpp_version != "DPP=2":
        sys.exit()

    # todo -- this is redundant remove it.
    configurator.cmdPrint( cli_cmd + " dpp_controller_start tcp_port=9090").split('\n')[1]

    configurator.cmd( cli_cmd+" log_level debug")
    info("Configurator: add a configurator object\n")
    dpp_configurator_id = configurator.cmd( cli_cmd + ' dpp_configurator_add ').split('\n')[1]
    info("Configurator: self sign the configurator\n")
    configurator.cmdPrint( cli_cmd + " dpp_configurator_sign conf=sta-psk psk={} ssid={} configurator={}".format(psk,ssid,str(dpp_configurator_id)) )

        

    f = open("sta2_0.staconf","r")
    lines = f.read()
    print("******************************\n")
    print("sta2 staconf BEFORE CONFIGURATION\n")
    print(lines)
    print("******************************")
    f.close()
    
    time.sleep(5)

    onboard_device(sta1,cli_cmd,sta2,"wpa_cli -p /var/run/wpa_supplicant2","00:00:00:00:00:82", psk,ssid.encode('hex'),"{}/DevID50/CredentialChain/ca-chain.cert.pem".format(cwd))

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
