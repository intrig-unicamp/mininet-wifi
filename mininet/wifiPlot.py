"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

import matplotlib.patches as patches
import matplotlib.pyplot as plt
from mininet.log import debug
from mpl_toolkits.mplot3d import Axes3D
import numpy as np

class plot3d (object):

    ax = None

    @classmethod
    def instantiateGraph(self, MIN_X, MIN_Y, MIN_Z, MAX_X, MAX_Y, MAX_Z):
        """instantiateGraph"""
        plt.ion()
        plt.title("Mininet-WiFi Graph")
        self.ax = plt.subplot(111, projection='3d')
        self.ax.set_xlabel('meters (x)')
        self.ax.set_ylabel('meters (y)')
        self.ax.set_zlabel('meters (z)')
        self.ax.set_xlim([MIN_X, MAX_X])
        self.ax.set_ylim([MIN_Y, MAX_Y])
        self.ax.set_zlim([MIN_Z, MAX_Z])
        self.ax.grid(True)

    @classmethod
    def instantiateAnnotate(self, node):
        """instantiateAnnotate"""

        x = '%.2f' % float(node.params['position'][0])
        y = '%.2f' % float(node.params['position'][1])
        z = '%.2f' % float(node.params['position'][2])

        node.plttxt = self.ax.text(float(x), float(y), float(z), node.name)

    @classmethod
    def instantiateNode(self, node):
        """Instantiate Node"""
        x = '%.2f' % float(node.params['position'][0])
        y = '%.2f' % float(node.params['position'][1])
        z = '%.2f' % float(node.params['position'][2])

        resolution = 40
        u = np.linspace(0, 2 * np.pi, resolution)
        v = np.linspace(0, np.pi, resolution)

        r = 1

        x = r * np.outer(np.cos(u), np.sin(v)) + float(x)
        y = r * np.outer(np.sin(u), np.sin(v)) + float(y)
        z = r * np.outer(np.ones(np.size(u)), np.cos(v)) + float(z)

        node.pltNode = self.ax.plot_surface(x, y, z, alpha=0.2, edgecolor='none', color='black')

    @classmethod
    def instantiateNodes(self, nodes):
        """Instantiate Nodes"""
        for node in nodes:
            self.instantiateAnnotate(node)
            self.instantiateNode(node)
            self.instantiateCircle(node)
        self.plotDraw()

    @classmethod
    def fig_exists(self):
        return plt.fignum_exists(1)

    @classmethod
    def graphPause(self):
        """Pause"""
        plt.pause(0.0001)

    @classmethod
    def graphUpdate(self, node):
        """Graph Update"""

        node.pltNode.remove()
        node.pltCircle.remove()
        node.plttxt.remove()

        self.instantiateCircle(node)
        self.instantiateNode(node)
        self.instantiateAnnotate(node)
        self.plotDraw()

    @classmethod
    def plotDraw(self):
        """plotDraw"""
        plt.draw()

    @classmethod
    def closePlot(self):
        """Close"""
        try:
            plt.close()
        except:
            pass

    @classmethod
    def instantiateCircle(self, node):
        """Instantiate Circle"""

        x = '%.2f' % float(node.params['position'][0])
        y = '%.2f' % float(node.params['position'][1])
        z = '%.2f' % float(node.params['position'][2])

        color = 'b'
        if node.type == 'station':
            color = 'g'
        elif node.type == 'vehicle':
            color = 'r'

        resolution = 100
        u = np.linspace(0, 2 * np.pi, resolution)
        v = np.linspace(0, np.pi, resolution)

        r = node.params['range']

        x = r * np.outer(np.cos(u), np.sin(v)) + float(x)
        y = r * np.outer(np.sin(u), np.sin(v)) + float(y)
        z = r * np.outer(np.ones(np.size(u)), np.cos(v)) + float(z)

        node.pltCircle = self.ax.plot_surface(x, y, z, alpha=0.2, edgecolor='none', color=color)


