"""
wifi setups for Mininet-WiFi.

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

"""

import os
import socket
import struct
import fcntl
import fileinput
import subprocess
import glob

import numpy as np
import scipy.spatial.distance as distance 
import matplotlib.patches as patches
import matplotlib.pyplot as plt
        

from mininet.mobility import gauss_markov, \
    truncated_levy_walk, random_direction, random_waypoint, random_walk


class checkNM ( object ):
        
    """
        add mac address inside of /etc/NetworkManager/NetworkManager.conf    
    """
    @classmethod 
    def checkNetworkManager(self, storeMacAddress): 
        self.storeMacAddress = storeMacAddress     
        self.printMac = False   
        unmatch = ""
        if(os.path.exists('/etc/NetworkManager/NetworkManager.conf')):
            if(os.path.isfile('/etc/NetworkManager/NetworkManager.conf')):
                self.resultIface = open('/etc/NetworkManager/NetworkManager.conf')
                lines=self.resultIface
        
            isNew=True
            for n in lines:
                if("unmanaged-devices" in n):
                    unmatch = n
                    echo = n
                    echo.replace(" ", "")
                    echo = echo[:-1]+";"
                    isNew = False
            if(isNew):
                os.system("echo '#' >> /etc/NetworkManager/NetworkManager.conf")
                unmatch = "#"
                echo = "[keyfile]\nunmanaged-devices="
            
            for n in range(len(self.storeMacAddress)): 
                if self.storeMacAddress[n] not in unmatch:
                    echo = echo + "mac:"
                    echo = echo + self.storeMacAddress[n] + ";"
                    self.printMac = True
                
            if(self.printMac):
                for line in fileinput.input('/etc/NetworkManager/NetworkManager.conf', inplace=1): 
                    if(isNew):
                        if line.__contains__('#'): 
                            print line.replace(unmatch, echo)
                        else:
                            print line.rstrip()
                    else:
                        if line.__contains__('unmanaged-devices'): 
                            print line.replace(unmatch, echo)
                        else:
                            print line.rstrip()
             
    @classmethod 
    def getMacAddress(self, wlanInterface):
        """
            get Mac Address of any Interface   
        """
        self.wlanInterface = wlanInterface
        self.storeMacAddress=[]
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', '%s'[:15]) % str(self.wlanInterface))
        self.storeMacAddress.append(''.join(['%02x:' % ord(char) for char in info[18:24]])[:-1])
        return self.storeMacAddress
    
    @classmethod   
    def APfile(self, apcommand, ap):
        """
            run an Access Point and create the file  
        """
        self.apcommand = apcommand + ("\' > %s.conf" % ap)  
        os.system(self.apcommand)
        self.cmd = ("hostapd -f apdebug.txt -B %s.conf" % ap)
        os.system(self.cmd)

