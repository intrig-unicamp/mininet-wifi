"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

import os
import subprocess
import glob

import matplotlib.patches as patches
import matplotlib.pyplot as plt
import numpy as np

from mininet.wifiMobilityModels import gauss_markov, \
    truncated_levy_walk, random_direction, random_waypoint, random_walk, reference_point_group, tvc

from mininet.wifiChannel import channelParameters    
from mininet.wifiDevices import deviceRange
from mininet.wifiMobilityModels import distance
from mininet.wifiPropagationModels import propagationModel
from mininet.wifiParameters import wifiParameters

class emulationEnvironment ( object ):
    
    propagation_Model = ''
    ismobility = False
    wpa_supplicantIsRunning = False
    isWiFi = False
    isCode = False
    continue_ = True
    physicalWlan = []
    apList = []
    staList = []
    manual_apRange = -10
    wifiRadios = 0    
    
    @classmethod
    def numberOfAssociatedStations( self, ap ):
        "Number of Associated Stations"
        cmd = 'iw dev %s-wlan0 station dump | grep Sta | grep -c ^' % ap     
        proc = subprocess.Popen("exec " + cmd, stdout=subprocess.PIPE,shell=True)   
        (out, err) = proc.communicate()
        output = out.rstrip('\n')
        ap.nAssociatedStations = int(output)

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
     
    def stop(self):
        """ Stop wireless Module """   
        if glob.glob("*.conf"):
            os.system( 'rm *.conf' )
        
        if glob.glob("*.txt"):
            os.system( 'rm *.txt' )
        
        os.system( 'rmmod mac80211_hwsim' )
        if emulationEnvironment.apList!=[]:
            os.system( 'killall -9 hostapd' )
        if emulationEnvironment.wpa_supplicantIsRunning:
            os.system( 'pkill -f \'wpa_supplicant -B\'' )
        
    def start(self, wifiRadios):
        """Starting environment"""
        emulationEnvironment.physicalWlan = getWlan.physical()  #Get Phisical Wlan(s)
        self.loadModule(wifiRadios) #Initatilize WiFi Module
        phyInt.totalPhy = phyInt.getPhy() #Get Phy Interfaces                    
        
        
class association( object ):
    
    systemLoss = 1    
    
    @classmethod    
    def parameters(self, node1, node2, dist, wlan, **params):
        """ Wifi Parameters """
        sta = node1
        ap = node2
        associated = True
        time = abs(sta.speed)
        
        if node1.func[wlan] == 'adhoc' or node1.func[wlan] == 'mesh':
            if emulationEnvironment.continue_:
                channelParameters(node1, None, wlan, dist, time)
        else:
            if ap == sta.associatedAp[wlan]:
                if dist > ap.range:                
                    station.iwCommand(sta, wlan, 'disconnect')
                    sta.associatedAp[wlan] = 'NoAssociated'
                    sta.rssi[wlan] = 0
                    sta.snr[wlan] = 0
                    emulationEnvironment.numberOfAssociatedStations(ap)
                else:
                    if emulationEnvironment.continue_:
                        channelParameters(node1, node2, wlan, dist, time)
            else:    
                if dist < ap.range:            
                    aps = 0
                    for n in range(0,len(sta.associatedAp)):
                        if 'ap' in str(sta.associatedAp[n]):
                            aps+=1
                    if len(sta.associatedAp) == aps:
                        associated = True
                    else:
                        associated = False
                        
            if ap == sta.associatedAp[wlan] or dist < ap.range:
                #Only if it is a mobility environment
                if emulationEnvironment.ismobility == True: 
                    changeAP = False
                    associationControl = dict ()
                    
                    """Association Control: mechanisms that optimize the use of the APs"""
                    if mobility.associationControl != '':
                        ac = mobility.associationControl
                        changeAP = self.associationControl(sta, ap, wlan, ac)
                        associationControl.setdefault( 'ac', ac )                
                            
                    #Go to handover    
                    if associated == False or changeAP == True:
                        mobility.handover(sta, ap, wlan, dist, changeAP, **associationControl)
               
    @classmethod    
    def setChannelParameters(self, node, **params):
        """ Set wifi Infrastrucure Parameters. Have to use models for loss, latency, bw.."""
        try:
            for wlan in range(0,len(node.func)):
                if node.func[wlan] == 'adhoc' or node.func[wlan] == 'mesh':
                    ref_distance = 0
                    for ref_sta in station.list:
                        if ref_sta != node:
                            sta = node
                            d = distance(sta, ref_sta)
                            dist = d.dist
                    ref_distance = ref_distance / len(self.stations)
                    self.parameters(sta, None, ref_distance, wlan, **params)          
                else:
                    for ap in emulationEnvironment.apList:
                        sta = node
                        d = distance(sta, ap)
                        dist = d.dist
                        self.parameters(sta, ap, dist, wlan, **params)
        except:
            pass
    
    
    @classmethod    
    def associationControl(self, node1, node2, wlan, ac):
        """Mechanisms that optimize the use of the APs"""
        changeAP = False
        
        if ac == "llf": #useful to llf (Least-loaded-first)
            apref = node1.associatedAp[wlan]
            if apref != 'NoAssociated':
                #accessPoint.numberOfAssociatedStations(apref)
                ref_llf = apref.nAssociatedStations
                if node2.nAssociatedStations+2 < ref_llf:
                    changeAP = True
            else:
                changeAP = True
        elif ac == "ssf": #useful to ssf (Strongest-signal-first)
            if emulationEnvironment.propagation_Model == '':
                emulationEnvironment.propagation_Model = 'friisPropagationLossModel'
            d = distance(node1, node2)
            ref_Distance = d.dist
            refValue = propagationModel(node1, node2, ref_Distance, wlan, emulationEnvironment.propagation_Model, self.systemLoss)
            if refValue.rssi > float(node1.rssi[wlan]+1):
                changeAP = True
        return changeAP   
    
    @classmethod    
    def doAssociation(self, ap, distance):
        """ Associate/Disassociate according the distance """
        associate = True
        if (distance > ap.range):
            associate = False
        return associate
            
