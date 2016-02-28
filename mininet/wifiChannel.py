"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

"""

from mininet.wifiEmulationEnvironment import emulationEnvironment
from wifiDevices import deviceDataRate
from wifiPropagationModels import propagationModel
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
        sta = (params.pop('sta', {}))
        dist = (params.pop('dist', {}))
        ap = (params.pop('ap', {}))
        wlan = (params.pop('wlan', {}))
        if ap == None:
            value = deviceDataRate(ap, sta, wlan, emulationEnvironment.ismobility)
            custombw = value.rate
            sta.rssi[wlan] = -50 - dist
            self.rate = custombw * (1.1 ** -dist)
            return self.rate
        else:
            value = deviceDataRate(ap, sta, wlan, emulationEnvironment.ismobility)
            custombw = value.rate
            model = emulationEnvironment.propagationModel
            if model == '':
                model = 'friisPropagationLossModel'
            systemLoss = 1
            if dist != 0: 
                if ap.equipmentModel == None:
                    value = propagationModel(sta, ap, dist, wlan, model, systemLoss)
                    sta.rssi[wlan] = value.rssi
                    self.rate = custombw * (1.1 ** -dist)
                    return self.rate
                else:
                    if sta.associatedAp[wlan] != 'NoAssociated':
                        value = deviceDataRate(ap, sta, wlan, emulationEnvironment.ismobility)
                        self.rate = value.rate
                        value = propagationModel(sta, ap, dist, wlan, model, systemLoss)
                        sta.rssi[wlan] = value.rssi
                    else:
                        self.rate = 0
                    return self.rate
            else:
                value = deviceDataRate(ap, sta, wlan, emulationEnvironment.ismobility)
                self.rate = value.rate
                return self.rate 
    
  
class interference ( object ):
    
    def __init__( self, sta=None, ap=None, wlan=0 ):
        self.calculate(sta, ap, wlan)
    
    def calculate (self, sta, ap, wlan):
        """Calculate Interference"""
        totalRange = 0
        noise = 0
        i=0
        signalPower = sta.rssi[wlan]
        try:
            for station in ap.associatedStations:
                if station != sta and sta.associatedAp[wlan] != 'NoAssociated':
                    dist = emulationEnvironment.getDistance(sta, station)
                    totalRange = sta.range + station.range
                    model = emulationEnvironment.propagationModel
                    systemLoss = 1
                    if dist < totalRange:
                        value = propagationModel(sta, station, dist, wlan, model, systemLoss)
                        n =  value.rssi + signalPower
                        noise =+ n
                        i+=1
            if i == 0:
                noisePower = 1
            else:
                noisePower = noise/i
            signalPower = sta.rssi[wlan]
            sta.snr[wlan] = self.signalToNoiseRatio(signalPower, noisePower)
        except:
            pass
            
    def signalToNoiseRatio(self, signalPower, noisePower):    
        """Calculate SNR"""
        snr =  signalPower - noisePower
        return snr

    def maxChannelNoise(self, sta, ap, wlan, modelValue):    
        snr = 25 #Depends of the equipment
        max_channel_noise = self.rssi[wlan] - snr
        
    def linkMargin(self, sta, ap, wlan, modelValue):    
        receive_sensitivity = -72 #Depends of the equipment
        link_margin = self.rssi[wlan] - receive_sensitivity
    