# coding: utf-8
#
#  Copyright (C) 2008-2010 Istituto per l'Interscambio Scientifico I.S.I.
#  You can contact us by email (isi@isi.it) or write to:
#  ISI Foundation, Viale S. Severo 65, 10133 Torino, Italy.
#
#  This program was written by André Panisson <panisson@gmail.com>
#
'''
Created on Jan 24, 2012
Modified by Ramon Fontes (ramonrf@dca.fee.unicamp.br)

@author: André Panisson
@contact: panisson@gmail.com
@organization: ISI Foundation, Torino, Italy
@source: https://github.com/panisson/pymobility
@copyright: http://dx.doi.org/10.5281/zenodo.9873
'''

import numpy as np
from numpy.random import rand
import threading
import time
import os

from mininet.log import debug, info
from mininet.wifiLink import link, Association
from mininet.wifiAssociationControl import associationControl
from mininet.wifiAdHocConnectivity import pairingAdhocNodes
from mininet.wifiMeshRouting import listNodes, meshRouting
from mininet.wmediumdConnector import WmediumdServerConn
from mininet.wifiPlot import plot2d, plot3d


class mobility (object):
    'Mobility'
    AC = ''  # association control method
    accessPoints = []
    stations = []
    adhocNodes = []
    meshNodes = []
    mobileNodes = []
    stationaryNodes = []
    continuePlot = 'plot2d.graphPause()'
    continueParams = 'time.sleep(0.001)'
    rec_rssi = False

    @classmethod
    def start(self, **mobilityparam):
        thread = threading.Thread(name='mobilityModel', target=self.models, kwargs=dict(mobilityparam,))
        thread.daemon = True
        thread.start()
        self.setWifiParameters()

    @classmethod
    def stop(self, **mobilityparam):
        debug('Starting mobility thread...\n')
        thread = threading.Thread(name='mobility', target=self.controlledMobility, kwargs=dict(mobilityparam,))
        thread.daemon = True
        thread.start()
        self.setWifiParameters()

    @classmethod
    def moveFactor(self, sta, diffTime):
        """
        :param sta: station
        :param diffTime: difference between initial and final time. Useful for calculating the speed
        """
        initialPosition = sta.params['initialPosition']
        finalPosition = sta.params['finalPosition']

        sta.params['position'] = initialPosition
        pos_x = float(finalPosition[0]) - float(initialPosition[0])
        pos_y = float(finalPosition[1]) - float(initialPosition[1])
        pos_z = float(finalPosition[2]) - float(initialPosition[2])

        self.speed(sta, pos_x, pos_y, pos_z, diffTime)
        pos = '%.2f,%.2f,%.2f' % (pos_x / diffTime, pos_y / diffTime, pos_z / diffTime)
        pos = pos.split(',')
        sta.moveFac = pos

    @classmethod
    def configure(self, *args, **kwargs):
        'configure Mobility Parameters'
        sta = args[0]
        stage = args[1]

        if 'position' in kwargs:
            if(stage == 'stop'):
                finalPosition = kwargs['position']
                sta.params['finalPosition'] = finalPosition.split(',')
            if(stage == 'start'):
                initialPosition = kwargs['position']
                sta.params['initialPosition'] = initialPosition.split(',')

        if 'time' in kwargs:
            time = kwargs['time']

        if(stage == 'start'):
            sta.startTime = time
        elif(stage == 'stop'):
            sta.endTime = time
            diffTime = sta.endTime - sta.startTime
            self.moveFactor(sta, diffTime)

    @classmethod
    def setWifiParameters(self):
        """
        Opens a thread for wifi parameters
        """
        thread = threading.Thread(name='wifiParameters', target=self.parameters)
        thread.daemon = True
        thread.start()

    @classmethod
    def speed(self, sta, pos_x, pos_y, pos_z, diffTime):
        """
        Calculates the speed
        
        :param sta: station
        :param pos_x: Position x
        :param pos_y: Position y
        :param pos_z: Position z
        :param diffTime: difference between start and stop time. Useful for calculating the speed
        """
        sta.params['speed'] = '%.2f' % abs(((pos_x + pos_y + pos_z) / diffTime))

    @classmethod
    def apOutOfRange(self, sta, ap, wlan, dist):
        """
        When ap is out of range
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        :param dist: distance between source and destination  
        """
        if ap == sta.params['associatedTo'][wlan]:
            debug('iw dev %s disconnect\n' % sta.params['wlan'][wlan])
            if 'encrypt' in ap.params and 'ieee80211r' not in ap.params:
                if ap.params['encrypt'][0] == 'wpa' or ap.params['encrypt'][0] == 'wpa2':
                    os.system('rm %s.staconf' % sta)
                    pidfile = "mn%d_%s_%s_wpa.pid" % (os.getpid(), sta.name, wlan)
                    os.system('pkill -f \'wpa_supplicant -B -Dnl80211 -P %s -i %s\'' % (pidfile, sta.params['wlan'][wlan]))
                    os.system('rm /var/run/wpa_supplicant/%s' % sta.params['wlan'][wlan])
            elif WmediumdServerConn.connected and not WmediumdServerConn.interference_enabled:
                cls = Association
                cls.setSNRWmediumd(sta, ap, snr=-10)
            sta.pexec('iw dev %s disconnect' % sta.params['wlan'][wlan])
            sta.params['associatedTo'][wlan] = ''
            sta.params['rssi'][wlan] = 0
            sta.params['snr'][wlan] = 0
            sta.params['channel'][wlan] = 0
            # sta.params['frequency'][wlan] = 0
        if sta in ap.params['associatedStations']:
            ap.params['associatedStations'].remove(sta)
        if ap in sta.params['apsInRange']:
            sta.params['apsInRange'].remove(ap)
            ap.params['stationsInRange'].pop(sta, None)
        link.recordParams(sta, ap)

    @classmethod
    def apInRange(self, sta, ap, wlan, dist):
        """
        When ap is in range
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        :param dist: distance between source and destination  
        """
        if self.rec_rssi:
            os.system('hwsim_mgmt -k %s %s >/dev/null 2>&1' % (sta.phyID[wlan], abs(int(sta.params['rssi'][wlan]))))
        if ap not in sta.params['apsInRange']:
            sta.params['apsInRange'].append(ap)
            rssi_ = link.setRSSI(sta, ap, wlan, dist)
            ap.params['stationsInRange'][sta] = rssi_
        else:
            rssi_ = link.setRSSI(sta, ap, wlan, dist)
            ap.params['stationsInRange'][sta] = rssi_
        if ap == sta.params['associatedTo'][wlan]:
            rssi_ = link.setRSSI(sta, ap, wlan, dist)
            sta.params['rssi'][wlan] = rssi_
            snr_ = link.setSNR(sta, wlan)
            sta.params['snr'][wlan] = snr_
            if sta not in ap.params['associatedStations']:
                ap.params['associatedStations'].append(sta)
            if dist >= 0.01:
                if WmediumdServerConn.connected and not WmediumdServerConn.interference_enabled:
                    if sta.lastpos != sta.params['position']:
                        cls = Association
                        cls.setSNRWmediumd(sta, ap, snr=sta.params['snr'][wlan])
                else:
                    link(sta, ap, wlan, dist)
        link.recordParams(sta, ap)

    @classmethod
    def checkAssociation(self, sta, wlan):
        """ 
        check association
        
        :param sta: station
        :param wlan: wlan ID
        """
        for ap in self.accessPoints:
            dist = link.getDistance(sta, ap)
            if dist > ap.params['range']:
                self.apOutOfRange(sta, ap, wlan, dist)

        for ap in self.accessPoints:
            dist = link.getDistance(sta, ap)
            if dist <= ap.params['range']:
                self.handover(sta, ap, wlan, dist)
                self.apInRange(sta, ap, wlan, dist)

    @classmethod
    def handover(self, sta, ap, wlan, dist):
        """
        handover
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        :param dist: distance between source and destination   
        """
        changeAP = False

        """Association Control: mechanisms that optimize the use of the APs"""
        if self.AC != '' and sta.params['associatedTo'][wlan] != ap \
            and sta.params['associatedTo'][wlan] != '':
            ac = self.AC
            value = associationControl(sta, ap, wlan, ac)
            changeAP = value.changeAP

        if sta.params['associatedTo'][wlan] == '' or changeAP == True:
            if ap not in sta.params['associatedTo']:
                cls = Association
                cls.printCon = False
                cls.associate_infra(sta, ap, wlan)
                self.updateAssociation(sta, ap, wlan)

    @classmethod
    def updateAssociation(self, sta, ap, wlan):
        """ 
        Updates attributes regarding the association
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """
        if sta.params['associatedTo'][wlan] != '' and sta in sta.params['associatedTo'][wlan].params['associatedStations']:
            sta.params['associatedTo'][wlan].params['associatedStations'].remove(sta)
            link.recordParams(sta, sta.params['associatedTo'][wlan])
        cls = Association
        cls.updateParams(sta, ap, wlan)
        sta.params['associatedTo'][wlan] = ap

        # necessary when a station is associated to a new AP
        if WmediumdServerConn.interference_enabled:
            self.updateWmediumdPos(sta)

    @classmethod
    def updateWmediumdPos(self, node):
        if node.isStationary and node.type != 'vehicle':
            info('Configuring wmediumd for %s...\n' % node)
            time.sleep(2)
        self.setWmediumdPos(node)

    @classmethod
    def setWmediumdPos(self, node):
        if node.lastpos != node.params['position']:
            time.sleep(0.0001)
            node.setPositionWmediumd()
            node.lastpos = node.params['position']

    @classmethod
    def controlledMobility(self, init_time=0, final_time=0, stations=None, aps=None, dstConn=None, srcConn=None,
                        plotNodes=None, MIN_X=0, MIN_Y=0, MIN_Z=0, MAX_X=0, MAX_Y=0, MAX_Z=0, AC='',
                        rec_rssi=False, is3d=False, DRAW=False, **params):
        """ 
        Used when the position of each node is previously defined
        
        :param init_time: time when the mobility starts
        :param final_time: time when the mobility stops
        :param stations: list of stations
        :param aps: list of access points
        :param srcConn: list of connections for source nodes
        :param dstConn: list of connections for destination nodes
        :param plotnodes: list of nodes to be plotted (including hosts and switches)
        :param MIN_X: Minimum value for X
        :param MIN_Y: Minimum value for Y
        :param MIN_Z: Minimum value for Z
        :param MAX_X: Maximum value for X
        :param MAX_Y: Maximum value for Y
        :param MAX_Z: Maximum value for Z
        :param AC: Association Control Method
        """
        self.rec_rssi = rec_rssi
        self.AC = AC
        t_end = time.time() + final_time
        t_initial = time.time() + init_time
        currentTime = time.time()
        i = 1

        self.stations = stations
        self.accessPoints = aps

        nodes = self.stations + self.accessPoints + plotNodes

        for node in nodes:
            if 'position' in node.params and 'initialPosition' not in node.params:
                self.stationaryNodes.append(node)
            if 'initialPosition' in node.params:
                node.params['position'] = node.params['initialPosition']
                node.params.pop("initialPosition", None)
                node.params.pop("finalPosition", None)
                if not node.isStationary:
                    self.mobileNodes.append(node)

        nodes = self.mobileNodes + self.stationaryNodes

        try:
            if DRAW:
                plot = self.instantiateGraph(MIN_X, MIN_Y, MAX_X, MAX_Y, nodes, srcConn, dstConn, MIN_Z, MAX_Z, is3d)
        except:
            info('Warning: running without GUI.\n')
            DRAW = False

        try:
            while True:
                if time.time() > t_end or time.time() < t_initial:
                    break
                if time.time() - currentTime >= i:
                    for node in self.mobileNodes:
                        if time.time() - currentTime >= node.startTime and time.time() - currentTime <= node.endTime:
                            x = '%.2f' % (float(node.params['position'][0]) + float(node.moveFac[0]))
                            y = '%.2f' % (float(node.params['position'][1]) + float(node.moveFac[1]))
                            z = '%.2f' % (float(node.params['position'][2]) + float(node.moveFac[2]))
                            node.params['position'] = x, y, z
                            if WmediumdServerConn.interference_enabled:
                                self.setWmediumdPos(node)
                        if DRAW:
                            plot.graphUpdate(node)
                    eval(self.continuePlot)
                    plot.graphPause()
                    i += 1
            self.mobileNodes = []
        except:
            pass

    @classmethod
    def instantiateGraph(self, MIN_X, MIN_Y, MAX_X, MAX_Y, nodes, srcConn, dstConn, MIN_Z=0, MAX_Z=0, is3d=False):
        if is3d:
            plot = plot3d
            plot.instantiateGraph(MIN_X, MIN_Y, MIN_Z, MAX_X, MAX_Y, MAX_Z)
            plot.graphInstantiateNodes(nodes)
        else:
            plot = plot2d
            plot.instantiateGraph(MIN_X, MIN_Y, MAX_X, MAX_Y)
            plot.plotGraph(nodes, srcConn, dstConn)
        return plot

    @classmethod
    def addNodes(self, stas, aps):
        self.stations = stas
        self.accessPoints = aps
        self.mobileNodes = self.stations

    @classmethod
    def models(self, stations=None, aps=None, model=None, stationaryNodes=[], min_v=0, max_v=0, seed=None,
               dstConn=None, srcConn=None, plotNodes=None, MAX_X=0, MAX_Y=0, AC='', rec_rssi=False,
               DRAW=False, **params):
        """ 
        Used when a mobility model is applied
        
        :param stations: list of stations
        :param aps: list of access points
        :param model: mobility model
        :param stationaryNodes: stationary nodes
        :param min_v: minimum velocity
        :param max_v: maximum velocity
        :param speed: speed
        :param srcConn: list of connections for source nodes
        :param dstConn: list of connections for destination nodes
        :param plotNodes: list of nodes to be plotted (including hosts and switches)
        :param MAX_X: Maximum value for X
        :param MAX_Y: Maximum value for Y
        """
        self.rec_rssi = rec_rssi
        np.random.seed(seed)
        self.AC = AC

        self.addNodes(stations, aps)
        nodes = self.stations + self.accessPoints + plotNodes

        # max waiting time
        MAX_WT = 100.

        for sta in self.stations:
            if sta.max_x == 0:
                sta.max_x = MAX_X
            if sta.max_y == 0:
                sta.max_y = MAX_Y
            if sta.max_v == 0:
                sta.max_v = max_v
            if sta.min_v == 0:
                sta.min_v = min_v

        try:
            if DRAW:
                self.instantiateGraph(0, 0, MAX_X, MAX_Y, nodes, srcConn, dstConn, 0, 0, is3d=False)
                plot2d.graphPause()
        except:
            info('Warning: running without GUI.\n')
            DRAW = False

        if stationaryNodes != None:
            debug('Configuring the mobility model %s' % model)

            if(model == 'RandomWalk'):  # Random Walk model
                mob = random_walk(stationaryNodes)
            elif(model == 'TruncatedLevyWalk'):  # Truncated Levy Walk model
                mob = truncated_levy_walk(stationaryNodes)
            elif(model == 'RandomDirection'):  # Random Direction model
                mob = random_direction(stationaryNodes, dimensions=(MAX_X, MAX_Y))
            elif(model == 'RandomWayPoint'):  # Random Waypoint model
                mob = random_waypoint(stationaryNodes, wt_max=MAX_WT)
            elif(model == 'GaussMarkov'):  # Gauss-Markov model
                mob = gauss_markov(stationaryNodes, alpha=0.99)
            elif(model == 'ReferencePoint'):  # Reference Point Group model
                mob = reference_point_group(stationaryNodes, dimensions=(MAX_X, MAX_Y), aggregation=0.5)
            elif(model == 'TimeVariantCommunity'):  # Time-variant Community Mobility Model
                mob = tvc(stationaryNodes, dimensions=(MAX_X, MAX_Y), aggregation=[0.5, 0.], epoch=[100, 100])
            else:
                raise Exception("Mobility Model not defined or doesn't exist!")

            if DRAW:
                self.startMobilityModelGraph(mob, stationaryNodes)
            else:
                self.startMobilityModelNoGraph(mob, stationaryNodes)

    @classmethod
    def startMobilityModelGraph(self, mob, nodes):
        """ 
        Useful for plotting graphs
        
        :param mob: mobility params
        :param nodes: list of nodes
        """
        for xy in mob:
            i = 0
            for node in nodes:
                node.params['position'] = '%.2f' % xy[i][0], '%.2f' % xy[i][1], 0.0
                if WmediumdServerConn.interference_enabled:
                    self.setWmediumdPos(node)
                i += 1
                plot2d.graphUpdate(node)
            eval(self.continuePlot)

    @classmethod
    def startMobilityModelNoGraph(self, mob, nodes):
        """ 
        Useful when graph is not required
        
        :param mob: mobility params
        :param nodes: list of nodes
        """
        for xy in mob:
            i = 0
            for node in nodes:
                node.params['position'] = '%.2f' % xy[i][0], '%.2f' % xy[i][1], 0.0
                if WmediumdServerConn.interference_enabled:
                    self.setWmediumdPos(node)
                i += 1
            time.sleep(0.5)

    @classmethod
    def parameters_(self, node=None):
        "Applies channel params and handover"
        if WmediumdServerConn.interference_enabled:
            self.setWmediumdPos(node)
        if node == None:
            nodes = self.stations
        else:
            if node.type == 'accessPoint':
                nodes = self.stations
            else:
                nodes = []
                nodes.append(node)
        for node_ in self.accessPoints:
            if 'link' in node_.params and node_.params['link'] == 'mesh':
                nodes.append(node_)

        self.configureLinks(nodes)

    @classmethod
    def parameters(self):
        "Applies channel params and handover"
        if WmediumdServerConn.interference_enabled and self.meshNodes != []:
            cls = Association
            for node in self.meshNodes:
                for wlan in range(0, len(node.params['wlan'])):
                    cls.meshAssociation(node, wlan)
        while True:
            self.configureLinks(self.mobileNodes)

    @classmethod
    def configureLinks(self, nodes):
        for node in nodes:
            for wlan in range(0, len(node.params['wlan'])):
                if node.func[wlan] == 'mesh' or node.func[wlan] == 'adhoc':
                    if not WmediumdServerConn.interference_enabled:
                        if node.func[wlan] == 'adhoc':
                            value = pairingAdhocNodes(node, wlan, self.adhocNodes)
                            dist = value.dist
                        else:
                            if node.type == 'vehicle':
                                node = node.params['carsta']
                                wlan = 0
                            dist = listNodes.pairingNodes(node, wlan, self.meshNodes)
                        if not WmediumdServerConn.connected and dist >= 0.01:
                            link(sta=node, wlan=wlan, dist=dist)
                    else:
                        if 'carsta' in node.params:
                            car = node.params['carsta']
                            car.params['position'] = node.params['position']
                            node = car
                        self.setWmediumdPos(node)
                else:
                    if WmediumdServerConn.interference_enabled:
                        if Association.bgscan != '':
                            self.setWmediumdPos(node)
                            for ap in self.accessPoints:
                                dist = link.getDistance(node, ap)
                                if dist <= ap.params['range']:
                                    self.handover(node, ap, wlan, dist)
                        else:
                            self.checkAssociation(node, wlan)
                    else:
                        self.checkAssociation(node, wlan)
        if not WmediumdServerConn.interference_enabled:
            if meshRouting.routing == 'custom':
                meshRouting(self.meshNodes)
        # have to verify this
        eval(self.continueParams)


