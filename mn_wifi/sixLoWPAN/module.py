# author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

import glob
import os
import subprocess
import logging
from mininet.log import debug, info
from sys import version_info as py_version_info


class module(object):
    "wireless module"

    externally_managed = False
    devices_created_dynamically = False

    def __init__(self, sensors, nwpans, iot_module):
        self.start(sensors, nwpans, iot_module)

    def start(self, sensors, nwpans, iot_module):
        """
        :param sensors: list of sensors
        :param nwpans: number of wifi radios
        :param iot_module: dir of a fakelb alternative module
        :param **params: ifb -  Intermediate Functional Block device"""
        wm = subprocess.call(['which', 'iwpan'],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if wm == 0:
            self.load_module(nwpans, iot_module)  # Initatilize WiFi Module
            self.assign_iface(sensors)  # iface assign
        else:
            info('*** iwpan will be used, but it is not installed.\n' \
                 '*** Please install iwpan with sudo util/install.sh -6.\n')
            exit(1)

    def load_module(self, n_radios, iot_module):
        """ Load WiFi Module
        :param n_radios: number of radios
        :param iot_module: dir of a fakelb alternative module"""
        debug('Loading %s virtual interfaces\n' % n_radios)
        if iot_module == 'fakelb':
            os.system('modprobe fakelb numlbs=%s' % n_radios)
        elif iot_module == 'mac802154_hwsim':
            os.system('modprobe mac802154_hwsim')
        else:
            os.system('insmod %s' % iot_module)

    @classmethod
    def kill_fakelb(cls):
        'Kill fakelb'
        info("*** Killing fakelb\n")
        os.system('rmmod fakelb')

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

        cls.kill_fakelb()

    def get_virtual_wpan(self):
        'Gets the list of virtual wlans that already exist'
        cmd = "iwpan dev 2>&1 | grep Interface " \
              "| awk '{print $2}'"

        if py_version_info < (3, 0):
            wlans = subprocess.check_output\
                (cmd, shell=True).split("\n")
        else:
            wlans = subprocess.check_output\
                (cmd, shell=True).decode('utf-8').split("\n")

        wlans.pop()
        wlan_list = sorted(wlans)
        wlan_list.sort(key=len, reverse=False)
        return wlan_list

    def getPhy(self):
        'Gets the list of virtual wlans that already exist'
        cmd = "iwpan dev | grep phy | " \
              "sed -ne 's/phy#\([0-9]\)/\\1/p'"

        if py_version_info < (3, 0):
            phy = subprocess.check_output\
                (cmd, shell=True).split("\n")
        else:
            phy = subprocess.check_output\
                (cmd, shell=True).decode('utf-8').split("\n")

        phy = sorted(phy)
        phy.pop(0)
        return phy

    def assign_iface(self, nodes):
        """Assign virtual interfaces for all nodes
        :param nodes: list of wireless nodes"""
        log_filename = '/tmp/mininetwifi-fakelb.log'
        self.logging_to_file("%s" % log_filename)
        try:
            debug("\n*** Configuring interfaces with appropriated network"
                  "-namespaces...\n")
            phy = self.getPhy()
            wlan_list = self.get_virtual_wpan()
            for node in nodes:
                for wlan in range(0, len(node.params['wpan'])):
                    os.system('iwpan phy phy%s set netns %s' % (phy[0], node.pid))
                    node.cmd('ip link set %s down' % wlan_list[0])
                    node.cmd('ip link set %s name %s'
                             % (wlan_list[0], node.params['wpan'][wlan]))
                    wlan_list.pop(0)
                    phy.pop(0)
        except:
            logging.exception("Warning:")
            info("Warning! Error when loading fakelb. "
                 "Please run sudo 'mn -c' before running your code.\n")
            info("Further information available at %s.\n" % log_filename)
            exit(1)

    def logging_to_file(self, filename):
        logging.basicConfig(filename=filename,
                            filemode='a',
                            level=logging.DEBUG,
                            format='%(asctime)s - %(levelname)s - %(message)s',
                           )
