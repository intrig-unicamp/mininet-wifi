"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

import os
import subprocess
import glob
import matplotlib.pyplot as plt
import numpy as np
import time

from mininet.log import debug
from mininet.wifiMobilityModels import gauss_markov, \
    truncated_levy_walk, random_direction, random_waypoint, random_walk, reference_point_group, tvc
from mininet.wifiChannel import channelParameters
from mininet.wifiAssociationControl import associationControl
from mininet.wifiEmulationEnvironment import emulationEnvironment
from mininet.wifiMeshRouting import listNodes, meshRouting
from mininet.wifiParameters import wifiParameters
from mininet.wifiPlot import plot


class getWlan( object ):
    
    @classmethod    
    def physical(self):
        self.phyInterfaces = []        
        self.phyInterfaces = (subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",
                                                      shell=True)).split('\n')
        self.phyInterfaces.pop()
        return self.phyInterfaces
    
    @classmethod    
    def virtual(self):
        self.newapif=[]
        self.apif = subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",
                                            shell=True).split('\n')
        for apif in self.apif:
            if apif not in emulationEnvironment.physicalWlan and apif!="":
                self.newapif.append(apif)
        self.newapif = sorted(self.newapif)
        self.newapif.sort(key=len, reverse=False)
        return self.newapif


class module( object ):
    """ Start and Stop mac80211_hwsim module """ 
    
    def __init__( self, **params ):
        
        action = (params.pop('action', {}))
        
        if action == 'start':
            wifiRadios = (params.pop('wifiRadios', {}))
            self.start(wifiRadios)
        elif action == 'stop':    
            self.stop()
           
    def loadModule(self, wifiRadios):
        """ Start wireless Module """
        os.system( 'modprobe mac80211_hwsim radios=%s' % wifiRadios )
        debug( 'Loading %s virtual interfaces\n' % wifiRadios)
            
    def stop(self):
        """ Stop wireless Module """   
        if glob.glob("*.conf"):
            os.system( 'rm *.conf' )
        
        if glob.glob("*.txt"):
            os.system( 'rm *.txt' )
        
        try:
            subprocess.check_output("lsmod | grep mac80211_hwsim",
                                                          shell=True)
            os.system( 'rmmod mac80211_hwsim' )
        except:
            pass
        
        if emulationEnvironment.apList!=[]:
            os.system( 'killall -9 hostapd' )
        if emulationEnvironment.wpa_supplicantIsRunning:
            os.system( 'pkill -f \'wpa_supplicant -B\'' )
        
    def start(self, wifiRadios):
        """Starting environment"""
        emulationEnvironment.physicalWlan = getWlan.physical()  #Get Phisical Wlan(s)
        self.loadModule(wifiRadios) #Initatilize WiFi Module
        emulationEnvironment.totalPhy = emulationEnvironment.getPhy() #Get Phy Interfaces                    
        
        