# define a Uniform Distribution
U = lambda MIN, MAX, SAMPLES: rand(*SAMPLES.shape) * (MAX - MIN) + MIN

# define a Truncated Power Law Distribution
P = lambda ALPHA, MIN, MAX, SAMPLES: ((MAX ** (ALPHA + 1.) - 1.) * rand(*SAMPLES.shape) + 1.) ** (1. / (ALPHA + 1.))

# define an Exponential Distribution
E = lambda SCALE, SAMPLES:-SCALE * np.log(rand(*SAMPLES.shape))

# *************** Palm state probability **********************
def pause_probability_init(pause_low, pause_high, speed_low, speed_high, max_x, max_y):
    alpha1 = ((pause_high + pause_low) * (speed_high - speed_low)) / (2 * np.log(speed_high / speed_low))
    delta1 = np.sqrt((max_x * max_x) + (max_y * max_y))
    return alpha1 / (alpha1 + delta1)

# *************** Palm residual ******************************
def residual_time(mean, delta, shape=(1,)):
    t1 = mean - delta;
    t2 = mean + delta;
    u = rand(*shape);
    residual = np.zeros(shape)
    if delta != 0.0:
        case_1_u = u < (2.*t1 / (t1 + t2))
        residual[case_1_u] = u[case_1_u] * (t1 + t2) / 2.
        residual[np.logical_not(case_1_u)] = t2 - np.sqrt((1. - u[np.logical_not(case_1_u)]) * (t2 * t2 - t1 * t1))
    else:
        residual = u * mean
    return residual

