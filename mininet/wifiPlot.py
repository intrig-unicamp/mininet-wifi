"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

import matplotlib.patches as patches
import matplotlib.pyplot as plt
from mininet.log import debug
from mpl_toolkits.mplot3d import Axes3D

class plot3d (object):
    
    ax = None
    
    @classmethod
    def instantiateGraph(self, MAX_X, MAX_Y, MAX_Z):
        """instantiateGraph"""
        plt.ion()
        plt.title("Mininet-WiFi Graph")
        self.ax = plt.subplot(111, projection='3d')
        self.ax.set_xlabel('meters (x)')
        self.ax.set_ylabel('meters (y)')
        self.ax.set_zlabel('meters (z)')
        self.ax.set_xlim([0, MAX_X])
        self.ax.set_ylim([0, MAX_Y])
        self.ax.set_zlim([0, MAX_Z])
        self.ax.grid(True)
        
    @classmethod
    def instantiateAnnotate(self, node):
        """instantiateAnnotate"""
        node.plttxt = self.ax.text(float(node.params['position'][0]), float(node.params['position'][1]), float(node.params['position'][2]), node.name)

    @classmethod
    def instantiateNode(self, node):
        x = float(node.params['position'][0])
        y = float(node.params['position'][1])
        z = float(node.params['position'][2])
        
        color = 'b'
        if node.type == 'station':
            color = 'g'
        elif node.type == 'vehicle':
            color = 'r'
            
        node.pltNode = self.ax.scatter(x, y, z, c=color, marker='o')

    @classmethod
    def graphInstantiateNodes(self, nodes):
        """Instantiate Nodes"""
        for node in nodes:
            self.instantiateAnnotate(node)
            self.instantiateNode(node)
            self.instantiateCircle(node)      
        self.plotDraw()  
        
    @classmethod
    def graphPause(self):
        """Pause"""
        plt.pause(0.001)
        
    @classmethod
    def graphUpdate(self, node):
        """Graph Update"""
        x = [float("{0:.5f}".format(float(node.params['position'][0])))]
        y = [float("{0:.5f}".format(float(node.params['position'][1])))]
        z = [float("{0:.5f}".format(float(node.params['position'][2])))]

        node.pltNode._offsets3d = x,y,z
        node.pltCircle._offsets3d = x,y,z
        node.plttxt._position3d = x[0], y[0], z[0]
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
    
        x = float(node.params['position'][0])
        y = float(node.params['position'][1])
        z = float(node.params['position'][2])
        
        color = 'b'
        if node.type == 'station':
            color = 'g'
        elif node.type == 'vehicle':
            color = 'r'
        
        s = node.params['range'] * 500            
        node.pltCircle = self.ax.scatter(x, y, z, c=color, marker='o', s=s, alpha=0.2)

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
        if hasattr(node.plttxt, 'xyann'): node.plttxt.xyann = (node.params['position'][0], 
                                        node.params['position'][1])  # newer MPL versions (>=1.4)
        else: node.plttxt.xytext = (node.params['position'][0], node.params['position'][1])

    @classmethod
    def circle(self, node):
        """drawCircle"""
        node.pltCircle.center = node.params['position'][0], node.params['position'][1]

    @classmethod
    def graphUpdate(self, node):
        """Graph Update"""
        if hasattr(node.plttxt, 'xyann'): node.plttxt.xyann = (node.params['position'][0], 
                                        node.params['position'][1])  # newer MPL versions (>=1.4)
        else: node.plttxt.xytext = (node.params['position'][0], node.params['position'][1])

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
    def instantiateGraph(self, MAX_X, MAX_Y, is3d=False):
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
        node.plttxt = self.ax.annotate(node, xy=(0, 0))

    @classmethod
    def updateCircleRadius(self, node):
        node.pltCircle.set_radius(node.params['range'])

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
        
        for c in range(0, len(srcConn)):
            line = self.plotLine2d([srcConn[c].params['position'][0], dstConn[c].params['position'][0]], \
                                   [srcConn[c].params['position'][1], dstConn[c].params['position'][1]], 'b')
            self.plotLine(line)

        #for wall in wallList:
        #    line = self.plotLine2d([wall.params['initPos'][0], wall.params['finalPos'][0]], \
        #                               [wall.params['initPos'][1], wall.params['finalPos'][1]], 'r', wall.params['width'])
        #    self.plotLine(line)