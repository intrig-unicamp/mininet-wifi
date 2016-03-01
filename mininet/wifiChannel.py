"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

"""

from wifiDevices import deviceDataRate
from wifiPropagationModels import propagationModel
from wifiMobilityModels import distance
import math


class channelParameters ( object ):
    """Channel Parameters""" 
    delay = 0
    loss = 0
    latency = 0
    rate = 0
    
    def __init__( self, param=None, **params ):
        
        if param in dir(self):
            self.__getattribute__(param)(params)
            
    def delay(self, params):
        """"Based on RandomPropagationDelayModel"""
        time = (params.pop('time', {}))
        dist = (params.pop('dist', {}))
        if time != 0:
            self.delay = dist/time
        else:
            self.delay = dist/10
        return self.delay   
    
    def latency(self, params):    
        dist = (params.pop('dist', {}))    
        self.latency = 2 + dist
        return self.latency
        
    def loss(self, params):  
        dist = (params.pop('dist', {}))
        if dist!=0:
            self.loss =  math.log10(dist * dist)
        else:
            self.loss = 0.1
        return self.loss
    
    def bw(self, params):
        node1 = (params.pop('sta', {}))
        node2 = (params.pop('ap', {}))
        dist = (params.pop('dist', {}))
        wlan = (params.pop('wlan', {}))
        propagModel = (params.pop('propagationModel', {}))
        isMobility = (params.pop('isMobility', {}))
        systemLoss = 1
        self.rate = 0
        model = propagModel
        if model == '':
            model = 'friisPropagationLossModel'
        value = deviceDataRate(node1, node2, wlan, isMobility)
        custombw = value.rate
        interference(node1, node2, model, wlan) 
        if node2 == None:
            node1.rssi[wlan] = -50 - dist
            self.rate = custombw * (1.1 ** -dist)
        else:
            if dist != 0: 
                value = propagationModel(node1, node2, dist, wlan, model, systemLoss)
                node1.rssi[wlan] = value.rssi
                if node2.equipmentModel == None:
                    self.rate = custombw * (1.1 ** -dist)
        return self.rate 
    
  
class interference ( object ):
    
    def __init__( self, node1=None, node2=None, propagation_Model=None, wlan=0 ):
        self.calculate(node1, node2, wlan, propagation_Model)
    
    def calculate (self, node1, node2, wlan, propagation_Model):
        """Calculate Interference"""
        sta = node1
        ap = node2
        model = propagation_Model
        totalRange = 0
        noise = 0
        i=0
        signalPower = sta.rssi[wlan]
        try:
            for station in ap.associatedStations:
                if station != sta and sta.associatedAp[wlan] != 'NoAssociated':
                    d = distance(sta, station)
                    dist = d.dist
                    totalRange = sta.range + station.range
                    systemLoss = 1
                    if dist < totalRange:
                        value = propagationModel(sta, station, dist, wlan, model, systemLoss)
                        n =  value.rssi + signalPower
                        noise =+ n
                        i+=1
            if i != 0:
                noisePower = noise/i
            signalPower = sta.rssi[wlan]
            if i != 0:
                sta.snr[wlan] = self.signalToNoiseRatio(signalPower, noisePower)
            else:
                sta.snr[wlan] = abs(signalPower)
        except:
            pass
            
    def signalToNoiseRatio(self, signalPower, noisePower):    
        """Calculating SNR"""
        snr =  signalPower - noisePower
        return snr

    def maxChannelNoise(self, node1, node2, wlan, modelValue):  
        """Have to work"""  
        #snr = 25 #Depends of the equipment
        #max_channel_noise = self.rssi[wlan] - snr
        
    def linkMargin(self, node1, node2, wlan, modelValue):    
        """Have to work"""
        #receive_sensitivity = -72 #Depends of the equipment
        #link_margin = self.rssi[wlan] - receive_sensitivity
    