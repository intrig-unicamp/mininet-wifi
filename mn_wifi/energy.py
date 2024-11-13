"""
   Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
   @author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""
import re

from threading import Thread as thread
from time import sleep, time

from mininet.log import error


class Energy(object):

    thread_ = None
    cat_dev = 'cat /proc/net/dev | grep {} |  awk \'{}\''

    def __init__(self, nodes):
        Energy.thread_ = thread(target=self.start, args=(nodes,))
        Energy.thread_.daemon = True
        Energy.thread_._keep_alive = True
        Energy.thread_.start()

    def start(self, nodes):

        for node in nodes:
            for intf in node.wintfs.values():
                intf.rx, intf.tx = 0, 0

        try:
            while self.thread_._keep_alive:
               sleep(1)  # set sleep time to 1 second
               for node in nodes:
                   for intf in node.wintfs.values():
                       intf.consumption += self.getTotalEnergyConsumption(intf)
        except:
            error("Error with the energy consumption function\n")

    def clean_and_convert(self, value):
        # Remove escape sequences ANSI
        ansi_escape = re.compile(r'\x1B[@-_][0-?]*[ -/]*[@-~]')
        cleaned_value = ansi_escape.sub('', value).replace("\n", "").replace("\r", "")

        try:
            return int(cleaned_value)
        except ValueError:
            print(f"Erro ao converter para inteiro: '{cleaned_value}'")
            return None  # Ou um valor padrão caso desejado

    def get_cat_dev(self, intf, col):
        p = '{print $%s}' % col
        value = intf.cmd(Energy.cat_dev.format(intf.name, p)).replace("\n", "")
        if value:
            return self.clean_and_convert(value)
        return 0

    def get_rx_packet(self, intf):
        rx = self.get_cat_dev(intf, 3)
        if rx != intf.rx:
            intf.rx = rx
            return True
        return False

    def get_tx_packet(self, intf):
        tx = self.get_cat_dev(intf, 11)
        if tx != intf.tx:
            intf.tx = tx
            return True
        return False

    def get_state(self, intf):
        if self.get_rx_packet(intf):
            return 'rx'
        elif self.get_tx_packet(intf):
            return 'tx'
        return 'idle'

    def get_energy(self, intf, factor):
        return intf.voltage * factor

    def getTotalEnergyConsumption(self, intf):
        state = self.get_state(intf)
        # energy to decrease = time * voltage (mA) * current
        if state == 'idle':
            return self.get_energy(intf, 0.273)
        elif state == 'tx':
            return self.get_energy(intf, 0.380)
        elif state == 'rx':
            return self.get_energy(intf, 0.313)
        elif state == 'sleep':
            return self.get_energy(intf, 0.033)
        return 0

    def shutdown_intfs(self, node):
        # passing through all the interfaces
        # but we need to consider interfaces separately
        for wlan in node.params['wlans']:
            self.ipLink(wlan)

    def ipLink(self, intf):
        "Configure ourselves using ip link"
        self.cmd('ip link set {} down'.format(intf.name))
