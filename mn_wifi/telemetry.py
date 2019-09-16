"""
@author: Ramon Fontes
@email: ramonreisfontes@gmail.com

usage:
telemetry(nodes, **params)

**params
   * single=True - opens a single window and put all nodes together
   * data_type - refer to statistics dir at /sys/class/ieee80211/{}/device/net/{}/statistics/{}
            - other data_types: rssi - gets the rssi value
"""

import matplotlib.pyplot as plt
import matplotlib.animation as animation
import os
import os.path
import time
import subprocess

from matplotlib import style
from os import path
from threading import Thread as thread
from datetime import date
from mn_wifi.node import AP


today = date.today()
style.use('fivethirtyeight')
start = time.time()

class telemetry(object):
    tx = {}
    nodes = []

    def __init__(self, nodes, **kwargs):
        kwargs['nodes'] = nodes
        thread_ = thread(target=self.start, kwargs=(kwargs))
        thread_.daemon = True
        thread_.start()

    def start(self, **kwargs):
        nodes = kwargs['nodes']
        ax = 'axes'
        arr = ''
        for node in nodes:
            i = nodes.index(node)
            arr += 'ax%s, ' % (i + 1)
        arr1 = (arr[:-2])
        setattr(self, ax, arr1)

        data_type = 'tx_packets'
        if 'data_type' in kwargs:
            data_type = kwargs['data_type']

        single = False
        if 'single' in kwargs:
            single = True
        if single:
            fig, (self.axes) = plt.subplots(1, 1, figsize=(10, 4))
        else:
            fig, (self.axes) = plt.subplots(1, (len(nodes)), figsize=(10, 4))
        self.nodes = nodes
        graph.start(nodes, fig, self.axes, single=single, data_type=data_type)

    @classmethod
    def calc(cls, tx_bytes, n):
        if n not in cls.tx:
            a = 0
        else:
            a = tx_bytes - cls.tx[n]
        cls.tx[n] = tx_bytes
        return a

    @classmethod
    def get_ifaces(cls, nodes, inNamespaceNodes, phys):
        cmd = 'ls /sys/class/ieee80211/{}/device/net/'
        ifaces = {}
        j = 0
        for phy in phys:
            i = phys.index(phy)
            try:
                ifaces_ = subprocess.check_output(cmd.format(phy), stderr=subprocess.PIPE,
                                                  shell=True).split("\n")
            except:
                node = inNamespaceNodes[j]
                ifaces_ = subprocess.check_output('util/m %s %s' % (node, cmd.format(phy)),
                                                  shell=True).split("\n")
                j += 1
            ifaces_.pop()
            ifaces[nodes[i]] = ifaces_.pop()
        return ifaces

    @classmethod
    def get_phys(cls, nodes, inNamespaceNodes):
        cmd = 'ls /sys/class/ieee80211/'
        isAP = False
        phys = []
        for node in nodes:
            if isinstance(node, AP) and not isAP:
                phys = subprocess.check_output(cmd,
                                               shell=True).split("\n")
                isAP = True
            else:
                if not isinstance(node, AP):
                    phys += subprocess.check_output('util/m %s %s' % (node, cmd),
                                                    shell=True).split("\n")
        phy_list = []
        for phy in phys:
            if 'mn' in phy:
                phy_list.append(phy)

        ifaces = cls.get_ifaces(nodes, inNamespaceNodes, phy_list)
        return phy_list, ifaces


def get_rssi(node, iface, time, filename):
    if isinstance(node, AP):
        rssi = 0
    else:
        cmd = "util/m {} iw dev {} link | grep signal | tr -d signal: | awk '{{print $1 $3}}'"
        rssi = subprocess.check_output('%s' % (cmd.format(node, iface)),
                                       shell=True).split("\n")
        if not rssi[0]:
            rssi = 0
        else:
            rssi = rssi[0]
    os.system("echo '%s,%s' >> %s" % (time, rssi, filename.format(node)))


def get_values_from_statistics(tx_bytes, time, node, filename):
    tx = telemetry.calc(float(tx_bytes[0]), node)
    os.system("echo '%s,%s' >> %s" % (time, tx, filename.format(node)))

class graph(object):

    nodes = []
    phys = []
    ifaces = {}
    data_type = None
    fig = None
    axes = None
    single = None
    colors = ['red', 'green', 'blue', 'black', 'yellow', 'pink']
    filename = None
    dir = 'cat /sys/class/ieee80211/{}/device/net/{}/statistics/{}'

    @classmethod
    def animate(cls, i):
        axes = cls.axes
        now = time.time() - start
        nodes_x = {}
        nodes_y = {}
        names = []

        for node in cls.nodes:
            names.append(cls.ifaces[node])
            nodes_x[node] = []
            nodes_y[node] = []
            arr = cls.nodes.index(node)

            if cls.data_type != 'rssi':
                if isinstance(node, AP):
                    tx_bytes = subprocess.check_output(
                        ("%s" % cls.dir).format(cls.phys[arr],
                                                cls.ifaces[node],
                                                cls.data_type),
                        shell=True).split("\n")
                else:
                    tx_bytes = subprocess.check_output(
                        'util/m %s ' % node + ('%s' % cls.dir).format(cls.phys[arr],
                                                                      cls.ifaces[node],
                                                                      cls.data_type),
                        shell=True).split("\n")

            if cls.data_type != 'rssi':
                get_values_from_statistics(tx_bytes, now, node, cls.filename)
            else:
                get_rssi(node, cls.ifaces[node], now, cls.filename)

            graph_data = open('%s' % (cls.filename.format(node)), 'r').read()
            lines = graph_data.split('\n')
            for line in lines:
                if len(line) > 1:
                    x, y = line.split(',')
                    nodes_x[node].append(float(x))
                    nodes_y[node].append(float(y))

        cls.fig.legend(#lines,  # The line objects
                   labels=names,  # The labels for each line
                   loc="center right",  # Position of legend
                   borderaxespad=0.1,  # Small spacing around legend box
                   title="Nodes"  # Title for the legend
                   )

        if cls.single:
            axes.clear()
            for node in cls.nodes:
                id = cls.nodes.index(node)
                axes.set_title(cls.data_type)
                axes.plot(nodes_x[node], nodes_y[node], color=cls.colors[id])
        else:
            for ax in axes:
                id = list(axes).index(ax)
                ax.clear()
                node = cls.nodes[id]
                ax.plot(nodes_x[node], nodes_y[node], color=cls.colors[id])

    @classmethod
    def start(cls, nodes, fig, axes, single, data_type):
        cls.nodes = nodes
        cls.fig = fig
        cls.axes = axes
        cls.single = single
        cls.data_type = data_type
        cls.filename = '%s-{}.txt' % data_type

        inNamespaceNodes = []
        for node in nodes:
            if not isinstance(node, AP):
                inNamespaceNodes.append(node)

        cls.phys, cls.ifaces = telemetry.get_phys(nodes, inNamespaceNodes)
        for node in nodes:
            if path.exists('%s' % (cls.filename.format(node))):
                os.system('rm %s' % (cls.filename.format(node)))
        ani = animation.FuncAnimation(fig, cls.animate, interval=1000)
        plt.show()
