"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

from mininet.wifiChannel import channelParameters
import os
import threading

class report ( object ):
    """ Propagation Models """
    
    dist = 0
        
    def __init__( self, node=None, dist=0 ):
        self.thread = threading.Thread(name='report', target=self.start, args=(node,dist,))
        self.thread.daemon = True
        self.thread.start()
     
    def start(self, node, d):
        dd = -1
        while self.dist <= d: 
            ap = node.params['associatedTo'][0]
            if ap != '':
                self.dist = channelParameters.getDistance(node, ap)
                if dd != self.dist:
                    os.system("echo %.3f %d >> data.text" % (self.dist, node.params['rssi'][0]))
                    dd = self.dist