# *********** Initial speed ***************************
def initial_speed(speed_mean, speed_delta, shape=(1,)):
    v0 = speed_mean - speed_delta
    v1 = speed_mean + speed_delta
    u = rand(*shape)

    return pow(v1, u) / pow(v0, u - 1)

def init_random_waypoint(nodes, min_x, min_y, max_x, max_y,
                         speed_low, speed_high, pause_low, pause_high):

    nr_nodes = nodes
    x = np.empty(nr_nodes)
    y = np.empty(nr_nodes)
    x_waypoint = np.empty(nr_nodes)
    y_waypoint = np.empty(nr_nodes)
    speed = np.empty(nr_nodes)

    pause_time = np.empty(nr_nodes)
    speed_low = speed_low
    speed_high = speed_high
    moving = np.ones(nr_nodes)
    speed_mean, speed_delta = (speed_low + speed_high) / 2., (speed_high - speed_low) / 2.
    pause_mean, pause_delta = (pause_low + pause_high) / 2., (pause_high - pause_low) / 2.

    # steady-state pause probability for Random Waypoint
    q0 = pause_probability_init(pause_low, pause_high, speed_low, speed_high, max_x, max_y)

    for i in range(nr_nodes):
        while True:
            if rand() < q0[i]:
                # moving[i] = 0.
                # speed_mean = np.delete(speed_mean, i)
                # speed_delta = np.delete(speed_delta, i)
                # M_0
                x1 = rand() * max_x[i]
                x2 = rand() * max_x[i]
                # M_1
                y1 = rand() * max_y[i]
                y2 = rand() * max_y[i]
                break
            else:
                # M_0
                x1 = rand() * max_x[i]
                x2 = rand() * max_x[i]
                # M_1
                y1 = rand() * max_y[i]
                y2 = rand() * max_y[i]

                # r is a ratio of the length of the randomly chosen path over
                # the length of a diagonal across the simulation area
                # ||M_1 - M_0||
                r = np.sqrt(((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)) / (max_x[i] * max_x[i] + max_y[i] * max_y[i]))
                if rand() < r:
                    moving[i] = 1.
                    break

        x[i] = x1
        y[i] = y1

        x_waypoint[i] = x2
        y_waypoint[i] = y2

    # steady-state positions
    # initially the node has traveled a proportion u2 of the path from (x1,y1) to (x2,y2)
    u2 = rand(*x.shape)
    x[:] = u2 * x + (1 - u2) * x_waypoint
    y[:] = u2 * y + (1 - u2) * y_waypoint

    # steady-state speed and pause time
    paused_bool = moving == 0.
    paused_idx = np.where(paused_bool)[0]
    pause_time[paused_idx] = residual_time(pause_mean, pause_delta, paused_idx.shape)
    speed[paused_idx] = 0.0

    moving_bool = np.logical_not(paused_bool)
    moving_idx = np.where(moving_bool)[0]
    pause_time[moving_idx] = 0.0
    speed[moving_idx] = initial_speed(speed_mean, speed_delta, moving_idx.shape)

    return x, y, x_waypoint, y_waypoint, speed, pause_time

