"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

import glob
import os
import subprocess
import re
import logging
from mininet.log import debug, error, info

class module(object):
    """ wireless module """

    wlan_list = []
    hwsim_ids = []
    prefix = ""
    externally_managed = False
    devices_created_dynamically = False
    phyID = 0

    @classmethod
    def loadModule(self, wifiRadios, alternativeModule=''):
        """ Load WiFi Module 
        
        :param wifiRadios: number of wifi radios
        :param alternativeModule: dir of a mac80211_hwsim alternative module
        """
        debug('Loading %s virtual interfaces\n' % wifiRadios)
        if not self.externally_managed:
            if alternativeModule == '':
                output_ = os.system('modprobe mac80211_hwsim radios=0 >/dev/null 2>&1')
            else:
                output_ = os.system('insmod %s radios=0 >/dev/null 2>&1' % alternativeModule)
            
            """output_ is different of zero in Kernel 3.13.x. radios=0 doesn't work in such kernel version"""
            if output_ == 0:
                self.__create_hwsim_mgmt_devices(wifiRadios)
            else:
                if wifiRadios == 0:
                    wifiRadios = 1 #Useful for tests in Kernels like Kernel 3.13.x
                if alternativeModule == '':
                    os.system('modprobe mac80211_hwsim radios=%s' % wifiRadios)
                else:
                    os.system('insmod %s radios=%s' % (alternativeModule, wifiRadios))
        else:
            self.devices_created_dynamically = True
            self.__create_hwsim_mgmt_devices(wifiRadios)

    @classmethod
    def __create_hwsim_mgmt_devices(cls, wifi_radios):
        # generate prefix
        phys = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name hwsim | cut -d/ -f 6 | sort",
                                       shell=True).split("\n")
        num = 0
        numokay = False
        cls.prefix = ""
        while not numokay:
            cls.prefix = "mn%02ds" % num
            numokay = True
            for phy in phys:
                if phy.startswith(cls.prefix):
                    num += 1
                    numokay = False
                    break
        try:
            for i in range(0, wifi_radios):
                p = subprocess.Popen(["hwsim_mgmt", "-c", "-n", cls.prefix + ("%02d" % i)], stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE, bufsize=-1)
                output, err_out = p.communicate()
                if p.returncode == 0:
                    m = re.search("ID (\d+)", output)
                    debug("\nCreated mac80211_hwsim device with ID %s" % m.group(1))
                    cls.hwsim_ids.append(m.group(1))
                else:
                    error("\nError on creating mac80211_hwsim device with name %s" % (cls.prefix + ("%02d" % i)))
                    error("\nOutput: %s" % output)
                    error("\nError: %s" % err_out)
        except:
            print "Warning! If you already had Mininet-WiFi installed, please run util/install.sh -W \
                                    and then make install. A new API for mac80211_hwsim has been created."

    @classmethod
    def stop(self):
        """ Stop wireless Module """
        if glob.glob("*.apconf"):
            os.system('rm *.apconf')
        if glob.glob("*.staconf"):
            os.system('rm *.staconf')
        if glob.glob("*wifiDirect.conf"):
            os.system('rm *wifiDirect.conf')
        if glob.glob("*.nodeParams"):
            os.system('rm *.nodeParams')

        if self.devices_created_dynamically:
            for hwsim_id in self.hwsim_ids:
                p = subprocess.Popen(["hwsim_mgmt", "-d", hwsim_id], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE, bufsize=-1)
                output, err_out = p.communicate()
                if p.returncode == 0:
                    m = re.search("ID (\d+)", output)
                    debug("\nDeleted mac80211_hwsim device with ID %s" % m.group(1))
                else:
                    error("\nError on deleting mac80211_hwsim device with ID %s" % hwsim_id)
                    error("\nOutput: %s" % output)
                    error("\nError: %s" % err_out)
        else:
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
                os.system('pkill -f \'hostapd -B mn%d\'' % os.getpid())
        except:
            pass 
            
        try:
            confnames = "mn%d_" % os.getpid()
            os.system('pkill -f \'wpa_supplicant -B -Dnl80211 -c%s\'' % confnames)
        except:
            pass

        try:
            pidfiles = "mn%d_" % os.getpid()
            os.system('pkill -f \'wpa_supplicant -B -Dnl80211 -P %s\'' % pidfiles)
        except:
            pass

        if not self.externally_managed:
            try:
                info("*** Killing wmediumd\n")
                os.system('pkill wmediumd')
            except:
                pass

    @classmethod
    def start(self, nodes, wifiRadios, alternativeModule='', **params):
        """Starts environment
        
        :param nodes: list of wireless nodes
        :param wifiRadios: number of wifi radios
        :param alternativeModule: dir of a mac80211_hwsim alternative module
        :param **params: ifb -  Intermediate Functional Block device
        """
        """kill hostapd if it is already running"""
        try:
            h = subprocess.check_output("ps -aux | grep -ic \'hostapd\'",
                                                          shell=True)
            if h >= 2:
                os.system('pkill -f \'hostapd\'')
        except:
            pass

        physicalWlans = self.getPhysicalWlan()  # Gets Physical Wlan(s)
        self.loadModule(wifiRadios, alternativeModule)  # Initatilize WiFi Module
        phys = self.getPhy()  # Get Phy Interfaces
        module.assignIface(nodes, physicalWlans, phys, **params)  # iface assign

    @classmethod
    def getPhysicalWlan(self):
        """Gets the list of physical wlans that already exist"""
        self.wlans = []
        self.wlans = (subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",
                                                      shell=True)).split('\n')
        self.wlans.pop()
        return self.wlans

    @classmethod
    def getPhy(self):
        """Gets all phys after starting the wireless module"""
        if self.devices_created_dynamically:
            phy = subprocess.check_output(
                "find /sys/kernel/debug/ieee80211 -regex \".*/%s.*/hwsim\" | cut -d/ -f 6 | sort" % self.prefix,
                shell=True).split("\n")
        else:
            phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name hwsim | cut -d/ -f 6 | sort",
                                          shell=True).split("\n")
        phy.pop()
        phy.sort(key=len, reverse=False)
        return phy
    
    @classmethod
    def loadIFB(self, wlans):
        """ Loads IFB
        
        :param wlans: Number of wireless interfaces
        """
        debug('\nLoading IFB: modprobe ifb numifbs=%s' % wlans)
        os.system('modprobe ifb numifbs=%s' % wlans)
    
    @classmethod
    def assignIface(self, nodes, physicalWlans, phys, **params):
        """Assign virtual interfaces for all nodes
        
        :param nodes: list of wireless nodes
        :param physicalWlans: list of Physical Wlans
        :param phys: list of phys
        :param **params: ifb -  Intermediate Functional Block device
        """
        
        log_filename = '/tmp/mininetwifi-mac80211_hwsim.log'
        self.logging_to_file("%s" % log_filename)
        
        if 'ifb' in params:
            ifb = params['ifb']
        else:
            ifb = False
        try:
            self.wlan_list = self.getWlanIface(physicalWlans)
            if ifb:
                self.loadIFB(len(self.wlan_list))
                ifbID = 0
            for node in nodes:
                if (node.type == 'station' or node.type == 'vehicle') or 'inNamespace' in node.params:    
                    node.ifb = []            
                    for wlan in range(0, len(node.params['wlan'])):
                        node.phyID[wlan] = self.phyID
                        self.phyID+=1
                        os.system('iw phy %s set netns %s' % (phys[0], node.pid))
                        node.cmd('ip link set %s name %s up' % (self.wlan_list[0], node.params['wlan'][wlan]))
                        if ifb:
                            node.ifbSupport(wlan, ifbID)  # Adding Support to IFB
                            ifbID += 1
                        self.wlan_list.pop(0)
                        phys.pop(0)
        except:
            logging.exception("Warning:")
            info("Warning! Error when loading mac80211_hwsim. Please run sudo 'mn -c' before running your code.\n")
            info("Further information available at %s.\n" % log_filename)
            exit(1)
        
            
    @classmethod
    def logging_to_file(self, filename):
        logging.basicConfig( filename=filename,
                             filemode='a',
                             level=logging.DEBUG,
                             format= '%(asctime)s - %(levelname)s - %(message)s',
                           )

    @classmethod
    def getWlanIface(self, physicalWlan):
        """Build a new wlan list removing the physical wlan
        
        :param physicalWlans: list of Physical Wlans
        """
        wlan_list = []
        iface_list = subprocess.check_output("iwconfig 2>/dev/null | grep IEEE | awk '{print $1}'",
                                            shell=True).split('\n')
        for iface in iface_list:
            if iface not in physicalWlan and iface != '':
                wlan_list.append(iface)
        wlan_list = sorted(wlan_list)
        wlan_list.sort(key=len, reverse=False)
        return wlan_list
