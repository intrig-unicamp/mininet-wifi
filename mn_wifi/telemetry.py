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
import numpy

from matplotlib import style
from os import path
from threading import Thread as thread
from datetime import date
from mn_wifi.node import AP


today = date.today()
style.use('fivethirtyeight')
start = time.time()
util_dir = str(os.path.realpath(__file__))[:-21] + 'util/m'

class telemetry(object):
    tx = {}
    nodes = []

    def __init__(self, nodes, **kwargs):
        kwargs['nodes'] = nodes
        parseData.thread_ = thread(target=self.start, kwargs=(kwargs))
        parseData.thread_.daemon = True
        parseData.thread_._keep_alive = True
        parseData.thread_.start()

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
        if data_type == 'position':
            parseData.min_x, parseData.min_y, parseData.max_x, parseData.max_y = 0, 0, 100, 100
            if 'min_x' in kwargs:
                parseData.min_x = kwargs['min_x']
            if 'min_y' in kwargs:
                parseData.min_y = kwargs['min_y']
            if 'max_x' in kwargs:
                parseData.max_x = kwargs['max_x']
            if 'max_y' in kwargs:
                parseData.max_y = kwargs['max_y']
            fig, (self.axes) = plt.subplots(1, 1, figsize=(10, 10))
            fig.canvas.set_window_title('Mininet-WiFi Graph')
            self.axes.set_xlabel('meters')
            self.axes.set_xlabel('meters')
            self.axes.set_xlim([parseData.min_x, parseData.max_x])
            self.axes.set_ylim([parseData.min_y, parseData.max_y])
        else:
            if single:
                fig, (self.axes) = plt.subplots(1, 1, figsize=(10, 4))
            else:
                fig, (self.axes) = plt.subplots(1, (len(nodes)), figsize=(10, 4))
            fig.canvas.set_window_title('Mininet-WiFi Graph')
        self.nodes = nodes
        parseData.start(nodes, fig, self.axes, single=single, data_type=data_type)

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
        wlan = 1
        for phy in phys:
            try:
                ifaces_ = subprocess.check_output(cmd.format(phy), stderr=subprocess.PIPE,
                                                  shell=True).split("\n")
            except:
                node = inNamespaceNodes[j]
                ifaces_ = subprocess.check_output('%s %s %s' % (util_dir, node, cmd.format(phy)),
                                                  shell=True).split("\n")
            ifaces_.pop()
            if nodes[j] not in ifaces:
                ifaces[nodes[j]] = []
            ifaces[nodes[j]].append(ifaces_.pop())

            if wlan == len(nodes[j].params['wlan']):
                j += 1
                wlan = 1
            else:
                wlan += 1

        return ifaces

    @classmethod
    def get_phys(cls, nodes, inNamespaceNodes):
        cmd = 'ls /sys/class/ieee80211/'
        isAP = False
        phys = []
        for node in nodes:
            if isinstance(node, AP) and not isAP:
                phys += subprocess.check_output(cmd,
                                               shell=True).split("\n")
                isAP = True
            else:
                if not isinstance(node, AP):
                    phys += subprocess.check_output('%s %s %s' % (util_dir, node, cmd),
                                                    shell=True).split("\n")
        phy_list = []
        phys = sorted(phys)
        for phy in phys:
            if 'mn' in phy:
                phy_list.append(phy)

        ifaces = cls.get_ifaces(nodes, inNamespaceNodes, phy_list)
        return phy_list, ifaces

def get_position(node, filename):
    x = node.params['position'][0]
    y = node.params['position'][1]
    os.system("echo '%s,%s' >> %s" % (x, y, filename.format(node)))

def get_rssi(node, iface, time, filename):
    if isinstance(node, AP):
        rssi = 0
    else:
        cmd = "{} {} iw dev {} link | grep signal | tr -d signal: | awk '{{print $1 $3}}'"
        rssi = subprocess.check_output('%s' % (cmd.format(util_dir, node, iface)),
                                       shell=True).split("\n")
        if not rssi[0]:
            rssi = 0
        else:
            rssi = rssi[0]
    os.system("echo '%s,%s' >> %s" % (time, rssi, filename.format(node)))


