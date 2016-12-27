"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

import glob
import os
import subprocess
from mininet.log import debug, info

class module(object):
    """ Start and Stop mac80211_hwsim module """

    wlan_list = []
    
    @classmethod
    def loadModule(self, wifiRadios, alternativeModule=''):
        """ Load wireless Module """
        if alternativeModule == '':
            os.system('modprobe mac80211_hwsim radios=%s' % wifiRadios)
            debug('Loading %s virtual interfaces\n' % wifiRadios)
        else:
            os.system('insmod %s radios=%s' % (alternativeModule, wifiRadios))
            debug('Loading %s virtual interfaces\n' % wifiRadios)

    @classmethod
    def stop(self):
        """ Stop wireless Module """
        if glob.glob("*.apconf"):
            os.system('rm *.apconf')

        try:
            subprocess.check_output("lsmod | grep mac80211_hwsim",
                                                          shell=True)
            os.system('rmmod mac80211_hwsim')
        except:
            pass
        
        try:
            (subprocess.check_output("lsmod | grep ifb",
                                                          shell=True))
            os.system('rmmod ifb')
        except:
            pass
        
        try:
            h = subprocess.check_output("ps -aux | grep -ic \'hostapd\'",
                                                          shell=True)
            if h >= 2:
                os.system('pkill -f \'hostapd\'')
        except:
            pass 
            
        try:
            h = subprocess.check_output("ps -aux | grep -ic \'wpa_supplicant -B -Dnl80211\'",
                                                          shell=True)
            if h >= 2:
                os.system('pkill -f \'wpa_supplicant -B -Dnl80211\'')
        except:
            pass 

    @classmethod
    def start(self, nodes, wifiRadios, alternativeModule='', **params):
        """Starting environment"""
        try:
            h = subprocess.check_output("ps -aux | grep -ic \'hostapd\'",
                                                          shell=True)
            if h >= 2:
                os.system('pkill -f \'hostapd\'')
        except:
            pass 
        try:
            (subprocess.check_output("lsmod | grep mac80211_hwsim",
                                                          shell=True))
            os.system('rmmod mac80211_hwsim')
        except:
            pass

        physicalWlan_list = self.getPhysicalWlan()  # Get Phisical Wlan(s)
        self.loadModule(wifiRadios, alternativeModule)  # Initatilize WiFi Module
        phy_list = self.getPhy()  # Get Phy Interfaces
        module.assignIface(nodes, physicalWlan_list, phy_list, **params)

    @classmethod
    def getPhysicalWlan(self):
        """Get the list of physical wlans that already exists in the machine"""
        self.wlans = []
        self.wlans = (subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",
                                                      shell=True)).split('\n')
        self.wlans.pop()
        return self.wlans

    @classmethod
    def getPhy(self):
        """ Get all phys after starting the wireless module """
        phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name hwsim | cut -d/ -f 6 | sort",
                                                             shell=True).split("\n")
        phy.pop()
        phy.sort(key=len, reverse=False)
        return phy
    
    @classmethod
    def loadIFB(self, wlans):
        debug('\nLoading IFB: modprobe ifb numifbs=%s' % wlans)
        os.system('modprobe ifb numifbs=%s' % wlans)
    
    @classmethod
    def assignIface(self, nodes, physicalWlan_list, phy_list, **params):
        """Assign virtual interfaces for all nodes"""
        if 'ifb' in params:
            ifb = params['ifb']
        else:
            ifb = False
        try:
            self.wlan_list = self.getWlanIface(physicalWlan_list)
            if ifb:
                self.loadIFB(len(self.wlan_list))
                ifbID = 0
            for node in nodes:
                if (node.type == 'station' or node.type == 'vehicle') or 'inNamespace' in node.params:    
                    node.ifb = []            
                    for wlan in range(0, len(node.params['wlan'])):
                        os.system('iw phy %s set netns %s' % (phy_list[0], node.pid))
                        node.cmd('ip link set %s name %s up' % (self.wlan_list[0], node.params['wlan'][wlan]))
                        if ifb:
                            node.ifbSupport(wlan, ifbID)  # Adding Support to IFB
                            ifbID += 1
                        if 'car' in node.name and node.type == 'station':
                            node.cmd('iw dev %s-wlan%s interface add %s-mp%s type mp' % (node, wlan, node, wlan))
                            node.cmd('ifconfig %s-mp%s up' % (node, wlan))
                            node.cmd('iw dev %s-mp%s mesh join %s' % (node, wlan, 'ssid'))
                            node.func[wlan] = 'mesh'
                        else:
                            if node.type != 'accessPoint':
                                if node.params['txpower'][wlan] != 20:
                                    node.setTxPower(node.params['wlan'][wlan], node.params['txpower'][wlan])
                        if node.type != 'accessPoint':
                            if node.params['mac'][wlan] == '':
                                iface = node.params['wlan'][wlan]
                                node.params['mac'][wlan] = node.getMAC(iface)
                            else:
                                iface = node.params['wlan'][wlan]
                                mac = node.params['mac'][wlan]
                                node.setMAC(mac, iface)
                        self.wlan_list.pop(0)
                        phy_list.pop(0)
        except:
            info("Something is wrong. Please, run sudo mn -c before running your code.\n")
            exit(1)

    @classmethod
    def getWlanIface(self, physicalWlan):
        """Build a new wlan list removing the physical wlan"""
        wlan_list = []
        iface_list = subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",
                                            shell=True).split('\n')
        for iface in iface_list:
            if iface not in physicalWlan and iface != '':
                wlan_list.append(iface)
        wlan_list = sorted(wlan_list)
        wlan_list.sort(key=len, reverse=False)
        return wlan_list
