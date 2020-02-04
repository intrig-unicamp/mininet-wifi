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

import os
import os.path
import time
import subprocess
import numpy
import matplotlib.pyplot as plt
import matplotlib.animation as animation

from matplotlib import style
from os import path
from threading import Thread as thread
from datetime import date
from mn_wifi.node import AP


today = date.today()
style.use('fivethirtyeight')
start = time.time()
util_dir = str(os.path.realpath(__file__))[:-21] + '/util/m'


class telemetry(object):
    tx = {}
    nodes = []

    def __init__(self, **kwargs):
        parseData.thread_ = thread(target=self.start, kwargs=(kwargs))
        parseData.thread_.daemon = True
        parseData.thread_._keep_alive = True
        parseData.thread_.start()

    def start(self, nodes=None, data_type='tx_packets', single=False,
              min_x=0, min_y=0, max_x=100, max_y=100, **kwargs):
        ax = 'axes'
        arr = ''
        for node in nodes:
            i = nodes.index(node)
            arr += 'ax%s, ' % (i + 1)
        arr1 = (arr[:-2])
        setattr(self, ax, arr1)

        fig = None
        if 'tool' not in kwargs:
            if data_type == 'position':
                parseData.min_x = min_x
                parseData.min_y = min_y
                parseData.max_x = max_x
                parseData.max_y = max_y
                fig, (self.axes) = plt.subplots(1, 1, figsize=(10, 10))
                fig.canvas.set_window_title('Mininet-WiFi Graph')
                self.axes.set_xlabel('meters')
                self.axes.set_xlabel('meters')
                self.axes.set_xlim([min_x, max_x])
                self.axes.set_ylim([min_y, max_y])
            else:
                if single:
                    fig, (self.axes) = plt.subplots(1, 1, figsize=(10, 4))
                else:
                    fig, (self.axes) = plt.subplots(1, (len(nodes)), figsize=(10, 4))
                fig.canvas.set_window_title('Mininet-WiFi Graph')
        self.nodes = nodes
        parseData(nodes, self.axes, single, data_type=data_type, fig=fig, **kwargs)

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
                                                  shell=True).decode().split("\n")
            except:
                node = inNamespaceNodes[j]
                ifaces_ = subprocess.check_output('%s %s %s' % (util_dir, node, cmd.format(phy)),
                                                  shell=True).decode().split("\n")
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
                                                shell=True).decode().split("\n")
                isAP = True
            else:
                if not isinstance(node, AP):
                    phys += subprocess.check_output('%s %s %s' % (util_dir, node, cmd),
                                                    shell=True).decode().split("\n")
        phy_list = []
        phys = sorted(phys)
        for phy in phys:
            if 'mn' in phy:
                phy_list.append(phy)

        ifaces = cls.get_ifaces(nodes, inNamespaceNodes, phy_list)
        return phy_list, ifaces


def get_position(node):
    x = node.position[0]
    y = node.position[1]
    z = node.position[2]
    return x, y, z


