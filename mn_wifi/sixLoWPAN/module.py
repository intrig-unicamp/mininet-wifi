# author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)


import os
import subprocess
import logging

from subprocess import check_output as co
from mininet.log import debug, info


class module(object):
    "wireless module"

    externally_managed = False
    devices_created_dynamically = False

    def __init__(self, nodes, nwpan, alt_module):
        self.physicalIfaces = []
        self.start(nodes, nwpan, alt_module)

    def start(self, nodes, nwpan, alt_module):
        """
        :param nodes: list of nodes
        :param nwpan: number of radios
        :param **params: ifb -  Intermediate Functional Block device"""
        wm = subprocess.call(['which', 'iwpan'],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        self.physicalIfaces = self.get_wpan_iface()
        if wm == 0:
            self.load_module(nwpan, alt_module, len(nodes))  # Initatilize Module
            n = 0
            for sensor in nodes:
                for _ in range(0, len(sensor.params['wpan'])):
                    if 'phy' not in sensor.params:
                        n += 1
                        if n > 2:
                            os.system('wpan-hwsim add >/dev/null 2>&1')
            self.assign_iface(nodes)  # iface assign
        else:
            info('*** iwpan will be used, but it is not installed.\n' \
                 '*** Please install iwpan with sudo util/install.sh -6.\n')
            exit(1)

    def load_module(self, nwpan, alt_module, len_nodes):
        """ Load Module
        :param nwpan: number of radios"""
        debug('Loading %s virtual interfaces\n' % nwpan)
        if alt_module:
            os.system('insmod %s radios=%s' % (alt_module, len_nodes))
        else:
            os.system('modprobe mac802154_hwsim')

    @classmethod
    def kill_mod(cls, module):
        if cls.module_loaded(module):
            info("*** Killing %s\n" % module)
            os.system('rmmod %s' % module)

    @classmethod
    def module_loaded(cls, module_name):
        "Checks whether module is running"
        lsmod_proc = subprocess.Popen(['lsmod'], stdout=subprocess.PIPE)
        grep_proc = subprocess.Popen(['grep', module_name],
                                     stdin=lsmod_proc.stdout, stdout=subprocess.PIPE)
        grep_proc.communicate()  # Block until finished
        return grep_proc.returncode == 0

    @classmethod
    def stop(cls):
        'Stop wireless Module'
        cls.kill_mod('mac802154_hwsim')

    def get_virtual_wpan(self):
        'Gets the list of virtual wlans that already exist'
        cmd = "iwpan dev 2>&1 | grep Interface " \
              "| awk '{print $2}'"

        wlans = subprocess.check_output\
            (cmd, shell=True).decode('utf-8').split("\n")

        wlans.pop()
        wlan_list = sorted(wlans)
        wlan_list.sort(key=len, reverse=False)
        return wlan_list

    @staticmethod
    def get_wpan_iface():
        "Gets physical wpans"
        wpan_list = []
        iface_list = co("iwpan dev 2>&1 | grep Interface | awk '{print $2}'",
                        shell=True).decode('utf-8').split('\n')
        for iface in iface_list:
            wpan_list.append(iface)
        wpan_list = sorted(wpan_list)
        wpan_list.pop(0)
        wpan_list.sort(key=len, reverse=False)
        return wpan_list

    def getPhy(self):
        'Gets the list of virtual wpans that already exist'
        cmd = "iwpan dev | grep phy | sed -ne 's/phy#\([0-9]\)/\\1/p'"

        phy = subprocess.check_output\
            (cmd, shell=True).decode('utf-8').split("\n")

        phy = sorted(phy)
        phy.pop(0)

        phy_len = len(self.physicalIfaces)
        for _ in range(phy_len):
            phy.pop(0)

        return phy

    def assign_iface(self, nodes):
        """Assign virtual interfaces for all nodes
        :param nodes: list of wireless nodes"""
        log_filename = '/tmp/mininetwifi-fakelb.log'
        self.logging_to_file(log_filename)
        debug("\n*** Configuring interfaces with appropriated network"
              "-namespaces...\n")
        phy = self.getPhy()
        wlan_list = [x for x in self.get_virtual_wpan() if x not in self.physicalIfaces]
        for id, node in enumerate(nodes):
            node.id = id
            for wpan in range(0, len(node.params['wpan'])):
                if 'phy' in node.params:
                    iface = f"lo{self.physicalIfaces[0]}"
                    result = subprocess.run(["ip", "link", "show", iface],
                                            stdout=subprocess.DEVNULL,
                                            stderr=subprocess.DEVNULL)
                    if result.returncode == 0:
                        os.system(f"ip link del {iface}")
                    os.system('ip link add link {} name lo{} type lowpan'.format(self.physicalIfaces[0], self.physicalIfaces[0]))
                    os.system('ip link set lo{} up'.format(self.physicalIfaces[0]))
                    self.physicalIfaces.pop(0)
                else:
                    os.system('iwpan phy phy{} set netns {}'.format(phy[0], node.pid))
                    node.cmd('ip link set {} down'.format(wlan_list[0]))
                    node.cmd('ip link set {} name {}'.format(wlan_list[0], node.params['wpan'][wpan]))
                    wlan_list.pop(0)
                    phy.pop(0)

    def logging_to_file(self, filename):
        logging.basicConfig(filename=filename,
                            filemode='a',
                            level=logging.DEBUG,
                            format='%(asctime)s - %(levelname)s - %(message)s',
                           )
