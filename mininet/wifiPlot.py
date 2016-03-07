"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

import matplotlib.patches as patches
import matplotlib.pyplot as plt

class plot (object):
    """Plot Graph: Useful when the position is previously defined.
                        Not useful for Mobility Models"""
    
    nodesPlotted = []
    pltCircle = {}
    pltNode = {}
    plttxt = {}
    ax = None    
          
    @classmethod
    def closePlot(self):
        """Close"""
        plt.close()  
    
    @classmethod
    def plotNode(self, node):
        """Update Draw"""
        self.pltNode[node].set_data(node.position[0], node.position[1])
        
    @classmethod
    def plotTxt(self, node):
        self.plttxt[node].xytext = node.position[0], node.position[1] 
     
    @classmethod
    def updateCircle(self, node):
        self.pltCircle[node].center = node.position[0], node.position[1]
    
    @classmethod
    def plotUpdate(self, node):
        self.pltNode[node].set_data(node.position[0], node.position[1])
        self.plttxt[node].xytext = node.position[0], node.position[1] 
        self.pltCircle[node].center = node.position[0], node.position[1]
        plt.title("Mininet-WiFi Graph")
        plt.draw() 
    
    @classmethod
    def instantiateGraph(self):
        plt.ion()
        self.ax = plt.subplot(111)
        
    @classmethod
    def instantiateNode(self, node, MAX_X, MAX_Y):
        ax = self.ax
        
        color = 'b'
        if node.type == 'station':
            color = 'g'
                
        self.pltNode[node], = ax.plot(range(MAX_X), range(MAX_Y), \
                                     linestyle='', marker='.', ms=10, mfc=color)
        
        self.plttxt[node] = ax.annotate(node, xy=(0, 0))
        
        self.pltCircle[node] = ax.add_patch(
            patches.Circle((0, 0),
            node.range, fill=True, alpha=0.1, color=color
            )
        )
        self.nodesPlotted.append(node)