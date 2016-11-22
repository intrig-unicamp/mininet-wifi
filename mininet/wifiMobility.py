"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""
import numpy as np
import time
import os

from mininet.log import debug, info
from mininet.wifiMobilityModels import gauss_markov, \
    truncated_levy_walk, random_direction, random_waypoint, random_walk, reference_point_group, tvc
from mininet.wifiChannel import channelParams, setAdhocChannelParams, setInfraChannelParams
from mininet.wifiAssociationControl import associationControl
from mininet.wifiMeshRouting import listNodes, meshRouting
from mininet.wifiPlot import plot

class mobility (object):
    """ Mobility """

    moveFac = {}
    associationControlMethod = False
    apList = []
    staList = []
    wallList = []
    DRAW = False
    continue_ = True
    isMobility = False
    MAX_X = 0
    MAX_Y = 0
    continuePlot = 'plot.graphPause()'

    @classmethod
    def moveFactor(self, sta, diffTime):
        """
            diffTime: important to calculate the speed  
        """
        initialPosition = sta.params['initialPosition']
        finalPosition = sta.params['finalPosition']

        sta.params['position'] = initialPosition
        pos_x = float(finalPosition[0]) - float(initialPosition[0])
        pos_y = float(finalPosition[1]) - float(initialPosition[1])
        pos_z = float(finalPosition[2]) - float(initialPosition[2])

        self.nodeSpeed(sta, pos_x, pos_y, pos_z, diffTime)
        pos = '%.5f,%.5f,%.5f' % (pos_x / diffTime, pos_y / diffTime, pos_z / diffTime)
        pos = pos.split(',')
        self.moveFac[sta] = pos

    @classmethod
    def nodeSpeed(self, sta, pos_x, pos_y, pos_z, diffTime):
        sta.params['speed'] = abs(((pos_x + pos_y + pos_z) / diffTime))

    @classmethod
    def handover(self, sta, ap, wlan, dist):
        """handover"""
        associated = True        
        
        if dist > ap.params['range']:
            if ap == sta.params['associatedTo'][wlan]:
                sta.pexec('iw dev %s disconnect' % sta.params['wlan'][wlan])
                sta.params['associatedTo'][wlan] = ''
                sta.params['rssi'][wlan] = 0
                ap.params['associatedStations'].remove(sta)
        else:
            if dist >= 0.01 and sta.params['associatedTo'][wlan] == ap:
                self.updateParams(sta, ap, wlan)
                setInfraChannelParams(sta, ap, wlan, dist, self.staList)
            
            if sta.params['associatedTo'][wlan] == '':        
                associated = False        
            else:        
                associated = False
    
            if ap == sta.params['associatedTo'][wlan] or dist <= ap.params['range']:
                changeAP = False
                """Association Control: mechanisms that optimize the use of the APs"""
                if self.associationControlMethod != False and sta.params['associatedTo'][wlan] != '':
                    ac = self.associationControlMethod
                    value = associationControl(sta, ap, wlan, ac)
                    changeAP = value.changeAP
    
                # Go to handover
                if associated == False or changeAP == True:
                    if ap not in sta.params['associatedTo']:
                        if sta.params['associatedTo'][wlan] == '' or changeAP == True:
                            if 'encrypt' not in ap.params:
                                self.associate_infra(sta, ap, wlan)
                            else:
                                if ap.params['encrypt'][0] == 'wpa' or ap.params['encrypt'][0] == 'wpa2':
                                    self.associate_wpa(sta, ap, wlan)
                                elif ap.params['encrypt'][0] == 'wep':
                                    self.associate_wep(sta, ap, wlan)
                            if dist >= 0.01 and sta.params['associatedTo'][wlan] != '':
                                setInfraChannelParams(sta, ap, wlan, dist, self.staList)
        
    @classmethod
    def verifyPasswd(self, sta, ap, wlan):
        if 'passwd' not in sta.params:
            passwd = ap.params['passwd'][0]
        else:
            passwd = sta.params['passwd'][wlan]
        return passwd
    
    @classmethod     
    def updateParams(self, sta, ap, wlan):
        sta.params['frequency'][wlan] = channelParams.frequency(ap, 0)
        sta.params['channel'][wlan] = ap.params['channel'][0]
    
    @classmethod     
    def updateAssociation(self, sta, ap, wlan):
        self.updateParams(sta, ap, wlan)
        sta.params['associatedTo'][wlan] = ap 
        ap.params['associatedStations'].append(sta)        
            
    @classmethod
    def associate_wep(self, sta, ap, wlan):
        passwd = self.verifyPasswd(sta, ap, wlan)
        sta.pexec('iw dev %s connect %s key d:0:%s' % (sta.params['wlan'][wlan], ap.params['ssid'][0], passwd))
        self.updateAssociation(sta, ap, wlan)
        
    @classmethod
    def associate_infra(self, sta, ap, wlan):
        debug('\niwconfig %s essid %s ap %s' % (sta.params['wlan'][wlan], ap.params['ssid'][0], ap.params['mac'][0]))
        sta.pexec('iwconfig %s essid %s ap %s' % (sta.params['wlan'][wlan], ap.params['ssid'][0], ap.params['mac'][0]))
        self.updateAssociation(sta, ap, wlan)
        
    @classmethod
    def associate_wpa(self, sta, ap, wlan):
        passwd = self.verifyPasswd(sta, ap, wlan)
        os.system('pkill -f \'wpa_supplicant -B -Dnl80211 -i %s\'' % sta.params['wlan'][wlan])
        sta.cmd("wpa_supplicant -B -Dnl80211 -i %s -c <(wpa_passphrase \"%s\" \"%s\")" \
                                             % (sta.params['wlan'][wlan], ap.params['ssid'][0], passwd))
        self.updateAssociation(sta, ap, wlan)

    @classmethod
    def positionDefined(self, init_time=0, final_time=0, stations=None, aps=None, walls=None, staMov=None,
                                dstConn=None, srcConn=None, plotnodes=None, MAX_X=0, MAX_Y=0):
        """ ongoing Mobility """
        t_end = time.time() + final_time
        t_initial = time.time() + init_time
        currentTime = time.time()
        i = 1
        
        self.staList = stations
        self.apList = aps
        self.wallList = walls
        nodes = self.staList + self.apList + plotnodes

        if self.DRAW == True:
            plot.instantiateGraph(MAX_X, MAX_Y)
            plot.plotGraph(nodes, srcConn, dstConn, MAX_X, MAX_Y)

        try:
            while time.time() < t_end and time.time() > t_initial:
                if time.time() - currentTime >= i:
                    for sta in staMov:
                        if time.time() - currentTime >= sta.startTime and time.time() - currentTime <= sta.endTime:
                            x = float(sta.params['position'][0]) + float(self.moveFac[sta][0])
                            y = float(sta.params['position'][1]) + float(self.moveFac[sta][1])
                            z = float(sta.params['position'][2]) + float(self.moveFac[sta][2])
                            sta.params['position'] = x, y, z
                        for wlan in range(0, len(sta.params['wlan'])):
                            self.nodeParameter(sta, wlan)
                        if self.DRAW:
                            eval(self.continuePlot)
                            plot.graphUpdate(sta)
                    i += 1
        except:
            pass

    @classmethod
    def models(self, model=None, staMov=None, min_v=0, max_v=0, seed=None, stations=None, aps=None,
               dstConn=None, srcConn=None, walls=None, plotNodes=None, MAX_X=0, MAX_Y=0):

        np.random.seed(seed)
        
        self.staList = stations
        self.apList = aps
        self.wallList = walls
        nodes = self.staList + self.apList + plotNodes

        # max waiting time
        MAX_WT = 100.

        for sta in self.staList:
            if sta.max_x == 0:
                sta.max_x = MAX_X
            if sta.max_y == 0:
                sta.max_y = MAX_Y
            if sta.max_v == 0:
                sta.max_v = max_v
            if sta.min_v == 0:
                sta.min_v = min_v

        if self.DRAW:
            plot.instantiateGraph(MAX_X, MAX_Y)
            plot.plotGraph(nodes, srcConn, dstConn, MAX_X, MAX_Y)
            plot.graphPause()

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
                raise Exception("Model not defined!")
            
            if self.DRAW:
                self.startMobilityModelGraph(mob, staMov)
            else:
                self.startMobilityModelNoGraph(mob, staMov)
    
    @classmethod
    def startMobilityModelGraph(self, mob, nodes):
        for xy in mob:
            i = 0
            for node in nodes:
                node.params['position'] = xy[i][0], xy[i][1], 0
                i += 1
                plot.graphUpdate(node)
            eval(self.continuePlot)
                
    @classmethod    
    def startMobilityModelNoGraph(self, mob, nodes):
        for xy in mob:
            i = 0
            for node in nodes:
                node.params['position'] = xy[i][0], xy[i][1], 0
                i += 1
            time.sleep(0.5)
                
    @classmethod
    def getAPsInRange(self, sta):
        for ap in self.apList:
            dist = channelParams.getDistance(sta, ap)
            if dist < ap.params['range']:
                if ap not in sta.params['apsInRange']:
                    sta.params['apsInRange'].append(ap)
            else:
                if ap in sta.params['apsInRange']:
                    sta.params['apsInRange'].remove(ap)

    @classmethod
    def nodeParameter(self, sta, wlan):
        for ap in self.apList:
            dist = channelParams.getDistance(sta, ap)
            self.getAPsInRange(sta)
            self.handover(sta, ap, wlan, dist)

    @classmethod
    def parameters(self):
        while self.continue_:
            for node in self.staList:
                for wlan in range(0, len(node.params['wlan'])):
                    if node.func[wlan] == 'mesh' or node.func[wlan] == 'adhoc':
                        if node.type == 'vehicle':
                            node = node.params['carsta']
                            wlan = 0
                        dist = listNodes.pairingNodes(node, wlan, self.staList)
                        if dist >= 0.01:
                            setAdhocChannelParams(node, wlan, dist, self.staList)
                    else:
                        self.nodeParameter(node, wlan)
            if meshRouting.routing == 'custom':
                meshRouting(self.staList)
            # have to verify this
            time.sleep(0.001)
