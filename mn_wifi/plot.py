"""

    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)


"""

import numpy as np
import matplotlib.patches as patches
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from mininet.log import debug


class plot3d (object):
    'Plot 3d Graphs'
    ax = None
    is3d = False

    @classmethod
    def instantiateGraph(cls, MIN_X, MIN_Y, MIN_Z, MAX_X, MAX_Y, MAX_Z):
        "instantiateGraph"
        plt.ion()
        plt.title("Mininet-WiFi Graph")
        cls.ax = plt.subplot(111, projection=Axes3D.name)
        cls.ax.set_xlabel('meters (x)')
        cls.ax.set_ylabel('meters (y)')
        cls.ax.set_zlabel('meters (z)')
        cls.ax.set_xlim([MIN_X, MAX_X])
        cls.ax.set_ylim([MIN_Y, MAX_Y])
        cls.ax.set_zlim([MIN_Z, MAX_Z])
        cls.ax.grid(True)

    @classmethod
    def instantiateAnnotate(cls, node):
        "instantiateAnnotate"
        x, y, z = cls.getPos(node)

        node.plttxt = cls.ax.text(x, y, z, node.name)

    @classmethod
    def instantiateNode(cls, node):
        "Instantiate Node"
        x, y, z = cls.getPos(node)

        resolution = 40
        u = np.linspace(0, 2 * np.pi, resolution)
        v = np.linspace(0, np.pi, resolution)

        r = 1

        x = r * np.outer(np.cos(u), np.sin(v)) + x
        y = r * np.outer(np.sin(u), np.sin(v)) + y
        z = r * np.outer(np.ones(np.size(u)), np.cos(v)) + z

        node.pltNode = cls.ax.plot_surface(x, y, z, alpha=0.2,
                                           edgecolor='none', color='black')

    @classmethod
    def instantiateNodes(cls, nodes):
        "Instantiate Nodes"
        cls.is3d = True
        for node in nodes:
            cls.instantiateAnnotate(node)
            cls.instantiateNode(node)
            cls.instantiateCircle(node)
            cls.draw()

    @classmethod
    def fig_exists(cls):
        return plt.fignum_exists(1)

    @classmethod
    def p(cls):
        plt.pause(0.0001)

    @classmethod
    def update(cls, node):
        "Graph Update"

        node.pltNode.remove()
        node.pltCircle.remove()
        node.plttxt.remove()

        cls.instantiateCircle(node)
        cls.instantiateNode(node)
        cls.instantiateAnnotate(node)
        cls.draw()

    @classmethod
    def draw(cls):
        plt.draw()

    @classmethod
    def closePlot(cls):
        try:
            plt.close()
        except:
            pass

    @classmethod
    def instantiateCircle(cls, node):
        "Instantiate Circle"
        from mn_wifi.node import Station, Car

        x, y, z = cls.getPos(node)
        color = 'b'
        if isinstance(node, Station):
            color = 'g'
        elif isinstance(node, Car):
            color = 'r'

        resolution = 100
        u = np.linspace(0, 2 * np.pi, resolution)
        v = np.linspace(0, np.pi, resolution)

        r = max(node.params['range'])

        x = r * np.outer(np.cos(u), np.sin(v)) + x
        y = r * np.outer(np.sin(u), np.sin(v)) + y
        z = r * np.outer(np.ones(np.size(u)), np.cos(v)) + z

        node.pltCircle = cls.ax.plot_surface(x, y, z, alpha=0.2,
                                             edgecolor='none',
                                             color=color)

    @classmethod
    def getPos(self, node):
        x = '%.2f' % float(node.params['position'][0])
        y = '%.2f' % float(node.params['position'][1])
        z = '%.2f' % float(node.params['position'][2])
        return float(x), float(y), float(z)


