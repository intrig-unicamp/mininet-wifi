# author: Ramon Fontes (ramon.fontes@ufrn.br)


import subprocess
import logging
from mininet.log import debug, info


class module(object):
    "btvirt emulator"

    externally_managed = False
    devices_created_dynamically = False

    def __init__(self, nodes):
        self.start(nodes)

    def start(self, nodes):
        """
        :param nodes: list of nodes
        :param btvirt: number of btvirt nodes
        :param **params: ifb -  Intermediate Functional Block device"""
        btvirt = subprocess.call(['which', 'btvirt'],
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if btvirt == 0:
            #self.load_module(nodes)  # Initatilize btvirt
            self.assign_iface(nodes)  # iface assign
        else:
            info('*** btvirt will be used, but it is not installed.\n' \
                 '*** Please install btvirt with sudo util/install.sh -6.\n')
            exit(1)

    def load_module(self, node, nbt):
        """ Load btvirt
        :param nbt: number of radios
        :param iot_module: dir of a fakelb alternative module"""
        debug('Loading %s virtual interfaces\n' % nbt)
        node.cmd('btvirt -B -l%s &' % nbt)

    @classmethod
    def kill_mod(cls, module):
        """Being killed by mn_wifi/clean"""
        #if cls.module_loaded(module):
        #info("*** Killing btvirt process\n")
        #os.system('pkill -f %s' % module)

    @classmethod
    def stop(cls):
        'Stop wireless Module'
        cls.kill_mod('btvirt')

    def get_virtual_bt(self):
        'Gets the list of virtual wlans that already exist'
        cmd = "iwpan dev 2>&1 | grep Interface " \
              "| awk '{print $2}'"

        wlans = subprocess.check_output\
            (cmd, shell=True).decode('utf-8').split("\n")

        wlans.pop()
        wlan_list = sorted(wlans)
        wlan_list.sort(key=len, reverse=False)
        return wlan_list

    def getPhy(self):
        'Gets the list of virtual wlans that already exist'
        cmd = "iwpan dev | grep phy | sed -ne 's/phy#\([0-9]\)/\\1/p'"

        phy = subprocess.check_output\
            (cmd, shell=True).decode('utf-8').split("\n")

        phy = sorted(phy)
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
        wlan_list = self.get_virtual_bt()
        for id, node in enumerate(nodes):
            node.id = id
            for wlan in range(0, len(node.params['bt'])):
                self.load_module(node, 1)
                #os.system('iwpan phy phy{} set netns {}'.format(phy[0], node.pid))
                #node.cmd('ip link set {} down'.format(wlan_list[0]))
                #node.cmd('ip link set {} name {}'.format(wlan_list[0], node.params['bt'][wlan]))
                #wlan_list.pop(0)
                #phy.pop(0)

    def logging_to_file(self, filename):
        logging.basicConfig(filename=filename,
                            filemode='a',
                            level=logging.DEBUG,
                            format='%(asctime)s - %(levelname)s - %(message)s',
                           )
