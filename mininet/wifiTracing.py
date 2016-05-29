"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

import time
from mininet.wifiMobility import mobility
import threading
from mininet.wifiPlot import plot

class tracingMobility( object ):

    def __init__( self, **params ):
       
        self.thread = threading.Thread(name='tracingMobility', target=self.mobility)
        self.thread.daemon = True
        self.thread.start()
        
    def plotGraph(self):
        nodeList = mobility.staList + mobility.apList
        for node in nodeList:
            plot.instantiateGraph(mobility.MAX_X, mobility.MAX_Y)
            plot.instantiateNode(node, mobility.MAX_X, mobility.MAX_Y)
            plot.instantiateAnnotate(node)
            plot.instantiateCircle(node)
            plot.graphUpdate(node)        
    
    def mobility(self):  
        if mobility.DRAW:
            self.plotGraph()
        currentTime = time.time()
        staList = mobility.staList
        continue_ = True
        nodeTime = {}
        nodeCurrentTime = {}
        for node in staList:
            nodeCurrentTime[node] = 1/node.params['speed']
            nodeTime[node] = float(1.0/node.params['speed'])
        while continue_:
            continue_ = False
            time_ = time.time() - currentTime
            for node in staList:
                continue_ = True
                while time_>=nodeCurrentTime[node] and len(node.trackingPos) != 0:
                    node.moveStationTo(node.trackingPos[0])
                    del node.trackingPos[0]
                    nodeCurrentTime[node] += nodeTime[node]
                if len(node.trackingPos) == 0:
                    staList.remove(node)            
            time.sleep(0.01)