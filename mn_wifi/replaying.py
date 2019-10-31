"""
    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

"""

from time import time, sleep
from threading import Thread as thread
import random
from pylab import math, cos, sin
from mininet.log import info
from mn_wifi.plot import plot2d, plot3d
from mn_wifi.mobility import mobility
from mn_wifi.link import wirelessLink
from mn_wifi.node import Station, AP


class replayingMobility(object):
    'Replaying Mobility Traces'
    timestamp = False

    def __init__(self, Mininet_wifi, nodes=None):
        mobility.thread_ = thread(name='replayingMobility',
                                  target=self.mobility,
                                  args=(nodes,Mininet_wifi,))
        mobility.thread_.daemon = True
        mobility.thread_._keep_alive = True
        mobility.thread_.start()

    def timestamp_(self, node, time_):
        if time_ >= float(node.time[0]):
            pos = node.position[0]
            del node.position[0]
            del node.time[0]
            mobility.set_pos(node, pos)

    def notimestamp_(self, node, time_):
        if time_ >= node.currentTime:
            for n in range(0, node.params['speed']):
                if len(node.position) > 0:
                    pos = node.position[0]
                    del node.position[0]
                    node.currentTime += node.timestamp
                    mobility.set_pos(node, pos)

    def mobility(self, nodes, Mininet_wifi):
        if nodes is None:
            nodes = Mininet_wifi.stations + Mininet_wifi.aps
        for node in nodes:
            if isinstance(node, Station):
                if 'position' in node.params and node not in mobility.stations:
                    mobility.stations.append(node)
            if isinstance(node, AP):
                if 'position' in node.params and node not in mobility.aps:
                    mobility.aps.append(node)

        if Mininet_wifi.draw:
            Mininet_wifi.isReplaying=False
            Mininet_wifi.checkDimension(nodes)
            plot = plot2d
            if Mininet_wifi.max_z != 0:
                plot = plot3d

        currentTime = time()
        for node in nodes:
            if 'speed' not in node.params:
                node.params['speed'] = 1.0
            node.currentTime = 1 / node.params['speed']
            node.timestamp = float(1.0 / node.params['speed'])
            if hasattr(node, 'time'):
                self.timestamp = True

        calc_pos = self.notimestamp_
        if self.timestamp:
            calc_pos = self.timestamp_

        while mobility.thread_._keep_alive:
            time_ = time() - currentTime
            if len(nodes) == 0:
                break
            for node in nodes:
                if hasattr(node, 'position'):
                    calc_pos(node, time_)
                    if len(node.position) == 0:
                        nodes.remove(node)
                    mobility.configLinks()
                    if Mininet_wifi.draw:
                        plot.update(node)
            if Mininet_wifi.draw:
                plot.pause()

    @classmethod
    def addNode(cls, node):
        if isinstance(node, Station):
            if hasattr(node, 'position'):
                position = node.position[0].split(' ')
                node.params['position'] = position[0].split(',')
            mobility.stations.append(node)
        elif isinstance(node, AP):
            mobility.aps.append(node)


class replayingBandwidth(object):
    'Replaying Bandwidth Traces'

    def __init__(self, Mininet_wifi):
        mobility.thread_ = thread(name='replayingBandwidth',
                                       target=self.throughput, args=(Mininet_wifi,))
        mobility.thread_.daemon = True
        mobility.thread_._keep_alive = True
        mobility.thread_.start()

    @classmethod
    def throughput(cls, Mininet_wifi):
        currentTime = time()
        stations = Mininet_wifi.stations
        while mobility.thread_._keep_alive:
            if len(stations) == 0:
                break
            time_ = time() - currentTime
            for sta in stations:
                if hasattr(sta, 'time'):
                    if time_ >= sta.time[0]:
                        wirelessLink.config_tc(sta, 0, sta.throughput[0], 0, 0)
                        # pos = '%d, %d, %d' % (sta.throughput[0], sta.throughput[0], 0)
                        # self.moveStationTo(sta, pos)
                        del sta.throughput[0]
                        del sta.time[0]
                        #info('%s\n' % sta.time[0])
                    if len(sta.time) == 1:
                        stations.remove(sta)
            # time.sleep(0.001)
        info("\nReplaying Process Finished!")


class replayingNetworkConditions(object):
    'Replaying Network Conditions'

    def __init__(self, Mininet_wifi, **kwargs):

        mobility.thread_ = thread( name='replayingNetConditions',
                                        target=self.behavior, args=(Mininet_wifi,) )
        mobility.thread_.daemon = True
        mobility.thread_._keep_alive = True
        mobility.thread_.start()

    @classmethod
    def behavior(cls, Mininet_wifi):
        seconds = 5
        info('Replaying process starting in %s seconds\n' % seconds)
        sleep(seconds)
        info('Replaying process has been started\n')
        currentTime = time()
        stations = Mininet_wifi.stations
        for sta in stations:
            sta.wintfs[0].freq = sta.get_freq(sta.wintfs[0])
        while mobility.thread_._keep_alive:
            if len(stations) == 0:
                break
            time_ = time() - currentTime
            for sta in stations:
                if hasattr(sta, 'time'):
                    if time_ >= sta.time[0]:
                        if sta.wintfs[0].associatedTo:
                            bw = sta.bw[0]
                            loss = sta.loss[0]
                            latency = sta.latency[0]
                            wirelessLink.config_tc(sta, 0, bw, loss, latency)
                        del sta.bw[0]
                        del sta.loss[0]
                        del sta.latency[0]
                        del sta.time[0]
                    if len(sta.time) == 0:
                        stations.remove(sta)
            sleep(0.001)
        info('Replaying process has finished!')

    @classmethod
    def addNode(cls, node):
        if isinstance(node, Station):
            mobility.stations.append(node)
        elif isinstance(node, AP):
            mobility.aps.append(node)


