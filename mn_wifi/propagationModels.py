"""author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        
        Implemented propagation models:
            (Indoors):
                Free-Space Propagation Model
                Log-Distance Propagation Model
                International Telecommunication Union (ITU) Propagation Model
            (Outdoors):
                Two-Ray-Ground Propagation Model"""

import math
from random import gauss
from time import sleep


class propagationModel(object):
    "Propagation Models"

    rssi = -62
    model = ''
    exp = 3  # Exponent
    sL = 1  # System Loss
    lF = 0  # Floor penetration loss factor
    pL = 0  # Power Loss Coefficient
    nFloors = 0  # Number of floors
    gRandom = 0  # Gaussian random variable
    variance = 2 # variance
    noise_threshold = -91
    cca_threshold = -90

    def __init__(self, node1=None, node2=None, dist=0, wlan=0):
        if self.model in dir(self):
            self.__getattribute__(self.model)(node1=node1, node2=node2,
                                              dist=dist, wlan=wlan)

    @classmethod
    def setAttr(cls, **kwargs):
        cls.model = 'logDistance'
        if 'model' in kwargs:
            cls.model = kwargs['model']
        if 'exp' in kwargs:
            cls.exp = kwargs['exp']
        if 'sL' in kwargs:
            cls.sL = kwargs['sL']
        if 'lF' in kwargs:
            cls.lF = kwargs['lF']
        if 'pL' in kwargs:
            cls.pL = kwargs['pL']
        if 'nFloors' in kwargs:
            cls.nFloors = kwargs['nFloors']
        if 'variance' in kwargs:
            cls.variance = kwargs['variance']
        if 'noise_threshold' in kwargs:
            cls.noise_threshold = kwargs['noise_threshold']
        if 'cca_threshold' in kwargs:
            cls.cca_threshold = kwargs['cca_threshold']

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
        pl = 10 * math.log10(numerator / denominator)

        return int(pl)

    def friis(self, **kwargs):
        """Friis Propagation Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        gr = kwargs['node1'].params['antennaGain'][kwargs['wlan']]
        pt = kwargs['node2'].params['txpower'][0]
        gt = kwargs['node2'].params['antennaGain'][0]
        d = kwargs['dist']
        gains = pt + gt + gr

        pl = self.pathLoss(kwargs['node1'], d, kwargs['wlan'])
        self.rssi = gains - pl

        return self.rssi

    def twoRayGround(self, **kwargs):
        """Two Ray Ground Propagation Loss Model (does not give a good result for
        a short distance)"""
        gr = kwargs['node1'].params['antennaGain'][kwargs['wlan']]
        hr = kwargs['node1'].params['antennaHeight'][kwargs['wlan']]
        pt = kwargs['node2'].params['txpower'][0]
        gt = kwargs['node2'].params['antennaGain'][0]
        ht = kwargs['node2'].params['antennaHeight'][0]
        gains = pt + gt + gr

        d = kwargs['dist']
        if kwargs['dist'] == 0:
            d = 0.1
        L = self.sL

        pldb = (pt * gt * gr * ht ** 2 * hr ** 2) / (d ** 4 * L)
        self.rssi = gains - int(pldb)

        return self.rssi

    def logDistance(self, **kwargs):
        """Log Distance Propagation Loss Model:
        ref_d (m): The distance at which the reference loss is
        calculated
        exponent: The exponent of the Path Loss propagation model, where 2
        is for propagation in free space
        (dist) is the distance between the transmitter and the receiver (m)"""
        gr = kwargs['node1'].params['antennaGain'][kwargs['wlan']]
        pt = kwargs['node2'].params['txpower'][0]
        gt = kwargs['node2'].params['antennaGain'][0]
        gains = pt + gt + gr
        ref_d = 1

        pl = self.pathLoss(kwargs['node1'], ref_d, kwargs['wlan'])

        d = kwargs['dist']
        if kwargs['dist'] == 0:
            d = 0.1

        pldb = 10 * self.exp * math.log10(d / ref_d)
        self.rssi = gains - (int(pl) + int(pldb))

        return self.rssi

    def logNormalShadowing(self, **kwargs):
        """Log-Normal Shadowing Propagation Loss Model:
        ref_d (m): The distance at which the reference loss is
        calculated
        exponent: The exponent of the Path Loss propagation model, where 2
        is for propagation in free space
        (d) is the distance between the transmitter and the receiver (m)
        gRandom is a Gaussian random variable"""
        gr = kwargs['node1'].params['antennaGain'][kwargs['wlan']]
        pt = kwargs['node2'].params['txpower'][0]
        gt = kwargs['node2'].params['antennaGain'][0]
        gRandom = self.gRandom
        gains = pt + gt + gr
        ref_d = 1

        pl = self.pathLoss(kwargs['node1'], ref_d, kwargs['wlan'])

        dist = kwargs['dist']
        if kwargs['dist'] == 0:
            dist = 0.1

        pldb = 10 * self.exp * math.log10(dist / ref_d) + gRandom
        self.rssi = gains - (int(pl) + int(pldb))

        return self.rssi

    def ITU(self, **kwargs):
        """International Telecommunication Union (ITU) Propagation Loss Model:"""
        gr = kwargs['node1'].params['antennaGain'][kwargs['wlan']]
        pt = kwargs['node2'].params['txpower'][0]
        gt = kwargs['node2'].params['antennaGain'][0]
        f = kwargs['node1'].params['frequency'][kwargs['wlan']] * 10 ** 3
        nFloors = self.nFloors  # Number of Floors
        gains = pt + gt + gr
        pL = self.pL
        lF = self.lF  # Floor penetration loss factor
        N = 28  # Power Loss Coefficient

        """Power Loss Coefficient Based on the Paper 
        Site-Specific Validation of ITU Indoor Path Loss Model at 2.4 GHz 
        from Theofilos Chrysikos, Giannis Georgopoulos and Stavros Kotsopoulos"""
        d = kwargs['dist']
        if kwargs['dist'] == 0:
            d = 0.1
        if d > 16:
            N = 38
        if pL != 0:
            N = pL

        pldb = 20 * math.log10(f) + N * math.log10(d) + lF * nFloors - 28
        self.rssi = gains - int(pldb)

        return self.rssi

    def young(self, **kwargs):
        "Young Propagation Loss Model"
        gr = kwargs['node1'].params['antennaGain'][kwargs['wlan']]
        hr = kwargs['node1'].params['antennaHeight'][kwargs['wlan']]
        gt = kwargs['node2'].params['antennaGain'][0]
        ht = kwargs['node2'].params['antennaHeight'][0]
        cf = 0.01075  # clutter factor

        d = kwargs['dist']
        if kwargs['dist'] == 0:
            d = 0.1

        self.rssi = int(d ** 4 / (gt * gr) * (ht * hr) ** 2 * cf)

        return self.rssi


ppm = propagationModel


class GetSignalRange(object):

    dist = 0

    def __init__(self, node=None, wlan=0, enable_interference=False):
        "Calculate the signal range given the propagation model"
        if ppm.model in dir(self):
            self.__getattribute__(ppm.model)(node=node, wlan=wlan,
                                             interference=enable_interference)

    def friis(self, **kwargs):
        """Path Loss Model:
        (f) signal frequency transmited(Hz)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        # Convert Ghz to Hz
        f = kwargs['node'].params['frequency'][kwargs['wlan']] * 10 ** 9
        txpower = kwargs['node'].params['txpower'][kwargs['wlan']]
        gain  = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        gains = txpower + (gain * 2)
        c = 299792458.0
        L = ppm.sL

        lambda_ = c / f  # lambda: wavelength (m)
        denominator = lambda_ ** 2
        self.dist = math.pow(10, ((-ppm.noise_threshold + gains +
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
        L = ppm.sL
        lambda_ = c / f  # lambda: wavelength (m)
        denominator = lambda_ ** 2
        numerator = (4 * math.pi * dist) ** 2 * L
        pl = 10 * math.log10(numerator / denominator)

        return pl

    def logDistance(self, **kwargs):
        """Log Distance Propagation Loss Model:
        ref_d (m): The distance at which the reference loss is
        calculated
        exponent: The exponent of the Path Loss propagation model, where 2 is
        for propagation in free space
        (dist) is the distance between the transmitter and the receiver (m)"""
        txpower = kwargs['node'].params['txpower'][kwargs['wlan']]
        gain = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        gains = txpower + (gain * 2)
        ref_d = 1

        pl = self.pathLoss(kwargs['node'], ref_d, kwargs['wlan'])
        self.dist = math.pow(10, ((-ppm.noise_threshold - pl + gains) /
                                  (10 * ppm.exp))) * ref_d

        return self.dist
    
    def logNormalShadowing(self, **kwargs):
        """Log-Normal Shadowing Propagation Loss Model"""
        from mininet.wifi.wmediumdConnector import WmediumdGRandom, \
            WmediumdServer

        ref_d = 1
        txpower = kwargs['node'].params['txpower'][kwargs['wlan']]
        gain = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        gains = txpower + (gain * 2)
        mean = 0
        variance = propagationModel.variance
        gRandom = float('%.2f' % gauss(mean, variance))
        propagationModel.gRandom = gRandom

        if kwargs['interference']:
            sleep(0.002) #notice problem when there are multiple threads
            WmediumdServer.update_gaussian_random(
                WmediumdGRandom(kwargs['node'].wmIface[kwargs['wlan']],
                                gRandom))

        pl = self.pathLoss(kwargs['node'], ref_d, kwargs['wlan']) - gRandom
        numerator = -ppm.noise_threshold - pl + gains
        denominator = 10 * ppm.exp

        self.dist = math.pow(10, (numerator / denominator)) * ref_d

        return self.dist

    def ITU(self, **kwargs):
        """International Telecommunication Union (ITU) Propagation Loss Model:"""
        f = kwargs['node'].params['frequency'][kwargs['wlan']] * 10 ** 3
        txpower = kwargs['node'].params['txpower'][kwargs['wlan']]
        gain  = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        gains = txpower + (gain * 2)
        N = 28  # Power Loss Coefficient
        lF = ppm.lF  # Floor penetration loss factor
        nFloors = ppm.nFloors  # Number of Floors

        self.dist = math.pow(10, ((-ppm.noise_threshold + gains -
                                   20 * math.log10(f) - lF * nFloors + 28)/N))

        return self.dist


class GetPowerGivenRange(object):
    "Get tx power when the signal range is set"
    txpower = 0

    def __init__(self, node, wlan, dist, enable_interference):
        "Calculate txpower given the signal range"
        if ppm.model in dir(self):
            self.__getattribute__(ppm.model)(node=node, wlan=wlan, dist=dist,
                                             interference=enable_interference)

    def friis(self, **kwargs):
        """Path Loss Model:
        distance is the range of the transmitter (m)
        (f) signal frequency transmited(Hz)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        # Convert Ghz to Hz
        f = kwargs['node'].params['frequency'][kwargs['wlan']] * 10 ** 9
        gain = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        c = 299792458.0
        d = kwargs['dist']
        lambda_ = c / f  # lambda: wavelength (m)
        denominator = lambda_ ** 2
        L = ppm.sL

        self.txpower = 10 * (
            math.log10((4 * math.pi) ** 2 * L * d ** 2)) + ppm.noise_threshold \
                       - 10 * math.log10(denominator) - (gain * 2)
        if self.txpower < 0:
            self.txpower = 1

        return self.txpower

    def pathLoss(self, node, dist, wlan):
        """Path Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        f = node.params['frequency'][wlan] * 10 ** 9  # Convert Ghz to Hz
        c = 299792458.0
        L = ppm.sL
        lambda_ = c / f  # lambda: wavelength (m)
        denominator = lambda_ ** 2
        numerator = (4 * math.pi * dist) ** 2 * L
        pl = 10 * math.log10(numerator / denominator)

        return pl

    def logDistance(self, **kwargs):
        """Log Distance Propagation Loss Model:
        ref_d (m): The distance at which the reference loss is
        calculated
        exponent: The exponent of the Path Loss propagation model, where 2 is
        for propagation in free space
        distance is the range of the transmitter (m)"""
        gain = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        g_fixed = (gain * 2)
        d = kwargs['dist']
        ref_d = 1
        pl = self.pathLoss(kwargs['node'], ref_d, kwargs['wlan'])
        numerator = math.pow(d / ref_d, 10 * ppm.exp) * 10 ** pl
        denominator = 10 ** - ppm.noise_threshold

        self.txpower = int(math.ceil(math.log10(numerator / denominator) - g_fixed))
        if self.txpower < 0:
            self.txpower = 1

        return self.txpower

    def logNormalShadowing(self, **kwargs):
        """Log-Normal Shadowing Propagation Loss Model
        distance is the range of the transmitter (m)"""
        from mininet.wifi.wmediumdConnector import WmediumdGRandom, \
            WmediumdServer

        mean = 0
        d = kwargs['dist']
        ref_d = 1
        gain = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        variance = propagationModel.variance
        gRandom = float('%.2f' % gauss(mean, variance))
        propagationModel.gRandom = gRandom

        if kwargs['interference']:
            sleep(0.001)  # notice problem when there are many threads
            WmediumdServer.update_gaussian_random(WmediumdGRandom(
                kwargs['node'].wmIface[kwargs['wlan']], gRandom))

        pl = self.pathLoss(kwargs['node'], ref_d, kwargs['wlan']) - gRandom

        self.txpower = 10 * ppm.exp * math.log10(d / ref_d) + \
                       ppm.noise_threshold + pl - (gain * 2)
        if self.txpower < 0:
            self.txpower = 1

        return self.txpower

    def ITU(self, **kwargs):
        """International Telecommunication Union (ITU) Propagation Loss Model:
        distance is the range of the transmitter (m)"""
        f = kwargs['node'].params['frequency'][kwargs['wlan']] * 10 ** 3
        gain = kwargs['node'].params['antennaGain'][kwargs['wlan']]
        d = kwargs['dist']
        lF = ppm.lF  # Floor penetration loss factor
        nFloors = ppm.nFloors  # Number of Floors
        N = 28  # Power Loss Coefficient

        self.txpower = N * math.log10(d) + ppm.noise_threshold + \
                       20 * math.log10(f) + lF * nFloors - 28 - (gain * 2)
        if self.txpower < 0:
            self.txpower = 1

        return self.txpower
