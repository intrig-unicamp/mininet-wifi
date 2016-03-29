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
    propagModel=''
    
    def __init__( self, node1, node2, wlan, dist, staList, time ):
        self.delay = self.delay(dist, time)
        self.latency = self.latency(dist)
        self.loss = self.loss(dist)
        self.bw = self.bw(node1, node2, dist, staList, wlan)
        self.tc(node1, wlan, self.bw, self.loss, self.latency, self.delay)
        interference(node1, node2, self.propagModel, staList, wlan)
            
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
    
    def bw(self, node1, node2, dist, staList, wlan):
        systemLoss = 1
        self.rate = 0
        if self.propagModel == '':
            self.propagModel = 'friisPropagationLossModel'
        value = deviceDataRate(node1, node2, wlan)
        custombw = value.rate
        if node2 == None:
            node1.rssi[wlan] = -50 - dist
            self.rate = custombw * (1.1 ** -dist)
        else:
            if dist != 0: 
                value = propagationModel(node1, node2, dist, wlan, self.propagModel, systemLoss)
                node1.rssi[wlan] = value.rssi
                if node2.equipmentModel == None:
                    self.rate = custombw * (1.1 ** -dist)
         
        return self.rate     
    
    def tc(self, node, wlan, bw, loss, latency, delay):
        if node.func[wlan] == 'mesh':
            iface = 'mp'
        else:
            iface = 'wlan'
        
        node.pexec("tc qdisc replace dev %s-%s%s \
            root handle 3: netem rate %.2fmbit \
            loss %.1f%% \
            latency %.2fms \
            delay %.2fms" % (node, iface, wlan, bw, loss, latency, delay))   
        #Reordering packets    
        node.pexec('tc qdisc add dev %s-%s%s parent 3:1 pfifo limit 1000' % (node, iface, wlan))  
        
        
class interference ( object ):
    """Calculate Interference"""
    
    def __init__( self, node1=None, node2=None, propagation_Model=None, staList=None, wlan=0 ):
        self.calculate(node1, node2, propagation_Model, staList, wlan)
    
    def calculate (self, node1, node2, propagation_Model, staList, wlan):
        
        sta = node1
        ap = node2
        model = propagation_Model
        totalRange = 0
        noise = 0
        i=0
        signalPower = sta.rssi[wlan]
        if node2 == None:
            for station in staList:
                if station != sta and sta.isAssociated[wlan] == True:
                    d = distance(sta, station)
                    dist = d.dist
                    totalRange = sta.range + station.range
                    systemLoss = 1
                    if dist < totalRange:
                        value = propagationModel(sta, station, dist, wlan, model, systemLoss)
                        n =  value.rssi + signalPower
                        noise =+ n
                        i+=1
        else:
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
    