class RandomWaypoint(object):

    def __init__(self, nodes, wt_max=None):
        '''
        Random Waypoint model.
        
        Required arguments:
        
          *nr_nodes*:
            Integer, the number of nodes.
          
        keyword arguments:
        
          *velocity*:
            Tuple of Integers, the minimum and maximum values for node velocity.
          
          *wt_max*:
            Integer, the maximum wait time for node pauses.
            If wt_max is 0 or None, there is no pause time.
        '''
        self.nodes = nodes
        self.nr_nodes = len(nodes)
        self.wt_max = wt_max
        self.init_stationary = True

    def __iter__(self):

        nr_nodes = len(self.nodes)
        NODES = np.arange(nr_nodes)

        max_v = U(0, 0, NODES)
        min_v = U(0, 0, NODES)
        max_x = U(0, 0, NODES)
        max_y = U(0, 0, NODES)
        min_x = U(0, 0, NODES)
        min_y = U(0, 0, NODES)

        MAX_X = max_x
        MAX_Y = max_y
        MIN_X = min_x
        MIN_Y = min_y
        MAX_V = max_v
        MIN_V = min_v

        for node in range(0, len(self.nodes)):
            MAX_V[node] = self.nodes[node].max_v
            MIN_V[node] = self.nodes[node].min_v
            MAX_X[node] = self.nodes[node].max_x
            MAX_Y[node] = self.nodes[node].max_y
            MIN_X[node] = self.nodes[node].min_x
            MIN_Y[node] = self.nodes[node].min_y

        # wt_min = 0.

        # if self.init_stationary:
        #    x, y, x_waypoint, y_waypoint, velocity, wt = \
        #        init_random_waypoint(self.nr_nodes, MIN_X, MIN_Y, MAX_X, MAX_Y, MIN_V, MAX_V, wt_min,
        #                     (self.wt_max if self.wt_max is not None else 0.))
        # else:
        NODES = np.arange(self.nr_nodes)
        x = U(MIN_X, MAX_X, NODES)
        y = U(MIN_Y, MAX_Y, NODES)
        x_waypoint = U(MIN_X, MAX_X, NODES)
        y_waypoint = U(MIN_Y, MAX_Y, NODES)
        wt = np.zeros(self.nr_nodes)
        velocity = U(MIN_V, MAX_V, NODES)

        theta = np.arctan2(y_waypoint - y, x_waypoint - x)
        costheta = np.cos(theta)
        sintheta = np.sin(theta)

        while True:
            # update node position
            x += velocity * costheta
            y += velocity * sintheta
            # calculate distance to waypoint
            d = np.sqrt(np.square(y_waypoint - y) + np.square(x_waypoint - x))
            # update info for arrived nodes
            arrived = np.where(np.logical_and(d <= velocity, wt <= 0.))[0]

            # step back for nodes that surpassed waypoint
            x[arrived] = x_waypoint[arrived]
            y[arrived] = y_waypoint[arrived]

            if self.wt_max:
                velocity[arrived] = 0.
                wt[arrived] = U(0, self.wt_max, arrived)
                # update info for paused nodes
                wt[np.where(velocity == 0.)[0]] -= 1.
                # update info for moving nodes
                arrived = np.where(np.logical_and(velocity == 0., wt < 0.))[0]

            try:
                if arrived.size > 0:
                    wx = U(MIN_X, MAX_X, arrived)
                    x_waypoint[arrived] = wx[arrived]
                    wy = U(MIN_Y, MAX_Y, arrived)
                    y_waypoint[arrived] = wy[arrived]
                    v = U(MIN_V, MAX_V, arrived)
                    velocity[arrived] = v[arrived]
                    theta[arrived] = np.arctan2(y_waypoint[arrived] - y[arrived], x_waypoint[arrived] - x[arrived])
                    costheta[arrived] = np.cos(theta[arrived])
                    sintheta[arrived] = np.sin(theta[arrived])
            except:
                pass

            self.velocity = velocity
            self.wt = wt
            yield np.dstack((x, y))[0]