def get_rssi(node, iface):
    if isinstance(node, AP):
        rssi = 0
    else:
        cmd = "{} {} iw dev {} link | grep signal | tr -d signal: | awk '{{print $1 $3}}'"
        rssi = subprocess.check_output('%s' % (cmd.format(util_dir, node, iface)),
                                       shell=True).decode().split("\n")
        if not rssi[0]:
            rssi = 0
        else:
            rssi = rssi[0]
    return rssi


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
    ani = None
    axes = None
    data_type = None
    fig = None
    filename = None
    single = None
    thread_ = None
    dir = 'cat /sys/class/ieee80211/{}/device/net/{}/statistics/{}'

    def __init__(self, nodes, axes, single, data_type, fig, **kwargs):
        self.start(nodes, axes, single, data_type, fig, **kwargs)

    @classmethod
    def fig_exists(cls):
        return plt.fignum_exists(1)

    def pub_msg(self, topic, msg):
        os.system("mosquitto_pub -t {} -m \'{}\'".format(topic, msg))

    def run_dojot(self, topic):
        for node in self.nodes:
            for wlan in range(0, len(node.params['wlan'])):
                msg = '{'
                if 'rssi' in self.data_type:
                    iface = self.ifaces[node][wlan]
                    rssi = get_rssi(node, iface)
                    msg += '\"{}\":{},'.format((node.name+'-rssi'), rssi)
                if 'position' in self.data_type:
                    pos = str(get_position(node))
                    msg += '\"{}\":\"{}\",'.format((node.name + '-pos'), pos)
                msg = msg[:-1] + '}'
                self.pub_msg(topic, msg)

    def animate(self, i):
        axes = self.axes
        time_ = time.time() - start
        nodes_x = {}
        nodes_y = {}
        names = []
        if not self.thread_._keep_alive:
            try:
                if self.data_type != 'position':
                    plt.close()
            except:
                pass
        time.sleep(1)
        for node in self.nodes:
            for wlan in range(0, len(node.params['wlan'])):
                if self.data_type == 'position':
                    if node.name not in names:
                        names.append(node.name)
                else:
                    names.append(self.ifaces[node][wlan])
                nodes_x[node] = []
                nodes_y[node] = []
                arr = self.nodes.index(node)

                if self.data_type != 'rssi' and self.data_type != 'position':
                    if isinstance(node, AP):
                        tx_bytes = subprocess.check_output(
                            ("%s" % self.dir).format(self.phys[arr],
                                                     self.ifaces[node][wlan],
                                                     self.data_type),
                            shell=True).decode().split("\n")
                    else:
                        tx_bytes = subprocess.check_output(
                            '%s %s ' % (util_dir, node) +
                            ('%s' % self.dir).format(self.phys[arr],
                                                     self.ifaces[node][wlan],
                                                     self.data_type),
                            shell=True).decode().split("\n")
                    get_values_from_statistics(tx_bytes, time_, node, self.filename)

                if self.data_type == 'rssi':
                    rssi = get_rssi(node, self.ifaces[node][wlan])
                    os.system("echo '%s,%s' >> %s" % (time_, rssi, self.filename.format(node)))
                elif self.data_type == 'position':
                    x, y, z = get_position(node)
                    os.system("echo '%s,%s' >> %s" % (x, y, self.filename.format(node)))

                graph_data = open('%s' % (self.filename.format(node)), 'r').read()
                lines = graph_data.split('\n')
                for line in lines:
                    if len(line) > 1:
                        x, y = line.split(',')
                        nodes_x[node].append(float(x))
                        nodes_y[node].append(float(y))

        if self.data_type == 'position':
            axes.clear()
            axes.set_xlim([self.min_x, self.max_x])
            axes.set_ylim([self.min_y, self.max_y])
            #axes.set_title('Mininet-WiFi Graph')
            for node in self.nodes:
                x = node.position[0]
                y = node.position[1]
                plt.scatter(x, y, color='black')
                axes.annotate(node.name, (x, y))
                circle = plt.Circle((x, y), int(node.wintfs[0].range),
                                    color=node.circle, alpha=0.1)
                axes.add_artist(circle)
        else:
            time.sleep(0.0001)
            self.fig.legend(  # lines,  # The line objects
                labels=names,  # The labels for each line
                loc="center right",  # Position of legend
                borderaxespad=0.1,  # Small spacing around legend box
                title="Nodes"  # Title for the legend
            )
            if self.single:
                axes.clear()
                for node in self.nodes:
                    id = self.nodes.index(node)
                    axes.set_title(self.data_type)
                    axes.plot(nodes_x[node], nodes_y[node], color=self.colors[id])
            else:
                for ax in axes:
                    id = list(axes).index(ax)
                    ax.clear()
                    node = self.nodes[id]
                    ax.plot(nodes_x[node], nodes_y[node], color=self.colors[id])

    def setCircleColor(self, node):
        if 'color' in node.params:
            node.circle = node.params['color']
        else:
            if isinstance(node, AP):
                node.circle = 'b'
            else:
                node.circle = 'g'

    def start(self, nodes, axes, single, data_type, fig, **kwargs):
        self.nodes = nodes
        self.fig = fig
        self.axes = axes
        self.single = single
        self.data_type = data_type
        self.filename = '%s-{}-mn-telemetry.txt' % data_type

        inNamespaceNodes = []
        for node in nodes:
            self.colors.append(numpy.random.rand(3,))
            self.setCircleColor(node)
            if not isinstance(node, AP):
                inNamespaceNodes.append(node)
                self.setCircleColor(node)

        self.phys, self.ifaces = telemetry.get_phys(nodes, inNamespaceNodes)
        interval = 1000
        if 'tool' in kwargs:
            topic = kwargs['dojot_topic']
            while self.thread_._keep_alive:
                self.run_dojot(topic)
                time.sleep(interval/1000)
        else:
            for node in nodes:
                if path.exists('%s' % (self.filename.format(node))):
                    os.system('rm %s' % (self.filename.format(node)))
            self.ani = animation.FuncAnimation(fig, self.animate, interval=interval)
            plt.show()