class station ( object ):
    """Station Class"""
    
    @classmethod   
    def setMac(self, sta):
        sta.pexec('ip link set %s-wlan0 down' % sta)
        sta.pexec('ip link set %s-wlan0 address %s' % (sta, sta.mac))
        sta.pexec('ip link set %s-wlan0 up' % sta)
     
    @classmethod    
    def assingIface(self, stations, virtualWlan):
        wlan = getWlan.virtual()
        for sta in stations:
            for i in range(0, sta.nWlans):
                w = virtualWlan.index(str(sta))
                os.system('iw phy %s set netns %s' % ( emulationEnvironment.totalPhy[w + i], sta.pid ))
                sta.cmd('ip link set %s name %s-wlan%s up' % (wlan[w + i], str(sta), i))  
                sta.frequency.append(0)
                sta.txpower.append(0)
                sta.snr.append(0)
                sta.rssi.append(0)
                sta.speed = 0
                sta.meshMac.append(0)
                sta.isAssociated.append('')
                sta.ssid.append('')
                sta.associatedAp.append('NoAssociated')
                sta.antennaHeight.append(2)
                sta.antennaGain.append(1)

    @classmethod    
    def confirmInfraAssociation(self, node1, node2, wlan):
        associated = ''
        if emulationEnvironment.printCon:
            print "Associating %s to %s" % (node1, node2)
        while(associated == '' or len(associated[0]) == 15):
            associated = self.isAssociated(node1, wlan)
        iface = str(node1)+'-wlan%s' % wlan
        wifiParameters.getWiFiParameters(node1, wlan, iface) 
        emulationEnvironment.numberOfAssociatedStations(node2)
        node1.associatedAp[wlan] = node2
        mobility.getAPsInRange(node1)
            
    @classmethod    
    def isAssociated(self, sta, iface):
        associated = sta.pexec("iw dev %s-wlan%s link" % (sta, iface))
        return associated
            
    @classmethod    
    def associate(self, node1, node2):
        """ Associate to an Access Point """ 
        node1.ifaceToAssociate += 1
        wlan = node1.ifaceToAssociate
        self.cmd_associate(node1, node2, wlan)        
        
    @classmethod    
    def cmd_associate(self, node1, node2, wlan):
        sta = node1
        ap = node2
                
        if sta.passwd == None:
            #sta.pexec('iw dev %s-wlan%s connect %s' % (sta, wlan, ap.ssid[0]))
            sta.pexec('iwconfig %s-wlan%s essid %s ap %s' % (sta, wlan, ap.ssid[0], ap.mac))
        elif sta.encrypt == 'wpa' or sta.encrypt == 'wpa2':
            self.associate_wpa(sta, wlan, ap.ssid[0], sta.passwd)
        elif sta.encrypt == 'wep':
            self.associate_wep(sta, wlan, ap.ssid[0], sta.passwd)
        self.confirmInfraAssociation(sta, ap, wlan)
        sta.associatedAp[wlan] = ap 
        ap.associatedStations.append(sta)
        sta.ssid[wlan] = ap.ssid[0]
        sta.wlanToAssociate+=1
                
    @classmethod    
    def associate_wpa(self, sta, wlan, ssid, passwd):
        sta.cmd("wpa_supplicant -B -D nl80211 -i %s-wlan%s -c <(wpa_passphrase \"%s\" \"%s\")" \
                % (sta, wlan, ssid, passwd))
    
    @classmethod    
    def associate_wep(self, sta, wlan, ssid, passwd):    
        sta.cmd('iw dev %s-wlan%s connect %s key 0:%s' \
                % (sta, wlan, ssid, passwd))
        
        
