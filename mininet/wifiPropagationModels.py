"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

import math

class propagationModel_ ( object ):
    """ Propagation Models """
    
    rssi = -62
    model = ''
    exp = 0 #Exponent
    sl = 2 #System Loss
        
    def __init__( self, node1=None, node2=None, dist=0, wlan=None ):
        if self.model in dir(self):
            self.__getattribute__(self.model)(node1, node2, dist, wlan)
     
    def receivedPower(self, node1, node2, wlan, modelValue):
        if node2 == None:
            txpower = node1.params['txpower'][wlan]
        else:
            txpower = node2.params['txpower'][0]
        #txgain = 24
        #rxgain = 24
        self.rssi = txpower + modelValue   
        return self.rssi
    
    def attenuation(self, node1, node2, dist, wlan):
        alpha = 1
        gT = node2.antennaGain[0] 
        gR = node1.antennaGain[wlan]
        
        L = -27.56 + 10 * alpha * math.log10(dist) + 20 * math.log(node1.params['frequency'][wlan])
        P = node2.params['txpower'][0] * gR * gT * L
    
    def friisPropagationLossModel(self, node1, node2, dist, wlan):
        """Friis Propagation Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""          
        f = (node1.params['frequency'][wlan] * 10**9) #Convert Ghz to Hz
        d = dist 
        c = 299792458.0 
        L = self.sl
        
        lambda_ = c / f # lambda: wavelength (m)
        numerator = lambda_**2
        denominator = (4 * math.pi * d)**2 * L
        modelValue = 10 * math.log10(numerator / denominator)
        self.receivedPower(node1, node2, wlan, modelValue)
        
    def twoRayGroundPropagationLossModel(self, node1, node2, dist, wlan):
        """Two Ray Ground Propagation Loss Model:
        (gT): Tx Antenna Gain (dBi)
        (gR): Rx Antenna Gain (dBi)
        (hT): Tx Antenna Height
        (hR): Rx Antenna Height
        (d) is the distance between the transmitter and the receiver (m)
        (L): System loss"""
        
        gT = node2.params['antennaGain'][0] 
        gR = node1.params['antennaGain'][wlan]
        hT = node2.params['antennaHeight'][0]
        hR = node1.params['antennaHeight'][wlan]
        d = dist
        L = self.systemLoss
        
        try:
            self.rssi = (node2.params['txpower'][0] * gT * gR * hT**2 * hR**2) / (d**4 * L)
            return self.rssi
        except:
            return self.rssi
            
    def logDistancePropagationLossModel(self, node1, node2, dist, wlan):
        """Log Distance Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is calculated
        referenceLoss (db): The reference loss at reference distance. Default for 1m is 46.6777
        exponent: The exponent of the Path Loss propagation model, where 2 is for propagation in free space
        (d) is the distance between the transmitter and the receiver (m)"""        
        referenceDistance = 1
        referenceLoss = 46.6777
        if dist == 0:
            dist = 0.1
        pathLossDb = 10 * self.exp * math.log10(dist / referenceDistance)
        rxc = - referenceLoss - pathLossDb
        self.rssi = node2.params['txpower'][0] + rxc
        return self.rssi
        
    def okumuraHataPropagationLossModel(self, node1, node2, distance, wlan):
        """Okumura Hata Propagation Loss Model:"""
        
    def jakesPropagationLossModel(self, node1, node2, distance, wlan):
        """Jakes Propagation Loss Model:"""
        