"""
   Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
   @author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

import warnings
import time
import numpy as np
import matplotlib.pyplot as plt

from threading import Thread as thread
from datetime import datetime
from time import sleep

from mininet.log import error
from mn_wifi.clean import Cleanup as CleanupWifi


class PlotEnergy:
    def __init__(self, nodes, title="Battery Consumption", **kwargs):
        warnings.filterwarnings("ignore")
        self.nodes = nodes
        self.running = True

        for node in self.nodes:
            node.down = False

        plt.ion()
        self.fig, self.ax = plt.subplots(figsize=(9, 6))

        self.bars = self.ax.bar(
            range(len(self.nodes)),
            [node.remaining_capacity for node in self.nodes],
            color=self.get_colors()
        )

        self.labels = [
            self.ax.text(i, self.nodes[i].remaining_capacity + 1,
                         self.get_percentage(self.nodes[i]),
                         ha='center', fontsize=12)
            for i in range(len(self.nodes))
        ]

        max_capacity = max(node.battery_capacity for node in self.nodes)
        self.ax.set_title(title)
        self.ax.set_ylabel("Battery Remaining (mAh)")
        self.ax.set_ylim(0, max_capacity * 1.1)
        self.ax.set_xticks(range(len(self.nodes)))
        self.ax.set_xticklabels([f'{node.name}' for node in self.nodes])
        self.fig.canvas.mpl_connect("close_event", self.on_close)
        self.run(nodes)

    def get_percentage(self, node):
        percentage = (node.remaining_capacity / node.battery_capacity) * 100
        return max(round(percentage, 1), 0)

    def get_colors(self):
        colors = []
        for node in self.nodes:
            if self.get_percentage(node) < 20:
                colors.append('red')
            elif self.get_percentage(node) < 50:
                colors.append('orange')
            else:
                colors.append('green')
        return colors

    def update(self):
        if not plt.fignum_exists(self.fig.number):
            self.stop()
            return

        for i, bar in enumerate(self.bars):
            bar.set_height(self.nodes[i].remaining_capacity)
            bar.set_color(self.get_colors()[i])

            self.labels[i].set_y(max(self.nodes[i].remaining_capacity, 0))
            self.labels[i].set_text("{}%".format(self.get_percentage(self.nodes[i])))

        self.fig.canvas.draw()
        self.fig.canvas.flush_events()

    def run(self, nodes):
        while self.running:
            for node in nodes:
                if node.remaining_capacity == 0 and not node.down:
                    self.node_down(node)
                    node.down = True
            self.update()
            time.sleep(1)

    def on_close(self, event):
        self.stop()

    def node_down(self, node):
        for intf in node.wintfs.values():
            node.pexec('ip link set {} down'.format(intf.name), shell=True)

    def node_up(self, node):
        for intf in node.wintfs.values():
            node.pexec('ip link set {} up'.format(intf.name), shell=True)

    def stop(self):
        self.running = False
        plt.close(self.fig)


class EnergyMonitor:
    thread_ = None

    def plotEnergy(self, **kwargs):
        EnergyMonitor.thread_ = thread(target=self.run_plot, kwargs=kwargs)
        EnergyMonitor.thread_.daemon = True
        EnergyMonitor.thread_._keep_alive = True
        EnergyMonitor.thread_.start()

    @classmethod
    def pause(cls):
        try:
            plt.pause(0.001)
        except:
            pass

    @classmethod
    def close(cls):
        try:
            plt.close()
        except:
            pass

    def run_plot(self, **kwargs):
        PlotEnergy(**kwargs)


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
                        node.remaining_capacity = node.battery_capacity - node.consumption
        except Exception as e:
            # Log errors during the monitoring process
            error("Error with the energy consumption function: %s\n" % e)

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
        self.pexec('ip link set %s down' % intf.name, shell=True)


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

        while self.thread_._keep_alive:
            sleep(1)  # set sleep time to 1 second
            for node in nodes:
                for intf in node.wintfs.values():
                    intf.consumption += float(self.getTotalEnergyConsumption(intf))
                    node.consumption += float(self.getTotalEnergyConsumption(intf))
                    node.remaining_capacity = node.battery_capacity - node.consumption
        try:
            while self.thread_._keep_alive:
                sleep(1)  # set sleep time to 1 second
                for node in nodes:
                    for intf in node.wintfs.values():
                        intf.consumption += float(self.getTotalEnergyConsumption(intf))
                        node.consumption += float(self.getTotalEnergyConsumption(intf))
                        node.remaining_capacity = node.battery_capacity - node.consumption
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