class phyInt ( object ):
    
    totalPhy = []
    
    @classmethod
    def getPhy(self):
        """ Get phy """ 
        phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name hwsim | cut -d/ -f 6 | sort", 
                                                             shell=True).split("\n")
        phy.pop()
        return phy
        
class station ( object ):

    list = []
    fixedPosition = []
    printCon = True
    
    @classmethod       
    def iwCommand(self, sta, wlan, *args):
        command = 'iw dev %s-wlan%s ' % (sta, wlan)
        sta.pexec(command + '%s' % args)
    
    @classmethod       
    def ipLinkCommand(self, sta, wlan, *args):
        command = 'ip link set %s-wlan%s' % (sta, wlan)
        sta.pexec(command + '%s' % args)
     
    @classmethod   
    def setMac(self, sta):
        self.ipLinkCommand(sta, 0, 'down')
        self.ipLinkCommand(sta, 0, ('address %s' % sta.mac))
        self.ipLinkCommand(sta, 0, 'up')
     
    @classmethod    
    def assingIface(self, stations, virtualWlan):
        w = getWlan.virtual()
        for sta in stations:
            for i in range(0, sta.nWlans):
                vwlan = virtualWlan.index(str(sta))
                #os.system('iw phy %s set rts 80' % phyInt.totalPhy[vwlan + i])
                os.system('iw phy %s set netns %s' % ( phyInt.totalPhy[vwlan + i], sta.pid ))
                sta.cmd('ip link set %s name %s-wlan%s' % (w[vwlan + i], str(sta), i))  
                sta.frequency.append(0)
                sta.txpower.append(0)
                sta.snr.append(0)
                sta.speed = 0
                sta.func.append('none')
                value = deviceRange(sta)
                sta.range = value.range-15
                sta.associatedAp.append('NoAssociated')
                sta.antennaHeight.append(0.1)
                sta.antennaGain.append(1)
                self.list.append(sta)
                
                             
    @classmethod
    def getWiFiParameters(self, sta, wlan):
        wifiParameters(param='get_frequency', node=sta, wlan=wlan)
        wifiParameters(param='get_tx_power', node=sta, wlan=wlan)   
        
    @classmethod    
    def confirmMeshAssociation(self, sta, iface, mpID, wlan):
        #associated = ''
        #while(associated == '' or len(associated) == 11):
        #cmd = 'ifconfig mp%s | grep -o \'TX b.*\' | cut -f2- -d\':\'' % mpID
        #sta.sendCmd(cmd)
        #associated = sta.waitOutput()
        self.getWiFiParameters(sta, wlan)  
    
    @classmethod    
    def confirmAdhocAssociation(self, sta, iface, wlan):
        associated = ''
        while(associated == '' or len(associated) == 0):
            sta.sendCmd("iw dev %s scan ssid | grep %s" % (iface, sta.ssid))
            associated = sta.waitOutput()
        self.getWiFiParameters(sta, wlan) 

    @classmethod    
    def confirmInfraAssociation(self, node1, node2, wlan):
        associated = ''
        if self.printCon:
            print "Associating %s to %s" % (node1, node2)
        while(associated == '' or len(associated[0]) == 15):
            associated = self.isAssociated(node1, wlan)
        self.getWiFiParameters(node1, wlan) 
        emulationEnvironment.numberOfAssociatedStations(node2)
        node1.associatedAp[wlan] = node2
            
    @classmethod    
    def isAssociated(self, sta, iface):
        associated = sta.pexec("iw dev %s-wlan%s link" % (sta, iface))
        return associated
            
    @classmethod    
    def associate(self, node1, node2):
        """ Associate to an Access Point """ 
        wlan = node1.ifaceToAssociate
        self.cmd_associate(node1, node2, wlan)        
        
    @classmethod    
    def cmd_associate(self, node1, node2, wlan):
        sta = node1
        ap = node2
        
        if sta.passwd == None:
            self.iwCommand(node1, wlan, ('connect %s' % ap.ssid))
        elif sta.encrypt == 'wpa' or sta.encrypt == 'wpa2':
            self.associate_wpa(sta, wlan, ap.ssid, sta.passwd)
        elif sta.encrypt == 'wep':
            self.associate_wep(sta, wlan, ap.ssid, sta.passwd)
        self.confirmInfraAssociation(sta, ap, wlan)
        sta.associatedAp[wlan] = ap 
        ap.associatedStations.append(sta)
                
    @classmethod    
    def associate_wpa(self, sta, wlan, ssid, passwd):
        sta.cmd("wpa_supplicant -B -D nl80211 -i %s-wlan%s -c <(wpa_passphrase \"%s\" \"%s\")" \
                % (sta, wlan, ssid, passwd))
    
    @classmethod    
    def associate_wep(self, sta, wlan, ssid, passwd):    
        sta.cmd('iw dev %s-wlan%s connect %s key 0:%s' \
                % (sta, wlan, ssid, passwd))
        
    @classmethod    
    def adhoc(self, sta, **params):
        """ Adhoc mode """   
        sta.ifaceToAssociate += 1
        wlan = sta.ifaceToAssociate
        sta.rssi[wlan] = -62
        sta.func[wlan] = 'adhoc'
        association.setChannelParameters(sta)
        self.iwCommand(sta, wlan, 'set type ibss')
        self.iwCommand(sta, wlan, ('ibss join %s 2412' % sta.ssid))
        print "associating %s ..." % sta
        iface = '%s-wlan%s' % (sta, wlan)
        self.confirmAdhocAssociation(sta, iface, wlan)
        
    @classmethod    
    def addMesh(self, sta, **params):
        """ Mesh mode """   
        sta.ifaceToAssociate += 1
        wlan = sta.ifaceToAssociate
        sta.rssi[wlan] = -62 
        sta.func[wlan] = 'mesh'
        self.iwCommand(sta, wlan, ('interface add %s-mp%s type mp' % (sta,wlan)))
        sta.cmd('iw dev %s-mp%s set %s' % (sta, wlan, sta.channel))
        sta.cmd('ifconfig %s-mp%s up' % (sta, wlan))
        sta.cmd('iw dev %s-mp%s mesh join %s' % (sta, wlan, sta.ssid))
        association.setChannelParameters(sta)
        print "associating %s ..." % sta
        iface = '%s-wlan%s' % (sta, wlan)
        mpID = wlan
        self.confirmMeshAssociation(sta, iface, mpID, wlan)    
    
                 
