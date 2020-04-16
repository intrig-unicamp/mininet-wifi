"""
   Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
   @author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

from threading import Thread as thread
from time import sleep, time


class Energy(object):

    thread_ = None

    def __init__(self, nodes):
        self.start_simulation = 0
        Energy.thread_ = thread(target=self.start, args=(nodes,))
        Energy.thread_.daemon = True
        Energy.thread_._keep_alive = True
        Energy.thread_.start()

    def start(self, nodes):
        self.start_simulation = self.get_time()

        for node in nodes:
            for intf in node.wintfs.values():
                intf.rx = 0
                intf.tx = 0

        try:
            while self.thread_._keep_alive:
                sleep(1)  # set sleep time to 1 second
                for node in nodes:
                    for intf in node.wintfs.values():
                        intf.consumption = self.getTotalEnergyConsumption(intf)
        except:
            pass

    @staticmethod
    def get_time():
        return time()

    def get_duration(self):
        return self.get_time() - self.start_simulation

    @staticmethod
    def get_rx_packet(intf):
        rx = int(intf.cmd("cat /proc/net/dev | grep %s |  awk \'{print $3}\'" % intf.name))
        if rx != intf.rx:
            intf.rx = rx
            return True

    @staticmethod
    def get_tx_packet(intf):
        tx = int(intf.cmd("cat /proc/net/dev | grep %s |  awk \'{print $11}\'" % intf.name))
        if tx != intf.tx:
            intf.tx = tx
            return True

    def getState(self, intf):
        if self.get_rx_packet(intf):
            return 'rx'
        elif self.get_tx_packet(intf):
            return 'tx'
        else:
            return 'idle'

    def getTotalEnergyConsumption(self, intf):
        state = self.getState(intf)
        # energy to decrease = time * voltage (mA) * current
        if state == 'idle':
            energy_to_decrease = self.get_duration() * 0.273 * intf.voltage
        elif state == 'tx':
            energy_to_decrease = self.get_duration() * 0.380 * intf.voltage
        elif state == 'rx':
            energy_to_decrease = self.get_duration() * 0.313 * intf.voltage
        elif state == 'sleep':
            energy_to_decrease = self.get_duration() * 0.033 * intf.voltage
        elif state == 'off':
            energy_to_decrease = 0
        return energy_to_decrease

    def shutdown_intfs(self, node):
        # passing through all the interfaces
        # but we need to consider interfaces separately
        for wlan in node.params['wlans']:
            self.ipLink(wlan)

    def ipLink(self, intf):
        "Configure ourselves using ip link"
        self.cmd('ip link set %s down' % intf.name)