class StochasticWalk(object):

    def __init__(self, nodes, FL_DISTR, VELOCITY_DISTR, WT_DISTR=None, border_policy='reflect', model=None):
        '''
        Base implementation for models with direction uniformly chosen from [0,pi]:
        random_direction, random_walk, truncated_levy_walk
        
        Required arguments:
        
          *nr_nodes*:
            Integer, the number of nodes.
          
          *dimensions*:
            Tuple of Integers, the x and y dimensions of the simulation area.
            
          *FL_DISTR*:
            A function that, given a set of samples, 
             returns another set with the same size of the input set.
            This function should implement the distribution of flight lengths
             to be used in the model.
             
          *VELOCITY_DISTR*:
            A function that, given a set of flight lengths, 
             returns another set with the same size of the input set.
            This function should implement the distribution of velocities
             to be used in the model, as random or as a function of the flight lengths.
          
        keyword arguments:
        
          *WT_DISTR*:
            A function that, given a set of samples, 
             returns another set with the same size of the input set.
            This function should implement the distribution of wait times
             to be used in the node pause.
            If WT_DISTR is 0 or None, there is no pause time.
            
          *border_policy*:
            String, either 'reflect' or 'wrap'. The policy that is used when the node arrives to the border.
            If 'reflect', the node reflects off the border.
            If 'wrap', the node reappears at the opposite edge (as in a torus-shaped area).
        '''
        self.b = [0]
        self.nodes = nodes
        self.collect_fl_stats = False
        self.collect_wt_stats = False
        self.border_policy = border_policy
        self.nr_nodes = len(nodes)
        self.FL_DISTR = FL_DISTR
        self.VELOCITY_DISTR = VELOCITY_DISTR
        self.WT_DISTR = WT_DISTR
        self.model = model

    def __iter__(self):
        def reflect(xy):
            # node bounces on the margins
            b = np.where(xy[:, 0] < MIN_X)[0]
            if b.size > 0:
                xy[b, 0] = 2 * MIN_X[b] - xy[b, 0]
                cosintheta[b, 0] = -cosintheta[b, 0]
            b = np.where(xy[:, 0] > MAX_X)[0]
            if b.size > 0:
                xy[b, 0] = 2 * MAX_X[b] - xy[b, 0]
                cosintheta[b, 0] = -cosintheta[b, 0]
            b = np.where(xy[:, 1] < MIN_Y)[0]
            if b.size > 0:
                xy[b, 1] = 2 * MIN_Y[b] - xy[b, 1]
                cosintheta[b, 1] = -cosintheta[b, 1]
            b = np.where(xy[:, 1] > MAX_Y)[0]
            if b.size > 0:
                xy[b, 1] = 2 * MAX_Y[b] - xy[b, 1]
                cosintheta[b, 1] = -cosintheta[b, 1]
            self.b = b

        def wrap(xy):
            b = np.where(xy[:, 0] < MIN_X)[0]
            if b.size > 0: xy[b, 0] += MAX_X[b]
            b = np.where(xy[:, 0] > MAX_X)[0]
            if b.size > 0: xy[b, 0] -= MAX_X[b]
            b = np.where(xy[:, 1] < MIN_Y)[0]
            if b.size > 0: xy[b, 1] += MAX_Y[b]
            b = np.where(xy[:, 1] > MAX_Y)[0]
            if b.size > 0: xy[b, 1] -= MAX_Y[b]
            self.b = b

        if self.border_policy == 'reflect':
            borderp = reflect
        elif self.border_policy == 'wrap':
            borderp = wrap
        else:
            borderp = self.border_policy

        NODES = np.arange(self.nr_nodes)

        max_x = U(0, 0, NODES)
        max_y = U(0, 0, NODES)
        min_x = U(0, 0, NODES)
        min_y = U(0, 0, NODES)

        MAX_X = max_x
        MAX_Y = max_y
        MIN_X = min_x
        MIN_Y = min_y

        for node in range(0, len(self.nodes)):
            MAX_X[node] = self.nodes[node].max_x
            MAX_Y[node] = self.nodes[node].max_y
            MIN_X[node] = self.nodes[node].min_x
            MIN_Y[node] = self.nodes[node].min_y

        xy = U(0, MAX_X[self.b], np.dstack((NODES, NODES))[0])
        fl = self.FL_DISTR(NODES)
        velocity = self.VELOCITY_DISTR(fl)
        theta = U(0, 1.8 * np.pi, NODES)
        cosintheta = np.dstack((np.cos(theta), np.sin(theta)))[0] * np.dstack((velocity, velocity))[0]
        wt = np.zeros(self.nr_nodes)

        if self.collect_fl_stats: self.fl_stats = list(fl)
        if self.collect_wt_stats: self.wt_stats = list(wt)

        while True:
            xy += cosintheta
            fl -= velocity

            # step back for nodes that surpassed fl
            arrived = np.where(np.logical_and(velocity > 0., fl <= 0.))[0]

            if arrived.size > 0:
                diff = fl.take(arrived) / velocity.take(arrived)
                xy[arrived] += np.dstack((diff, diff))[0] * cosintheta[arrived]

            # apply border policy
            borderp(xy)

            if self.WT_DISTR:
                velocity[arrived] = 0.
                wt[arrived] = self.WT_DISTR(arrived)
                if self.collect_wt_stats: self.wt_stats.extend(wt[arrived])
                # update info for paused nodes
                wt[np.where(velocity == 0.)[0]] -= 1.
                arrived = np.where(np.logical_and(velocity == 0., wt < 0.))[0]

            # update info for moving nodes
            if arrived.size > 0:
                theta = U(0, 2 * np.pi, arrived)
                fl[arrived] = self.FL_DISTR(arrived)
                if self.collect_fl_stats: self.fl_stats.extend(fl[arrived])
                if self.model == 'RandomDirection':
                    velocity[arrived] = self.VELOCITY_DISTR(fl[arrived][0])[arrived]
                elif self.model == 'TruncatedLevyWalk':
                    velocity[arrived] = self.VELOCITY_DISTR(fl[arrived])
                else:
                    velocity[arrived] = self.VELOCITY_DISTR(fl[arrived])[arrived]
                v = velocity[arrived]
                cosintheta[arrived] = np.dstack((v * np.cos(theta), v * np.sin(theta)))[0]
            yield xy

class RandomWalk(StochasticWalk):

    def __init__(self, nodes, border_policy='reflect'):
        '''
        Random Walk mobility model.
        This model is based in the Stochastic Walk, but both the flight length and node velocity distributions are in fact constants,
        set to the *distance* and *velocity* parameters. The waiting time is set to None.
        
        Required arguments:
        
          *nr_nodes*:
            Integer, the number of nodes.
          
        keyword arguments:
        
          *velocity*:
            Double, the value for the constant node velocity. Default is 1.0
          
          *distance*:
            Double, the value for the constant distance traveled in each step. Default is 1.0
            
          *border_policy*:
            String, either 'reflect' or 'wrap'. The policy that is used when the node arrives to the border.
            If 'reflect', the node reflects off the border.
            If 'wrap', the node reappears at the opposite edge (as in a torus-shaped area).
        '''
        nr_nodes = len(nodes)
        NODES = np.arange(nr_nodes)
        VELOCITY = U(0, 0, NODES)
        velocity = VELOCITY
        distance = VELOCITY

        for node in range(0, len(nodes)):
            velocity[node] = nodes[node].constantVelocity
            distance[node] = nodes[node].constantDistance

            if velocity[node] > distance[node]:
                # In this implementation, each step is 1 second,
                # it is not possible to have a velocity larger than the distance
                raise Exception('Velocity must be <= Distance')

        fl = np.zeros(nr_nodes) + distance
        vel = np.zeros(nr_nodes) + velocity

        FL_DISTR = lambda SAMPLES: np.array(fl[:len(SAMPLES)])
        VELOCITY_DISTR = lambda FD: np.array(vel[:len(FD)])

        StochasticWalk.__init__(self, nodes, FL_DISTR, VELOCITY_DISTR, border_policy=border_policy)

