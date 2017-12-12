"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""
import numpy as np
import matplotlib.patches as patches
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from mininet.log import debug


class plot3d (object):

    ax = None
    is3d = False

    @classmethod
    def instantiateGraph(cls, MIN_X, MIN_Y, MIN_Z, MAX_X, MAX_Y, MAX_Z):
        """instantiateGraph"""
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
        """instantiateAnnotate"""

        x = '%.2f' % float(node.params['position'][0])
        y = '%.2f' % float(node.params['position'][1])
        z = '%.2f' % float(node.params['position'][2])

        node.plttxt = cls.ax.text(float(x), float(y), float(z), node.name)

    @classmethod
    def instantiateNode(cls, node):
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

        node.pltNode = cls.ax.plot_surface(x, y, z, alpha=0.2,
                                           edgecolor='none', color='black')

    @classmethod
    def instantiateNodes(cls, nodes):
        """Instantiate Nodes"""
        cls.is3d = True
        for node in nodes:
            cls.instantiateAnnotate(node)
            cls.instantiateNode(node)
            cls.instantiateCircle(node)
            cls.plotDraw()

    @classmethod
    def fig_exists(cls):
        return plt.fignum_exists(1)

    @classmethod
    def graphPause(cls):
        """Pause"""
        plt.pause(0.0001)

    @classmethod
    def graphUpdate(cls, node):
        """Graph Update"""

        node.pltNode.remove()
        node.pltCircle.remove()
        node.plttxt.remove()

        cls.instantiateCircle(node)
        cls.instantiateNode(node)
        cls.instantiateAnnotate(node)
        cls.plotDraw()

    @classmethod
    def plotDraw(cls):
        """plotDraw"""
        plt.draw()

    @classmethod
    def closePlot(cls):
        """Close"""
        try:
            plt.close()
        except:
            pass

    @classmethod
    def instantiateCircle(cls, node):
        """Instantiate Circle"""
        from mininet.node import Station, Car

        x = '%.2f' % float(node.params['position'][0])
        y = '%.2f' % float(node.params['position'][1])
        z = '%.2f' % float(node.params['position'][2])

        color = 'b'
        if isinstance(node, Station):
            color = 'g'
        elif isinstance(node, Car):
            color = 'r'

        resolution = 100
        u = np.linspace(0, 2 * np.pi, resolution)
        v = np.linspace(0, np.pi, resolution)

        r = max(node.params['range'])

        x = r * np.outer(np.cos(u), np.sin(v)) + float(x)
        y = r * np.outer(np.sin(u), np.sin(v)) + float(y)
        z = r * np.outer(np.ones(np.size(u)), np.cos(v)) + float(z)

        node.pltCircle = cls.ax.plot_surface(x, y, z, alpha=0.2,
                                             edgecolor='none', color=color)


class plot2d (object):

    ax = None

    @classmethod
    def closePlot(cls):
        """Close"""
        try:
            plt.close()
        except:
            pass

    @classmethod
    def text(cls, node):
        """draw text"""
        x = '%.2f' % float(node.params['position'][0])
        y = '%.2f' % float(node.params['position'][1])

        # newer MPL versions (>=1.4)
        if hasattr(node.plttxt, 'xyann'): node.plttxt.xyann = (x, y)
        else: node.plttxt.xytext = (x, y)

    @classmethod
    def circle(cls, node):
        """drawCircle"""
        x = '%.2f' % float(node.params['position'][0])
        y = '%.2f' % float(node.params['position'][1])

        node.pltCircle.center = x, y

    @classmethod
    def graphUpdate(cls, node):
        """Graph Update"""
        x = '%.2f' % float(node.params['position'][0])
        y = '%.2f' % float(node.params['position'][1])

        # newer MPL versions (>=1.4)
        if hasattr(node.plttxt, 'xyann'): node.plttxt.xyann = (x, y)
        else: node.plttxt.xytext = (x, y)

        node.pltNode.set_data(x, y)
        node.pltCircle.center = x, y

    @classmethod
    def graphPause(cls):
        """Pause"""
        plt.pause(0.001)

    @classmethod
    def plotDraw(cls):
        "plotDraw"
        plt.draw()

    @classmethod
    def plotScatter(cls, nodesx, nodesy):
        "plotScatter"
        return plt.scatter(nodesx, nodesy, color='red', marker='s')

    @classmethod
    def plotLine2d(cls, nodesx, nodesy, color='', ls='-', lw=1):
        "plotLine2d"
        return plt.Line2D(nodesx, nodesy, color=color, ls=ls, lw=lw)

    @classmethod
    def plotLineTxt(cls, x, y, i):
        "plotLineTxt"
        title = 'Av.%s' % i
        plt.text(x, y, title, ha='left', va='bottom', fontsize=8, color='g')

    @classmethod
    def plotLine(cls, line):
        "plotLine"
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
        from mininet.node import Station, Car
        ax = cls.ax

        color = 'b'
        if isinstance(node, Station):
            color = 'g'
        elif isinstance(node, Car):
            color = 'r'

        node.pltNode, = ax.plot(1, 1, linestyle='', marker='.', ms=10,
                                mfc=color)

    @classmethod
    def instantiateCircle(cls, node):
        "instantiateCircle"
        from mininet.node import Station, Car
        ax = cls.ax

        color = 'b'
        if isinstance(node, Station):
            color = 'g'
        elif isinstance(node, Car):
            color = 'r'

        node.pltCircle = ax.add_patch(
            patches.Circle((0, 0), max(node.params['range']),
                           fill=True, alpha=0.1, color=color
                          )
        )

    @classmethod
    def instantiateAnnotate(cls, node):
        "instantiateAnnotate"
        node.plttxt = cls.ax.annotate(node, xy=(0, 0))

    @classmethod
    def updateCircleRadius(cls, node):
        node.pltCircle.set_radius(max(node.params['range']))

    @classmethod
    def updateCircleColor(cls, node, color):
        node.pltCircle.set_color(color)

    @classmethod
    def instantiateNodes(cls, node):
        cls.instantiateAnnotate(node)
        cls.instantiateCircle(node)
        cls.instantiateNode(node)
        cls.graphUpdate(node)

    @classmethod
    def plotGraph(cls, wifiNodes=[], connections=[]):
        "Plot Graph"
        debug('Enabling Graph...\n')
        for node in wifiNodes:
            x = '%.2f' % float(node.params['position'][0])
            y = '%.2f' % float(node.params['position'][1])
            cls.instantiateNodes(node)
            node.pltNode.set_data(x, y)
            cls.text(node)
            cls.circle(node)

        if 'src' in connections:
            for c in range(0, len(connections['src'])):
                ls = connections['ls'][c]
                src_x = '%.2f' \
                        % float(connections['src'][c].params['position'][0])
                src_y = '%.2f' \
                        % float(connections['src'][c].params['position'][1])
                dst_x = '%.2f'\
                        % float(connections['dst'][c].params['position'][0])
                dst_y = '%.2f' \
                        % float(connections['dst'][c].params['position'][1])
                line = cls.plotLine2d([src_x, dst_x], \
                                       [src_y, dst_y], 'b', ls=ls)
                cls.plotLine(line)
