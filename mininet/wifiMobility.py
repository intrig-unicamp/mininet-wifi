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
from mininet.wifiChannel import channelParameters
from mininet.wifiAssociationControl import associationControl
from mininet.wifiMeshRouting import listNodes, meshRouting
from mininet.wifiPlot import plot

class mobility (object):
    """ Mobility """

    MAX_X = 50
    MAX_Y = 50
    moveFac = {}
    initialPosition = {}
    finalPosition = {}
    associationControlMethod = False
    apList = []
    staList = []
    wallList = []
    dic = dict()
    DRAW = False
    continue_ = True
    isMobility = False

    @classmethod
    def moveFactor(self, sta, diffTime, initialPosition, finalPosition):
        """
            Moving nodes
            diffTime: important to calculate the speed  
        """
        self.initialPosition[sta] = initialPosition
        self.finalPosition[sta] = finalPosition

        sta.params['position'] = initialPosition
        pos_x = float(finalPosition[0]) - float(initialPosition[0])
        pos_y = float(finalPosition[1]) - float(initialPosition[1])
        pos_z = float(finalPosition[2]) - float(initialPosition[2])

        self.nodeSpeed(sta, pos_x, pos_y, pos_z, diffTime)

        pos = '%.5f,%.5f,%.5f' % (pos_x / diffTime, pos_y / diffTime, pos_z / diffTime)
        pos = pos.split(',')
        self.moveFac[sta] = pos
        return pos

    @classmethod
    def nodeSpeed(self, sta, pos_x, pos_y, pos_z, diffTime):
        sta.params['speed'] = abs(((pos_x + pos_y + pos_z) / diffTime))

    @classmethod
    def handover(self, sta, ap, wlan, distance, changeAP, ac=None):
        """handover"""
        if ac == 'llf' or ac == 'ssf' and sta.params['associatedTo'][wlan] != ap:
            if sta.params['associatedTo'][wlan] != '':
                sta.params['associatedTo'][wlan].params['associatedStations'].remove(sta)
            sta.pexec('iw dev %s disconnect' % sta.params['wlan'][wlan])
            debug ('\niwconfig %s essid %s ap %s' % (sta.params['wlan'][wlan], ap.params['ssid'][0], ap.params['mac'][wlan]))
            sta.pexec('iwconfig %s essid %s ap %s' % (sta.params['wlan'][wlan], ap.params['ssid'][0], ap.params['mac'][wlan]))
            sta.params['associatedTo'][wlan] = ap
            sta.params['frequency'][wlan] = channelParameters.frequency(ap, 0)
            ap.params['associatedStations'].append(sta)
        elif ap not in sta.params['associatedTo']:
            # Useful for stations with more than one wifi iface
            if sta.params['associatedTo'][wlan] == '':
                if 'encrypt' not in ap.params:
                    debug('\niwconfig %s essid %s ap %s' % (sta.params['wlan'][wlan], ap.params['ssid'][0], ap.params['mac'][wlan]))
                    sta.pexec('iwconfig %s essid %s ap %s' % (sta.params['wlan'][wlan], ap.params['ssid'][0], ap.params['mac'][wlan]))
                else:
                    if ap.encrypt == 'wpa' or ap.encrypt == 'wpa2':
                        os.system('pkill -f \'wpa_supplicant -B -Dnl80211 -i %s\'' % sta.params['wlan'][wlan])
                        debug("\nwpa_supplicant -B -Dnl80211 -i %s -c <(wpa_passphrase \"%s\" \"%s\")\n" \
                                                             % (sta.params['wlan'][wlan], ap.params['ssid'][0], sta.params['passwd'][0]))
                        sta.cmd("wpa_supplicant -B -Dnl80211 -i %s -c <(wpa_passphrase \"%s\" \"%s\")" \
                                                             % (sta.params['wlan'][wlan], ap.params['ssid'][0], sta.params['passwd'][0]))
                    elif ap.encrypt == 'wep':
                        debug('iw dev %s connect %s key d:0:%s' \
                                                                % (sta.params['wlan'][wlan], ap.params['ssid'][0], sta.params['passwd'][0]))
                        sta.cmd('iw dev %s connect %s key d:0:%s' \
                                                            % (sta.params['wlan'][wlan], ap.params['ssid'][0], sta.params['passwd'][0]))
                sta.params['frequency'][wlan] = channelParameters.frequency(ap, 0)
                ap.params['associatedStations'].append(sta)
                sta.params['associatedTo'][wlan] = ap                  

    @classmethod
    def mobilityPositionDefined(self, initial_time, final_time):
        """ ongoing Mobility """
        t_end = time.time() + final_time
        t_initial = time.time() + initial_time
        currentTime = time.time()
        i = 1
        nodes = self.staList + self.apList

        dic = dict()
        dic['max_x'] = self.MAX_X
        dic['max_y'] = self.MAX_Y
        
        staMov = []
        for sta in self.staList:
            if sta in self.finalPosition:
                staMov.append(sta)

        if self.DRAW == True:
            plot.instantiateGraph(self.MAX_X, self.MAX_Y)
            plot.plotGraph(nodes, mobility.wallList, staMov, **dic)

        try:
            while time.time() < t_end and time.time() > t_initial:
                if self.continue_:
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
                                plot.graphPause()
                                plot.graphUpdate(sta)
                        i += 1
        except:
            info('Error! Mobility stopped!\n')

    @classmethod
    def models(self, nodes=None, model=None, staMov=None, min_v=0, max_v=0, seed=None,
               **mobilityparam):

        np.random.seed(seed)

        # simulation area (units)
        MAX_X, MAX_Y = self.MAX_X, self.MAX_Y

        dic = dict()
        dic['max_x'] = MAX_X
        dic['max_y'] = MAX_Y

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

        debug('Setting the mobility model %s' % model)

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
            plot.instantiateGraph(self.MAX_X, self.MAX_Y)
            plot.plotGraph(nodes, mobility.wallList, staMov, **dic)

        for xy in mob:
            i = 0
            for n in range (0, len(nodes)):
                node = nodes[n]
                if node in staMov:
                    if 'station' == node.type:
                        node.params['position'] = xy[i][0], xy[i][1], 0
                        i += 1
                        if self.DRAW:
                            plot.pltNode[node].set_data(xy[:, 0], xy[:, 1])
                            plot.drawTxt(node)
                            plot.drawCircle(node)
            if self.DRAW:
                plot.graphUpdate(node)
                plot.graphPause()
                
    @classmethod
    def getAPsInRange(self, sta):
        for ap in mobility.apList:
            if 'position' not in sta.params:
                sta.params['position'] = 0,0,0
            dist = channelParameters.getDistance(sta, ap)
            if dist < ap.params['range']:
                if ap not in sta.params['apsInRange']:
                    sta.params['apsInRange'].append(ap)
            else:
                if ap in sta.params['apsInRange']:
                    sta.params['apsInRange'].remove(ap)

    @classmethod
    def nodeParameter(self, sta, wlan):
        for ap in mobility.apList:
            dist = channelParameters.getDistance(sta, ap)
            self.getAPsInRange(sta)
            self.setChannelParameters(sta, ap, dist, wlan)

    @classmethod
    def parameters(self):
        while self.continue_:
            listNodes.ssid_ID = 0
            for node in self.staList:
                for wlan in range(0, len(node.params['wlan'])):
                    if node.func[wlan] != 'mesh' and node.func[wlan] != 'adhoc':
                        self.nodeParameter(node, wlan)
                    elif node.func[wlan] == 'mesh' :
                        dist = listNodes.pairingNodes(node, wlan, self.staList)
                        if dist != 0:
                            channelParameters(node, None, wlan, dist, self.staList, 0)
                    else:
                        if dist != 0:
                            channelParameters(node, None, wlan, dist, self.staList, 0)
            if meshRouting.routing == 'custom':
                for node in mobility.staList:
                    for wlan in range(0, len(node.params['wlan'])):
                        if node.func[wlan] == 'mesh':
                            """Mesh Routing"""
                            try:
                                meshRouting.customMeshRouting(node, wlan, self.staList)
                            except:
                                pass
                listNodes.clearList()
            # have to verify this
            # time.sleep(0.01)

    @classmethod
    def setChannelParameters(self, sta, ap, dist, wlan):
        """ Wifi Parameters """
        associated = True
        # time = abs(sta.params['speed'])
        staList = self.staList

        if ap == sta.params['associatedTo'][wlan]:
            if dist > ap.params['range']:
                debug('\niw dev %s disconnect' % sta.params['wlan'][wlan])
                sta.pexec('iw dev %s disconnect' % sta.params['wlan'][wlan])
                sta.params['associatedTo'][wlan] = ''
                sta.params['rssi'][wlan] = 0
                ap.params['associatedStations'].remove(sta)
            else:
                channelParameters(sta, ap, wlan, dist, staList, 0)
        else:
            if dist < ap.params['range']:
                if sta.params['associatedTo'][wlan] == '':
                    associated = False
            else:
                associated = False
        if ap == sta.params['associatedTo'][wlan] or dist < ap.params['range']:
            changeAP = False
            ac = None
            sta.params['frequency'][wlan] = channelParameters.frequency(ap, 0)
            sta.params['channel'][wlan] = ap.params['channel'][0]

            """Association Control: mechanisms that optimize the use of the APs"""
            if self.associationControlMethod != False:
                ac = self.associationControlMethod
                value = associationControl(sta, ap, wlan, ac)
                changeAP = value.changeAP

            # Go to handover
            if associated == False or changeAP == True:
                self.handover(sta, ap, wlan, dist, changeAP, ac)
                channelParameters(sta, ap, wlan, dist, staList, 0)
        # have to verify this
        time.sleep(0.01)