class RandomDirection(StochasticWalk):

    def __init__(self, nodes, dimensions, wt_max=None, border_policy='reflect'):
        '''
        Random Direction mobility model.
        This model is based in the Stochastic Walk. The flight length is chosen from a uniform distribution, 
        with minimum 0 and maximum set to the maximum dimension value.
        The velocity is also chosen from a uniform distribution, with boundaries set by the *velocity* parameter.
        If wt_max is set, the waiting time is chosen from a uniform distribution with values between 0 and wt_max.
        If wt_max is not set, waiting time is set to None.
        
        Required arguments:
        
          *nr_nodes*:
            Integer, the number of nodes.
          
          *dimensions*:
            Tuple of Integers, the x and y dimensions of the simulation area.
          
        keyword arguments:
        
          *wt_max*:
            Double, maximum value for the waiting time distribution.
            If wt_max is set, the waiting time is chosen from a uniform distribution with values between 0 and wt_max.
            If wt_max is not set, the waiting time is set to None.
            Default is None.
            
          *border_policy*:
            String, either 'reflect' or 'wrap'. The policy that is used when the node arrives to the border.
            If 'reflect', the node reflects off the border.
            If 'wrap', the node reappears at the opposite edge (as in a torus-shaped area).
        '''
        nr_nodes = len(nodes)
        NODES = np.arange(nr_nodes)

        max_v = U(0, 0, NODES)
        min_v = U(0, 0, NODES)

        MAX_V = max_v
        MIN_V = min_v

        for node in range(0, len(nodes)):
            MAX_V[node] = nodes[node].max_v
            MIN_V[node] = nodes[node].min_v

        FL_MAX = max(dimensions)

        FL_DISTR = lambda SAMPLES: U(0, FL_MAX, SAMPLES)
        if wt_max:
            WT_DISTR = lambda SAMPLES: U(0, wt_max, SAMPLES)
        else:
            WT_DISTR = None
        VELOCITY_DISTR = lambda FD: U(MIN_V, MAX_V, FD)

        StochasticWalk.__init__(self, nodes, FL_DISTR, VELOCITY_DISTR, WT_DISTR, border_policy, model='RandomDirection')

class TruncatedLevyWalk(StochasticWalk):

    def __init__(self, nodes, FL_EXP=-2.6, FL_MAX=50., WT_EXP=-1.8, WT_MAX=100., border_policy='reflect'):
        '''
        Truncated Levy Walk mobility model, based on the following paper:
        Injong Rhee, Minsu Shin, Seongik Hong, Kyunghan Lee, and Song Chong. On the Levy-Walk Nature of Human Mobility. 
            In 2008 IEEE INFOCOM - Proceedings of the 27th Conference on Computer Communications, pages 924-932. April 2008.
        
        The implementation is a special case of the more generic Stochastic Walk, 
        in which both the flight length and waiting time distributions are truncated power laws,
        with exponents set to FL_EXP and WT_EXP and truncated at FL_MAX and WT_MAX.
        The node velocity is a function of the flight length.
        
        Required arguments:
        
          *nr_nodes*:
            Integer, the number of nodes.
          
        keyword arguments:
        
          *FL_EXP*:
            Double, the exponent of the flight length distribution. Default is -2.6
            
          *FL_MAX*:
            Double, the maximum value of the flight length distribution. Default is 50
          
          *WT_EXP*:
            Double, the exponent of the waiting time distribution. Default is -1.8
            
          *WT_MAX*:
            Double, the maximum value of the waiting time distribution. Default is 100
            
          *border_policy*:
            String, either 'reflect' or 'wrap'. The policy that is used when the node arrives to the border.
            If 'reflect', the node reflects off the border.
            If 'wrap', the node reappears at the opposite edge (as in a torus-shaped area).
        '''
        FL_DISTR = lambda SAMPLES: P(FL_EXP, 1., FL_MAX, SAMPLES)
        if WT_EXP and WT_MAX:
            WT_DISTR = lambda SAMPLES: P(WT_EXP, 1., WT_MAX, SAMPLES)
        else:
            WT_DISTR = None
        VELOCITY_DISTR = lambda FD: np.sqrt(FD) / 10.

        StochasticWalk.__init__(self, nodes, FL_DISTR, VELOCITY_DISTR, WT_DISTR, border_policy, model='TruncatedLevyWalk')

class HeterogeneousTruncatedLevyWalk(StochasticWalk):

    def __init__(self, nodes, dimensions, WT_EXP=-1.8, WT_MAX=100., FL_EXP=-2.6, FL_MAX=50., border_policy='reflect'):
        '''
        This is a variant of the Truncated Levy Walk mobility model.
        This model is based in the Stochastic Walk.
        The waiting time distribution is a truncated power law with exponent set to WT_EXP and truncated WT_MAX.
        The flight length is a uniform distribution, different for each node. These uniform distributions are 
        created by taking both min and max values from a power law with exponent set to FL_EXP and truncated FL_MAX.
        The node velocity is a function of the flight length.
        
        Required arguments:
        
          *nr_nodes*:
            Integer, the number of nodes.
          
          *dimensions*:
            Tuple of Integers, the x and y dimensions of the simulation area.
          
        keyword arguments:
        
          *WT_EXP*:
            Double, the exponent of the waiting time distribution. Default is -1.8
            
          *WT_MAX*:
            Double, the maximum value of the waiting time distribution. Default is 100
        
          *FL_EXP*:
            Double, the exponent of the flight length distribution. Default is -2.6
            
          *FL_MAX*:
            Double, the maximum value of the flight length distribution. Default is 50
            
          *border_policy*:
            String, either 'reflect' or 'wrap'. The policy that is used when the node arrives to the border.
            If 'reflect', the node reflects off the border.
            If 'wrap', the node reappears at the opposite edge (as in a torus-shaped area).
        '''
        nr_nodes = len(nodes)
        NODES = np.arange(nr_nodes)
        FL_MAX = P(-1.8, 10., FL_MAX, NODES)
        FL_MIN = FL_MAX / 10.

        FL_DISTR = lambda SAMPLES: rand(len(SAMPLES)) * (FL_MAX[SAMPLES] - FL_MIN[SAMPLES]) + FL_MIN[SAMPLES]
        WT_DISTR = lambda SAMPLES: P(WT_EXP, 1., WT_MAX, SAMPLES)
        VELOCITY_DISTR = lambda FD: np.sqrt(FD) / 10.

        StochasticWalk.__init__(self, nr_nodes, dimensions, FL_DISTR, VELOCITY_DISTR, WT_DISTR=WT_DISTR, border_policy=border_policy)

def random_waypoint(*args, **kwargs):
    return iter(RandomWaypoint(*args, **kwargs))

def stochastic_walk(*args, **kwargs):
    return iter(StochasticWalk(*args, **kwargs))

def random_walk(*args, **kwargs):
    return iter(RandomWalk(*args, **kwargs))

def random_direction(*args, **kwargs):
    return iter(RandomDirection(*args, **kwargs))