class getWlan( object ):
    
    @classmethod    
    def physical(self):
        self.phyInterfaces = []        
        self.phyInterfaces = (subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",shell=True)).split('\n')
        self.phyInterfaces.pop()
        return self.phyInterfaces
    
    @classmethod    
    def virtual(self):
        self.newapif=[]
        self.apif = subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",shell=True).split('\n')
        for apif in self.apif:
            if apif not in module.physicalWlan and apif!="":
                self.newapif.append(apif)
        self.newapif = sorted(self.newapif)
        self.newapif.sort(key=len, reverse=False)
        return self.newapif

class module( object ):
    """
        Starts and Stop the module   
    """            
    wifiRadios = 0
    isWiFi = False
    physicalWlan = []
    isCode = False
    #thread = multiprocessing.Process()
        
    @classmethod    
    def _start_module(self, wifiRadios):
        """
             Start wireless Module 
        """
        os.system( 'modprobe mac80211_hwsim radios=%s' % wifiRadios )
     
    @classmethod
    def _stop_module(self):
        """
            Stop wireless Module 
        """   
        if glob.glob("*.conf"):
            os.system( 'rm *.conf' )
        
        if glob.glob("*.txt"):
            os.system( 'rm *.txt' )
       
        #self.thread.terminate()
        os.system( 'rmmod mac80211_hwsim' )
        os.system( 'killall -9 hostapd' )
        
        
class association( object ):
    
    ssid = {}
    
    @classmethod    
    def setAdhocParameters(self, host, mode, **params):
        """
            Set wifi AdHoc Parameters. Have to use models for loss, latency, bw...    
        """
        self.mode = mode
        latency = 10
        self.host = host
        #delay = 5 * distance
        try:
            options = dict( params )
            self.interface = options[ 'interface' ]
        except:
            
            self.interface = 'wlan0'
        
        bandwidth = wifiParameters.set_bw(mode)
        #self.host.cmd(host, "tc qdisc replace dev %s-%s root netem rate %.2fmbit latency %.2fms delay %.2fms" % (host, self.interface, rate, latency, delay)) 
        self.host.cmd("tc qdisc add dev %s-%s root tbf rate %smbit latency %sms burst 1540" % (str(host), self.interface, bandwidth, latency)) 

    
    @classmethod    
    def setInfraParameters(self, sta, mode, distance):
        """
            Set wifi Infrastrucure Parameters. Have to use models for loss, latency, bw...    
        """
        station.mode(str(sta), mode)
            
        seconds = 3
        self.src = str(sta)
        try:
            """Based on RandomPropagationDelayModel (ns3)"""
            seconds = abs(mobility.speed[self.src])
        except:
            pass
        self.host = sta
        latency = wifiParameters.latency(distance)
        loss = wifiParameters.loss(distance)
        delay = wifiParameters.delay(distance, seconds)
        bw = wifiParameters.bw(distance, mode)  
        self.host.pexec("tc qdisc replace dev %s-wlan0 root netem rate %.2fmbit loss %.1f%% latency %.2fms delay %.2fms" % (sta, bw, loss, latency, delay)) 
        #os.system('util/m %s tc qdisc replace dev %s-wlan0 root netem rate %.2fmbit latency %.2fms delay %.2fms' % (self.host, self.host, bandwidth, latency, delay))
        #self.host.cmd("tc qdisc replace dev %s-wlan0 root tbf rate %.2fmbit latency %.2fms burst 15k" % (self.host, rate, latency)) 
        associate = self.doAssociation(mode, distance)
        if associate == False:
            mobility.handover(self.host)
            
    @classmethod    
    def doAssociation(self, mode, distance):
        """
            Associate/Disassociate according the distance   
        """
        associate = True
        
        if (mode=='a' and distance > 33):
            associate = False
        elif(mode=='b' and distance > 50):
            associate = False
        elif(mode=='g' and distance > 33):
            associate = False
        elif(mode=='n' and distance > 70):
            associate = False
        elif(mode=='ac' and distance > 100):
            associate = False 
            
        return associate
            

class phyInt ( object ):
    
    phy = {}
    nextIface = 0
    totalPhy = []
    
    @classmethod 
    def setNextIface(self):
        """
            Go to next Interface
        """   
        self.nextIface+=1
         
    @classmethod
    def getPhy(self):
        """
            Get phy
        """ 
        phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name hwsim | cut -d/ -f 6 | sort", 
                                                             shell=True).split("\n")
        phy.pop()
        return phy
     
    @classmethod
    def phyInt(self):
        """
            get Wireless Interface
        """   
        return subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'", shell=True).split("\n")
    
        
