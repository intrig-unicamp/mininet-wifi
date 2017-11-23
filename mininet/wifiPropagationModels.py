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
from random import gauss
from time import sleep
from mininet.log import error

class propagationModel(object):
    """ Propagation Models """

    rssi = -62
    model = 'logDistancePropagationLossModel'
    exp = 3  # Exponent
    sL = 1  # System Loss
    lF = 0  # Floor penetration loss factor
    pL = 0  # Power Loss Coefficient
    nFloors = 0  # Number of floors
    gRandom = 0  # Gaussian random variable
    variance = 2 # variance

    def __init__(self, node1=None, node2=None, dist=0, wlan=0):
        if self.model in dir(self):
            self.__getattribute__(self.model)(node1=node1, node2=node2,
                                              dist=dist, wlan=wlan)

    @classmethod
    def setAttr(cls, model, **kwargs):
        cls.model = model
        if 'exp' in kwargs:
            cls.exp = kwargs['exp']
        if 'sL' in kwargs:
            cls.exp = kwargs['sL']
        if 'lF' in kwargs:
            cls.exp = kwargs['lF']
        if 'pL' in kwargs:
            cls.exp = kwargs['pL']
        if 'nFloors' in kwargs:
            cls.exp = kwargs['nFloors']
        if 'variance' in kwargs:
            cls.exp = kwargs['variance']

    def pathLoss(self, node1, dist, wlan):
        """Path Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        f = node1.params['frequency'][wlan] * 10 ** 9  # Convert Ghz to Hz
        c = 299792458.0
        L = self.sL

        if dist == 0:
            dist = 0.1

        lambda_ = c / f  # lambda: wavelength (m)
        denominator = lambda_ ** 2
        numerator = (4 * math.pi * dist) ** 2 * L
        pathLoss_ = 10 * math.log10(numerator / denominator)

        return int(pathLoss_)

    def friisPropagationLossModel(self, **kwargs):
        """Friis Propagation Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        gr = kwargs['node1'].params['antennaGain'][kwargs['wlan']]
        pt = kwargs['node2'].params['txpower'][0]
        gt = kwargs['node2'].params['antennaGain'][0]
        gains = pt + gt + gr

        pathLoss = self.pathLoss(kwargs['node1'], kwargs['dist'], kwargs['wlan'])
        self.rssi = gains - pathLoss

        return self.rssi

    def twoRayGroundPropagationLossModel(self, **kwargs):
        """Two Ray Ground Propagation Loss Model (does not give a good result for
        a short distance)"""
        gr = kwargs['node1'].params['antennaGain'][kwargs['wlan']]
        hr = kwargs['node1'].params['antennaHeight'][kwargs['wlan']]
        pt = kwargs['node2'].params['txpower'][0]
        gt = kwargs['node2'].params['antennaGain'][0]
        ht = kwargs['node2'].params['antennaHeight'][0]
        gains = pt + gt + gr

        dist = kwargs['dist']
        if kwargs['dist'] == 0:
            dist = 0.1
        L = self.sL

        pathLossDb = (pt * gt * gr * ht ** 2 * hr ** 2) / (dist ** 4 * L)
        self.rssi = gains - int(pathLossDb)

        return self.rssi

    def logDistancePropagationLossModel(self, **kwargs):
        """Log Distance Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is
        calculated
        exponent: The exponent of the Path Loss propagation model, where 2
        is for propagation in free space
        (dist) is the distance between the transmitter and the receiver (m)"""
        gr = kwargs['node1'].params['antennaGain'][kwargs['wlan']]
        pt = kwargs['node2'].params['txpower'][0]
        gt = kwargs['node2'].params['antennaGain'][0]
        referenceDistance = 1
        pathLoss = self.pathLoss(kwargs['node1'], referenceDistance, kwargs['wlan'])
        gains = pt + gt + gr

        dist = kwargs['dist']
        if kwargs['dist'] == 0:
            dist = 0.1

        pathLossDb = 10 * self.exp * math.log10(dist / referenceDistance)
        self.rssi = gains - (int(pathLoss) + int(pathLossDb))

        return self.rssi

    def logNormalShadowingPropagationLossModel(self, **kwargs):
        """Log-Normal Shadowing Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is
        calculated
        exponent: The exponent of the Path Loss propagation model, where 2
        is for propagation in free space
        (d) is the distance between the transmitter and the receiver (m)
        gRandom is a Gaussian random variable"""
        gr = kwargs['node1'].params['antennaGain'][kwargs['wlan']]
        pt = kwargs['node2'].params['txpower'][0]
        gt = kwargs['node2'].params['antennaGain'][0]
        referenceDistance = 1
        gRandom = self.gRandom
        gains = pt + gt + gr
        pathLoss = self.pathLoss(kwargs['node1'], referenceDistance, kwargs['wlan'])

        dist = kwargs['dist']
        if kwargs['dist'] == 0:
            dist = 0.1

        pathLossDb = 10 * self.exp * math.log10(dist / referenceDistance) + \
                     gRandom
        self.rssi = gains - (int(pathLoss) + int(pathLossDb))

        return self.rssi

    def ITUPropagationLossModel(self, **kwargs):
        """International Telecommunication Union (ITU) Propagation Loss Model:"""
        gr = kwargs['node1'].params['antennaGain'][kwargs['wlan']]
        pt = kwargs['node2'].params['txpower'][0]
        gt = kwargs['node2'].params['antennaGain'][0]
        f = kwargs['node1'].params['frequency'][kwargs['wlan']] * 10 ** 3
        N = 28  # Power Loss Coefficient
        pL = self.pL
        lF = self.lF  # Floor penetration loss factor
        nFloors = self.nFloors  # Number of Floors
        gains = pt + gt + gr

        """Power Loss Coefficient Based on the Paper 
        Site-Specific Validation of ITU Indoor Path Loss Model at 2.4 GHz 
        from Theofilos Chrysikos, Giannis Georgopoulos and Stavros Kotsopoulos"""
        dist = kwargs['dist']
        if kwargs['dist'] == 0:
            dist = 0.1
        if dist > 16:
            N = 38
        if pL != 0:
            N = pL

        pathLossDb = 20 * math.log10(f) + N * math.log10(dist) + \
                     lF * nFloors - 28
        self.rssi = gains - int(pathLossDb)

        return self.rssi

    def youngModel(self, **kwargs):
        """Young Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is
        calculated
        exponent: The exponent of the Path Loss propagation model, where 2 is
        for propagation in free space
        (d) is the distance between the transmitter and the receiver (m)"""
        gr = kwargs['node1'].params['antennaGain'][kwargs['wlan']]
        hr = kwargs['node1'].params['antennaHeight'][kwargs['wlan']]
        gt = kwargs['node2'].params['antennaGain'][0]
        ht = kwargs['node2'].params['antennaHeight'][0]
        cf = 0.01075  # clutter factor

        dist = kwargs['dist']
        if kwargs['dist'] == 0:
            dist = 0.1

        self.rssi = int(dist ** 4 / (gt * gr) * (ht * hr) ** 2 * cf)

        return self.rssi

