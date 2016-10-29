"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

from wifiDevices import deviceDataRate
from wifiPropagationModels import propagationModel_
from scipy.spatial.distance import pdist
import numpy as np
import random

class channelParameters (object):
    """Channel Parameters"""

    delay_ = 0
    loss_ = 0
    bw_ = 0
    latency_ = 0
    rate = 0
    dist = 0
    noise = 0
    i = 0
    sl = 1  # System Loss
    lF = 0  # Floor penetration loss factor
    pL = 0  # Power Loss Coefficient
    nFloors = 0  # Number of floors
    gRandom = 0  # Gaussian random variable
    equationLoss = '(dist * 2) / 100'
    equationDelay = '(dist / 10) + 1'
    equationLatency = '2 + dist'
    equationBw = 'custombw * (1.1 ** -dist)'

    def __init__(self, node1, node2, wlan, dist, staList, time):
        self.dist = dist
        # self.calculateInterference(node1, node2, dist, staList, wlan)
        self.delay_ = self.delay(self.dist, time)
        self.latency_ = self.latency(self.dist)
        self.loss_ = self.loss(self.dist)
        self.bw_ = self.bw(node1, node2, self.dist, wlan)
        self.tc(node1, wlan, self.bw_, self.loss_, self.latency_, self.delay_)

    @classmethod
    def getDistance(self, src, dst):
        """ Get the distance between two nodes """
        pos_src = src.params['position']
        pos_dst = dst.params['position']
        points = np.array([(pos_src[0], pos_src[1], pos_src[2]), (pos_dst[0], pos_dst[1], pos_dst[2])])
        dist = pdist(points)
        return float(dist)

    @classmethod
    def delay(self, dist, time):
        """"Based on RandomPropagationDelayModel"""
        self.delay_ = eval(self.equationDelay)
        return self.delay_

    @classmethod
    def latency(self, dist):
        self.latency_ = eval(self.equationLatency)
        return self.latency_

    @classmethod
    def loss(self, dist):
        self.loss_ = eval(self.equationLoss)
        return self.loss_

    @classmethod
    def bw(self, sta, ap, dist, wlan, isReplay = False):
        if propagationModel_.model == '':
            propagationModel_.model = 'friisPropagationLossModel'
        
        lF = self.lF
        sl = self.sl
        nFloors = self.nFloors
        gRandom = self.gRandom
        pL = self.pL
        if ap == None:
            gT = 0
            hT = 0
            pT = sta.params['txpower'][wlan]
            gR = sta.params['antennaGain'][wlan]
            hR = sta.params['antennaHeight'][wlan]
            if self.i != 0:
                dist = self.dist / self.i
            value = propagationModel_(sta, ap, dist, wlan, pT, gT, gR, hT, hR, sl, lF, pL, nFloors, gRandom)
            sta.params['rssi'][wlan] = value.rssi
        else:
            pT = ap.params['txpower'][0]
            gT = ap.params['antennaGain'][0]
            hT = ap.params['antennaHeight'][0]
            gR = sta.params['antennaGain'][wlan]
            hR = sta.params['antennaHeight'][wlan]
            if isReplay == False:
                value = propagationModel_(sta, ap, dist, wlan, pT, gT, gR, hT, hR, sl, lF, pL, nFloors, gRandom)
                sta.params['rssi'][wlan] = value.rssi  # random.uniform(value.rssi-1, value.rssi+1)
        value = deviceDataRate(sta, ap, wlan)
        custombw = value.rate
        self.rate = eval(self.equationBw)
        if self.rate <= 0:
            self.rate = 0.1
        return self.rate

    @classmethod
    def tc(self, sta, wlan, bw, loss, latency, delay):
        """Applying TC"""
        sta.pexec("tc qdisc replace dev %s \
            root handle 2: netem rate %.2fmbps \
            loss %.1f%% \
            latency %.2fms \
            delay %.2fms "  % (sta.params['wlan'][wlan], bw, loss, latency, delay))
            #corrupt 0.1%%" % (sta.params['wlan'][wlan], bw, loss, latency, delay))

    def calculateInterference (self, sta, ap, dist, staList, wlan):
        """Calculating Interference"""
        self.noise = 0
        noisePower = 0
        self.i = 0
        signalPower = sta.params['rssi'][wlan]

        if ap == None:
            for station in staList:
                if station != sta and sta.params['associatedTo'][wlan] != '':
                    self.calculateNoise(sta, station, signalPower, wlan)
        else:
            for station in ap.params['associatedStations']:
                if station != sta and sta.params['associatedTo'][wlan] != '':
                    self.calculateNoise(sta, station, signalPower, wlan)
        if self.noise != 0:
            noisePower = self.noise / self.i
            self.dist = self.dist + dist
            signalPower = sta.params['rssi'][wlan]
            snr = self.signalToNoiseRatio(signalPower, noisePower)
            sta.params['snr'][wlan] = random.uniform(snr - 1, snr + 1)
        else:
            sta.params['snr'][wlan] = 0

    def calculateNoise(self, sta, station, signalPower, wlan):
        dist = self.getDistance(sta, station)
        totalRange = sta.range + station.range
        if dist < totalRange:
            value = propagationModel_(sta, station, dist, wlan)
            n = value.rssi + signalPower
            self.noise += n
            self.i += 1
            self.dist += dist

    @classmethod
    def frequency(self, node, wlan):
        freq = 0
        if node.params['channel'][wlan] == 1:
            freq = 2.412
        elif node.params['channel'][wlan] == 2:
            freq = 2.417
        elif node.params['channel'][wlan] == 3:
            freq = 2.422
        elif node.params['channel'][wlan] == 4:
            freq = 2.427
        elif node.params['channel'][wlan] == 5:
            freq = 2.432
        elif node.params['channel'][wlan] == 6:
            freq = 2.437
        elif node.params['channel'][wlan] == 7:
            freq = 2.442
        elif node.params['channel'][wlan] == 8:
            freq = 2.447
        elif node.params['channel'][wlan] == 9:
            freq = 2.452
        elif node.params['channel'][wlan] == 10:
            freq = 2.457
        elif node.params['channel'][wlan] == 11:
            freq = 2.462
        return freq

    def signalToNoiseRatio(self, signalPower, noisePower):
        """Calculating SNR margin"""
        snr = signalPower - noisePower
        return snr

    def maxChannelNoise(self, node1, node2, wlan, modelValue):
        """Have to work"""
        # snr = 25 #Depends of the equipment
        # max_channel_noise = self.rssi[wlan] - snr

    def linkMargin(self, node1, node2, wlan, modelValue):
        """Have to work"""
        # receive_sensitivity = -72 #Depends of the equipment
        # link_margin = self.rssi[wlan] - receive_sensitivity