class station ( object ):
    
    nextWlan = {}   
    staMode = {}
    doAssociation = {}
    staPhy = []
    nextIface = 0
    currentPhy = 0
    associatedAP = {}
    
    @classmethod    
    def confirmAdhocAssociation(self, host, interface, ssid):
        associated = ''
        self.host = host
        while(associated == '' or len(associated) == 0):
            self.host.sendCmd("iw dev %s scan ssid | grep %s" % (interface, ssid))
            associated = self.host.waitOutput()
    
    @classmethod    
    def confirmInfraAssociation(self, host):
        associated = ''
        self.host = host
        while(associated == '' or len(associated) == 16):
            self.host.sendCmd("iw dev %s-wlan0 link" % str(host))
            associated = self.host.waitOutput()
            
    @classmethod    
    def mode(self, sta, mode):
        self.staMode[sta] = mode
    
    @classmethod    
    def associate(self, sta, ssid):
        """
            Station Associate to an Access Point
        """   
        self.host = sta
        self.host.cmd("iw dev %s-wlan0 connect %s" % (sta, ssid))
        self.confirmInfraAssociation(self.host)
          
    @classmethod    
    def adhoc(self, host, ssid, mode, **params):
        """
            Adhoc mode
        """   
        self.ssid = ssid
        self.mode = mode
        self.host = host
        hasIface = False   
        try:
            options = dict( params )
            self.interface = options[ 'interface' ]
            hasIface = True
        except:
            hasIface = False
        
        if hasIface: # if new iface
            association.setAdhocParameters(self.host, mode, **params)
            self.host.cmd("iw dev %s-%s set type ibss" % (str(host), self.interface))
            self.host.cmd("iw dev %s-%s ibss join %s 2412" % (str(host), self.interface, self.ssid))
            print "associating %s ..." % str(host)
            interface = '%s-%s' % (str(host), self.interface)
            self.confirmAdhocAssociation(self.host, interface, self.ssid)
        else: # if not
            association.setAdhocParameters(self.host, mode, **params)
            self.host.cmd("iw dev %s-wlan0 set type ibss" % str(host))
            self.host.cmd("iw dev %s-wlan0 ibss join %s 2412" % (str(host), self.ssid))
            print "associating %s ..." % str(host)
            interface = '%s-wlan0' % str(host)
            self.confirmAdhocAssociation(self.host, interface, self.ssid)
            
    @classmethod    
    def addWlan(self, station):
        """
            Add phy Interface to Stations
        """ 
        phyInt.phy[station] = phyInt.totalPhy[self.currentPhy][3:]
        os.system("iw phy phy%s set netns %s" % (phyInt.phy[station], station.pid)) 
        wif = station.cmd("iwconfig 2>&1 | grep IEEE | awk '{print $1}'").split("\n")
        wif.pop()
        for iface in wif:
            if iface[:4]=="wlan":
                try:
                    self.nextWlan[str(station)] += 1
                except:
                    self.nextWlan[str(station)] = 0
                netxWlan = self.nextWlan[str(station)] 
                self.renameIface(station, netxWlan, iface)
        self.currentPhy+=1
     
    @classmethod    
    def renameIface(self, station, nextWlan, iface):
        """
            Rename wireless interface if necessary
        """
        iface = iface[:-1]
        station.cmd('ip link set dev %s name %s-wlan%s' % (iface, station, nextWlan))
        station.cmd('ifconfig %s-wlan%s up' % (station, nextWlan))    
                        
            