def truncated_levy_walk(*args, **kwargs):
    return iter(TruncatedLevyWalk(*args, **kwargs))

def heterogeneous_truncated_levy_walk(*args, **kwargs):
    return iter(HeterogeneousTruncatedLevyWalk(*args, **kwargs))

def gauss_markov(nodes, velocity_mean=1., alpha=1., variance=1.):
    '''
    Gauss-Markov Mobility Model, as proposed in 
    Camp, T., Boleng, J. & Davies, V. A survey of mobility models for ad hoc network research. 
    Wireless Communications and Mobile Computing 2, 483-502 (2002).
    
    Required arguments:
    
      *nr_nodes*:
        Integer, the number of nodes.
      
    keyword arguments:
    
      *velocity_mean*:
        The mean velocity
        
      *alpha*:
        The tuning parameter used to vary the randomness
        
      *variance*:
        The randomness variance
    '''
    nr_nodes = len(nodes)
    NODES = np.arange(nr_nodes)

    max_x = U(0, 0, NODES)
    max_y = U(0, 0, NODES)
    min_x = U(0, 0, NODES)
    min_y = U(0, 0, NODES)

    MAX_X = max_x
    MAX_Y = max_y
    MIN_X = min_x
    MIN_Y = min_y

    for node in range(0, len(nodes)):
        MAX_X[node] = nodes[node].max_x
        MAX_Y[node] = nodes[node].max_y
        MIN_X[node] = nodes[node].min_x
        MIN_Y[node] = nodes[node].min_y

    x = U(MIN_X, MAX_X, NODES)
    y = U(MIN_Y, MAX_Y, NODES)
    velocity = np.zeros(nr_nodes) + velocity_mean
    theta = U(0, 2 * np.pi, NODES)
    angle_mean = theta
    alpha2 = 1.0 - alpha
    alpha3 = np.sqrt(1.0 - alpha * alpha) * variance

    while True:

        x = x + velocity * np.cos(theta)
        y = y + velocity * np.sin(theta)

        # node bounces on the margins
        b = np.where(x < MIN_X)[0]
        x[b] = 2 * MIN_X[b] - x[b]; theta[b] = np.pi - theta[b]; angle_mean[b] = np.pi - angle_mean[b]
        b = np.where(x > MAX_X)[0]
        x[b] = 2 * MAX_X[b] - x[b]; theta[b] = np.pi - theta[b]; angle_mean[b] = np.pi - angle_mean[b]
        b = np.where(y < MIN_Y)[0]
        y[b] = 2 * MIN_Y[b] - y[b]; theta[b] = -theta[b]; angle_mean[b] = -angle_mean[b]
        b = np.where(y > MAX_Y)[0]
        y[b] = 2 * MAX_Y[b] - y[b]; theta[b] = -theta[b]; angle_mean[b] = -angle_mean[b]
        # calculate new speed and direction based on the model
        velocity = (alpha * velocity +
                    alpha2 * velocity_mean +
                    alpha3 * np.random.normal(0.0, 1.0, nr_nodes))

        theta = (alpha * theta +
                    alpha2 * angle_mean +
                    alpha3 * np.random.normal(0.0, 1.0, nr_nodes))

        yield np.dstack((x, y))[0]

def reference_point_group(nodes, dimensions, velocity=(0.1, 1.), aggregation=0.1):
    '''
    Reference Point Group Mobility model, discussed in the following paper:
    
        Xiaoyan Hong, Mario Gerla, Guangyu Pei, and Ching-Chuan Chiang. 1999. 
        A group mobility model for ad hoc wireless networks. In Proceedings of the 
        2nd ACM international workshop on Modeling, analysis and simulation of 
        wireless and mobile systems (MSWiM '99). ACM, New York, NY, USA, 53-60.
    
    In this implementation, group trajectories follow a random direction model,
    while nodes follow a random walk around the group center.
    The parameter 'aggregation' controls how close the nodes are to the group center.
    
    Required arguments:
    
      *nr_nodes*:
        list of integers, the number of nodes in each group.
      
      *dimensions*:
        Tuple of Integers, the x and y dimensions of the simulation area.
        
    keyword arguments:
    
      *velocity*:
        Tuple of Doubles, the minimum and maximum values for group velocity.
        
      *aggregation*:
        Double, parameter (between 0 and 1) used to aggregate the nodes in the group.
        Usually between 0 and 1, the more this value approximates to 1,
        the nodes will be more aggregated and closer to the group center.
        With a value of 0, the nodes are randomly distributed in the simulation area.
        With a value of 1, the nodes are close to the group center.
    '''
    nr_nodes = len(nodes)
    try:
        iter(nr_nodes)
    except TypeError:
        nr_nodes = [nr_nodes]

    NODES = np.arange(sum(nr_nodes))

    groups = []
    prev = 0
    for (i, n) in enumerate(nr_nodes):
        groups.append(np.arange(prev, n + prev))
        prev += n

    g_ref = np.empty(sum(nr_nodes), dtype=np.int)
    for (i, g) in enumerate(groups):
        for n in g:
            g_ref[n] = i

    FL_MAX = max(dimensions)
    MIN_V, MAX_V = velocity
    FL_DISTR = lambda SAMPLES: U(0, FL_MAX, SAMPLES)
    VELOCITY_DISTR = lambda FD: U(MIN_V, MAX_V, FD)

    MAX_X, MAX_Y = dimensions
    x = U(0, MAX_X, NODES)
    y = U(0, MAX_Y, NODES)
    velocity = 1.
    theta = U(0, 2 * np.pi, NODES)
    costheta = np.cos(theta)
    sintheta = np.sin(theta)

    GROUPS = np.arange(len(groups))
    g_x = U(0, MAX_X, GROUPS)
    g_y = U(0, MAX_X, GROUPS)
    g_fl = FL_DISTR(GROUPS)
    g_velocity = VELOCITY_DISTR(g_fl)
    g_theta = U(0, 2 * np.pi, GROUPS)
    g_costheta = np.cos(g_theta)
    g_sintheta = np.sin(g_theta)

    while True:

        x = x + velocity * costheta
        y = y + velocity * sintheta

        g_x = g_x + g_velocity * g_costheta
        g_y = g_y + g_velocity * g_sintheta

        for (i, g) in enumerate(groups):

            # step to group direction + step to group center
            x_g = x[g]
            y_g = y[g]
            c_theta = np.arctan2(g_y[i] - y_g, g_x[i] - x_g)

            x[g] = x_g + g_velocity[i] * g_costheta[i] + aggregation * np.cos(c_theta)
            y[g] = y_g + g_velocity[i] * g_sintheta[i] + aggregation * np.sin(c_theta)

        # node and group bounces on the margins
        b = np.where(x < 0)[0]
        if b.size > 0:
            x[b] = -x[b]; costheta[b] = -costheta[b]
            g_idx = np.unique(g_ref[b]); g_costheta[g_idx] = -g_costheta[g_idx]
        b = np.where(x > MAX_X)[0]
        if b.size > 0:
            x[b] = 2 * MAX_X - x[b]; costheta[b] = -costheta[b]
            g_idx = np.unique(g_ref[b]); g_costheta[g_idx] = -g_costheta[g_idx]
        b = np.where(y < 0)[0]
        if b.size > 0:
            y[b] = -y[b]; sintheta[b] = -sintheta[b]
            g_idx = np.unique(g_ref[b]); g_sintheta[g_idx] = -g_sintheta[g_idx]
        b = np.where(y > MAX_Y)[0]
        if b.size > 0:
            y[b] = 2 * MAX_Y - y[b]; sintheta[b] = -sintheta[b]
            g_idx = np.unique(g_ref[b]); g_sintheta[g_idx] = -g_sintheta[g_idx]

        # update info for nodes
        theta = U(0, 2 * np.pi, NODES)
        costheta = np.cos(theta)
        sintheta = np.sin(theta)

        # update info for arrived groups
        g_fl = g_fl - g_velocity
        g_arrived = np.where(np.logical_and(g_velocity > 0., g_fl <= 0.))[0]

        if g_arrived.size > 0:
            g_theta = U(0, 2 * np.pi, g_arrived)
            g_costheta[g_arrived] = np.cos(g_theta)
            g_sintheta[g_arrived] = np.sin(g_theta)
            g_fl[g_arrived] = FL_DISTR(g_arrived)
            g_velocity[g_arrived] = VELOCITY_DISTR(g_fl[g_arrived])

        yield np.dstack((x, y))[0]

