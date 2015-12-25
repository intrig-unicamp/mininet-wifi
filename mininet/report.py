"""***Reports***
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)"""

import os
import time

class report (object):
    
    storeData = ''
    
    @classmethod 
    def graph(self):
        self.cmd = ("echo \'")
        self.cmd = self.cmd + self.storeData
        command = self.cmd + ("\' > report.txt") 
        print os.system(command)
        #print command
        
        
    @classmethod
    def retrieveData(self, sta=None, ap=None, distance=None, t=None, bw=None, associated=None):
    #def retrieveData(self, time=None, bw=None, distance=None):
        self.time = time.time() - t
        self.bw = bw
        self.distance = distance 
        
        if self.bw == -1:
            self.bw = 0
         
        data = '%s, %s, %.3f, %s, %.3f, %s \n' % (sta, ap, self.time, self.bw, self.distance, associated)
        self.storeData = self.storeData + data
        if self.time>=60 and self.time<61:
            self.graph()