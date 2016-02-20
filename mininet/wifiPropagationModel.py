"""
wifi Propagation Model to Mininet-WiFi.

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

"""
import math

class propagationModel ( object ):
    """ Propagation Models """
            
    def __init__( self, sta=None, ap=None, distance=0, wlan=None, model=None, systemLoss=1 ):
        self.model = model
        self.L = systemLoss
        
        if self.model in dir(self):
            self.__getattribute__(self.model)(sta, ap, distance, wlan)
            
    def friisPropagationLossModel(self, sta, ap, distance, wlan):
        """Friis Propagation Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""          
        
        f = (sta.frequency[wlan] * 10**9) #Convert Ghz to Hz
        d = distance 
        c = 299792458.0 
        L = self.L 
        
        lambda_ = c / f # lambda: wavelength (m)
        numerator = lambda_**2
        denominator = (4 * math.pi * d)**2 * L
        
        try:
            sta.rssi[wlan] = ap.txpower + 10 * math.log10(numerator / denominator)
        except:
            sta.rssi[wlan] = 0
        
    def twoRayGroundPropagationLossModel(self, sta, ap, distance, wlan):
        """Two Ray Ground Propagation Loss Model:
        (gT): Tx Antenna Gain (dBi)
        (gR): Rx Antenna Gain (dBi)
        (hT): Tx Antenna Height
        (hR): Rx Antenna Height
        (d) is the distance between the transmitter and the receiver (m)
        (L): System loss"""
       
        gT = ap.antennaGain[wlan] 
        gR = sta.antennaGain[wlan]
        hT = ap.antennaHeight[wlan]
        hR = sta.antennaHeight[wlan]
        d = int(distance)
        L = self.L
        
        try:
            sta.rssi[wlan] =  ap.txpower + 10 * math.log10(gT * gR * hT**2 * hR**2 / d**4 * L)
        except:
            sta.rssi[wlan] = 0
            
    def logDistancePropagationLossModel(self, sta, ap, distance, wlan):
        """Log Distance Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is calculated
        referenceLoss (db): The reference loss at reference distance. Default for 1m is 46.6777
        exponent: The exponent of the Path Loss propagation model, where 2 is for propagation in free space
        (d) is the distance between the transmitter and the receiver (m)"""        
        referenceDistance = 1
        referenceLoss = 46.6777
        exponent = 2
        d = int(distance)
        
        pathLossDb = 10 * exponent * math.log10(d / referenceDistance)
        rxc = - referenceLoss - pathLossDb
        sta.rssi[wlan] = ap.txpower + rxc
        
    def okumuraHataPropagationLossModel(self, sta, ap, distance, wlan):
        """Okumura Hata Propagation Loss Model:"""
        
    def jakesPropagationLossModel(self, sta, ap, distance, wlan):
        """Jakes Propagation Loss Model:"""
        