class plot2d (object):
    'Plot 2d Graphs'
    ax = None
    q_lock = ''
    lines = {}

    @classmethod
    def closePlot(cls):
        try:
            plt.close()
        except:
            pass

    @classmethod
    def getxy(cls, node):
        x = '%.2f' % float(node.params['position'][0])
        y = '%.2f' % float(node.params['position'][1])
        return float(x), float(y)

    @classmethod
    def text(cls, node, x, y):
        "draw text"
        # newer MPL versions (>=1.4)
        if hasattr(node.plttxt, 'xyann'): node.plttxt.xyann = (x, y)
        else: node.plttxt.xytext = (x, y)

    @classmethod
    def circle(cls, node, x, y):
        node.pltCircle.center = x, y

    @classmethod
    def update(cls, node):
        "Graph Update"
        x, y = cls.getxy(node)
        cls.text(node, x, y)
        node.pltNode.set_data(x, y)
        node.pltCircle.center = x, y

    @classmethod
    def pause(cls):
        plt.pause(0.001)

    @classmethod
    def draw(cls):
        plt.draw()

    @classmethod
    def scatter(cls, nodesx, nodesy):
        return plt.scatter(nodesx, nodesy, color='red', marker='s')

    @classmethod
    def line2d(cls, nodesx, nodesy, color='', ls='-', lw=1):
        return plt.Line2D(nodesx, nodesy, color=color, ls=ls, lw=lw)

    @classmethod
    def lineTxt(cls, x, y, i):
        title = 'Av.%s' % i
        plt.text(x, y, title, ha='left', va='bottom', fontsize=8, color='g')

    @classmethod
    def line(cls, line):
        ax = cls.ax
        ax.add_line(line)

    @classmethod
    def instantiateGraph(cls, MIN_X, MIN_Y, MAX_X, MAX_Y):
        "instantiateGraph"
        plt.ion()
        plt.title("Mininet-WiFi Graph")
        cls.ax = plt.subplot(111)
        cls.ax.set_xlabel('meters')
        cls.ax.set_ylabel('meters')
        cls.ax.set_xlim([MIN_X, MAX_X])
        cls.ax.set_ylim([MIN_Y, MAX_Y])
        cls.ax.grid(True)

    @classmethod
    def fig_exists(cls):
        return plt.fignum_exists(1)

    @classmethod
    def instantiateNode(cls, node):
        "instantiateNode"
        ax = cls.ax
        node.pltNode, = ax.plot(1, 1, marker='.', ms=5, color='black')

    @classmethod
    def instantiateCircle(cls, node):
        "instantiateCircle"
        ax = cls.ax
        color = cls.set_def_color(node)

        node.pltCircle = ax.add_patch(
            patches.Circle((0, 0), max(node.params['range']),
                           fill=True, alpha=0.1, color=color))

    @classmethod
    def set_def_color(cls, node):
        from mn_wifi.node import Station, Car
        if 'color' in node.params:
            color = node.params['color']
        else:
            color = 'b'
            if isinstance(node, Station):
                color = 'g'
            elif isinstance(node, Car):
                color = 'r'
        return color

    @classmethod
    def instantiateAnnotate(cls, node):
        node.plttxt = cls.ax.annotate(node, xy=(0, 0))

    @classmethod
    def updateCircleRadius(cls, node):
        node.pltCircle.set_radius(max(node.params['range']))

    @classmethod
    def setCircleColor(cls, node, color):
        node.pltCircle.set_color(color)

    @classmethod
    def setAnnotateColor(cls, node, color):
        node.plttxt.set_color(color)

    @classmethod
    def setNodeColor(cls, node, color):
        node.pltNode.set_c(color)

    @classmethod
    def setNodeMarker(cls, node, marker=''):
        node.pltNode.set_marker(marker)

    @classmethod
    def instantiateNodes(cls, node):
        cls.instantiateAnnotate(node)
        cls.instantiateCircle(node)
        cls.instantiateNode(node)
        cls.update(node)

    @classmethod
    def plotGraph(cls, nodes, conn):
        "Plot Graph"
        debug('Enabling Graph...\n')
        for node in nodes:
            x, y = cls.getxy(node)
            cls.instantiateNodes(node)
            node.pltNode.set_data(x, y)
            cls.text(node, x, y)
            cls.circle(node, x, y)

        if 'src' in conn:
            for c in range(0, len(conn['src'])):
                ls = conn['ls'][c]
                src = conn['src'][c]
                dst = conn['dst'][c]
                cls.addLine(src, dst, ls)

    @classmethod
    def hideNode(cls, node):
        node.pltCircle.set_visible(False)
        node.plttxt.set_visible(False)
        node.pltNode.set_visible(False)

    @classmethod
    def showNode(cls, node):
        node.pltCircle.set_visible(True)
        node.plttxt.set_visible(True)
        node.pltNode.set_visible(True)

    @classmethod
    def hideLine(cls, src, dst):
        conn_ = src.name + dst.name
        cls.lines[conn_].set_visible(False)

    @classmethod
    def showLine(cls, src, dst):
        conn_ = src.name + dst.name
        cls.lines[conn_].set_visible(True)

    @classmethod
    def addLine(cls, src, dst, ls='-'):
        src_x = '%.2f' % float(src.params['position'][0])
        src_y = '%.2f' % float(src.params['position'][1])
        dst_x = '%.2f' % float(dst.params['position'][0])
        dst_y = '%.2f' % float(dst.params['position'][1])
        line = cls.line2d([float(src_x), float(dst_x)],
                          [float(src_y), float(dst_y)], 'b', ls=ls)
        conn_ = src.name + dst.name
        cls.lines[conn_] = line
        cls.line(line)


class plotGraph(object):
    'Plot Graphs'
    def __init__(self, **kwargs):
        'init plotgraph'
        self.instantiateGraph(**kwargs)

    def instantiateGraph(self, **kwargs):
        "instantiatePlotGraph"
        if kwargs['min_z'] == 0 and kwargs['max_z'] == 0:
            plot2d.instantiateGraph(kwargs['min_x'], kwargs['min_y'],
                                    kwargs['max_x'], kwargs['max_y'])
            plot2d.plotGraph(kwargs['nodes'], kwargs['conn'])
        else:
            plot3d.instantiateGraph(kwargs['min_x'], kwargs['min_y'],
                                    kwargs['min_z'], kwargs['max_x'],
                                    kwargs['max_y'], kwargs['max_z'])
            plot3d.instantiateNodes(kwargs['nodes'])
