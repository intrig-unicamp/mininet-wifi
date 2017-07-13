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

class propagationModel(object):
    """ Propagation Models """

    rssi = -62
    model = ''
    exp = 2  # Exponent
    sL = 1  # System Loss
    lF = 0  # Floor penetration loss factor
    pL = 0  # Power Loss Coefficient
    nFloors = 0  # Number of floors
    gRandom = 0  # Gaussian random variable

    def __init__(self, sta=None, ap=None, dist=0, wlan=0, pT=14, gT=5, gR=5, hT=1, hR=1):
        """pT = Tx Power
           gT = Tx Antenna Gain
           gR = Rx Antenna Gain
           hT = Tx Antenna Height
           hR = Rx Antenna Height
        """
        if self.model == '':
            self.model = 'friisPropagationLossModel'
        if self.model in dir(self):
            self.__getattribute__(self.model)(sta, dist, wlan, pT, gT, gR, hT, hR)

    def pathLoss(self, sta, dist, wlan):
        """Path Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        f = sta.params['frequency'][wlan] * 10 ** 9  # Convert Ghz to Hz
        c = 299792458.0
        L = self.sL
        if dist == 0:
            dist = 0.1

        lambda_ = c / f  # lambda: wavelength (m)
        denominator = lambda_ ** 2
        numerator = (4 * math.pi * dist) ** 2 * L
        pathLoss_ = 10 * math.log10(numerator / denominator)

        return int(pathLoss_)

    def friisPropagationLossModel(self, sta, dist, wlan, pT, gT, gR, hT, hR):
        """Friis Propagation Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        pathLoss = self.pathLoss(sta, dist, wlan)
        self.rssi = pT + gT + gR - pathLoss

        return self.rssi

    def twoRayGroundPropagationLossModel(self, sta, dist, wlan, pT, gT, gR, hT, hR):
        """Two Ray Ground Propagation Loss Model (does not give a good result for a short distance):
        (gT): Tx Antenna Gain (dBi)
        (gR): Rx Antenna Gain (dBi)
        (hT): Tx Antenna Height
        (hR): Rx Antenna Height
        (dist) is the distance between the transmitter and the receiver (m)
        (L): System loss"""
        if dist == 0:
            dist = 0.1
        L = self.sL

        pathLossDb = (pT * gT * gR * hT ** 2 * hR ** 2) / (dist ** 4 * L)
        self.rssi = pT + gT + gR - int(pathLossDb)

        return self.rssi

    def logDistancePropagationLossModel(self, sta, dist, wlan, pT, gT, gR, hT, hR):
        """Log Distance Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is calculated
        exponent: The exponent of the Path Loss propagation model, where 2 is for propagation in free space
        (dist) is the distance between the transmitter and the receiver (m)"""
        referenceDistance = 1
        pathLoss = self.pathLoss(sta, referenceDistance, wlan)
        if dist == 0:
            dist = 0.1

        pathLossDb = 10 * self.exp * math.log10(dist / referenceDistance)
        self.rssi = pT + gT + gR - (int(pathLoss) + int(pathLossDb))

        return self.rssi

    def logNormalShadowingPropagationLossModel(self, sta, dist, wlan, pT, gT, gR, hT, hR):
        """Log-Normal Shadowing Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is calculated
        exponent: The exponent of the Path Loss propagation model, where 2 is for propagation in free space
        (d) is the distance between the transmitter and the receiver (m)
        gRandom is a Gaussian random variable"""
        referenceDistance = 1
        gRandom = self.gRandom
        pathLoss = self.pathLoss(sta, referenceDistance, wlan)
        if dist == 0:
            dist = 0.1

        pathLossDb = 10 * self.exp * math.log10(dist / referenceDistance) + gRandom
        self.rssi = pT + gT + gR - (int(pathLoss) + int(pathLossDb))

        return self.rssi

    def ITUPropagationLossModel(self, sta, dist, wlan, pT, gT, gR, hT, hR):
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
        self.rssi = gains - int(pathLossDb)

        return self.rssi

    def youngModel(self, sta, dist, wlan, pT, gT, gR, hT, hR):
        """Young Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is calculated
        exponent: The exponent of the Path Loss propagation model, where 2 is for propagation in free space
        (d) is the distance between the transmitter and the receiver (m)"""
        cf = 0.01075  # clutter factor
        if dist == 0:
            dist = 0.1

        self.rssi = int(dist ** 4 / (gT * gR) * (hT * hR) ** 2 * cf)

        return self.rssi

class distanceByPropagationModel(object):

    dist = 0
    exp = 2  # Exponent
    sL = 1  # System Loss
    lF = 0  # Floor penetration loss factor
    pL = 0  # Power Loss Coefficient
    nFloors = 0  # Number of floors
    gRandom = 0  # Gaussian random variable

    def __init__(self, node=None, wlan=0):
        self.lF = propagationModel.lF
        self.sL = propagationModel.sL
        self.pL = propagationModel.pL
        self.exp = propagationModel.exp
        self.nFloors = propagationModel.nFloors
        model = propagationModel.model

        if model == '':
            model = 'friisPropagationLossModel'
        if model in dir(self):
            self.__getattribute__(model)(node, wlan)

    def friisPropagationLossModel(self, node, wlan):
        """Path Loss Model:
        (f) signal frequency transmited(Hz)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        f = node.params['frequency'][wlan] * 10 ** 9  # Convert Ghz to Hz
        c = 299792458.0
        L = self.sL

        lambda_ = c / f  # lambda: wavelength (m)
        denominator = lambda_ ** 2
        self.dist = math.pow(10, ((95 + 10 * math.log10(denominator)) / (10 - math.log10((4 * math.pi) ** 2 * L))) / (L))

        return self.dist

    def pathLoss(self, node, dist, wlan):
        """Path Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        f = node.params['frequency'][wlan] * 10 ** 9  # Convert Ghz to Hz
        c = 299792458.0
        L = self.sL

        lambda_ = c / f  # lambda: wavelength (m)
        denominator = lambda_ ** 2
        numerator = (4 * math.pi * dist) ** 2 * L
        pathLoss_ = 10 * math.log10(numerator / denominator)

        return pathLoss_

    def logDistancePropagationLossModel(self, node, wlan):
        """Log Distance Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is calculated
        exponent: The exponent of the Path Loss propagation model, where 2 is for propagation in free space
        (dist) is the distance between the transmitter and the receiver (m)"""
        referenceDistance = 1
        txpower = node.params['txpower'][wlan]
        pathLoss = self.pathLoss(node, referenceDistance, wlan)
        self.dist = math.pow(10, ((95 - pathLoss + txpower) / (10 * self.exp)) + math.log10(referenceDistance))
        
        return self.dist
    
    def logNormalShadowingPropagationLossModel(self, node, wlan):
        """Log-Normal Shadowing Propagation Loss Model"""
        #Have to check this formula
        referenceDistance = 1
        txpower = node.params['txpower'][wlan]
        pathLoss = self.pathLoss(node, referenceDistance, wlan)
        self.dist = math.pow(10, ((95 - pathLoss + txpower) / (10 * self.exp)) + math.log10(referenceDistance))

        return self.dist

    def ITUPropagationLossModel(self, sta, wlan):
        """International Telecommunication Union (ITU) Propagation Loss Model:"""
        f = sta.params['frequency'][wlan] * 10 ** 3
        N = 28  # Power Loss Coefficient
        lF = self.lF  # Floor penetration loss factor
        nFloors = self.nFloors  # Number of Floors
        self.dist = math.pow(10, ((95 - 20 * math.log10(f) - lF * nFloors + 28)/N))

        return self.dist
