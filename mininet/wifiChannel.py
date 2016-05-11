"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

from wifiDevices import deviceDataRate
from wifiPropagationModels import propagationModel
from wifiEmulationEnvironment import emulationEnvironment
from scipy.spatial.distance import pdist
import numpy as np
import math
import random

class channelParameters ( object ):
    """Channel Parameters""" 
    
    delay = 0
    loss = 0
    latency = 0
    rate = 0
    dist = 0
    noise = 0
    i = 0
    
    def __init__( self, node1, node2, wlan, dist, staList, time ):
        self.calculateInterference(node1, node2, emulationEnvironment.propagation_Model, staList, wlan)
        self.delay = self.delay(dist, time)
        self.latency = self.latency(dist)
        self.loss = self.loss(dist)
        self.bw = self.bw(node1, node2, dist, wlan)
        self.tc(node1, wlan, self.bw, self.loss, self.latency, self.delay)
        
    @classmethod
    def getDistance(self, src, dst):
        """ Get the distance between two nodes """
        pos_src = src.position
        pos_dst = dst.position
        points = np.array([(pos_src[0], pos_src[1], pos_src[2]), (pos_dst[0], pos_dst[1], pos_dst[2])])
        self.dist = pdist(points)
        return self.dist
            
    def delay(self, dist, time):
        """"Based on RandomPropagationDelayModel"""
        if time != 0:
            self.delay = dist/time
        else:
            self.delay = dist/10
        return self.delay   
    
    def latency(self, dist):    
        self.latency = 2 + dist
        return self.latency
        
    def loss(self, dist):  
        if dist!=0:
            self.loss =  math.log10(dist * dist)
        else:
            self.loss = 0.1
        return self.loss
    
    def bw(self, node1, node2, dist, wlan):
        systemLoss = 1
        self.rate = 0
        if emulationEnvironment.propagation_Model == '':
            emulationEnvironment.propagation_Model = 'friisPropagationLossModel'
        value = deviceDataRate(node1, node2, wlan)
        custombw = value.rate
        self.rate = value.rate
        if node2 == None:
            node1.rssi[wlan] = -50 - dist
            self.rate = custombw * (1.1 ** -dist)
        else:
            if dist != 0: 
                value = propagationModel(node1, node2, dist, wlan, emulationEnvironment.propagation_Model, systemLoss)
                node1.rssi[wlan] = value.rssi
                if node2.equipmentModel == None:
                    self.rate = custombw * (1.1 ** -dist)     
        self.rate = self.rate - self.loss*2
        if self.rate <= 0:
            self.rate = 1
        return self.rate  
    
    def tc(self, node, wlan, bw, loss, latency, delay):
        if node.func[wlan] == 'mesh':
            iface = 'mp'
        else:
            iface = 'wlan'
        
        if emulationEnvironment.loss != 0:
            loss = emulationEnvironment.loss
        
        bw = random.uniform(bw-1, bw+1)
        
        node.pexec("tc qdisc replace dev %s-%s%s \
            root handle 2: netem rate %.2fmbit \
            loss %.1f%% \
            latency %.2fms \
            delay %.2fms \
            corrupt 0.1%%" % (node, iface, wlan, bw, loss, latency, delay))   
        
    def calculateInterference (self, sta, ap, propagation_Model, staList, wlan):        
        model = propagation_Model
        self.noise = 0
        noisePower = 0
        self.i=0
        signalPower = sta.rssi[wlan]
        
        if ap == None:
            for station in staList:
                if station != sta and sta.isAssociated[wlan] == True:
                    self.calculateNoise(sta, station, signalPower, wlan, model)
        else:
            for station in ap.associatedStations:
                if station != sta and sta.associatedAp[wlan] != 'NoAssociated':
                    self.calculateNoise(sta, station, signalPower, wlan, model)
        
        if self.noise != 0:
            noisePower = self.noise/self.i
        signalPower = sta.rssi[wlan]
        sta.snr[wlan] = self.signalToNoiseRatio(signalPower, noisePower)
            
    def calculateNoise(self, sta, station, signalPower, wlan, model):
        dist = self.getDistance(sta, station)
        totalRange = sta.range + station.range
        systemLoss = 1
        if dist < totalRange:
            value = propagationModel(sta, station, dist, wlan, model, systemLoss)
            n =  value.rssi + signalPower
            self.noise =+ n
            self.i+=1        
            
    def signalToNoiseRatio(self, signalPower, noisePower):    
        """Calculating SNR margin"""
        snr = signalPower - noisePower
        return snr

    def maxChannelNoise(self, node1, node2, wlan, modelValue):  
        """Have to work"""  
        #snr = 25 #Depends of the equipment
        #max_channel_noise = self.rssi[wlan] - snr
        
    def linkMargin(self, node1, node2, wlan, modelValue):    
        """Have to work"""
        #receive_sensitivity = -72 #Depends of the equipment
        #link_margin = self.rssi[wlan] - receive_sensitivity
    