class plot2d (object):

    ax = None

    @classmethod
    def closePlot(self):
        """Close"""
        try:
            plt.close()
        except:
            pass

    @classmethod
    def text(self, node):
        """draw text"""
        x = '%.2f' % float(node.params['position'][0])
        y = '%.2f' % float(node.params['position'][1])

        if hasattr(node.plttxt, 'xyann'): node.plttxt.xyann = (x, y)  # newer MPL versions (>=1.4)
        else: node.plttxt.xytext = (x, y)

    @classmethod
    def circle(self, node):
        """drawCircle"""
        x = '%.2f' % float(node.params['position'][0])
        y = '%.2f' % float(node.params['position'][1])

        node.pltCircle.center = x, y

    @classmethod
    def graphUpdate(self, node):
        """Graph Update"""
        x = '%.2f' % float(node.params['position'][0])
        y = '%.2f' % float(node.params['position'][1])

        if hasattr(node.plttxt, 'xyann'): node.plttxt.xyann = (x, y)  # newer MPL versions (>=1.4)
        else: node.plttxt.xytext = (x, y)

        node.pltNode.set_data(x, y)
        node.pltCircle.center = x, y

    @classmethod
    def graphPause(self):
        """Pause"""
        plt.pause(0.001)

    @classmethod
    def plotDraw(self):
        "plotDraw"
        plt.draw()

    @classmethod
    def plotScatter(self, nodesx, nodesy):
        "plotScatter"
        return plt.scatter(nodesx, nodesy, color='red', marker='s')

    @classmethod
    def plotLine2d(self, nodesx, nodesy, color='', ls='-', lw=1):
        "plotLine2d"
        return plt.Line2D(nodesx, nodesy, color=color, ls=ls, lw=lw)

    @classmethod
    def plotLineTxt(self, x, y, i):
        "plotLineTxt"
        title = 'Av.%s' % i
        plt.text(x, y, title, ha='left', va='bottom', fontsize=8, color='g')

    @classmethod
    def plotLine(self, line):
        "plotLine"
        ax = self.ax
        ax.add_line(line)

    @classmethod
    def instantiateGraph(self, MIN_X, MIN_Y, MAX_X, MAX_Y):
        "instantiateGraph"
        plt.ion()
        plt.title("Mininet-WiFi Graph")
        self.ax = plt.subplot(111)
        self.ax.set_xlabel('meters')
        self.ax.set_ylabel('meters')
        self.ax.set_xlim([MIN_X, MAX_X])
        self.ax.set_ylim([MIN_Y, MAX_Y])
        self.ax.grid(True)

    @classmethod
    def fig_exists(self):
        return plt.fignum_exists(1)

    @classmethod
    def instantiateNode(self, node):
        "instantiateNode"
        ax = self.ax

        color = 'b'
        if node.type == 'station':
            color = 'g'
        elif node.type == 'vehicle':
            color = 'r'

        node.pltNode, = ax.plot(1, 1, linestyle='', marker='.', ms=10, mfc=color)

    @classmethod
    def instantiateCircle(self, node):
        "instantiateCircle"
        ax = self.ax

        color = 'b'
        if node.type == 'station':
            color = 'g'
        elif node.type == 'vehicle':
            color = 'r'

        node.pltCircle = ax.add_patch(
            patches.Circle((0, 0),
            node.params['range'], fill=True, alpha=0.1, color=color
            )
        )

    @classmethod
    def instantiateAnnotate(self, node):
        "instantiateAnnotate"
        node.plttxt = self.ax.annotate(node, xy=(0, 0))

    @classmethod
    def updateCircleRadius(self, node):
        node.pltCircle.set_radius(node.params['range'])

    @classmethod
    def instantiateNodes(self, node):
        self.instantiateAnnotate(node)
        self.instantiateCircle(node)
        self.instantiateNode(node)
        self.graphUpdate(node)

    @classmethod
    def plotGraph(self, wifiNodes=[], connections=[]):
        "Plot Graph"
        debug('Enabling Graph...\n')
        for node in wifiNodes:
            x = '%.2f' % float(node.params['position'][0])
            y = '%.2f' % float(node.params['position'][1])
            self.instantiateNodes(node)
            node.pltNode.set_data(x, y)
            self.text(node)
            self.circle(node)

        for c in range(0, len(connections['src'])):
            ls = connections['ls'][c]
            src_x = '%.2f' % float(connections['src'][c].params['position'][0])
            src_y = '%.2f' % float(connections['src'][c].params['position'][1])
            dst_x = '%.2f' % float(connections['dst'][c].params['position'][0])
            dst_y = '%.2f' % float(connections['dst'][c].params['position'][1])
            line = self.plotLine2d([src_x, dst_x], \
                                   [src_y, dst_y], 'b', ls=ls)
            self.plotLine(line)
