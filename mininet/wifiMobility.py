"""
Provides support to mobility

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""
import numpy as np
import time
import os

from mininet.log import debug, info
from mininet.wifiMobilityModels import gauss_markov, \
    truncated_levy_walk, random_direction, random_waypoint, random_walk, reference_point_group, tvc
from mininet.wifiChannel import setChannelParams
from mininet.wifiAssociationControl import associationControl
from mininet.wifiMeshRouting import listNodes, meshRouting
from mininet.wmediumdConnector import WmediumdServerConn
from mininet.wifiPlot import plot2d, plot3d
from mininet.link import Association

class mobility (object):
    """ Mobility """

    associationControlMethod = ''
    accessPoints = []
    stations = []
    mobilityNodes = []
    staticNodes = []
    plotNodes = []
    DRAW = False
    isMobility = False
    MAX_X = 0
    MAX_Y = 0
    MAX_Z = 0
    is3d = False
    continuePlot = 'plot2d.graphPause()'
    continueParams = 'time.sleep(0.001)'
    rec_rssi = False

    @classmethod
    def moveFactor(self, sta, diffTime):
        """
        :param sta: station
        :param diffTime: difference between start and stop time. Useful for calculating the speed
        """
        initialPosition = sta.params['initialPosition']
        finalPosition = sta.params['finalPosition']

        sta.params['position'] = initialPosition
        pos_x = float(finalPosition[0]) - float(initialPosition[0])
        pos_y = float(finalPosition[1]) - float(initialPosition[1])
        pos_z = float(finalPosition[2]) - float(initialPosition[2])

        self.nodeSpeed(sta, pos_x, pos_y, pos_z, diffTime)
        pos = '%.2f,%.2f,%.2f' % (pos_x / diffTime, pos_y / diffTime, pos_z / diffTime)
        pos = pos.split(',')
        sta.moveFac = pos

    @classmethod
    def nodeSpeed(self, sta, pos_x, pos_y, pos_z, diffTime):
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
            if 'encrypt' in ap.params:
                if ap.params['encrypt'][0] == 'wpa' or ap.params['encrypt'][0] == 'wpa2':
                    os.system('rm %s.staconf' % sta)
                    pidfile = "mn%d_%s_%s_wpa.pid" % (os.getpid(), sta.name, wlan)
                    os.system('pkill -f \'wpa_supplicant -B -Dnl80211 -P %s -i %s\'' % (pidfile, sta.params['wlan'][wlan]))
                    os.system('rm /var/run/wpa_supplicant/%s' % sta.params['wlan'][wlan])
            sta.pexec('iw dev %s disconnect' % sta.params['wlan'][wlan])
            if WmediumdServerConn.connected and WmediumdServerConn.interference_enabled and dist >= 0.01:
                """do nothing"""
            elif WmediumdServerConn.connected and dist >= 0.01:
                cls = Association
                cls.setSNRWmediumd(sta, ap, snr=-10)
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
        setChannelParams.recordParams(sta, ap)
            
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
            rssi_ = setChannelParams.setRSSI(sta, ap, wlan, dist)
            ap.params['stationsInRange'][sta] = rssi_
        else:
            rssi_ = setChannelParams.setRSSI(sta, ap, wlan, dist)
            ap.params['stationsInRange'][sta] = rssi_
        if ap == sta.params['associatedTo'][wlan]:
            if not WmediumdServerConn.interference_enabled:
                rssi_ = setChannelParams.setRSSI(sta, ap, wlan, dist)
                sta.params['rssi'][wlan] = rssi_
                snr_ = setChannelParams.setSNR(sta, wlan)
                sta.params['snr'][wlan] = snr_
            if sta not in ap.params['associatedStations']:
                ap.params['associatedStations'].append(sta)
            if dist >= 0.01:
                if WmediumdServerConn.connected:
                    if WmediumdServerConn.interference_enabled:
                        cls = Association
                        cls.setPositionWmediumd(sta)
                    else:
                        cls = Association
                        cls.setSNRWmediumd(sta, ap, snr=sta.params['snr'][wlan])
                else:
                    setChannelParams(sta, ap, wlan, dist)
        setChannelParams.recordParams(sta, ap)
                
    @classmethod
    def handoverCheck(self, sta, wlan):
        """ 
        handover check
        
        :param sta: station
        :param wlan: wlan ID
        """
        for ap in self.accessPoints:
            dist = setChannelParams.getDistance(sta, ap)
            if dist > ap.params['range']:
                self.apOutOfRange(sta, ap, wlan, dist)                   
            
        for ap in self.accessPoints:
            dist = setChannelParams.getDistance(sta, ap)
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
        if self.associationControlMethod != '' and sta.params['associatedTo'][wlan] != ap \
            and sta.params['associatedTo'][wlan] != '':
            ac = self.associationControlMethod
            value = associationControl(sta, ap, wlan, ac)
            changeAP = value.changeAP

        if sta.params['associatedTo'][wlan] == '' or changeAP == True:
            if ap not in sta.params['associatedTo']:
                cls = Association
                if 'encrypt' not in ap.params:
                    cls.associate_noEncrypt(sta, ap, wlan)
                else:
                    if ap.params['encrypt'][0] == 'wpa' or ap.params['encrypt'][0] == 'wpa2':
                        cls.associate_wpa(sta, ap, wlan)
                    elif ap.params['encrypt'][0] == 'wep':
                        cls.associate_wep(sta, ap, wlan)
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
            setChannelParams.recordParams(sta, sta.params['associatedTo'][wlan])
        cls = Association
        cls.updateParams(sta, ap, wlan)
        sta.params['associatedTo'][wlan] = ap
    
    @classmethod
    def definedPosition(self, init_time=0, final_time=0, stations=None, aps=None, dstConn=None, srcConn=None, 
                        plotNodes=None, MAX_X=0, MAX_Y=0, MAX_Z=0, AC='', rec_rssi=False):
        """ 
        Used when the position of each node is previously defined
        
        :param init_time: time when the mobility starts
        :param final_time: time when the mobility stops
        :param stations: list of stations
        :param aps: list of access points
        :param srcConn: list of connections for source nodes
        :param dstConn: list of connections for destination nodes
        :param plotnodes: list of nodes to be plotted (including hosts and switches)
        :param MAX_X: Maximum value for X
        :param MAX_Y: Maximum value for Y
        :param MAX_Z: Maximum value for Z
        :param AC: Association Control Method
        """
        self.rec_rssi = rec_rssi
        self.associationControlMethod = AC
        t_end = time.time() + final_time
        t_initial = time.time() + init_time
        currentTime = time.time()
        i = 1
        
        self.stations = stations
        self.accessPoints = aps
        
        nodes = self.stations + self.accessPoints + plotNodes
        
        for node in nodes:
            if 'position' in node.params and 'initialPosition' not in node.params:
                self.staticNodes.append(node)
            if 'initialPosition' in node.params:
                node.params['position'] = node.params['initialPosition']
                node.params.pop("initialPosition", None)
                node.params.pop("finalPosition", None)
                self.mobilityNodes.append(node)
                
        self.plotNodes = self.mobilityNodes + self.staticNodes

        try:
            if self.DRAW == True:
                if self.is3d:
                    plot = plot3d
                    plot.instantiateGraph(MAX_X, MAX_Y, MAX_Z)
                    plot.graphInstantiateNodes(self.plotNodes)
                else:
                    plot = plot2d
                    plot.instantiateGraph(MAX_X, MAX_Y)
                    plot.plotGraph(self.plotNodes, srcConn, dstConn)
        except:
            info('Warning: This OS does not support GUI. Running without GUI.\n')
            self.DRAW = False
        
        try:
            while True:
                if time.time() > t_end or time.time() < t_initial:
                    break
                if time.time() - currentTime >= i:
                    for node in self.mobilityNodes:
                        if time.time() - currentTime >= node.startTime and time.time() - currentTime <= node.endTime:
                            x = '%.2f' % (float(node.params['position'][0]) + float(node.moveFac[0]))
                            y = '%.2f' % (float(node.params['position'][1]) + float(node.moveFac[1]))
                            z = '%.2f' % (float(node.params['position'][2]) + float(node.moveFac[2]))
                            node.params['position'] = x, y, z
                        if self.DRAW:
                            eval(self.continuePlot)
                            plot.graphUpdate(node)
                        #self.parameters_(node)
                    i += 1
            self.mobilityNodes = []
        except:
            pass
        
    @classmethod
    def addNodes(self, stas, aps):
        self.stations = stas
        self.accessPoints = aps
        self.mobilityNodes = self.stations

    @classmethod
    def models(self, stations=None, aps=None, model=None, staMov=None, min_v=0, max_v=0, seed=None,
               dstConn=None, srcConn=None, plotNodes=None, MAX_X=0, MAX_Y=0, AC='', rec_rssi=False):
        """ 
        Used when a mobility model is applied
        
        :param stations: list of stations
        :param aps: list of access points
        :param model: mobility model
        :param staMov: list of nodes with mobility
        :param min_v: minimum velocity
        :param max_v: maximum velocity
        :param speed: speed
        :param srcConn:  list of connections for source nodes
        :param dstConn:  list of connections for destination nodes
        :param plotNodes: list of nodes to be plotted (including hosts and switches)
        :param MAX_X: Maximum value for X
        :param MAX_Y: Maximum value for Y
        """
        self.rec_rssi = rec_rssi
        np.random.seed(seed)
        
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
            if self.DRAW:
                plot2d.instantiateGraph(MAX_X, MAX_Y)
                plot2d.plotGraph(nodes, srcConn, dstConn)
                plot2d.graphPause()
        except:
            info('Warning: This OS does not support GUI. Running without GUI.\n')
            self.DRAW = False

        if staMov != None:            
            debug('Configuring the mobility model %s' % model)
    
            if(model == 'RandomWalk'):  # Random Walk model
                mob = random_walk(staMov)
            elif(model == 'TruncatedLevyWalk'):  # Truncated Levy Walk model
                mob = truncated_levy_walk(staMov)
            elif(model == 'RandomDirection'):  # Random Direction model
                mob = random_direction(staMov, dimensions=(MAX_X, MAX_Y))
            elif(model == 'RandomWayPoint'):  # Random Waypoint model
                mob = random_waypoint(staMov, wt_max=MAX_WT)
            elif(model == 'GaussMarkov'):  # Gauss-Markov model
                mob = gauss_markov(staMov, alpha=0.99)
            elif(model == 'ReferencePoint'):  # Reference Point Group model
                mob = reference_point_group(staMov, dimensions=(MAX_X, MAX_Y), aggregation=0.5)
            elif(model == 'TimeVariantCommunity'):  # Time-variant Community Mobility Model
                mob = tvc(staMov, dimensions=(MAX_X, MAX_Y), aggregation=[0.5, 0.], epoch=[100, 100])
            else:
                raise Exception("Mobility Model not defined or doesn't exist!")
            
            if self.DRAW:
                self.startMobilityModelGraph(mob, staMov)
            else:
                self.startMobilityModelNoGraph(mob, staMov)
    
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
                i += 1
            time.sleep(0.5)
            
    @classmethod
    def parameters_(self, node=None):
        """ 
        have to check it!
        Applies channel params and handover
        """
        if node == None:
            nodes = self.stations 
        else:
            nodes = []
            nodes.append(node)  
        for node_ in self.accessPoints:
            if 'link' in node_.params and node_.params['link'] == 'mesh':
                nodes.append(node_)

        for node in nodes:
            for wlan in range(0, len(node.params['wlan'])):
                if node.func[wlan] == 'mesh' or node.func[wlan] == 'adhoc':
                    if node.type == 'vehicle':
                        node = node.params['carsta']
                        wlan = 0
                    dist = listNodes.pairingNodes(node, wlan, nodes)
                    if WmediumdServerConn.connected == False and dist >= 0.01:
                        setChannelParams(sta=node, wlan=wlan, dist=dist)
                else:
                    self.handoverCheck(node, wlan)
        if meshRouting.routing == 'custom':
            meshRouting(nodes)
        # have to verify this
        eval(self.continueParams)        
    
    @classmethod
    def parameters(self):
        """ 
        Applies channel params and handover
        """
        meshNodes = []
        for node in self.mobilityNodes:
            for wlan in range(0, len(node.params['wlan'])):
                if node.func[wlan] == 'mesh' or node.func[wlan] == 'adhoc':
                    meshNodes.append(node)
        
        while True:
            for node in self.mobilityNodes:
                for wlan in range(0, len(node.params['wlan'])):
                    if node.func[wlan] == 'mesh' or node.func[wlan] == 'adhoc':
                        if node.type == 'vehicle':
                            node = node.params['carsta']
                            wlan = 0
                        dist = listNodes.pairingNodes(node, wlan, meshNodes)
                        if WmediumdServerConn.connected == False and dist >= 0.01:
                            setChannelParams(sta=node, wlan=wlan, dist=dist)
                    else:
                        self.handoverCheck(node, wlan)
            if meshRouting.routing == 'custom':
                meshRouting(meshNodes)
            # have to verify this
            eval(self.continueParams)