def tvc(nodes, dimensions, velocity=(0.1, 1.), aggregation=[0.5, 0.], epoch=[100, 100]):
    '''
    Time-variant Community Mobility Model, discussed in the paper
    
        Wei-jen Hsu, Thrasyvoulos Spyropoulos, Konstantinos Psounis, and Ahmed Helmy, 
        "Modeling Time-variant User Mobility in Wireless Mobile Networks," INFOCOM 2007, May 2007.
    
    This is a variant of the original definition, in the following way:
    - Communities don't have a specific area, but a reference point where the 
       community members aggregate around.
    - The community reference points are not static, but follow a random direction model.
    - You can define a list of epoch stages, each value is the duration of the stage.
       For each stage a different aggregation value is used (from the aggregation parameter).
    - Aggregation values should be doubles between 0 and 1.
       For aggregation 0, there's no attraction point and the nodes move in a random walk model.
       For aggregation near 1, the nodes move closer to the community reference point.
       
    Required arguments:
    
      *nr_nodes*:
        list of integers, the number of nodes in each group.
      
      *dimensions*:
        Tuple of Integers, the x and y dimensions of the simulation area.
        
    keyword arguments:
    
      *velocity*:
        Tuple of Doubles, the minimum and maximum values for community velocities.
        
      *aggregation*:
        List of Doubles, parameters (between 0 and 1) used to aggregate the nodes around the community center.
        Usually between 0 and 1, the more this value approximates to 1,
        the nodes will be more aggregated and closer to the group center.
        With aggregation 0, the nodes are randomly distributed in the simulation area.
        With aggregation near 1, the nodes are closer to the group center.
        
      *epoch*:
        List of Integers, the number of steps each epoch stage lasts.
    '''
    nr_nodes = len(nodes)
    if len(aggregation) != len(epoch):
        raise Exception("The parameters 'aggregation' and 'epoch' should be of same size")

    try:
        iter(nr_nodes)
    except TypeError:
        nr_nodes = [nr_nodes]

    NODES = np.arange(sum(nr_nodes))

    epoch_total = sum(epoch)

    def AGGREGATION(t):
        acc = 0
        for i in range(len(epoch)):
            acc += epoch[i]
            if t % epoch_total <= acc: return aggregation[i]
        raise Exception("Something wrong here")

    groups = []
    prev = 0
    for (i, n) in enumerate(nr_nodes):
        groups.append(np.arange(prev, n + prev))
        prev += n

    g_ref = np.empty(sum(nr_nodes), dtype=np.int)
    for (i, g) in enumerate(groups):
        for n in g:
            g_ref[n] = i

    FL_MAX = max(dimensions)
    MIN_V, MAX_V = velocity
    FL_DISTR = lambda SAMPLES: U(0, FL_MAX, SAMPLES)
    VELOCITY_DISTR = lambda FD: U(MIN_V, MAX_V, FD)

    def wrap(x, y):
        b = np.where(x < 0)[0]
        if b.size > 0:
            x[b] += MAX_X
        b = np.where(x > MAX_X)[0]
        if b.size > 0:
            x[b] -= MAX_X
        b = np.where(y < 0)[0]
        if b.size > 0:
            y[b] += MAX_Y
        b = np.where(y > MAX_Y)[0]
        if b.size > 0:
            y[b] -= MAX_Y

    MAX_X, MAX_Y = dimensions
    x = U(0, MAX_X, NODES)
    y = U(0, MAX_Y, NODES)
    velocity = 1.
    theta = U(0, 2 * np.pi, NODES)
    costheta = np.cos(theta)
    sintheta = np.sin(theta)

    GROUPS = np.arange(len(groups))
    g_x = U(0, MAX_X, GROUPS)
    g_y = U(0, MAX_X, GROUPS)
    g_fl = FL_DISTR(GROUPS)
    g_velocity = VELOCITY_DISTR(g_fl)
    g_theta = U(0, 2 * np.pi, GROUPS)
    g_costheta = np.cos(g_theta)
    g_sintheta = np.sin(g_theta)

    t = 0

    while True:

        t += 1
        # get aggregation value for this step
        aggr = AGGREGATION(t)

        x = x + velocity * costheta
        y = y + velocity * sintheta

        # move reference point only if nodes have to go there
        if aggr > 0:

            g_x = g_x + g_velocity * g_costheta
            g_y = g_y + g_velocity * g_sintheta

            # group wrap around when outside the margins (torus shaped area)
            wrap(g_x, g_y)

            # update info for arrived groups
            g_arrived = np.where(np.logical_and(g_velocity > 0., g_fl <= 0.))[0]
            g_fl = g_fl - g_velocity

            if g_arrived.size > 0:
                g_theta = U(0, 2 * np.pi, g_arrived)
                g_costheta[g_arrived] = np.cos(g_theta)
                g_sintheta[g_arrived] = np.sin(g_theta)
                g_fl[g_arrived] = FL_DISTR(g_arrived)
                g_velocity[g_arrived] = VELOCITY_DISTR(g_fl[g_arrived])

            # update node position according to group center
            for (i, g) in enumerate(groups):

                # step to group direction + step to reference point
                x_g = x[g]
                y_g = y[g]

                dy = g_y[i] - y_g
                dx = g_x[i] - x_g
                c_theta = np.arctan2(dy, dx)

                # invert angle if wrapping around
                invert = np.where((np.abs(dy) > MAX_Y / 2) != (np.abs(dx) > MAX_X / 2))[0]
                c_theta[invert] = c_theta[invert] + np.pi

                x[g] = x_g + g_velocity[i] * g_costheta[i] + aggr * np.cos(c_theta)
                y[g] = y_g + g_velocity[i] * g_sintheta[i] + aggr * np.sin(c_theta)

        # node wrap around when outside the margins (torus shaped area)
        wrap(x, y)

        # update info for nodes
        theta = U(0, 2 * np.pi, NODES)
        costheta = np.cos(theta)
        sintheta = np.sin(theta)

        yield np.dstack((x, y))[0]