class plot (object):
    """Plot Graph: Useful when the position is previously defined.
                        Not useful for Mobility Models"""
    
    def __init__( self, src, dst, **params ):
            
        close = (params.pop('close', {}))
        if close == True:
            self.closePlot()
        else:    
            self.startPlot(src, dst)
           
    def closePlot(self):
        """Close"""
        plt.close()  
        
    def plotNode(self, node, ax):
        """Plot Node"""
        plt.ion()
   
        if node.type == 'station':
            color = 'green'
            mobility.plttxt[node] = ax.annotate(node, xy=(node.position[0], node.position[1]))
        elif node.type == 'accessPoint':
            color = 'blue'
            self.plotCircle(node, ax, 'b')
            plt.text(int(node.position[0]), int(node.position[1]), node)  
        
        mobility.pltNode[node], = ax.plot(range(mobility.MAX_X), range(mobility.MAX_Y), \
                                     linestyle='', marker='.', ms=12, mfc=color)
        mobility.pltNode[node].set_data(node.position[0], node.position[1])
        mobility.nodesPlotted.append(node)
        
    def plotUpdate(self, node):
        """Update Draw"""
        mobility.pltNode[node].set_data(node.position[0], node.position[1])
        mobility.plttxt[node].xytext = (node.position[0], node.position[1])
     
    def plotCircle(self, node, ax, color):
        """Plot Circle"""
        mobility.pltCircle[node] = ax.add_patch(
            patches.Circle((node.position[0], node.position[1]),
            node.range, fill=True, alpha=0.1, color=color
            )
        )
        
    def startPlot(self, src, dst):
        """Start"""
        plt.ion()
        ax = plt.subplot(111)
        
        if src.type == 'station' and dst.type == 'station' and src not in mobility.nodesPlotted:
            self.plotNode(src, ax)
            self.plotCircle(src, ax, 'g')
        else:
            if src.type != 'station' or dst.type != 'station':
                if dst.type == 'station' and dst not in mobility.nodesPlotted:
                    self.plotNode(dst, ax)
                elif src.type == 'station' and src not in mobility.nodesPlotted:
                    self.plotNode(src, ax)
        
        if src.type == 'station' and dst.type == 'station':
            mobility.pltCircle[src].center = src.position[0], src.position[1]
            self.plotUpdate(src)            
        else:
            if  src.type == 'station':
                self.plotUpdate(src)
            elif dst.type == 'station':
                self.plotUpdate(dst)
             
        if dst.type == 'accessPoint' and dst not in mobility.nodesPlotted:
            ap = dst
        elif src.type == 'accessPoint' and src not in mobility.nodesPlotted:
            ap = src
            
        if src.type == 'accessPoint' and src not in mobility.nodesPlotted \
                        or dst.type == 'accessPoint' and dst not in mobility.nodesPlotted:
            self.plotNode(ap, ax)
        
        plt.title("Mininet-WiFi Graph")
        plt.draw()     
        
        
