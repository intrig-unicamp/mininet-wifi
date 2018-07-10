"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

import glob
import os
import re
import subprocess
import logging
from sys import version_info as py_version_info
from mininet.log import debug, info, error


class module(object):
    "wireless module"

    wlan_list = []
    hwsim_ids = []
    prefix = ""
    externally_managed = False
    devices_created_dynamically = False
    phyID = 0

    @classmethod
    def load_module(cls, n_radios, alt_module):
        """Load WiFi Module
        :param n_radios: number of wifi radios
        :param alt_module: dir of a mac80211_hwsim alternative module"""
        debug('Loading %s virtual wifi interfaces\n' % n_radios)
        if not cls.externally_managed:
            if alt_module:
                output_ = os.system('insmod %s radios=0 >/dev/null 2>&1'
                                    % alt_module)
            else:
                output_ = os.system('modprobe mac80211_hwsim radios=0 '
                                    '>/dev/null 2>&1')

            """output_ is different of zero in Kernel 3.13.x. radios=0 doesn't
             work in such kernel version"""
            if output_ == 0:
                cls.__create_hwsim_mgmt_devices(n_radios)
            else:
                # Useful for tests in Kernels like Kernel 3.13.x
                if n_radios == 0:
                    n_radios = 1
                if alt_module:
                    os.system('modprobe mac80211_hwsim radios=%s' % n_radios)
                else:
                    os.system('insmod %s radios=%s' % (alt_module,
                                                       n_radios))
        else:
            cls.devices_created_dynamically = True
            cls.__create_hwsim_mgmt_devices(n_radios)

    @classmethod
    def __create_hwsim_mgmt_devices(cls, n_radios):
        # generate prefix
        if py_version_info < (3, 0):
            phys = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name "
                                           "hwsim | cut -d/ -f 6 | sort",
                                           shell=True).split("\n")
        else:
            phys = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name "
                                           "hwsim | cut -d/ -f 6 | sort",
                                           shell=True).decode('utf-8').split("\n")
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
            for i in range(0, n_radios):
                p = subprocess.Popen(["hwsim_mgmt", "-c", "-n", cls.prefix +
                                      ("%02d" % i)], stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE, bufsize=-1)
                output, err_out = p.communicate()
                if p.returncode == 0:
                    if py_version_info < (3, 0):
                        m = re.search("ID (\d+)", output)
                    else:
                        m = re.search("ID (\d+)", output.decode())
                    debug("Created mac80211_hwsim device with ID %s\n" % m.group(1))
                    cls.hwsim_ids.append(m.group(1))
                else:
                    error("\nError on creating mac80211_hwsim device with name %s"
                          % (cls.prefix + ("%02d" % i)))
                    error("\nOutput: %s" % output)
                    error("\nError: %s" % err_out)
        except:
            info("Warning! If you already had Mininet-WiFi installed "
                 "please run util/install.sh -W and then sudo make install.\n")

    @classmethod
    def kill_hostapd(cls):
        'Kill hostapd'
        info("*** Killing hostapd\n")
        os.system('pkill -f \'hostapd -B mn%d\'' % os.getpid())

    @classmethod
    def kill_wmediumd(cls):
        'Kill wmediumd'
        info("*** Killing wmediumd\n")
        os.system('pkill -f wmediumd >/dev/null 2>&1')

    @classmethod
    def kill_mac80211_hwsim(cls):
        'Kill mac80211_hwsim'
        info("*** Killing mac80211_hwsim\n")
        os.system('rmmod mac80211_hwsim >/dev/null 2>&1')

    @classmethod
    def stop(cls):
        'Stop wireless Module'
        if glob.glob("*.apconf"):
            os.system('rm *.apconf')
        if glob.glob("*.staconf"):
            os.system('rm *.staconf')
        if glob.glob("*wifiDirect.conf"):
            os.system('rm *wifiDirect.conf')
        if glob.glob("*.nodeParams"):
            os.system('rm *.nodeParams')

        try:
            (subprocess.check_output("lsmod | grep ifb", shell=True))
            os.system('rmmod ifb')
        except:
            pass

        try:
            confnames = "mn%d_" % os.getpid()
            os.system('pkill -f \'wpa_supplicant -B -Dnl80211 -c%s\''
                      % confnames)
        except:
            pass

        try:
            pidfiles = "mn%d_" % os.getpid()
            os.system('pkill -f \'wpa_supplicant -B -Dnl80211 -P %s\''
                      % pidfiles)
        except:
            pass

        cls.kill_mac80211_hwsim()

    @classmethod
    def start(cls, nodes, n_radios, alt_module, **params):
        """Starts environment

        :param nodes: list of wireless nodes
        :param n_radios: number of wifi radios
        :param alt_module: dir of a mac80211_hwsim alternative module
        :param **params: ifb -  Intermediate Functional Block device"""
        """kill hostapd if it is already running"""
        try:
            h = subprocess.check_output("ps -aux | grep -ic \'hostapd\'",
                                        shell=True)
            if h >= 2:
                os.system('pkill -f \'hostapd\'')
        except:
            pass

        physicalWlans = cls.get_physical_wlan()  # Gets Physical Wlan(s)
        cls.load_module(n_radios, alt_module)  # Initatilize WiFi Module
        phys = cls.get_phy()  # Get Phy Interfaces
        module.assign_iface(nodes, physicalWlans, phys, **params)  # iface assign

    @classmethod
    def get_physical_wlan(cls):
        'Gets the list of physical wlans that already exist'
        cls.wlans = []
        if py_version_info < (3, 0):
            cls.wlans = (subprocess.check_output("iw dev 2>&1 | grep Interface "
                                                 "| awk '{print $2}'",
                                                 shell=True)).split("\n")
        else:
            cls.wlans = (subprocess.check_output("iw dev 2>&1 | grep Interface "
                                                 "| awk '{print $2}'",
                                                 shell=True)).decode('utf-8').split("\n")
        cls.wlans.pop()
        return cls.wlans

    @classmethod
    def get_phy(cls):
        'Gets all phys after starting the wireless module'
        if py_version_info < (3, 0):
            phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name "
                                          "hwsim | cut -d/ -f 6 | sort",
                                          shell=True).split("\n")
        else:
            phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name "
                                          "hwsim | cut -d/ -f 6 | sort",
                                          shell=True).decode('utf-8').split("\n")
        phy.pop()
        phy.sort(key=len, reverse=False)
        return phy

    @classmethod
    def load_ifb(cls, wlans):
        """ Loads IFB

        :param wlans: Number of wireless interfaces
        """
        debug('\nLoading IFB: modprobe ifb numifbs=%s' % wlans)
        os.system('modprobe ifb numifbs=%s' % wlans)

    @classmethod
    def assign_iface(cls, nodes, physicalWlans, phys, **params):
        """Assign virtual interfaces for all nodes

        :param nodes: list of wireless nodes
        :param physicalWlans: list of Physical Wlans
        :param phys: list of phys
        :param **params: ifb -  Intermediate Functional Block device"""
        from mn_wifi.node import Station, Car

        log_filename = '/tmp/mininetwifi-mac80211_hwsim.log'
        cls.logging_to_file("%s" % log_filename)

        if 'ifb' in params:
            ifb = params['ifb']
        else:
            ifb = False
        try:
            cls.wlan_list = cls.get_wlan_iface(physicalWlans)
            if ifb:
                cls.load_ifb(len(cls.wlan_list))
                ifbID = 0
            debug("\n*** Configuring interfaces with appropriated network"
                  "-namespaces...\n")
            for node in nodes:
                if (isinstance(node, Station) or isinstance(node, Car)) \
                        or 'inNamespace' in node.params:
                    node.ifb = []
                    for wlan in range(0, len(node.params['wlan'])):
                        node.phyID[wlan] = cls.phyID
                        cls.phyID += 1
                        if py_version_info < (3, 0):
                            rfkill = subprocess.check_output(
                                'rfkill list | grep %s | awk \'{print $1}\''
                                '| tr -d ":"' % phys[0], shell=True).split('\n')
                        else:
                            rfkill = subprocess.check_output(
                                'rfkill list | grep %s | awk \'{print $1}\''
                                '| tr -d ":"' % phys[0],
                                shell=True).decode('utf-8').split('\n')
                        debug('rfkill unblock %s\n' % rfkill[0])
                        os.system('rfkill unblock %s' % rfkill[0])
                        os.system('iw phy %s set netns %s' % (phys[0], node.pid))
                        node.cmd('ip link set %s down' % cls.wlan_list[0])
                        node.cmd('ip link set %s name %s'
                                 % (cls.wlan_list[0], node.params['wlan'][wlan]))
                        if ifb:
                            node.ifbSupport(wlan, ifbID)  # Adding Support to IFB
                            ifbID += 1
                        cls.wlan_list.pop(0)
                        phys.pop(0)
        except:
            logging.exception("Warning:")
            info("Warning! Error when loading mac80211_hwsim. "
                 "Please run sudo 'mn -c' before running your code.\n")
            info("Further information available at %s.\n" % log_filename)
            exit(1)

    @classmethod
    def logging_to_file(cls, filename):
        logging.basicConfig(filename=filename,
                            filemode='a',
                            level=logging.DEBUG,
                            format='%(asctime)s - %(levelname)s - %(message)s',
                           )

    @classmethod
    def get_wlan_iface(cls, physicalWlan):
        """Build a new wlan list removing the physical wlan

        :param physicalWlans: list of Physical Wlans"""
        wlan_list = []

        if py_version_info < (3, 0):
            iface_list = subprocess.check_output("iw dev 2>&1 | grep Interface | "
                                                 "awk '{print $2}'",
                                                 shell=True).split('\n')

        else:
            iface_list = subprocess.check_output("iw dev 2>&1 | grep Interface | "
                                                 "awk '{print $2}'",
                                                 shell=True).decode('utf-8').split('\n')
        for iface in iface_list:
            if iface not in physicalWlan and iface != '':
                wlan_list.append(iface)
        wlan_list = sorted(wlan_list)
        wlan_list.sort(key=len, reverse=False)
        return wlan_list