class accessPoint ( object ):    

    apChannel = {}
    apMode = {}
    apName = []
    apSSID = {}
    apPhy = []
    number = 0
    
    @classmethod
    def start(self, bs, nextIface, ssid, mode, channel, 
              country_code, auth_algs, wpa, wpa_key_mgmt, rsn_pairwise, wpa_passphrase):
        """
            Starts an Access Point
        """
        self.apName.append(bs)
        self.apSSID[str(bs)] = ssid
        self.apMode[str(bs)] = mode
        self.cmd = ("echo \'")
        """General Configurations"""             
        self.cmd = self.cmd + ("interface=%s" % nextIface) # the interface used by the AP
        """Not using at the moment"""
        self.cmd = self.cmd + ("\ndriver=nl80211")
        if(ssid!=None):
            self.cmd = self.cmd + ("\nssid=%s" % ssid) # the name of the AP
        if(mode=="g" or mode=="n"):
            self.cmd = self.cmd + ("\nhw_mode=g") 
        elif (mode=="b"):
            self.cmd = self.cmd + ("\nhw_mode=b") 
        elif (mode=="a"):
            self.cmd = self.cmd + ("\nhw_mode=a")
        if(channel!=None):
            self.cmd = self.cmd + ("\nchannel=%s" % channel) # the channel to use 
        if(mode=="ac"):
            self.cmd = self.cmd + ("\nwme_enabled=1") 
            self.cmd = self.cmd + ("\nieee80211ac=1")
        self.cmd = self.cmd + ("\nwme_enabled=1") 
        self.cmd = self.cmd + ("\nieee80211n=1")
        if(mode=="n"):
            self.cmd = self.cmd + ("\nht_capab=[HT40+][SHORT-GI-40][DSSS_CCK-40]")
        
        #Not used yet!
        if(country_code!=None):
            self.cmd = self.cmd + ("\ncountry_code=%s" % country_code) # the country code
        if(auth_algs!=None):
            self.cmd = self.cmd + ("\nauth_algs=%s" % auth_algs) # 1=wpa, 2=wep, 3=both
        if(wpa!=None):
            self.cmd = self.cmd + ("\nwpa=%s" % wpa) # WPA2 only
        if(wpa_key_mgmt!=None):
            self.cmd = self.cmd + ("\nwpa_key_mgmt=%s" % wpa_key_mgmt ) 
        if(rsn_pairwise!=None):
            self.cmd = self.cmd + ("\nrsn_pairwise=%s" % rsn_pairwise)  
        if(wpa_passphrase!=None):
            self.cmd = self.cmd + ("\nwpa_passphrase=%s" % wpa_passphrase)                        
        
        #elif(len(self.baseStationName)>self.countAP and len(self.baseStationName) != 1):
        #    """From AP2"""
        #    self.cmd = self.apcommand
            #self.cmd = self.cmd + "\n"
        #    self.cmd = self.cmd + ("\nbss=%s" % self.newapif[self.nextIface]) # the interface used by the AP
        #    if(self.ssid!=None):
        #        self.cmd = self.cmd + ("\nssid=%s" % self.ssid ) # the name of the AP
                #self.cmd = self.cmd + ("\nssid=%s" % self.ssid) # the name of the AP
        #    if(self.auth_algs!=None):
        #        self.cmd = self.cmd + ("\nauth_algs=%s" % self.auth_algs) # 1=wpa, 2=wep, 3=both
        #    if(self.wpa!=None):
        #        self.cmd = self.cmd + ("\nwpa=%s" % self.wpa) # WPA2 only
        #    if(self.wpa_key_mgmt!=None):
        #        self.cmd = self.cmd + ("\nwpa_key_mgmt=%s" % self.wpa_key_mgmt ) 
        #    if(self.rsn_pairwise!=None):
        #        self.cmd = self.cmd + ("\nrsn_pairwise=%s" % self.rsn_pairwise)  
        #    if(self.wpa_passphrase!=None):
        #        self.cmd = self.cmd + ("\nwpa_passphrase=%s" % self.wpa_passphrase)  
        #    self.countAP = len(self.baseStationName)
        #    self.apcommand = ""
        return self.cmd
        
    @classmethod
    def apBridge(self, ap, iface):
        """
            AP Bridge
        """  
        os.system("ovs-vsctl add-port %s %s" % (ap, iface))
        
    @classmethod
    def setBw(self, newapif, mode):
        """
            Set bw to AP 
        """  
        self.newapif = newapif
        bandwidth = wifiParameters.set_bw(mode)
        os.system("tc qdisc add dev %s root tbf rate %smbit latency 2ms burst 15k" % (self.newapif, bandwidth))   
        
    #@classmethod
    #def position(self, ap, pos):
    #    self.apPosition[ap] = pos

