"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

import time
import threading
import random
from pylab import math, cos, sin
from mininet.log import info
from mininet.wifiPlot import plot2d, plot3d
from mininet.wifiMobility import mobility
from mininet.wifiChannel import channelParams, setInfraChannelParams
from mininet.wifiDevices import deviceDataRate


def instantiateGraph(mininet):
        MAX_X = mininet.MAX_X
        MAX_Y = mininet.MAX_Y
        nodes = mininet.stations + mininet.accessPoints
        for node in nodes:
            replayingMobility.addNode(node)    
        plot2d.instantiateGraph(MAX_X, MAX_Y)
        plot2d.plotGraph(nodes, [], [], MAX_X, MAX_Y)

class replayingMobility(object):
    """Replaying Mobility Traces"""
    
    def __init__(self, mininet):
        mobility.isMobility = True
        self.thread = threading.Thread(name='replayingMobility', target=self.mobility, args=(mininet,))
        self.thread.daemon = True
        self.thread.start()

    def mobility(self, mininet):
        if mobility.DRAW:
            instantiateGraph(mininet)
        currentTime = time.time()
        staList = mininet.stations
        continue_ = True
        stations = []
        for node in staList:
            if 'speed' in node.params:
                stations.append(node)
                node.currentTime = 1 / node.params['speed']
                node.time = float(1.0 / node.params['speed'])
        while continue_:
            continue_ = False
            time_ = time.time() - currentTime
            for node in stations:
                continue_ = True
                while time_ >= node.currentTime and len(node.position) != 0:
                    node.moveStationTo(node.position[0])
                    del node.position[0]
                    node.currentTime += node.time
                if len(node.position) == 0:
                    stations.remove(node)
            
    @classmethod
    def addNode(self, node):
        if node.type == 'station':
            if hasattr(node, 'position'):
                node.params['position'] = node.position[0]
            mobility.staList.append(node)
        elif (node.type == 'accessPoint'):
            mobility.apList.append(node)

class replayingBandwidth(object):
    """Replaying Bandwidth Traces"""

    def __init__(self, **params):
        self.thread = threading.Thread(name='replayingBandwidth', target=self.throughput)
        self.thread.daemon = True
        self.thread.start()

    def throughput(self):
        # if mobility.DRAW:
        #    plotGraph()
        currentTime = time.time()
        staList = mobility.staList
        continue_ = True
        while continue_:
            continue_ = False
            time_ = time.time() - currentTime
            for sta in staList:
                if hasattr(sta, 'time'):
                    continue_ = True
                    if time_ >= sta.time[0]:
                        channelParams.tc(sta, 0, sta.throughput[0], 0, 0, 0)
                        # pos = '%d, %d, %d' % (sta.throughput[0], sta.throughput[0], 0)
                        # self.moveStationTo(sta, pos)
                        del sta.throughput[0]
                        del sta.time[0]
                        info('%s\n' % sta.time[0])
                    if len(sta.time) == 1:
                        staList.remove(sta)
            # time.sleep(0.001)
        info("\nReplaying Process Finished!")

    def moveStationTo(self, sta, pos):
        x = pos[0]
        y = pos[1]
        sta.params['position'] = x, y, 0
        # mobility.getAPsInRange(sta)
        if mobility.DRAW:
            try:
                plot2d.graphUpdate(sta)
            except:
                pass


class replayingNetworkBehavior(object):
    """Replaying RSSI Traces"""
    
    def __init__(self, **kwargs):
        """
            Replaying Network Behavior
        """        
        #mobility.isMobility = True
        self.thread = threading.Thread( name='replayingRSSI', target=self.behavior )
        self.thread.daemon = True
        self.thread.start()

    def behavior(self):
        info('\nReplaying process starting in 20 seconds')
        time.sleep(20)
        info('\nReplaying process has been started')
        currentTime = time.time()
        staList = mobility.staList
        for sta in staList:
            sta.params['frequency'][0] = channelParams.frequency(sta, 0)
        continue_ = True
        while continue_:
            continue_ = False
            time_ = time.time() - currentTime
            for sta in staList:
                if hasattr(sta, 'time'):
                    continue_ = True
                    if time_ >= sta.time[0]:
                        ap = sta.params['associatedTo'][0]  # get AP
                        if ap != '':
                            bw = sta.bw[0]
                            loss = sta.loss[0]
                            delay = sta.delay[0]
                            latency = sta.latency[0]
                            channelParams.tc(sta, 0, bw, loss, latency, delay)
                        del sta.bw[0]
                        del sta.loss[0]
                        del sta.delay[0]
                        del sta.latency[0]
                        del sta.time[0]
                    if len(sta.time) == 0:
                        staList.remove(sta)
            time.sleep(0.01)
        info('Replaying process has finished!')

    @classmethod
    def addNode(self, node):
        if node.type == 'station':
            mobility.staList.append(node)
        elif (node.type == 'accessPoint'):
            mobility.apList.append(node)

class replayingRSSI(object):
    """Replaying RSSI Traces"""
    
    print_bw = False
    print_loss = False
    print_delay = False
    print_latency = False
    print_distance = False
    
    def __init__(self, propagationModel='friisPropagationLossModel', n=32, **kwargs):
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
        
        mobility.isMobility = True
        self.thread = threading.Thread(name='replayingRSSI', target=self.rssi, args=(propagationModel, n))
        self.thread.daemon = True
        self.thread.start()

    def rssi(self, propagationModel='', n=0):
        if mobility.DRAW:
            instantiateGraph()
        currentTime = time.time()
        staList = mobility.staList
        ang = {}
        for sta in staList:
            ang[sta] = random.uniform(0, 360)
            sta.params['frequency'][0] = channelParams.frequency(sta, 0)
        continue_ = True
        while continue_:
            continue_ = False
            time_ = time.time() - currentTime
            for sta in staList:
                if hasattr(sta, 'time'):
                    continue_ = True
                    if time_ >= sta.time[0]:
                        ap = sta.params['associatedTo'][0]  # get AP
                        sta.params['rssi'][0] = sta.rssi[0]
                        if ap != '':
                            dist = self.calculateDistance(sta, ap, sta.rssi[0], propagationModel, n)
                            self.moveStationTo(sta, ap, dist, ang[sta])
                            setInfraChannelParams(sta, ap, dist, 0)
                        del sta.rssi[0]
                        del sta.time[0]
                    if len(sta.time) == 0:
                        staList.remove(sta)
            time.sleep(0.01)

    def moveStationTo(self, sta, ap, dist, ang):
        x = dist * cos(ang) + int(ap.params['position'][0])
        y = dist * sin(ang) + int(ap.params['position'][1])
        sta.params['position'] = x, y, 0
        mobility.getAPsInRange(sta)
        if mobility.DRAW:
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

    def calculateDistance (self, sta, ap, signalLevel, model=None, n=32.0):

        pT = ap.params['txpower'][0]
        gT = ap.params['antennaGain'][0]
        gR = sta.params['antennaGain'][0]

        if model in dir(self):
            dist = self.__getattribute__(model)(sta, ap, pT, gT, gR, signalLevel, n)
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
