"""
Device Provisioning Protocol (DPP) | Wi-Fi Easy Connect

References:
- https://w1.fi/cgit/hostap/tree/wpa_supplicant/README-DPP
- https://www.wi-fi.org/discover-wi-fi/wi-fi-easy-connect

"""

import binascii
import time
import os
from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi
from mn_wifi.node import OVSKernelAP, UserAP, physicalAP

def topology():
    net = Mininet_wifi()

    info(">> Creating AP (as DPP Configurator)...\n")

    ap1 = net.addAccessPoint('ap1', cls=OVSKernelAP, ssid="DPP", channel="1", encrypt="wpa2", ieee80211w="2",  
    hostapd_flags='-dd > /tmp/hostapd.txt',
    config='ctrl_interface=/var/run/hostapd,'
        'wpa_key_mgmt=DPP WPA-PSK,'
        'wpa_passphrase=12345678,'
        'rsn_pairwise=CCMP,'
        'wpa_pairwise=CCMP,'
    )

    info(">> Creating STA1 (as DPP Enrollee)...\n")
    sta1 = net.addStation('sta1', 
        encrypt='wpa2',
        mac='00:00:00:00:00:05',
        wpasup_flags='-d -f /tmp/debug-sta1.txt',
        wpasup_globals='ctrl_interface=DIR=/var/run/wpa_supplicant\n'
        'update_config=1\n'
        'ctrl_interface_group=0\n'
        'pmf=2\n'
        'dpp_config_processing=2',
        config=""
    )

    # Configure Nodes
    net.configureNodes()
    net.addLink(sta1, ap1)

    # Start Network
    net.build()
    ap1.start([])

    # For wireless isolation hack. Put a normal flow in there so stations
    # can ping each other
    ap1.cmd('ovs-ofctl add-flow ap1 "priority=10,actions=in_port,normal"')

    ##############################################################################################################
    ssid_hexdump = binascii.hexlify(b"DPP").decode()
    passwd = binascii.hexlify(b"12345678").decode()

    # Configuring STA1 as Enrollee | Bootstrap method: QR Code
    bootstrap_id1 = sta1.cmd(f"wpa_cli -i sta1-wlan0 dpp_bootstrap_gen type=qrcode chan=81/1 mac=000000000005")
    # Retrieve Bootstrap QR Code URI
    bootstrap_uri1 = sta1.cmd(f"wpa_cli -i sta1-wlan0 dpp_bootstrap_get_uri {bootstrap_id1}")
    info(f">> STA1 Bootstrap URI: {bootstrap_uri1}")

    # STA listen to DPP requests in the band channel 1
    sta1.cmd("wpa_cli -i sta1-wlan0 dpp_listen 2412 30000")

    wpa_configs = [file for file in os.listdir(".") if file.endswith(".staconf")]
    ap_configs = [file for file in os.listdir(".") if file.endswith(".apconf")]

    with open(wpa_configs[0], "r") as f:
        lines = f.read()
        info("\n------------[wpa_supplicant.conf]-------------------\n")
        info(lines)
        info("-----------------------------------------------------\n")

    with open(ap_configs[0], "r") as f:
        lines = f.read()
        info("\n-----------------[hostapd.conf]---------------------\n")
        info(lines)
        info("-----------------------------------------------------\n")

    # Configurator (AP) side
    # Self-Configuring AP as DPP Configurator
    conf_id = ap1.cmd("hostapd_cli -i ap1-wlan1 dpp_configurator_add")
    conf_key = ap1.cmd(f"hostapd_cli -i ap1-wlan1 dpp_configurator_get_key {conf_id}")
    conf_sign = ap1.cmd(f"hostapd_cli -i ap1-wlan1 dpp_configurator_sign conf=ap-dpp configurator={conf_id} ssid={ssid_hexdump}")

    # Reading Enrollee Bootstraping QR-Code URI
    bootstrapping_info_id1 = ap1.cmd(f"hostapd_cli -i ap1-wlan1 dpp_qr_code \"{bootstrap_uri1}\"")

    # Send provisioning request from AP as Configurator to STA1 as Enrollee
    cmd = f"hostapd_cli -i ap1-wlan1 dpp_auth_init peer={bootstrapping_info_id1} conf=sta-psk pass={passwd} ssid={ssid_hexdump} configurator={conf_id}"
    ap1.cmd(cmd)
    time.sleep(10)

    sta1.cmd("wpa_cli -i sta1-wlan0 save_config")

    info("\n*** Checking STA DPP onboarding in the AP... \n")
    check = sta1.cmd("wpa_cli -i sta1-wlan0 status")
    
    info(f"{check}")
    
    if "wpa_state=COMPLETED" in check:  # This state is entered when the full authentication process is completed.
        info("\n[OK] Enrollee associated with Access Point. DPP Successful.\n\n") 
    else:
        info("\n[NOK] Enrollee NOT associated with Access Point. DPP Error.\n\n")

    CLI(net)
    
    info("\n*** Stopping network\n")
    net.stop()


if __name__ == "__main__":
    if os.path.exists("/tmp/hostapd.txt"):
        os.remove("/tmp/hostapd.txt")

    if os.path.exists("/tmp/debug-sta1.txt"):
        os.remove("/tmp/debug-sta1.txt")
    
    setLogLevel('info')
    topology()
