"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

from mininet.wifiMobilityModels import distance
import os
import threading

class report ( object ):
    """ Propagation Models """
    
    rssi = -62
    dist = 0
        
    def __init__( self, node=None, d=0 ):
        #self.start(node, d)
        self.thread = threading.Thread(name='report', target=self.start, args=(node,d,))
        self.thread.daemon = True
        self.thread.start()
     
    def start(self, node, d):  
        while self.dist <= d:   
            ap = node.associatedAp[0]
            d = distance(node, ap)
            self.dist = d.dist
            os.system("echo %d %d >> data.text" % (self.dist, node.rssi[0]))
    