class mobility ( object ):    
    """ Mobility """          
    
    MAX_X = 50
    MAX_Y = 50
    moveFac = {}
    initialPosition = {}
    finalPosition = {}
    
    @classmethod   
    def moveFactor(self, sta, diffTime, initialPosition, finalPosition):      
        """
            Moving nodes
            diffTime: important to calculate the speed  
        """
        self.initialPosition[sta] = initialPosition
        self.finalPosition[sta] = finalPosition
        
        sta.position = initialPosition
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
        emulationEnvironment.numberOfAssociatedStations(ap)
        
    @classmethod 
    def instatiateNodes(self, node):
        plot.instantiateAnnotate(node)
        plot.instantiateCircle(node)
        plot.instantiateNode(node, self.MAX_X, self.MAX_Y)
        plot.graphUpdate(node)  
            
    @classmethod 
    def mobilityPositionDefined(self, initial_time, final_time, staMov):
        """ ongoing Mobility """        
        t_end = time.time() + final_time
        t_initial = time.time() + initial_time
        currentTime = time.time()
        i=1
        
        if emulationEnvironment.DRAW == True:
            debug('Enabling Graph...\n')
            plot.instantiateGraph(self.MAX_X, self.MAX_Y)
            for sta in emulationEnvironment.staList:
                self.instatiateNodes(sta)
                if sta not in staMov:
                    plot.pltNode[sta].set_data(sta.position[0],sta.position[1])
                    plot.drawTxt(sta)
                    plot.drawCircle(sta)   
            for ap in emulationEnvironment.apList:
                self.instatiateNodes(ap)  
                for c in ap.connections:
                    line = plot.plotLine2d([ap.connections[c].position[0],ap.position[0]], \
                                           [ap.connections[c].position[1],ap.position[1]], 'b')
                    plot.plotLine(line)
        try:
            while time.time() < t_end and time.time() > t_initial:
                if emulationEnvironment.continue_:
                    if time.time() - currentTime >= i:
                        for sta in emulationEnvironment.staList:
                            if time.time() - currentTime >= sta.startTime and time.time() - currentTime <= sta.endTime:
                                sta.position[0] = float(sta.position[0]) + float(self.moveFac[sta][0])
                                sta.position[1] = float(sta.position[1]) + float(self.moveFac[sta][1])
                                sta.position[2] = float(sta.position[2]) + float(self.moveFac[sta][2])
                            for wlan in range(0, sta.nWlans):
                                self.nodeParameter(sta, wlan) 
                            if emulationEnvironment.DRAW:
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

        if emulationEnvironment.DRAW:
            debug('Enabling Graph...\n')
            plot.instantiateGraph(self.MAX_X, self.MAX_Y)
            for node in nodes:
                self.instatiateNodes(node)
                if node not in staMov or 'accessPoint' == node.type:
                    plot.pltNode[node].set_data(node.position[0],node.position[1])
                    plot.drawTxt(node)
                    plot.drawCircle(node)  
                    if node.type == 'accessPoint': 
                        for c in node.connections:
                            line = plot.plotLine2d([node.connections[c].position[0],node.position[0]], \
                                                   [node.connections[c].position[1],node.position[1]], 'b')
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
                                node.position = xy[i][0], xy[i][1], 0
                                i += 1                       
                                if emulationEnvironment.DRAW:
                                    plot.pltNode[node].set_data(xy[:,0],xy[:,1])
                                    plot.drawTxt(node)
                                    plot.drawCircle(node)
                            #self.parameters()
                    if emulationEnvironment.DRAW:
                            plt.title("Mininet-WiFi Graph")
                            plt.draw()   
        except:
            pass               
    
    @classmethod 
    def getAPsInRange(self, sta):
        for ap in emulationEnvironment.apList:
            dist = channelParameters.getDistance(sta, ap)
            if dist < ap.range + sta.range:
                if ap not in sta.inRangeAPs:
                    sta.inRangeAPs.append(ap)
            else:
                if ap in sta.inRangeAPs:
                    sta.inRangeAPs.remove(ap)

    @classmethod 
    def nodeParameter(self, sta, wlan):
        for ap in emulationEnvironment.apList:
            if 'wlan' not in ap.params:
                dist = channelParameters.getDistance(sta, ap)
                self.getAPsInRange(sta)
                self.setChannelParameters(sta, ap, dist, wlan)  
                            
    @classmethod                
    def parameters(self):
        while emulationEnvironment.continue_:
            try:
                for node in emulationEnvironment.staList: 
                    for wlan in range(0, node.nWlans):
                        if node.func[wlan] == 'mesh' or node.func[wlan] == 'adhoc':
                            dist = listNodes.pairingNodes(node, wlan, emulationEnvironment.staList)
                            channelParameters(node, None, wlan, dist, emulationEnvironment.staList, abs(node.speed))
                        else:
                            self.nodeParameter(node, wlan)
                if emulationEnvironment.meshRouting == 'custom':
                    for node in emulationEnvironment.staList:       
                        for wlan in range(0, node.nWlans):
                            if node.func[wlan] == 'mesh':
                                """Mesh Routing"""                    
                                meshRouting.customMeshRouting(node, wlan, emulationEnvironment.staList)    
                    listNodes.clearList()
            except:
                pass
    
    @classmethod    
    def setChannelParameters(self, sta, ap, dist, wlan):
        """ Wifi Parameters """
        associated = True
        time = abs(sta.speed)
        staList = emulationEnvironment.staList
        
        if ap == sta.associatedAp[wlan]:
            if dist > ap.range + sta.range:  
                sta.pexec('iw dev %s-wlan%s disconnect' % (sta, wlan))
                sta.associatedAp[wlan] = 'NoAssociated'
                sta.rssi[wlan] = 0
                sta.snr[wlan] = 0
                emulationEnvironment.numberOfAssociatedStations(ap)
            else:
                #if emulationEnvironment.continue_:
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
            if emulationEnvironment.associationControlMethod != False:
                ac = emulationEnvironment.associationControlMethod              
                value = associationControl(sta, ap, wlan, ac)
                changeAP = value.changeAP
                association_Control.setdefault( 'ac', ac )                
                
            #Go to handover    
            if associated == False or changeAP == True:
                self.handover(sta, ap, wlan, dist, changeAP, **association_Control)
                channelParameters(sta, ap, wlan, dist, staList, time)