class distanceByPropagationModel(object):

    dist = 0
    exp = 4  # Exponent
    sL = 1  # System Loss
    lF = 0  # Floor penetration loss factor
    pL = 0  # Power Loss Coefficient
    nFloors = 0  # Number of floors
    gRandom = 0  # Gaussian random variable
    NOISE_LEVEL = 92

    def __init__(self, node=None, wlan=0, enable_interference=False):
        self.lF = propagationModel.lF
        self.sL = propagationModel.sL
        self.pL = propagationModel.pL
        self.exp = propagationModel.exp
        self.nFloors = propagationModel.nFloors
        self.gRandom = propagationModel.gRandom
        model = propagationModel.model

        if model == '':
            model = 'logDistancePropagationLossModel'
        if model in dir(self):
            self.__getattribute__(model)(node=node, wlan=wlan,
                                         interference=enable_interference)

    def friisPropagationLossModel(self, **kwargs):
        """Path Loss Model:
        (f) signal frequency transmited(Hz)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        # Convert Ghz to Hz
        f = kwargs['node'].params['frequency'][kwargs['wlan']] * 10 ** 9
        c = 299792458.0
        L = self.sL
        txpower = kwargs['node'].params['txpower'][kwargs['wlan']]
        antGain  = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        gains = txpower + (antGain * 2)

        lambda_ = c / f  # lambda: wavelength (m)
        denominator = lambda_ ** 2
        self.dist = math.pow(10, ((self.NOISE_LEVEL + gains +
                                   10 * math.log10(denominator)) /
                                  10 - math.log10((4 * math.pi) ** 2 * L)) / (2))

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

    def logDistancePropagationLossModel(self, **kwargs):
        """Log Distance Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is
        calculated
        exponent: The exponent of the Path Loss propagation model, where 2 is
        for propagation in free space
        (dist) is the distance between the transmitter and the receiver (m)"""
        referenceDistance = 1
        txpower = kwargs['node'].params['txpower'][kwargs['wlan']]
        antGain  = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        gains = txpower + (antGain * 2)

        pathLoss = self.pathLoss(kwargs['node'], referenceDistance, kwargs['wlan'])
        self.dist = math.pow(10, ((self.NOISE_LEVEL - pathLoss + gains) /
                                  (10 * self.exp))) * referenceDistance

        return self.dist
    
    def logNormalShadowingPropagationLossModel(self, **kwargs):
        """Log-Normal Shadowing Propagation Loss Model"""
        from mininet.wmediumdConnector import WmediumdGaussianRandom, \
            WmediumdServerConn

        referenceDistance = 1
        txpower = kwargs['node'].params['txpower'][kwargs['wlan']]
        antGain  = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        gains = txpower + (antGain * 2)
        mean = 0
        variance = propagationModel.variance
        gRandom = float('%.2f' % gauss(mean, variance))
        propagationModel.gRandom = gRandom

        if kwargs['interference']:
            sleep(0.002) #notice problem when there are many threads
            WmediumdServerConn.update_gaussian_random(
                WmediumdGaussianRandom(kwargs['node'].wmIface[kwargs['wlan']],
                                       gRandom))

        pathLoss = self.pathLoss(kwargs['node'], referenceDistance,
                                 kwargs['wlan']) - gRandom
        self.dist = math.pow(10, ((self.NOISE_LEVEL - pathLoss + gains) /
                                  (10 * self.exp))) * referenceDistance

        return self.dist

    def ITUPropagationLossModel(self, **kwargs):
        """International Telecommunication Union (ITU) Propagation Loss Model:"""
        f = kwargs['node'].params['frequency'][kwargs['wlan']] * 10 ** 3
        txpower = kwargs['node'].params['txpower'][kwargs['wlan']]
        antGain  = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        gains = txpower + (antGain * 2)
        N = 28  # Power Loss Coefficient
        lF = self.lF  # Floor penetration loss factor
        nFloors = self.nFloors  # Number of Floors
        self.dist = math.pow(10, ((self.NOISE_LEVEL + gains -
                                   20 * math.log10(f) - lF * nFloors + 28)/N))

        return self.dist


class powerForRangeByPropagationModel(object):

    txpower = 0
    exp = 4  # Exponent
    sL = 1  # System Loss
    lF = 0  # Floor penetration loss factor
    pL = 0  # Power Loss Coefficient
    nFloors = 0  # Number of floors
    gRandom = 0  # Gaussian random variable

    def __init__(self, node, wlan, dist, enable_interference):
        self.lF = propagationModel.lF
        self.sL = propagationModel.sL
        self.pL = propagationModel.pL
        self.exp = propagationModel.exp
        self.nFloors = propagationModel.nFloors
        self.gRandom = propagationModel.gRandom
        model = propagationModel.model

        if model == '':
            model = 'logDistancePropagationLossModel'
        if model in dir(self):
            self.__getattribute__(model)(node=node, wlan=wlan, dist=dist,
                                         interference=enable_interference)

    def friisPropagationLossModel(self, **kwargs):
        """Path Loss Model:
        distance is the range of the transmitter (m)
        (f) signal frequency transmited(Hz)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        # Convert Ghz to Hz
        f = kwargs['node'].params['frequency'][kwargs['wlan']] * 10 ** 9
        c = 299792458.0
        L = self.sL
        antGain = kwargs['node'].params['antennaGain'][kwargs['wlan']]

        lambda_ = c / f  # lambda: wavelength (m)
        denominator = lambda_ ** 2

        self.txpower = 10 * (
            math.log10((4 * math.pi) ** 2 * L * kwargs['dist'] ** 2)) \
                       - 92 - 10 * math.log10(denominator) - (antGain * 2)
        if self.txpower < 1:
            error('*** Error: tx power is negative!\n')
            exit(1)

        return self.txpower

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

    def logDistancePropagationLossModel(self, **kwargs):
        """Log Distance Propagation Loss Model:
        referenceDistance (m): The distance at which the reference loss is
        calculated
        exponent: The exponent of the Path Loss propagation model, where 2 is
        for propagation in free space
        distance is the range of the transmitter (m)"""
        referenceDistance = 1
        antGain = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        gains_fixed = (antGain * 2)

        pathLoss = self.pathLoss(kwargs['node'], referenceDistance, kwargs['wlan'])

        self.txpower = int(math.ceil(math.log10(
            (math.pow(kwargs['dist'] / referenceDistance, 10 * self.exp) *
             10 ** pathLoss) / 10 ** 92) - gains_fixed))
        if self.txpower < 1:
            error('*** Error: tx power is negative!\n')
            exit(1)

        return self.txpower

    def logNormalShadowingPropagationLossModel(self, **kwargs):
        """Log-Normal Shadowing Propagation Loss Model
        distance is the range of the transmitter (m)"""
        from mininet.wmediumdConnector import WmediumdGaussianRandom, \
            WmediumdServerConn

        referenceDistance = 1
        antGain = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        mean = 0
        variance = propagationModel.variance
        gRandom = float('%.2f' % gauss(mean, variance))
        propagationModel.gRandom = gRandom

        if kwargs['interference']:
            sleep(0.001)  # notice problem when there are many threads
            WmediumdServerConn.update_gaussian_random(WmediumdGaussianRandom(
                kwargs['node'].wmIface[kwargs['wlan']], gRandom))

        pathLoss = self.pathLoss(kwargs['node'], referenceDistance,
                                 kwargs['wlan']) - gRandom

        self.txpower = 10 * self.exp * math.log10(
            kwargs['dist'] / referenceDistance) - 92 + pathLoss - (antGain * 2)
        if self.txpower < 1:
            error('*** Error: tx power is negative!\n')
            exit(1)

        return self.txpower

    def ITUPropagationLossModel(self, **kwargs):
        """International Telecommunication Union (ITU) Propagation Loss Model:
        distance is the range of the transmitter (m)"""
        f = kwargs['node'].params['frequency'][kwargs['wlan']] * 10 ** 3
        antGain = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        N = 28  # Power Loss Coefficient
        lF = self.lF  # Floor penetration loss factor
        nFloors = self.nFloors  # Number of Floors

        self.txpower = N * math.log10(kwargs['dist']) - 92 + \
                       20 * math.log10(f) + lF * nFloors - 28 - (antGain * 2)
        if self.txpower < 1:
            error('*** Error: tx power is negative!\n')
            exit(1)

        return self.txpower