class replayingRSSI(object):
    'Replaying RSSI Traces'

    print_bw = False
    print_loss = False
    print_latency = False
    print_distance = False

    def __init__(self, Mininet_wifi, propagationModel='friis',
                 n=32, **kwargs):
        """ propagationModel = Propagation Model
            n: Power Loss Coefficient """
        for key in kwargs:
            setattr(self, key, kwargs[key])

        mobility.thread_ = thread(name='replayingRSSI', target=self.rssi,
                                       args=(Mininet_wifi, propagationModel, n))
        mobility.thread_.daemon = True
        mobility.thread_._keep_alive = True
        mobility.thread_.start()

    def rssi(self, Mininet_wifi, propagationModel='', n=0):
        currentTime = time()
        staList = Mininet_wifi.stations
        ang = {}
        for sta in staList:
            ang[sta] = random.uniform(0, 360)
            sta.wintfs[0].freq = sta.get_freq(sta.wintfs[0])
        while mobility.thread_._keep_alive:
            if len(staList) == 0:
                break
            time_ = time() - currentTime
            for sta in staList:
                if hasattr(sta, 'time'):
                    if time_ >= sta.time[0]:
                        ap = sta.wintfs[0].associatedTo  # get AP
                        sta.wintfs[0].rssi = sta.rssi[0]
                        if ap != '':
                            rssi = sta.rssi[0]
                            dist = int('%d' % self.calculateDistance(sta, ap, rssi,
                                                                     propagationModel, n))
                            self.setPos(Mininet_wifi, sta, ap, dist, ang[sta])
                            wirelessLink(sta, ap, dist, wlan=0, ap_wlan=0)
                        del sta.rssi[0]
                        del sta.time[0]
                    if len(sta.time) == 0:
                        staList.remove(sta)
            sleep(0.01)

    @classmethod
    def setPos(cls, Mininet_wifi, sta, ap, dist, ang):
        x = float('%.2f' % (dist * cos(ang) + int(ap.params['position'][0])))
        y = float('%.2f' % (dist * sin(ang) + int(ap.params['position'][1])))
        sta.params['position'] = x, y, 0
        mobility.configLinks(sta)
        if Mininet_wifi.draw:
            try:
                plot2d.update(sta)
            except:
                pass
        # sta.verifyingNodes(sta)

    def calculateDistance(self, sta, ap, rssi, propagationModel, n=32.0):

        pT = ap.wintfs[0].txpower
        gT = ap.wintfs[0].antennaGain
        gR = sta.wintfs[0].antennaGain
        if propagationModel in dir(self):
            dist = self.__getattribute__(propagationModel)(sta, ap, pT, gT, gR,
                                                           rssi, n)
            return dist

    @classmethod
    def pathLoss(cls, sta, ap, dist, wlan=0):
        """Path Loss Model:
        (f) signal frequency transmited(Hz)
        (d) is the distance between the transmitter and the receiver (m)
        (c) speed of light in vacuum (m)
        (L) System loss"""
        f = sta.wintfs[wlan].freq * 10 ** 9  # Convert Ghz to Hz
        c = 299792458.0
        L = 1
        if dist == 0:
            dist = 0.1
        lambda_ = c / f  # lambda: wavelength (m)
        denominator = lambda_ ** 2
        numerator = (4 * math.pi * dist) ** 2 * L
        pathLoss_ = 10 * math.log10(numerator / denominator)

        return pathLoss_

    @classmethod
    def friis(cls, sta, ap, pT, gT, gR, signalLevel, n):
        """Based on Free Space Propagation Model"""
        c = 299792458.0
        L = 2.0
        freq = sta.wintfs[0].freq * 10 ** 9  # Convert Ghz to Hz
        gains = gR + gT + pT
        lambda_ = float(c) / float(freq)  # lambda: wavelength (m)
        numerator = 10.0 ** (abs(signalLevel - gains) / 10.0)
        dist = (lambda_ / (4.0 * math.pi)) * ((numerator / L) ** (0.5))

        return dist

    def logDistance(self, sta, ap, pT, gT, gR, signalLevel, n):
        """Based on Log Distance Propagation Loss Model"""
        gains = gR + gT + pT
        ref_dist = 1
        exp = 2
        pathLossDb = self.pathLoss(sta, ap, ref_dist)
        rssi = gains - signalLevel - pathLossDb
        dist = 10 ** ((rssi + 10 * exp * math.log10(ref_dist)) / (10 * exp))

        return dist

    @classmethod
    def ITU(cls, sta, ap, pT, gT, gR, signalLevel, N):
        """Based on International Telecommunication Union (ITU) Propagation
        Loss Model"""
        lF = 0  # Floor penetration loss factor
        nFloors = 0  # Number of Floors
        gains = pT + gT + gR
        freq = sta.wintfs[0].freq * 10 ** 3
        dist = 10.0 ** ((-20.0 * math.log10(freq) - lF * nFloors + 28.0 +
                         abs(signalLevel - gains)) / N)

        return dist
