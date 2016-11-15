"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

import matplotlib.patches as patches
import matplotlib.pyplot as plt
from mininet.log import debug

class plot (object):

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
        if hasattr(node.plttxt, 'xyann'): node.plttxt.xyann = (node.params['position'][0], 
                                                                           node.params['position'][1])  # newer MPL versions (>=1.4)
        else: node.plttxt.xytext = (node.params['position'][0], node.params['position'][1])

    @classmethod
    def circle(self, node):
        """drawCircle"""
        node.pltCircle.center = node.params['position'][0], node.params['position'][1]

    @classmethod
    def graphUpdate(self, node):
        """Update Graph"""
        if hasattr(node.plttxt, 'xyann'): node.plttxt.xyann = (node.params['position'][0], 
                                                                           node.params['position'][1])  # newer MPL versions (>=1.4)
        else: self.plttxt[node].xytext = (node.params['position'][0], node.params['position'][1])

        node.pltNode.set_data(node.params['position'][0], node.params['position'][1])
        node.pltCircle.center = node.params['position'][0], node.params['position'][1]
        self.plotDraw()

    @classmethod
    def graphPause(self):
        """Pause"""
        plt.pause(0.001)

    @classmethod
    def plotDraw(self):
        """plotDraw"""
        plt.draw()

    @classmethod
    def plotScatter(self, nodesx, nodesy):
        """plotScatter"""
        return plt.scatter(nodesx, nodesy, color='red', marker='s')

    @classmethod
    def plotLine2d(self, nodesx, nodesy, color='', ls='-', lw=1):
        """plotLine2d"""
        return plt.Line2D(nodesx, nodesy, color=color, ls=ls, lw=lw)

    @classmethod
    def plotLineTxt(self, x, y, i):
        """plotLineTxt"""
        title = 'Av.%s' % i
        plt.text(x, y, title, ha='left', va='bottom', fontsize=8, color='g')

    @classmethod
    def plotLine(self, line):
        """plotLine"""
        ax = self.ax
        ax.add_line(line)

    @classmethod
    def instantiateGraph(self, MAX_X, MAX_Y):
        """instantiateGraph"""
        plt.ion()
        plt.title("Mininet-WiFi Graph")
        self.ax = plt.subplot(111)
        self.ax.set_xlabel('meters')
        self.ax.set_ylabel('meters')
        self.ax.set_xlim([0, MAX_X])
        self.ax.set_ylim([0, MAX_Y])
        self.ax.grid(True)

    @classmethod
    def instantiateNode(self, node, MAX_X, MAX_Y):
        """instantiateNode"""
        ax = self.ax

        color = 'b'
        if node.type == 'station':
            color = 'g'
        elif node.type == 'vehicle':
            color = 'r'

        node.pltNode, = ax.plot(range(MAX_X), range(MAX_Y), \
                                     linestyle='', marker='.', ms=10, mfc=color)

    @classmethod
    def updateCircleRadius(self, node):
        node.pltCircle.set_radius(node.params['range'])

    @classmethod
    def instantiateCircle(self, node):
        """instantiateCircle"""
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
        """instantiateAnnotate"""
        ax = self.ax
        node.plttxt = ax.annotate(node, xy=(0, 0))

    @classmethod
    def graphInstantiateNodes(self, node, MAX_X, MAX_Y):
        self.instantiateAnnotate(node)
        self.instantiateCircle(node)
        self.instantiateNode(node, MAX_X, MAX_Y)
        self.graphUpdate(node)

    @classmethod
    def plotGraph(self, wifiNodes=[], srcConn=[], dstConn=[], MAX_X=0, MAX_Y=0):
        """ Plot Graph """

        debug('Enabling Graph...\n')           
        for node in wifiNodes:
            self.graphInstantiateNodes(node, MAX_X, MAX_Y)
            node.pltNode.set_data(node.params['position'][0], node.params['position'][1])
            self.text(node)
            self.circle(node)
            self.graphUpdate(node)
        
        for c in range(0, len(srcConn)):
            line = self.plotLine2d([srcConn[c].params['position'][0], dstConn[c].params['position'][0]], \
                                   [srcConn[c].params['position'][1], dstConn[c].params['position'][1]], 'b')
            self.plotLine(line)

        #for wall in wallList:
        #    line = self.plotLine2d([wall.params['initPos'][0], wall.params['finalPos'][0]], \
        #                               [wall.params['initPos'][1], wall.params['finalPos'][1]], 'r', wall.params['width'])
        #    self.plotLine(line)