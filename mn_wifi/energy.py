"""
   Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
   @author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""
import re

from threading import Thread as thread
from datetime import datetime
from time import sleep, time

from mininet.log import error


class BitZigBeeEnergy(object):
    """
    adapted from:
        Polastre et al. (2004): "Telos: Enabling ultra-low power wireless research."
        Ye et al. (2002): "An energy-efficient MAC protocol for wireless sensor networks."
    """

    thread_ = None
    cat_dev = 'cat /proc/net/dev | grep {} | awk \'{}\''

    def __init__(self, nodes):
        """
        Initializes the BitZigBeeEnergy monitoring class.
        Spawns a background thread to monitor energy consumption.
        """
        # Zigbee energy cost per byte in Joules (adjust based on literature or experiments)
        self.cost_tx = 0.00012  # Energy per transmitted byte (J/byte)
        self.cost_rx = 0.00009  # Energy per received byte (J/byte)

        # Conversion factor: Joules to Watt-hours
        self.joules_to_wh = 0.1 / 3600

        # Start monitoring thread
        BitZigBeeEnergy.thread_ = thread(target=self.start, args=(nodes,))
        BitZigBeeEnergy.thread_.daemon = True
        BitZigBeeEnergy.thread_._keep_alive = True
        BitZigBeeEnergy.thread_.start()

    def start(self, nodes):
        """
        Main loop for monitoring energy consumption for each node and its interfaces.
        """
        # Initialize consumption counters
        for node in nodes:
            node.consumption = 0
            for intf in node.wintfs.values():
                intf.rx_bytes, intf.tx_bytes = 0, 0

        try:
            # Continuously monitor byte usage and calculate energy consumption
            while self.thread_._keep_alive:
                sleep(0.1)  # Monitor every second
                for node in nodes:
                    for intf in node.wintfs.values():
                        # Calculate energy based on bytes transferred
                        energy_consumed = self.get_bytes_consumption(node, intf)
                        intf.consumption += energy_consumed
                        node.consumption += energy_consumed
        except Exception as e:
            # Log errors during the monitoring process
            error(f"Error with the energy consumption function: {e}\n")

    def get_cat_dev(self, intf, col):
        """
        Reads the specific column value from /proc/net/dev for an interface.
        Args:
            intf: Interface object.
            col: Column number to extract (2: RX bytes, 10: TX bytes).
        Returns:
            int: The value from the specified column.
        """
        p = '{print $%s}' % col
        value = intf.pexec(BitZigBeeEnergy.cat_dev.format(intf.name, p), shell=True)[0].replace("\n", "")
        return int(value) if value else 0

    def get_rx_bytes(self, intf):
        """
        Retrieves the received bytes for the interface.
        """
        return self.get_cat_dev(intf, 2)  # Column 2: RX bytes

    def get_tx_bytes(self, intf):
        """
        Retrieves the transmitted bytes for the interface.
        """
        return self.get_cat_dev(intf, 10)  # Column 10: TX bytes

    def get_bytes_consumption(self, node, intf):
        """
        Calculates the energy consumption based on bytes transmitted and received.
        Args:
            intf: Interface object.
        Returns:
            float: Energy consumed (in Joules) for the interface since the last update.
        """
        # Get current byte counters
        rx_bytes = self.get_rx_bytes(intf)
        tx_bytes = self.get_tx_bytes(intf)

        # Calculate the difference since the last read
        rx_diff = rx_bytes - intf.rx_bytes
        tx_diff = tx_bytes - intf.tx_bytes

        # udpate with the last read
        intf.rx_bytes = rx_bytes
        intf.tx_bytes = tx_bytes

        # Calculate energy consumption in Joules
        energy_in_joules = (rx_diff * self.cost_rx) + (tx_diff * self.cost_tx)

        # Convert Joules to Watt-hours
        energy_in_wh = energy_in_joules * self.joules_to_wh

        # Produce log
        current_datetime = datetime.now()
        formatted_datetime = current_datetime.strftime("%Y-%m-%d %H:%M:%S")
        node.pexec('echo {},{},{},{} >> /tmp/net-consumption.log'.format(formatted_datetime, tx_diff, rx_diff, energy_in_wh), shell=True)

        return energy_in_wh

    def shutdown_intfs(self, node):
        """
        Shuts down all wireless interfaces for a node.
        Args:
            node: Node object.
        """
        for wlan in node.params['wlans']:
            self.ipLink(wlan)

    def ipLink(self, intf):
        """
        Brings down the specified interface.
        Args:
            intf: Interface name or object.
        """
        self.pexec(f'ip link set {intf.name} down', shell=True)


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
            node.consumption = 0
            for intf in node.wintfs.values():
                intf.rx, intf.tx = 0, 0

        try:
            while self.thread_._keep_alive:
                sleep(1)  # set sleep time to 1 second
                for node in nodes:
                    for intf in node.wintfs.values():
                        intf.consumption += float(self.getTotalEnergyConsumption(intf))
                        node.consumption += float(self.getTotalEnergyConsumption(intf))
        except:
            error("Error with the energy consumption function\n")

    def get_cat_dev(self, intf, col):
        p = '{print $%s}' % col
        value = intf.pexec(Energy.cat_dev.format(intf.name, p), shell=True)[0].replace("\n", "")
        if value:
            return value
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
        return intf.voltage * factor * 0.1 / 3600 / 1000

    def getTotalEnergyConsumption(self, intf):
        state = self.get_state(intf)
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
        self.pexec('ip link set {} down'.format(intf.name), shell=True)
