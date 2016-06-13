"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

import matplotlib.patches as patches
import matplotlib.pyplot as plt

class plot (object):
    
    nodesPlotted = []
    pltCircle = {}
    pltNode = {}
    plttxt = {}
    ax = None    
          
    @classmethod
    def closePlot(self):
        """Close"""
        try:
            plt.close()
        except:
            pass
    
    @classmethod
    def drawNode(self, node):
        """Update Draw"""
        self.pltNode[node].set_data(node.params['position'][0], node.params['position'][1])
        
    @classmethod
    def drawTxt(self, node):
        """drawTxt""" 
        if hasattr(self.plttxt[node],'xyann'): self.plttxt[node].xyann=(node.params['position'][0], node.params['position'][1] ) # newer MPL versions (>=1.4)
        else: self.plttxt[node].xytext=(node.params['position'][0], node.params['position'][1] )
        #self.plttxt[node].xytext = node.position[0], node.position[1] 
        
    @classmethod
    def drawCircle(self, node):
        """drawCircle""" 
        self.pltCircle[node].center = node.params['position'][0], node.params['position'][1]        
    
    @classmethod
    def graphUpdate(self, node):
        """Update Graph""" 
        if hasattr(self.plttxt[node],'xyann'): self.plttxt[node].xyann=(node.params['position'][0], node.params['position'][1] ) # newer MPL versions (>=1.4)
        else: self.plttxt[node].xytext=(node.params['position'][0], node.params['position'][1] )
        #self.plttxt[node].xytext = node.position[0], node.position[1] 
        
        self.pltNode[node].set_data(node.params['position'][0], node.params['position'][1])
        self.pltCircle[node].center = node.params['position'][0], node.params['position'][1]
        plt.draw() 
        
    @classmethod
    def plotDraw(self):
        """plotDraw"""
        plt.draw() 

    @classmethod
    def plotScatter(self, nodesx, nodesy):
        """plotScatter"""  
        return plt.scatter(nodesx,nodesy, color = 'red', marker = 's')
    
    @classmethod
    def plotLine2d(self, nodesx, nodesy, color='', lw=1):     
        """plotLine2d"""      
        return plt.Line2D(nodesx, nodesy, color=color, lw=lw)
    
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
        self.ax.set_xlim([0,MAX_X]) 
        self.ax.set_ylim([0,MAX_Y])
        self.ax.grid(True)        
        
    @classmethod
    def instantiateNode(self, node, MAX_X, MAX_Y):
        """instantiateNode"""
        ax = self.ax       
        
        color = 'b'
        if node.type == 'station':
            color = 'g'
        
        self.pltNode[node], = ax.plot(range(MAX_X), range(MAX_Y), \
                                     linestyle='', marker='.', ms=10, mfc=color)
        
        self.nodesPlotted.append(node)
        
    @classmethod
    def updateCircleRadius(self, node):
        self.pltCircle[node].set_radius(node.range)
    
    @classmethod
    def instantiateCircle(self, node):
        """instantiateCircle"""
        ax = self.ax
        
        color = 'b'
        if node.type == 'station':
            color = 'g'
        
        self.pltCircle[node] = ax.add_patch(
            patches.Circle((0, 0),
            node.range, fill=True, alpha=0.1, color=color
            )
        )
        
    @classmethod
    def instantiateAnnotate(self, node):
        """instantiateAnnotate"""
        ax = self.ax
        self.plttxt[node] = ax.annotate(node, xy=(0, 0))