class mobility ( object ):    
    """
        Mobility 
    """          
    speed = {}
    nodePosition = {}      
    nodesPlotted = []
    plotGraph = False
    no_moving = True
    d = ''
    s = ''
    plotap = {}
    plotsta = {}
    cancelPlot = False
    MAX_X = 50
    MAX_Y = 50
    DRAW = False
    
    @classmethod 
    def closePlot(self):
        plt.close()
    
    @classmethod   
    def range(self, mode):
        if (mode=='a'):
            self.distance = 33
        elif(mode=='b'):
            self.distance = 50
        elif(mode=='g'):
            self.distance = 33 
        elif(mode=='n'):
            self.distance = 70
        elif(mode=='ac'):
            self.distance = 100 
            
        return self.distance
    
    @classmethod   
    def move(self, node, diffTime, speed, startposition, endposition):      
        """
            Moving nodes
            diffTime: important to calculate the speed  
        """
        pos_x = float(endposition[0]) - float(startposition[0])
        pos_y = float(endposition[1]) - float(startposition[1])
        pos_z = float(endposition[2]) - float(startposition[2])
        
        self.nodePosition[node] = pos_x, pos_y, pos_z
        self.speed[node] = ((pos_x + pos_y + pos_z)/diffTime) 
        
        pos = '%.5f,%.5f,%.5f' % (pos_x/diffTime, pos_y/diffTime, pos_z/diffTime)
        pos = pos.split(',')
        return pos       
    
    @classmethod 
    def plot(self, src, dst, pos_src, pos_dst):
        MAX_X = self.MAX_X
        MAX_Y = self.MAX_Y
        
        plt.ion()
        ax = plt.subplot(111)
        
        if self.no_moving:  
            if str(dst)[:2] == 'ap' and str(dst) not in self.nodesPlotted:
                self.plotap[str(dst)], = ax.plot(range(MAX_X), range(MAX_Y), linestyle='', marker='.', ms=12, mfc='blue')
                ax.add_patch(
                    patches.Circle((pos_dst[0], pos_dst[1]),
                    self.range(accessPoint.apMode[str(dst)]), fill=True,  alpha=0.1
                    )
                )
                self.plotap[str(dst)].set_data(pos_dst[0],pos_dst[1])
                self.nodesPlotted.append(str(dst))
                plt.text(int(pos_dst[0]), int(pos_dst[1]), str(dst))
                
            elif str(src)[:2] == 'ap' and str(src) not in self.nodesPlotted:
                self.plotap[str(src)], = ax.plot(range(MAX_X), range(MAX_Y), linestyle='', marker='.', ms=12, mfc='blue')
                ax.add_patch(
                    patches.Circle((pos_src[0], pos_src[1]),
                    self.range(accessPoint.apMode[str(src)]), fill=True,  alpha=0.1
                    )
                )
                self.plotap[str(src)].set_data(pos_src[0],pos_src[1])
                self.nodesPlotted.append(str(src))
                plt.text(int(pos_src[0]), int(pos_src[1]), str(src)) 
                
            if str(dst)[:3] == 'sta' and str(dst) not in self.nodesPlotted:
                self.plotsta[str(dst)], = ax.plot(range(MAX_X), range(MAX_Y), linestyle='', marker='.', ms=12, mfc='blue')
                self.nodesPlotted.append(str(dst))
                self.plotsta[str(dst)].set_data(pos_dst[0],pos_dst[1])
                plt.text(int(pos_dst[0]), int(pos_dst[1]), str(dst))
            
            if str(src)[:3] == 'sta' and str(src) not in self.nodesPlotted:
                self.plotsta[str(src)], = ax.plot(range(MAX_X), range(MAX_Y), linestyle='', marker='.', ms=12, mfc='blue')
                self.nodesPlotted.append(str(src))
                self.plotsta[str(src)].set_data(pos_src[0],pos_src[1])
                plt.text(int(pos_src[0]), int(pos_src[1]), str(src)) 
            plt.title("Mininet-WiFi Graph")
            plt.show()
        else:
            if str(src)[:3] == 'sta' and str(src) not in self.nodesPlotted:
                self.plotsta[str(src)], = ax.plot(range(MAX_X), range(MAX_Y), linestyle='', marker='.', ms=12, mfc='blue')
                self.nodesPlotted.append(str(src))
            if str(dst)[:3] == 'sta' and str(dst) not in self.nodesPlotted:
                self.plotsta[str(dst)], = ax.plot(range(MAX_X), range(MAX_Y), linestyle='', marker='.', ms=12, mfc='blue')
                self.nodesPlotted.append(str(dst))
            
            if str(dst)[:2] == 'ap' and str(dst) not in self.nodesPlotted:
                self.plotap[str(dst)], = ax.plot(range(MAX_X), range(MAX_Y), linestyle='', marker='.', ms=12, mfc='blue')
                
                ax.add_patch(
                    patches.Circle((pos_dst[0], pos_dst[1]),
                    self.range(accessPoint.apMode[str(dst)]), fill=True,  alpha=0.1
                    )
                )
                self.plotap[str(dst)].set_data(pos_dst[0],pos_dst[1])
                self.nodesPlotted.append(str(dst))
                plt.text(int(pos_dst[0]), int(pos_dst[1]), str(dst))
                 
            elif str(src)[:2] == 'ap' and str(src) not in self.nodesPlotted:
                self.plotap[str(src)], = ax.plot(range(MAX_X), range(MAX_Y), linestyle='', marker='.', ms=12, mfc='blue')
                ax.add_patch(
                    patches.Circle((pos_src[0], pos_src[1]),
                    self.range(accessPoint.apMode[str(src)]), fill=True,  alpha=0.1
                    )
                )
                self.plotap[str(src)].set_data(pos_src[0],pos_src[1])
                self.nodesPlotted.append(str(src))
                plt.text(int(pos_src[0]), int(pos_src[1]), str(src)) 
                
            if str(src)[:2] != 'ap':
                self.plotsta[str(src)].set_data(pos_src[0],pos_src[1])
            if str(dst)[:2] != 'ap':
                self.plotsta[str(dst)].set_data(pos_dst[0],pos_dst[1])
            plt.title("Mininet-WiFi Graph")
            plt.draw()            
        
    @classmethod 
    def getDistance(self, src, dst):
        """
            Get the distance between two points  
        """
        pos_src = self.nodePosition[str(src)]
        pos_dst = self.nodePosition[str(dst)]
        if self.plotGraph and self.cancelPlot==False:
            self.plot(src, dst, pos_src, pos_dst)
        points = np.array([(pos_src[0], pos_src[1], pos_src[2]), (pos_dst[0], pos_dst[1], pos_dst[2])])
        dist = distance.pdist(points)
        return dist
    
    @classmethod 
    def printDistance(self, src, dst):
        """
            Print the distance between two points
        """
        self.src = src
        self.dst = dst
        
        dist = self.getDistance(src, dst)
        print ("The distance between %s and %s is %.2f meters\n" % (src, dst, float(dist)))
    
    @classmethod   
    def printPosition(self, node):
        """
            Print position of STAs and APs  
        """
        self.node = str(node)
        
        self.pos_x = self.nodePosition[self.node][0]
        self.pos_y = self.nodePosition[self.node][1]
        self.pos_z = self.nodePosition[self.node][2]   
        print "----------------\nPosition of %s\n----------------\nPosition X: %.2f\nPosition Y: %.2f\nPosition Z: %.2f\n" % (self.node, float(self.pos_x), float(self.pos_y), float(self.pos_z))
        
    @classmethod   
    def handover(self, host):
        src = str(host)
        disassociate = True
        for ap in accessPoint.apName:
            dst = str(ap)
            distance = float(self.getDistance(src, dst))
            if distance < self.range(accessPoint.apMode[dst]):
                host.pexec("iw dev %s-wlan0 connect %s" % (src, accessPoint.apSSID[dst]))
                disassociate = False
            if disassociate:
                host.pexec("iw dev %s-wlan0 disconnect" % (host))
            
    @classmethod   
    def models(self, wifiNodes, associatedAP, startPosition, stationName, modelName,
               max_x, max_y, min_v, max_v):
        
        self.modelName = modelName
        
        # set this to true if you want to plot node positions
        self.DRAW = self.plotGraph
        
        self.cancelPlot = True
        
        # number of nodes
        nr_nodes = stationName
        
        # simulation area (units)
        MAX_X, MAX_Y = max_x, max_y
        
        # max and min velocity
        MIN_V, MAX_V = min_v, max_v
        
        # max waiting time
        MAX_WT = 100.
        
        if self.DRAW:
            plt.ion()
            ax = plt.subplot(111)
            line, = ax.plot(range(MAX_X), range(MAX_X), linestyle='', marker='.', ms=12, mfc='blue')
                
        np.random.seed(0xffff)
        
        if(self.modelName=='RandomWalk'):
            ## Random Walk model
            mob = random_walk(nr_nodes, dimensions=(MAX_X, MAX_Y))
        elif(self.modelName=='TruncatedLevyWalk'):
            ## Truncated Levy Walk model
            mob = truncated_levy_walk(nr_nodes, dimensions=(MAX_X, MAX_Y))
        elif(self.modelName=='RandomDirection'):
            ## Random Direction model
            mob = random_direction(nr_nodes, dimensions=(MAX_X, MAX_Y))
        elif(self.modelName=='RandomWaypoint'):
            ## Random Waypoint model
            mob = random_waypoint(nr_nodes, dimensions=(MAX_X, MAX_Y), velocity=(MIN_V, MAX_V), wt_max=MAX_WT)
        elif(self.modelName=='GaussMarkov'):
            ## Gauss-Markov model
            mob = gauss_markov(nr_nodes, dimensions=(MAX_X, MAX_Y), alpha=0.99)
        else:
            print 'Model not defined or wrong!'
        
        ## Reference Point Group model
        #groups = [4 for _ in range(10)]
        #nr_nodes = sum(groups)
        #rpg = reference_point_group(groups, dimensions=(MAX_X, MAX_Y), aggregation=0.5)
        
        ## Time-variant Community Mobility Model
        #groups = [4 for _ in range(10)]
        #nr_nodes = sum(groups)
        #tvcm = tvc(groups, dimensions=(MAX_X, MAX_Y), aggregation=[0.5,0.], epoch=[100,100])
        oneTime = []
                
        if modelName!='':
            try:
                for xy in mob:
                    if self.DRAW:
                        line.set_data(xy[:,0],xy[:,1])
                        for n in range (0,len(wifiNodes)):
                            self.position = []
                            if str(wifiNodes[n])[:2]=='ap' and str(wifiNodes[n]) not in oneTime:
                                self.position.append(startPosition[str(wifiNodes[n])][0])
                                self.position.append(startPosition[str(wifiNodes[n])][1])
                                self.position.append(0)
                                self.nodePosition[str(wifiNodes[n])] = self.position
                                plt.plot([startPosition[str(wifiNodes[n])][0]], [startPosition[str(wifiNodes[n])][1]], 'ro')
                                plt.text(int(startPosition[str(wifiNodes[n])][0]), int(startPosition[str(wifiNodes[n])][1]), str(wifiNodes[n]))
                                ax.add_patch(
                                    patches.Circle((startPosition[str(wifiNodes[n])][0], startPosition[str(wifiNodes[n])][1]),
                                    self.range(accessPoint.apMode[str(str(wifiNodes[n]))]), fill=True,  alpha=0.1
                                    )
                                )
                                oneTime.append(str(wifiNodes[n]))
                            elif str(wifiNodes[n])[:2]!='ap':
                                self.position.append(xy[n][0])
                                self.position.append(xy[n][1])
                                self.position.append(0)
                                self.nodePosition[str(wifiNodes[n])] = self.position
                                distance = self.getDistance(src = str(wifiNodes[n]), dst = associatedAP[str(wifiNodes[n])])
                                association.setInfraParameters(wifiNodes[n], station.staMode[str(wifiNodes[n])], distance)
                        plt.title("Mininet-WiFi Graph")
                        plt.draw()
                    else:
                        for n in range (0,len(wifiNodes)):
                            self.position = []
                            if str(wifiNodes[n])[:2]=='ap' and str(wifiNodes[n]) not in oneTime:
                                self.position.append(startPosition[str(wifiNodes[n])][0])
                                self.position.append(startPosition[str(wifiNodes[n])][1])
                                self.position.append(0)
                                self.nodePosition[str(wifiNodes[n])] = self.position
                                oneTime.append(str(wifiNodes[n]))
                            if str(wifiNodes[n])[:2]!='ap':
                                self.position.append(xy[n][0])
                                self.position.append(xy[n][1])
                                self.position.append(0)
                                self.nodePosition[str(wifiNodes[n])] = self.position
                                distance = self.getDistance(src = str(wifiNodes[n]), dst = associatedAP[str(wifiNodes[n])])
                                association.setInfraParameters(wifiNodes[n], station.staMode[str(wifiNodes[n])], distance)
            except:
                print "Graph Stopped!"  
    
    