def get_values_from_statistics(tx_bytes, time, node, filename):
    tx = telemetry.calc(float(tx_bytes[0]), node)
    os.system("echo '%s,%s' >> %s" % (time, tx, filename.format(node)))

class parseData(object):

    nodes = []
    phys = []
    colors = []
    ifaces = {}
    min_x = 0
    min_y = 0
    max_x = 100
    max_y = 100
    data_type = None
    fig = None
    axes = None
    single = None
    ani = None
    filename = None
    thread_ = None
    dir = 'cat /sys/class/ieee80211/{}/device/net/{}/statistics/{}'

    @classmethod
    def closeGraph(cls):
        try:
            cls.ani.event_source.stop()
            del cls.ani
        except:
            pass

    @classmethod
    def fig_exists(cls):
        return plt.fignum_exists(1)

    @classmethod
    def animate(cls, i):
        axes = cls.axes
        now = time.time() - start
        nodes_x = {}
        nodes_y = {}
        names = []
        if not cls.thread_._keep_alive:
            exit()

        for node in cls.nodes:
            for wlan in range(0, len(node.params['wlan'])):
                if cls.data_type == 'position':
                    if node.name not in names:
                        names.append(node.name)
                else:
                    names.append(cls.ifaces[node][wlan])
                nodes_x[node] = []
                nodes_y[node] = []
                arr = cls.nodes.index(node)

                if cls.data_type != 'rssi' and cls.data_type != 'position':
                    if isinstance(node, AP):
                        tx_bytes = subprocess.check_output(
                            ("%s" % cls.dir).format(cls.phys[arr],
                                                    cls.ifaces[node][wlan],
                                                    cls.data_type),
                            shell=True).split("\n")
                    else:
                        tx_bytes = subprocess.check_output(
                            '%s %s ' % (util_dir, node) + ('%s' % cls.dir).format(cls.phys[arr],
                                                                          cls.ifaces[node][wlan],
                                                                          cls.data_type),
                            shell=True).split("\n")

                if cls.data_type == 'rssi':
                    get_rssi(node, cls.ifaces[node][wlan], now, cls.filename)
                elif cls.data_type == 'position':
                    get_position(node, cls.filename)
                else:
                    get_values_from_statistics(tx_bytes, now, node, cls.filename)

                graph_data = open('%s' % (cls.filename.format(node)), 'r').read()
                lines = graph_data.split('\n')
                for line in lines:
                    if len(line) > 1:
                        x, y = line.split(',')
                        nodes_x[node].append(float(x))
                        nodes_y[node].append(float(y))

        if cls.data_type == 'position':
            axes.clear()
            axes.set_xlim([cls.min_x, cls.max_x])
            axes.set_ylim([cls.min_y, cls.max_y])
            #axes.set_title('Mininet-WiFi Graph')
            for node in cls.nodes:
                x = node.params['position'][0]
                y = node.params['position'][1]
                plt.scatter(x, y, color='black')
                axes.annotate(node.name, (x, y))
                circle = plt.Circle((x, y), int(node.params['range'][0]),
                                    color=node.circle, alpha=0.1)
                axes.add_artist(circle)
        else:
            cls.fig.legend(  # lines,  # The line objects
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
        cls.filename = '%s-{}-mn-telemetry.txt' % data_type

        inNamespaceNodes = []
        for node in nodes:
            cls.colors.append(numpy.random.rand(3,))
            node.circle = 'b'
            if not isinstance(node, AP):
                inNamespaceNodes.append(node)
                node.circle = 'g'

        cls.phys, cls.ifaces = telemetry.get_phys(nodes, inNamespaceNodes)
        for node in nodes:
            if path.exists('%s' % (cls.filename.format(node))):
                os.system('rm %s' % (cls.filename.format(node)))
        cls.ani = animation.FuncAnimation(fig, cls.animate, interval=1000)
        plt.show()
