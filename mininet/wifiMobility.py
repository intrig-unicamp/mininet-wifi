"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""
import matplotlib.pyplot as plt
import numpy as np
import time
import subprocess

from mininet.log import debug
from mininet.wifiMobilityModels import gauss_markov, \
    truncated_levy_walk, random_direction, random_waypoint, random_walk, reference_point_group, tvc
from mininet.wifiChannel import channelParameters
from mininet.wifiAssociationControl import associationControl
from mininet.wifiMeshRouting import listNodes, meshRouting
from mininet.wifiPlot import plot

        
class mobility ( object ):    
    """ Mobility """          
    
    MAX_X = 50
    MAX_Y = 50
    moveFac = {}
    initialPosition = {}
    finalPosition = {}
    associationControlMethod = False
    apList = []
    staList = []
    DRAW = False
    continue_ = True
    
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
        
        pos = '%.5f,%.5f,%.5f' % (pos_x/diffTime, pos_y/diffTime, pos_z/diffTime)
        pos = pos.split(',')
        self.moveFac[sta] = pos
        return pos    
        
    @classmethod  
    def nodeSpeed(self, sta, pos_x, pos_y, pos_z, diffTime):
        sta.speed = ((pos_x + pos_y + pos_z)/diffTime) 
  
    @classmethod  
    def handover(self, sta, ap, wlan, distance, changeAP, ac=None, **params):
        """handover"""
        if ac == 'llf' or ac == 'ssf':
            sta.pexec('iw dev %s-wlan%s disconnect' % (sta, wlan))
            #sta.pexec('iw dev %s-wlan%s connect %s' % (sta, wlan, ap.ssid[0]))
            sta.pexec('iwconfig %s-wlan%s essid %s ap %s' % (sta, wlan, ap.ssid[0], ap.mac))
            sta.associatedAp[wlan] = ap
        elif ap not in sta.associatedAp:
            #Useful for stations with more than one wifi iface
            if sta.associatedAp[wlan] == 'NoAssociated':
                #sta.pexec('iw dev %s-wlan%s connect %s' % (sta, wlan, ap.ssid[0]))
                sta.pexec('iwconfig %s-wlan%s essid %s ap %s' % (sta, wlan, ap.ssid[0], ap.mac))
                ap.associatedStations.append(sta)
                sta.associatedAp[wlan] = ap        
        self.numberOfAssociatedStations(ap)
        
    @classmethod 
    def graphInstantiateNodes(self, node):
        plot.instantiateAnnotate(node)
        plot.instantiateCircle(node)
        plot.instantiateNode(node, self.MAX_X, self.MAX_Y)
        plot.graphUpdate(node)  
            
    @classmethod
    def numberOfAssociatedStations( self, ap ):
        "Number of Associated Stations"
        cmd = 'iw dev %s-wlan0 station dump | grep Sta | grep -c ^' % ap     
        proc = subprocess.Popen("exec " + cmd, stdout=subprocess.PIPE,shell=True)   
        (out, err) = proc.communicate()
        output = out.rstrip('\n')
        ap.nAssociatedStations = int(output)        
    
    @classmethod 
    def mobilityPositionDefined(self, initial_time, final_time, staMov):
        """ ongoing Mobility """        
        t_end = time.time() + final_time
        t_initial = time.time() + initial_time
        currentTime = time.time()
        i=1
        
        if self.DRAW == True:
            debug('Enabling Graph...\n')
            plot.instantiateGraph(self.MAX_X, self.MAX_Y)
            for sta in self.staList:
                self.graphInstantiateNodes(sta)
                if sta not in staMov:
                    plot.pltNode[sta].set_data(sta.params['position'][0],sta.params['position'][1])
                    plot.drawTxt(sta)
                    plot.drawCircle(sta)   
            for ap in mobility.apList:
                self.graphInstantiateNodes(ap)  
                for c in ap.connections:
                    line = plot.plotLine2d([ap.connections[c].params['position'][0],ap.params['position'][0]], \
                                           [ap.connections[c].params['position'][1],ap.params['position'][1]], 'b')
                    plot.plotLine(line)
        try:
            while time.time() < t_end and time.time() > t_initial:
                if self.continue_:
                    if time.time() - currentTime >= i:
                        for sta in self.staList:
                            if time.time() - currentTime >= sta.startTime and time.time() - currentTime <= sta.endTime:
                                x = float(sta.params['position'][0]) + float(self.moveFac[sta][0])
                                y = float(sta.params['position'][1]) + float(self.moveFac[sta][1])
                                z = float(sta.params['position'][2]) + float(self.moveFac[sta][2])
                                sta.params['position'] = x, y, z
                            for wlan in range(0, sta.nWlans):
                                self.nodeParameter(sta, wlan) 
                            if self.DRAW:
                                plot.graphUpdate(sta)
                        i+=1
        except:
            print 'Error! Mobility stopped!'        
    
    @classmethod   
    def models(self, nodes=None, model=None, max_x=None, max_y=None, min_v=None, 
               max_v=None, seed=None, staMov=None, **mobilityparam):
        
        self.modelName = model
        np.random.seed(seed)
        
        # number of nodes
        nr_nodes = len(staMov)
        
        # simulation area (units)
        MAX_X, MAX_Y = max_x, max_y
        
        # max and min velocity
        MIN_V, MAX_V = min_v, max_v
        
        # max waiting time
        MAX_WT = 100.
        
        if(self.modelName=='RandomWalk'): ## Random Walk model            
            mob = random_walk(nr_nodes, dimensions=(MAX_X, MAX_Y)) 
        elif(self.modelName=='TruncatedLevyWalk'): ## Truncated Levy Walk model
            mob = truncated_levy_walk(nr_nodes, dimensions=(MAX_X, MAX_Y)) 
        elif(self.modelName=='RandomDirection'): ## Random Direction model            
            mob = random_direction(nr_nodes, dimensions=(MAX_X, MAX_Y), velocity=(MIN_V, MAX_V)) 
        elif(self.modelName=='RandomWayPoint'): ## Random Waypoint model           
            mob = random_waypoint(nr_nodes, dimensions=(MAX_X, MAX_Y), velocity=(MIN_V, MAX_V), wt_max=MAX_WT) 
        elif(self.modelName=='GaussMarkov'): ## Gauss-Markov model           
            mob = gauss_markov(nr_nodes, dimensions=(MAX_X, MAX_Y), alpha=0.99) 
        elif(self.modelName=='ReferencePoint'): ## Reference Point Group model           
            mob = reference_point_group(nr_nodes, dimensions=(MAX_X, MAX_Y), aggregation=0.5) 
        elif(self.modelName=='TimeVariantCommunity'): ## Time-variant Community Mobility Model            
            mob = tvc(nr_nodes, dimensions=(MAX_X, MAX_Y), aggregation=[0.5,0.], epoch=[100,100])        
        else:
            print 'Model not defined!'

        if self.DRAW:
            debug('Enabling Graph...\n')
            plot.instantiateGraph(self.MAX_X, self.MAX_Y)
            for node in nodes:
                self.graphInstantiateNodes(node)
                if node not in staMov or 'accessPoint' == node.type:
                    plot.pltNode[node].set_data(node.params['position'][0],node.params['position'][1])
                    plot.drawTxt(node)
                    plot.drawCircle(node)  
                    if node.type == 'accessPoint': 
                        for c in node.connections:
                            line = plot.plotLine2d([node.connections[c].params['position'][0],node.params['position'][0]], \
                                                   [node.connections[c].params['position'][1],node.params['position'][1]], 'b')
                            plot.plotLine(line)
                            
        #Sometimes getting the error: Failed to connect to generic netlink.
        try:
            if model!='':
                for xy in mob:              
                    i = 0  
                    for n in range (0,len(nodes)):
                        node = nodes[n]
                        if node in staMov:
                            if 'station' == node.type:
                                node.params['position'] = xy[i][0], xy[i][1], 0
                                i += 1                       
                                if self.DRAW:
                                    plot.pltNode[node].set_data(xy[:,0],xy[:,1])
                                    plot.drawTxt(node)
                                    plot.drawCircle(node)
                            #self.parameters()
                    if self.DRAW:
                            plt.title("Mininet-WiFi Graph")
                            plt.draw()   
        except:
            pass               
    
    @classmethod 
    def getAPsInRange(self, sta):
        for ap in mobility.apList:
            dist = channelParameters.getDistance(sta, ap)
            if dist < ap.range + sta.range:
                if ap not in sta.inRangeAPs:
                    sta.inRangeAPs.append(ap)
            else:
                if ap in sta.inRangeAPs:
                    sta.inRangeAPs.remove(ap)

    @classmethod 
    def nodeParameter(self, sta, wlan):
        for ap in mobility.apList:
            if 'wlan' not in ap.params:
                dist = channelParameters.getDistance(sta, ap)
                self.getAPsInRange(sta)
                self.setChannelParameters(sta, ap, dist, wlan)  
                            
    @classmethod                
    def parameters(self):
        while self.continue_:
            try:
                for node in self.staList: 
                    for wlan in range(0, node.nWlans):
                        if node.func[wlan] == 'mesh' or node.func[wlan] == 'adhoc':
                            dist = listNodes.pairingNodes(node, wlan, self.staList)
                            if dist!=0:
                                channelParameters(node, None, wlan, dist, self.staList, abs(node.speed))
                        else:
                            self.nodeParameter(node, wlan)
                if meshRouting.routing == 'custom':
                    for node in mobility.staList:       
                        for wlan in range(0, node.nWlans):
                            if node.func[wlan] == 'mesh':
                                """Mesh Routing"""                    
                                meshRouting.customMeshRouting(node, wlan, self.staList)    
                    listNodes.clearList()
            except:
                pass
    
    @classmethod    
    def setChannelParameters(self, sta, ap, dist, wlan):
        """ Wifi Parameters """
        associated = True
        time = abs(sta.speed)
        staList = self.staList
        
        if ap == sta.associatedAp[wlan]:
            if dist > ap.range + sta.range:  
                sta.pexec('iw dev %s-wlan%s disconnect' % (sta, wlan))
                sta.associatedAp[wlan] = 'NoAssociated'
                sta.params['rssi'][wlan] = 0
                sta.params['snr'][wlan] = 0
                self.numberOfAssociatedStations(ap)
            else:
                channelParameters(sta, ap, wlan, dist, staList, time)
        else:   
            if dist < ap.range + sta.range:            
                aps = 0
                for n in range(0,len(sta.associatedAp)):
                    if str(sta.associatedAp[n]) != 'NoAssociated':
                        aps+=1
                if len(sta.associatedAp) == aps:
                    associated = True
                else:
                    associated = False
            else:
                associated = False
        if ap == sta.associatedAp[wlan] or dist < (ap.range + sta.range):
            #Only if it is a mobility environment
            changeAP = False
            association_Control = dict ()
            
            """Association Control: mechanisms that optimize the use of the APs"""
            if self.associationControlMethod != False:
                ac = self.associationControlMethod              
                value = associationControl(sta, ap, wlan, ac)
                changeAP = value.changeAP
                association_Control.setdefault( 'ac', ac )                
                
            #Go to handover    
            if associated == False or changeAP == True:
                self.handover(sta, ap, wlan, dist, changeAP, **association_Control)
                channelParameters(sta, ap, wlan, dist, staList, time)