class wifiParameters ( object ):
    """
        WiFi Parameters 
    """
    @classmethod
    def printNoiseInfo(self, host): 
        """
            Get noise info **in development**
        """
        print self.host.cmd('iw dev %s-wlan0 survey noise %d' % (host, int(str(host)[3:])+60))    
    
    @classmethod
    def latency(self, distance):        
        latency = 5 + distance
        return latency
        
    @classmethod
    def loss(self, distance):        
        loss = 0.01 + distance/10
        return loss
    
    @classmethod
    def delay(self, distance, seconds):
        """"Based on RandomPropagationDelayModel (ns3)"""
        delay = distance/seconds
        return delay
        
    @classmethod
    def bw(self, distance, mode):
        bw = self.set_bw(mode) - distance/10    
        return bw
    
    #@classmethod
    #def pathLoss(self):
    #    pass
    
    @classmethod    
    def set_bw(self, mode):
        """
            set the Bandwidth according Mode
        """
        self.bandwidth = 0
        if (mode=='a'):
            self.bandwidth = 54
        elif(mode=='b'):
            self.bandwidth = 11
        elif(mode=='g'):
            self.bandwidth = 54 
        elif(mode=='n'):
            self.bandwidth = 600
        elif(mode=='ac'):
            self.bandwidth = 6777 
            
        return self.bandwidth