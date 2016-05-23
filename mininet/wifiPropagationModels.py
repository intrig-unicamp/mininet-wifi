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
    sl = 1 #System Loss
        
    def __init__( self, sta=None, ap=None, dist=0, wlan=None, pT=14, gT=5, gR=5, hT=1, hR=1 ):
        """pT = Tx Power
           gT = Tx Antenna Gain
           gR = Rx Antenna Gain
           hT = Tx Antenna Height
           hR = Rx Antenna Height
        """        
        if self.model in dir(self):
            self.__getattribute__(self.model)(sta, ap, dist, wlan, pT, gT, gR, hT, hR)
     
    def attenuation(self, node1, node2, dist, wlan):
        """have to work on"""
        #alpha = 1
        #gT = node2.antennaGain[0] 
        #gR = node1.antennaGain[wlan]
        
        #L = -27.56 + 10 * alpha * math.log10(dist) + 20 * math.log(node1.params['frequency'][wlan])
        #P = node2.params['txpower'][0] * gR * gT * L
        
    def pathLoss(self, sta, ap, dist, wlan):
        """Path Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""          
        f = (sta.params['frequency'][wlan] * 10**9) #Convert Ghz to Hz
        d = dist 
        c = 299792458.0 
        L = self.sl     
       
        lambda_ = c / f # lambda: wavelength (m)
        denominator = lambda_**2 
        numerator = (4 * math.pi * d)**2 * L
        pathLoss_ = 10 * math.log10(numerator / denominator) 
        return pathLoss_
    
    def friisPropagationLossModel(self, sta, ap, dist, wlan, pT, gT, gR, hT, hR):
        """Friis Propagation Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""   
        pathLoss = self.pathLoss(sta, ap, dist, wlan)
        self.rssi = pT + gT + gR - pathLoss - 27.55
        return self.rssi
        
    def twoRayGroundPropagationLossModel(self, sta, ap, dist, wlan, pT, gT, gR, hT, hR):
        """Two Ray Ground Propagation Loss Model:
        (gT): Tx Antenna Gain (dBi)
        (gR): Rx Antenna Gain (dBi)
        (hT): Tx Antenna Height
        (hR): Rx Antenna Height
        (d) is the distance between the transmitter and the receiver (m)
        (L): System loss"""        
        d = dist
        L = self.systemLoss
        
        self.rssi = ( pT * gT * gR * hT**2 * hR**2 ) / ( d**4 * L )        
        return self.rssi
            
    def logDistancePropagationLossModel(self, sta, ap, dist, wlan, pT, gT, gR, hT, hR):
        """Log Distance Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is calculated
        exponent: The exponent of the Path Loss propagation model, where 2 is for propagation in free space
        (d) is the distance between the transmitter and the receiver (m)"""    
        referenceDistance = 1
        pathLoss = self.pathLoss(sta, ap, referenceDistance, wlan)
        
        pathLossDb = 10 * self.exp * math.log10(dist / referenceDistance)  
        self.rssi = pT + gT + gR - ( pathLoss + pathLossDb ) - 27.55
        return self.rssi
        
    def okumuraHataPropagationLossModel(self, node1, node2, distance, wlan):
        """Okumura Hata Propagation Loss Model:"""
        
    def jakesPropagationLossModel(self, node1, node2, distance, wlan):
        """Jakes Propagation Loss Model:"""
        