"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

from mininet.wifiDevices import deviceDataRate
from wifiPropagationModels import propagationModel
from scipy.spatial.distance import pdist
import numpy as np
import random
import os


class setChannelParams (object):

    dist = 0
    noise = 0
    sl = 1  # System Loss
    lF = 0  # Floor penetration loss factor
    pL = 0  # Power Loss Coefficient
    nFloors = 0  # Number of floors
    gRandom = 0  # Gaussian random variable
    equationLoss = '(dist * 2) / 1000'
    equationDelay = '(dist / 10) + 1'
    equationLatency = '2 + dist'
    equationBw = 'custombw * (1.1 ** -dist)'
    ifb = False

    def __init__(self, sta=None, ap=None, wlan=0, dist=0):
        """"
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        :param dist: distance between source and destination
        """

        # self.calculateInterference(node1, node2, dist, staList, wlan)
        delay_ = self.setDelay(dist)
        latency_ = self.setLatency(dist)
        loss_ = self.setLoss(dist)
        bw_ = self.setBW(sta=sta, ap=ap, dist=dist, wlan=wlan)
        # sta.params['rssi'][wlan] = self.setRSSI(sta, ap, wlan, dist)
        # sta.params['snr'][wlan] = self.setSNR(sta, wlan)
        self.tc(sta, wlan, bw_, loss_, latency_, delay_)

    @classmethod
    def getDistance(self, src, dst):
        """ Get the distance between two nodes 
        
        :param src: source node
        :param dst: destination node
        """
        pos_src = src.params['position']
        pos_dst = dst.params['position']
        points = np.array([(pos_src[0], pos_src[1], pos_src[2]), (pos_dst[0], pos_dst[1], pos_dst[2])])
        return float(pdist(points))

    @classmethod
    def setDelay(self, dist):
        """"Based on RandomPropagationDelayModel
        
        :param dist: distance between source and destination
        """
        return eval(self.equationDelay)

    @classmethod
    def setLatency(self, dist):
        """
        :param dist: distance between source and destination
        """
        return eval(self.equationLatency)

    @classmethod
    def setLoss(self, dist):
        """
        :param dist: distance between source and destination
        """
        return eval(self.equationLoss)

    @classmethod
    def setBW(self, sta=None, ap=None, wlan=0, dist=0):
        """set BW

        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        :param dist: distance between source and destination
        """
        value = deviceDataRate(sta, ap, wlan)
        custombw = value.rate
        rate = eval(self.equationBw)

        if rate <= 0.0:
            rate = 0.1
        return rate

    @classmethod
    def setRSSI(self, sta=None, ap=None, wlan=0, dist=0):
        """set RSSI
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        :param dist: distance
        """
        lF = self.lF
        sl = self.sl
        nFloors = self.nFloors
        gRandom = self.gRandom
        pL = self.pL
        gT = 0
        hT = 0
        pT = sta.params['txpower'][wlan]
        gR = sta.params['antennaGain'][wlan]
        hR = sta.params['antennaHeight'][wlan]

        value = propagationModel(sta, ap, dist, wlan, pT, gT, gR, hT, hR, sl, lF, pL, nFloors, gRandom)
        return float(value.rssi)  # random.uniform(value.rssi-1, value.rssi+1)

    @classmethod
    def setSNR(self, sta, wlan):
        """set SNR
        
        :param sta: station
        :param wlan: wlan ID
        """
        return float('%.2f' % (sta.params['rssi'][wlan] - (-91.0)))

    @classmethod
    def recordParams(self, sta, ap):
        """ Records node params to a file
        
        :param sta: station
        :param ap: access point
        """
        if sta != None and 'recordNodeParams' in sta.params:
            os.system('echo \'%s\' > %s.nodeParams' % (sta.params, sta.name))
        if ap != None and 'recordNodeParams' in ap.params:
            os.system('echo \'%s\' > %s.nodeParams' % (ap.params, ap.name))

    @classmethod
    def tc(self, sta, wlan, bw, loss, latency, delay):
        """Applies TC
        
        :param sta: station
        :param wlan: wlan ID
        :param bw: bandwidth (mbps)
        :param loss: loss (%)
        :param latency: latency (ms)
        :param delay: delay (ms)
        """

        if self.ifb:
            sta.pexec("tc qdisc replace dev ifb%s \
                root handle 2: netem rate %.2fmbit \
                loss %.1f%% \
                latency %.2fms \
                delay %.2fms " % (sta.ifb[wlan], bw, loss, latency, delay))
        if 'encrypt' in sta.params:
            """tbf is applied to encrypt, cause we have noticed troubles with wpa_supplicant and netem"""
            tc = 'tc qdisc replace dev %s root handle 1: tbf '\
                 'rate %.2fmbit '\
                 'burst 15000b '\
                 'lat %.2fms' % (sta.params['wlan'][wlan], bw, latency)
            sta.pexec(tc)
        else:
            tc = "tc qdisc replace dev %s " \
                 "root handle 2: netem " \
                 "rate %.2fmbit " \
                 "loss %.1f%% " \
                 "latency %.2fms " \
                 "delay %.2fms" % (sta.params['wlan'][wlan], bw, loss, latency, delay)
            sta.pexec(tc)
                # corrupt 0.1%%" % (sta.params['wlan'][wlan], bw, loss, latency, delay))

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
            value = propagationModel(sta, station, dist, wlan)
            n = value.rssi + signalPower
            self.noise += n
            self.i += 1
            self.dist += dist

    def signalToNoiseRatio(self, signalPower, noisePower):
        """Calculating SNR margin"""
        snr = signalPower - noisePower
        return snr

    @classmethod
    def frequency(self, node, wlan):
        """Gets frequency based on channel number
        
        :param node: node
        :param wlan: wlan ID
        """

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
        elif node.params['channel'][wlan] == 36:
            freq = 5.18
        elif node.params['channel'][wlan] == 40:
            freq = 5.2
        elif node.params['channel'][wlan] == 44:
            freq = 5.22
        elif node.params['channel'][wlan] == 48:
            freq = 5.24
        return freq