class mobility ( object ):    
    """ Mobility """          
    associationControl = None
    DRAW = False
    pltNode = {}
    plttxt = {}
    pltCircle = {}
    nodesPlotted = []
    MAX_X = 50
    MAX_Y = 50
    
    @classmethod   
    def move(self, sta, diffTime, speed, startposition, endposition):      
        """
            Moving nodes
            diffTime: important to calculate the speed  
        """
        pos_x = float(endposition[0]) - float(startposition[0])
        pos_y = float(endposition[1]) - float(startposition[1])
        pos_z = float(endposition[2]) - float(startposition[2])
        
        sta.position = pos_x, pos_y, pos_z
        self.nodeSpeed(sta, pos_x, pos_y, pos_z, diffTime) 
        
        pos = '%.5f,%.5f,%.5f' % (pos_x/diffTime, pos_y/diffTime, pos_z/diffTime)
        pos = pos.split(',')
        return pos    
    
    @classmethod 
    def nodeSpeed(self, sta, pos_x, pos_y, pos_z, diffTime):
        sta.speed = ((pos_x + pos_y + pos_z)/diffTime) 
  
    @classmethod 
    def printDistance(self, src, dst):
        """ Print the distance between two points """
        d = distance(src, dst)
        dist = d.dist
        print ("The distance between %s and %s is %.2f meters\n" % (src, dst, float(dist)))
    
    @classmethod   
    def printPosition(self, node):
        """ Print position of STAs and APs """
        self.pos_x = node.position[0]
        self.pos_y = node.position[1]
        self.pos_z = node.position[2]   
        print "----------------\nPosition of %s\n---------------- \
        \nPosition X: %.2f \
        \nPosition Y: %.2f \
        \nPosition Z: %.2f\n" % (str(node), float(self.pos_x), float(self.pos_y), float(self.pos_z))
    
    @classmethod   
    def handover(self, sta, ap, wlan, distance, changeAP, ac=None, **params):
        """handover"""
        if ac == 'llf' or ac == 'ssf':
            station.iwCommand(sta, wlan, 'disconnect')
            station.iwCommand(sta, wlan, ('connect %s' % ap.ssid))
            station.getWiFiParameters(sta, wlan)
            sta.associatedAp[wlan] = ap
        elif ap not in sta.associatedAp:
            #Useful for stations with more than one wifi iface
            if 'ap' not in str(sta.associatedAp[wlan]):
                station.iwCommand(sta, wlan, ('connect %s' % ap.ssid))
                station.getWiFiParameters(sta, wlan)
                ap.associatedStations.append(sta)
                sta.associatedAp[wlan] = ap        
        emulationEnvironment.numberOfAssociatedStations(ap)
            
    @classmethod   
    def models(self, wifiNodes=None, model=None, max_x=None, max_y=None, min_v=None, max_v=None, 
               manual_aprange=-10, n_staMov=None, ismobility=None, AC=None, seed=None, plot=False,
               **mobilityparam):
        
        emulationEnvironment.manual_apRange = manual_aprange
        self.modelName = model
        emulationEnvironment.ismobility = ismobility
        self.associationControl = AC
        np.random.seed(seed)
        
        # set this to true if you want to plot node positions
        self.DRAW = plot
        
        # number of nodes
        nr_nodes = n_staMov
        
        # simulation area (units)
        MAX_X, MAX_Y = max_x, max_y
        
        # max and min velocity
        MIN_V, MAX_V = min_v, max_v
        
        # max waiting time
        MAX_WT = 100.
        
        if self.DRAW:
            plt.ion()
            ax = plt.subplot(111)
            line, = ax.plot(range(mobility.MAX_X), range(mobility.MAX_X), linestyle='', marker='.', ms=10, mfc='blue')
            self.plottxt = {}
            
            for node in wifiNodes:
                if node.type == 'station' and str(node) not in station.fixedPosition:
                    self.plottxt[str(node)] = ax.annotate(str(node), xy=(0, 0))
                    
                if str(node) in station.fixedPosition:
                    self.plottxt[node] = ax.annotate(node, xy=(node.position[0], node.position[1]))
                    self.plotsta[node], = ax.plot(range(mobility.MAX_X), range(mobility.MAX_Y), \
                                                  linestyle='', marker='.', ms=12, mfc='green')
                    self.plotsta[node].set_data(node.position[0], node.position[1])
                               
        if(self.modelName=='RandomWalk'):
            ## Random Walk model
            mob = random_walk(nr_nodes, dimensions=(MAX_X, MAX_Y))
        elif(self.modelName=='TruncatedLevyWalk'):
            ## Truncated Levy Walk model
            mob = truncated_levy_walk(nr_nodes, dimensions=(MAX_X, MAX_Y))
        elif(self.modelName=='RandomDirection'):
            ## Random Direction model
            mob = random_direction(nr_nodes, dimensions=(MAX_X, MAX_Y), velocity=(MIN_V, MAX_V))
        elif(self.modelName=='RandomWayPoint'):
            ## Random Waypoint model
            mob = random_waypoint(nr_nodes, dimensions=(MAX_X, MAX_Y), velocity=(MIN_V, MAX_V), wt_max=MAX_WT)
        elif(self.modelName=='GaussMarkov'):
            ## Gauss-Markov model
            mob = gauss_markov(nr_nodes, dimensions=(MAX_X, MAX_Y), alpha=0.99)
        elif(self.modelName=='ReferencePoint'):
            ## Reference Point Group model
            mob = reference_point_group(nr_nodes, dimensions=(MAX_X, MAX_Y), aggregation=0.5) 
        elif(self.modelName=='TimeVariantCommunity'):
            ## Time-variant Community Mobility Model
            mob = tvc(nr_nodes, dimensions=(MAX_X, MAX_Y), aggregation=[0.5,0.], epoch=[100,100])         
        else:
            print 'Model not defined or wrong!'
        
        once = []
        if model!='':
            for xy in mob:
                if self.DRAW:
                    line.set_data(xy[:,0],xy[:,1])
                for n in range (0,len(wifiNodes)):
                    node = wifiNodes[n]
                    if 'accessPoint' == node.type and str(node) not in once:
                        ap = node
                        pos_zero = ap.startPosition[0]
                        pos_one = ap.startPosition[1]
                        ap.position = pos_zero, pos_one, 0     
                        if self.DRAW:
                            plt.plot([pos_zero], [pos_one], 'ro')
                            plt.text(int(pos_zero), int(pos_one), str(node))
                            ax.add_patch(
                                patches.Circle((pos_zero, pos_one),
                                ap.range, fill=True, alpha=0.1, color='b'
                                )
                            )
                        once.append(str(wifiNodes[n]))
                    elif 'accessPoint' != node.type:
                        if str(node) not in station.fixedPosition:
                            pos_zero = xy[n][0]
                            pos_one = xy[n][1]
                            if self.DRAW:
                                self.plottxt[str(node)].xytext = (pos_zero, pos_one)
                            node.position = pos_zero, pos_one, 0
                            association.setChannelParameters(node)                         
                if self.DRAW:
                    plt.title("Mininet-WiFi Graph")
                    plt.draw()