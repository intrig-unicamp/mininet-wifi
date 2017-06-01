"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

import time
import threading
import random
from pylab import math, cos, sin
from mininet.log import info
from mininet.wifiNet import mininetWiFi
from mininet.wifiPlot import plot2d, plot3d
from mininet.wifiMobility import mobility
from mininet.wifiLink import link
from mininet.wifiDevices import deviceDataRate

def instantiateGraph(mininet):
        MAX_X = mininetWiFi.MAX_X
        MAX_Y = mininetWiFi.MAX_Y
        nodes = mininet.stations + mininet.accessPoints
        for node in nodes:
            replayingMobility.addNode(node)
        plot2d.instantiateGraph(MAX_X, MAX_Y)
        plot2d.plotGraph(nodes, [], [])

class replayingMobility(object):
    """Replaying Mobility Traces"""
    
    def __init__(self, mininet):
        mininetWiFi.isMobility = True
        self.thread = threading.Thread(name='replayingMobility', target=self.mobility, args=(mininet,))
        self.thread.daemon = True
        self.thread.start()

    def mobility(self, mininet):
        if mininetWiFi.DRAW:
            instantiateGraph(mininet)
        currentTime = time.time()
        staList = mininet.stations
        stations = []
        for node in staList:
            if 'speed' in node.params:
                stations.append(node)
                node.currentTime = 1 / node.params['speed']
                node.time = float(1.0 / node.params['speed'])
        while True:
            time_ = time.time() - currentTime
            if len(stations) == 0:
                break
            for node in stations:
                while time_ >= node.currentTime and len(node.position) != 0:
                    node.moveNodeTo(node.position[0])
                    #mobility.parameters_(node)
                    del node.position[0]
                    node.currentTime += node.time
                if len(node.position) == 0:
                    stations.remove(node)
            
    @classmethod
    def addNode(self, node):
        if node.type == 'station':
            if hasattr(node, 'position'):
                node.params['position'] = node.position[0]
            mobility.stations.append(node)
        elif (node.type == 'accessPoint'):
            mobility.accessPoints.append(node)

class replayingBandwidth(object):
    """Replaying Bandwidth Traces"""

    def __init__(self, mininet, **params):
        self.thread = threading.Thread(name='replayingBandwidth', target=self.throughput, args=(mininet,))
        self.thread.daemon = True
        self.thread.start()

    def throughput(self, mininet):
        currentTime = time.time()
        stations = mininet.stations
        while True:
            if len(stations) == 0:
                break
            time_ = time.time() - currentTime
            for sta in stations:
                if hasattr(sta, 'time'):
                    if time_ >= sta.time[0]:
                        link.tc(sta, 0, sta.throughput[0], 0, 0, 0)
                        # pos = '%d, %d, %d' % (sta.throughput[0], sta.throughput[0], 0)
                        # self.moveStationTo(sta, pos)
                        del sta.throughput[0]
                        del sta.time[0]
                        #info('%s\n' % sta.time[0])
                    if len(sta.time) == 1:
                        stations.remove(sta)
            # time.sleep(0.001)
        info("\nReplaying Process Finished!")

    def moveNodeTo(self, sta, pos):
        x = pos[0]
        y = pos[1]
        sta.params['position'] = x, y, 0
        # mobility.getAPsInRange(sta)
        if mininetWiFi.DRAW:
            try:
                plot2d.graphUpdate(sta)
            except:
                pass


class replayingNetworkBehavior(object):
    """Replaying RSSI Traces"""
    
    def __init__(self, mininet, **kwargs):
        """
            Replaying Network Behavior
        """        
        self.thread = threading.Thread( name='replayingRSSI', target=self.behavior, args=(mininet,) )
        self.thread.daemon = True
        self.thread.start()

    def behavior(self, mininet):
        seconds = 5
        info('Replaying process starting in %s seconds\n' % seconds)
        time.sleep(seconds)
        info('Replaying process has been started\n')
        currentTime = time.time()
        stations = mininet.stations
        for sta in stations:
            sta.params['frequency'][0] = link.frequency(sta, 0)
        while True:
            if len(stations) == 0:
                break
            time_ = time.time() - currentTime
            for sta in stations:
                if hasattr(sta, 'time'):
                    if time_ >= sta.time[0]:
                        if sta.params['associatedTo'][0] != '':
                            bw = sta.bw[0]
                            loss = sta.loss[0]
                            delay = sta.delay[0]
                            latency = sta.latency[0]
                            link.tc(sta, 0, bw, loss, latency, delay)
                        del sta.bw[0]
                        del sta.loss[0]
                        del sta.delay[0]
                        del sta.latency[0]
                        del sta.time[0]
                    if len(sta.time) == 0:
                        stations.remove(sta)
            time.sleep(0.001)
        info('Replaying process has finished!')

    @classmethod
    def addNode(self, node):
        if node.type == 'station':
            mobility.stations.append(node)
        elif (node.type == 'accessPoint'):
            mobility.accessPoints.append(node)

