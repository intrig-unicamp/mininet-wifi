"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com
        
        Implemented propagation models:
            (Indoors):
                Free-Space Propagation Model
                Log-Distance Propagation Model
                International Telecommunication Union (ITU) Propagation Model
            (Outdoors):
                Two-Ray-Ground Propagation Model
"""

import math

class propagationModel_ (object):
    """ Propagation Models """

    rssi = -62
    model = ''
    exp = 2  # Exponent
    sl = 1  # System Loss
    lF = 0  # Floor penetration loss factor
    pL = 0  # Power Loss Coefficient
    nFloors = 0  # Number of floors
    gRandom = 0  # Gaussian random variable

    def __init__(self, sta=None, ap=None, dist=0, wlan=None, pT=14, gT=5, gR=5, hT=1, hR=1, sl=1,
                  lF=0, pL=0, nFloors=0, gRandom=0):
        """pT = Tx Power
           gT = Tx Antenna Gain
           gR = Rx Antenna Gain
           hT = Tx Antenna Height
           hR = Rx Antenna Height
        """
        self.lF = lF
        self.sl = sl
        self.pL = pL
        self.nFloors = nFloors
        if self.model in dir(self):
            self.__getattribute__(self.model)(sta, ap, dist, wlan, pT, gT, gR, hT, hR)

    def pathLoss(self, sta, ap, dist, wlan):
        """Path Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        f = sta.params['frequency'][wlan] * 10 ** 9  # Convert Ghz to Hz
        c = 299792458.0
        L = self.sl
        if dist == 0:
            dist = 0.1
        
        lambda_ = c / f  # lambda: wavelength (m)
        denominator = lambda_ ** 2
        numerator = (4 * math.pi * dist) ** 2 * L
        pathLoss_ = 10 * math.log10(numerator / denominator)
        
        return pathLoss_

    def friisPropagationLossModel(self, sta, ap, dist, wlan, pT, gT, gR, hT, hR):
        """Friis Propagation Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        pathLoss = self.pathLoss(sta, ap, dist, wlan)
        self.rssi = pT + gT + gR - pathLoss
        
        return self.rssi

    def twoRayGroundPropagationLossModel(self, sta, ap, dist, wlan, pT, gT, gR, hT, hR):
        """Two Ray Ground Propagation Loss Model (does not give a good result for a short distance):
        (gT): Tx Antenna Gain (dBi)
        (gR): Rx Antenna Gain (dBi)
        (hT): Tx Antenna Height
        (hR): Rx Antenna Height
        (d) is the distance between the transmitter and the receiver (m)
        (L): System loss"""
        if dist == 0:
            dist = 0.1
        L = self.sl

        pathLossDb = (pT * gT * gR * hT ** 2 * hR ** 2) / (dist ** 4 * L)
        self.rssi = pT + gT + gR - pathLossDb

        return self.rssi

    def logDistancePropagationLossModel(self, sta, ap, dist, wlan, pT, gT, gR, hT, hR):
        """Log Distance Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is calculated
        exponent: The exponent of the Path Loss propagation model, where 2 is for propagation in free space
        (d) is the distance between the transmitter and the receiver (m)"""
        referenceDistance = 1
        pathLoss = self.pathLoss(sta, ap, referenceDistance, wlan)
        if dist == 0:
            dist = 0.1

        pathLossDb = 10 * self.exp * math.log10(dist / referenceDistance)
        self.rssi = pT + gT + gR - (pathLoss + pathLossDb)

        return self.rssi

    def logNormalShadowingPropagationLossModel(self, sta, ap, dist, wlan, pT, gT, gR, hT, hR):
        """Log-Normal Shadowing Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is calculated
        exponent: The exponent of the Path Loss propagation model, where 2 is for propagation in free space
        (d) is the distance between the transmitter and the receiver (m)
        gRandom is a Gaussian random variable"""
        referenceDistance = 1
        gRandom = self. gRandom
        pathLoss = self.pathLoss(sta, ap, referenceDistance, wlan)
        if dist == 0:
            dist = 0.1

        pathLossDb = 10 * self.exp * math.log10(dist / referenceDistance) + gRandom
        self.rssi = pT + gT + gR - (pathLoss + pathLossDb)

        return self.rssi

    def ITUPropagationLossModel(self, sta, ap, dist, wlan, pT, gT, gR, hT, hR):
        """International Telecommunication Union (ITU) Propagation Loss Model:"""
        f = sta.params['frequency'][wlan] * 10 ** 3
        N = 28  # Power Loss Coefficient
        pL = self.pL
        lF = self.lF  # Floor penetration loss factor
        nFloors = self.nFloors  # Number of Floors
        gains = pT + gT + gR
        """Power Loss Coefficient Based on the Paper 
        Site-Specific Validation of ITU Indoor Path Loss Model at 2.4 GHz 
        from Theofilos Chrysikos, Giannis Georgopoulos and Stavros Kotsopoulos"""
        if dist > 16:
            N = 38
        if dist == 0:
            dist = 0.1
        if pL != 0:
            N = pL
            
        pathLossDb = 20 * math.log10(f) + N * math.log10(dist) + lF * nFloors - 28
        self.rssi = gains - pathLossDb
        
        return self.rssi

    def youngModel(self, sta, ap, dist, wlan, pT, gT, gR, hT, hR):
        """Young Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is calculated
        exponent: The exponent of the Path Loss propagation model, where 2 is for propagation in free space
        (d) is the distance between the transmitter and the receiver (m)"""
        cf = 0.01075  # clutter factor
        if dist == 0:
            dist = 0.1

        self.rssi = dist ** 4 / (gT * gR) * (hT * hR) ** 2 * cf

        return self.rssi

    def okumuraHataPropagationLossModel(self, node1, node2, distance, wlan):
        """Okumura Hata Propagation Loss Model:"""

    def jakesPropagationLossModel(self, node1, node2, distance, wlan):
        """Jakes Propagation Loss Model:"""

    def attenuation(self, node1, node2, dist, wlan):
        """have to work on"""
        # alpha = 1
        # gT = node2.antennaGain[0]
        # gR = node1.antennaGain[wlan]

        # L = -27.56 + 10 * alpha * math.log10(dist) + 20 * math.log(node1.params['frequency'][wlan])
        # P = node2.params['txpower'][0] * gR * gT * L