class replayingRSSI(object):
    """Replaying RSSI Traces"""
    
    print_bw = False
    print_loss = False
    print_delay = False
    print_latency = False
    print_distance = False
    
    def __init__(self, mininet, propagationModel='friisPropagationLossModel', n=32, **kwargs):
        """
            propagationModel = Propagation Model
            n: Power Loss Coefficient
        """
        if 'print_bw' in kwargs:
            self.print_bw = True
        if 'print_loss' in kwargs:
            self.print_loss = True
        if 'print_delay' in kwargs:
            self.print_delay = False
        if 'print_latency' in kwargs:
            self.print_latency = True
        if 'print_distance' in kwargs:    
            self.print_distance = True
        
        mininetWiFi.isMobility = True
        self.thread = threading.Thread(name='replayingRSSI', target=self.rssi, args=(mininet, propagationModel, n))
        self.thread.daemon = True
        self.thread.start()

    def rssi(self, mininet, propagationModel='', n=0):
        #if mobility.DRAW:
        #    instantiateGraph(mininet)
        currentTime = time.time()
        staList = mininet.stations
        ang = {}
        for sta in staList:
            ang[sta] = random.uniform(0, 360)
            sta.params['frequency'][0] = link.frequency(sta, 0)
        while True:
            if len(staList) == 0:
                break
            time_ = time.time() - currentTime
            for sta in staList:
                if hasattr(sta, 'time'):
                    if time_ >= sta.time[0]:
                        ap = sta.params['associatedTo'][0]  # get AP
                        sta.params['rssi'][0] = sta.rssi[0]
                        if ap != '':
                            rssi = sta.rssi[0]
                            dist = int('%d' % self.calculateDistance(sta, ap, rssi, propagationModel, n))
                            self.moveNodeTo(sta, ap, dist, ang[sta])
                            link(sta, ap, 0, dist)
                        del sta.rssi[0]
                        del sta.time[0]
                    if len(sta.time) == 0:
                        staList.remove(sta)
            time.sleep(0.01)

    def moveNodeTo(self, sta, ap, dist, ang):

        x = float('%.2f' %  (dist * cos(ang) + int(ap.params['position'][0])))
        y = float('%.2f' %  (dist * sin(ang) + int(ap.params['position'][1])))
        sta.params['position'] = x, y, 0
        mobility.parameters_(sta)
        if mininetWiFi.DRAW:
            try:
                plot2d.graphUpdate(sta)
            except:
                pass
        # sta.verifyingNodes(sta)

    def calculateRate(self, sta, ap, dist):
        value = deviceDataRate(sta, ap, 0)
        custombw = value.rate
        rate = value.rate / 2.5
        
        if 'equipmentModel' not in ap.params.keys():
            rate = custombw * (1.1 ** -dist)
        if rate <= 0:
            rate = 1
        return rate

    def calculateDistance (self, sta, ap, rssi, propagationModel, n=32.0):

        pT = ap.params['txpower'][0]
        gT = ap.params['antennaGain'][0]
        gR = sta.params['antennaGain'][0]
        if propagationModel in dir(self):
            dist = self.__getattribute__(propagationModel)(sta, ap, pT, gT, gR, rssi, n)
            return dist

    def pathLoss(self, sta, ap, dist, wlan=0):
        """Path Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        f = sta.params['frequency'][wlan] * 10 ** 9  # Convert Ghz to Hz
        c = 299792458.0
        L = 1
        if dist == 0:
            dist = 0.1
        lambda_ = c / f  # lambda: wavelength (m)
        denominator = lambda_ ** 2
        numerator = (4 * math.pi * dist) ** 2 * L
        pathLoss_ = 10 * math.log10(numerator / denominator)

        return pathLoss_

    def friisPropagationLossModel(self, sta, ap, pT, gT, gR, signalLevel, n):
        """Based on Free Space Propagation Model"""
        c = 299792458.0
        L = 2.0
        freq = sta.params['frequency'][0] * 10 ** 9  # Convert Ghz to Hz
        gains = gR + gT + pT
        lambda_ = float(c) / float(freq)  # lambda: wavelength (m)
        numerator = 10.0 ** (abs(signalLevel - gains) / 10.0)
        dist = (lambda_ / (4.0 * math.pi)) * ((numerator / L) ** (0.5))

        return dist

    def logDistancePropagationLossModel(self, sta, ap, pT, gT, gR, signalLevel, n):
        """Based on Log Distance Propagation Loss Model"""
        gains = gR + gT + pT
        referenceDistance = 1
        exp = 2
        pathLossDb = self.pathLoss(sta, ap, referenceDistance)
        rssi = gains - signalLevel - pathLossDb
        dist = 10 ** ((rssi + 10 * exp * math.log10(referenceDistance)) / (10 * exp))

        return dist

    def ITUPropagationLossModel(self, sta, ap, pT, gT, gR, signalLevel, N):
        """Based on International Telecommunication Union (ITU) Propagation Loss Model"""
        lF = 0  # Floor penetration loss factor
        nFloors = 0  # Number of Floors
        gains = pT + gT + gR
        freq = sta.params['frequency'][0] * 10 ** 3
        dist = 10.0 ** ((-20.0 * math.log10(freq) - lF * nFloors + 28.0 + abs(signalLevel - gains)) / N)

        return dist
