"""
wifi setups to Mininet-WiFi.

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

"""

import os
import socket
import struct
import fcntl
import fileinput
import subprocess
import glob
import math
import time

import numpy as np
import scipy.spatial.distance as distance 
import matplotlib.patches as patches
import matplotlib.pyplot as plt
        
from mininet.mobility import gauss_markov, \
    truncated_levy_walk, random_direction, random_waypoint, random_walk

class checkNM ( object ):
    """ add mac address inside of /etc/NetworkManager/NetworkManager.conf """
    @classmethod 
    def checkNetworkManager(self, mac):
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
            
            if mac not in unmatch:
                echo = echo + "mac:" + mac + ';'
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
                #necessary to add mac address before starting the ap
                time.sleep(2)
             
    @classmethod 
    def getMacAddress(self, ap, wlan):
        """ get Mac Address of any Interface """
        iface = str(ap.virtualWlan) + str(wlan)
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', '%s'[:15]) % iface)
        mac = (''.join(['%02x:' % ord(char) for char in info[18:24]])[:-1])
        self.checkNetworkManager(mac)
    
    @classmethod   
    def APfile(self, cmd, ap, wlan):
        """ run an Access Point and create the config file  """
        iface = str(ap.virtualWlan) + str(wlan)
        apcommand = cmd + ("\' > %s.conf" % iface)  
        os.system(apcommand)
        cmd = ("hostapd -B %s.conf" % iface)
        subprocess.check_output(cmd, shell=True)


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
            if apif not in module.physicalWlan and apif!="":
                self.newapif.append(apif)
        self.newapif = sorted(self.newapif)
        self.newapif.sort(key=len, reverse=False)
        return self.newapif

class module( object ):
    """ Starts and Stop the module """            
    wifiRadios = 0
    isWiFi = False
    physicalWlan = []
    virtualWlan = []
    isCode = False
        
    @classmethod    
    def _start_module(self, wifiRadios):
        """ Start wireless Module """
        os.system( 'modprobe mac80211_hwsim radios=%s' % wifiRadios )
     
    @classmethod
    def _stop_module(self):
        """ Stop wireless Module """   
        if glob.glob("*.conf"):
            os.system( 'rm *.conf' )
        
        if glob.glob("*.txt"):
            os.system( 'rm *.txt' )
       
        os.system( 'rmmod mac80211_hwsim' )
        if accessPoint.exists:
            os.system( 'killall -9 hostapd' )
        
    @classmethod
    def startEnvironment(self):
        self.physicalWlan = getWlan.physical()  #Get Phisical Wlan(s)
        self.isWiFi=True
        self._start_module(module.wifiRadios) #Initatilize WiFi Module
        phyInt.totalPhy = phyInt.getPhy() #Get Phy Interfaces                    
        
class association( object ):
    
    @classmethod    
    def setAdhocParameters(self, sta, iface):
        """ Set wifi AdHoc Parameters. Have to use models for loss, latency, bw... """
        latency = 2
        #delay = 5 * distance
        bandwidth = wifiParameters.set_bw(sta.mode)
        sta.pexec("tc qdisc replace dev %s-wlan%s \
            root handle 3: netem rate %smbit \
            latency %sms" % (sta, iface, bandwidth, latency))    
        #Reordering packets    
        sta.pexec('tc qdisc add dev %s-wlan%s parent 3:1 pfifo limit 1000' % (sta, iface))        


    @classmethod    
    def parameters(self, sta, ap, distance, wlan):
        """ Wifi Parameters """
        seconds = 3
        try:
            """Based on RandomPropagationDelayModel (ns3)"""
            seconds = abs(sta.speed)
        except:
            pass
        
        latency = wifiParameters.latency(distance)
        loss = wifiParameters.loss(distance, sta.mode)
        delay = wifiParameters.delay(distance, seconds)
        bw = wifiParameters.bw(distance, sta, ap)  
        
        sta.pexec("tc qdisc replace dev %s-wlan%s \
            root handle 1: netem rate %.2fmbit \
            loss %.1f%% \
            latency %.2fms \
            delay %.2fms" % (sta, wlan, bw, loss, latency, delay))    
        #Reordering packets    
        sta.pexec('tc qdisc replace dev %s-wlan%s parent 1:1 pfifo limit 1000' % (sta, wlan))
        #sta.pexec('tc qdisc del dev %s-wlan%s root' % (sta, wlan))
        
        if str(ap) == sta.ifaceAssociatedToAp[wlan]:
            associated = self.doAssociation(sta.mode, ap, distance) 
        else:
            aps = 0
            for n in range(0,len(sta.ifaceAssociatedToAp)):
                if 'ap' in sta.ifaceAssociatedToAp[n]:
                    aps+=1
            if len(sta.ifaceAssociatedToAp) == aps:
                associated = True
            else:
                associated = False
        
        #Only if is a mobility topology
        if mobility.ismobility == True: 
            changeAP = False
            mobilityReason = dict ()      
            
            """useful to llf (Least-loaded-first)"""
            if mobility.leastLoadFirst == True:
                llf = False
                for apref in accessPoint.list:
                    if str(apref) == sta.ifaceAssociatedToAp[wlan]:
                        accessPoint.numberOfAssociatedStations(apref)
                        ref_llf = apref.nAssociatedStations
                        llf = True
            
                if llf == True:
                    accessPoint.numberOfAssociatedStations(ap)
                    if ap.nAssociatedStations+2 < ref_llf:
                        mobilityReason.setdefault( 'reason', 'llf' )
                        changeAP = True
            
            """useful to ssf (Strongest-signal-first)"""
            if accessPoint.manual_apRange!=-10:
                for apref in accessPoint.list:
                    if str(apref) == sta.ifaceAssociatedToAp[wlan]:
                        ref_Distance = mobility.getDistance(sta,apref)
                        if ref_Distance > accessPoint.manual_apRange:
                            changeAP = True
                if distance <= accessPoint.manual_apRange and changeAP == True: 
                    mobilityReason.setdefault( 'reason', 'ssf' )
                    changeAP = True
                else:
                    changeAP = False
                    
            #Go to handover    
            if associated == False or changeAP == True:
                mobility.handover(sta, ap, wlan, distance, changeAP, **mobilityReason)
                
                    
    @classmethod    
    def setInfraParameters(self, sta, ap, distance, wlan):
        """ Set wifi Infrastrucure Parameters. Have to use models for loss, latency, bw.."""
        if wlan != '':
            self.parameters(sta, ap, distance, wlan)
        else:
            for wlan in range(0, sta.nWlans):
                self.parameters(sta, ap, distance, wlan)
            
    @classmethod    
    def doAssociation(self, mode, ap, distance):
        """ Associate/Disassociate according the distance """
        associate = True
        if (distance > ap.range):
            associate = False
        return associate
            
class phyInt ( object ):
    
    phy = {}
    totalPhy = []
    
    @classmethod
    def getPhy(self):
        """ Get phy """ 
        phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name hwsim | cut -d/ -f 6 | sort", 
                                                             shell=True).split("\n")
        phy.pop()
        return phy
        
class station ( object ):

    indexStaIface = {}
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
    def assingIface(self, stations):
        wlan = getWlan.virtual()
        for sta in stations:
            if sta.type == 'station':
                for i in range(0, sta.nWlans):
                    vwlan = module.virtualWlan.index(str(sta))
                    os.system('iw phy %s set netns %s' % ( phyInt.totalPhy[vwlan + i], sta.pid ))
                    sta.cmd('ip link set %s name %s-wlan%s' % (wlan[vwlan + i], str(sta), i))   
      
    @classmethod
    def getWiFiParameters(self, sta, interface):
        wifiParameters.get_frequency(sta, interface)
        wifiParameters.get_tx_power(sta, interface)    
        wifiParameters.get_rsi(sta, interface)  
        
    @classmethod    
    def confirmMeshAssociation(self, sta, interface, mpID):
        associated = ''
        while(associated == '' or len(associated) == 11):
            cmd = 'ifconfig mp%s | grep -o \'TX b.*\' | cut -f2- -d\':\'' % mpID
            sta.sendCmd(cmd)
            associated = sta.waitOutput()
        self.getWiFiParameters(sta, interface)   
    
    @classmethod    
    def confirmAdhocAssociation(self, sta, interface):
        associated = ''
        while(associated == '' or len(associated) == 0):
            sta.sendCmd("iw dev %s scan ssid | grep %s" % (interface, sta.ssid))
            associated = sta.waitOutput()
        self.getWiFiParameters(sta, interface) 
    
    @classmethod    
    def confirmInfraAssociation(self, sta, wlan, ap):
        associated = ''
        if self.printCon:
            print "Associating %s to %s" % (sta, ap)
        while(associated == '' or len(associated[0]) == 15):
            associated = self.isAssociated(sta, wlan)
        interface = str(sta)+'-wlan%s' % wlan
        self.getWiFiParameters(sta, interface)   
            
    @classmethod    
    def isAssociated(self, sta, iface):
        associated = sta.pexec("iw dev %s-wlan%s link" % (sta, iface))
        return associated
            
    @classmethod    
    def associate(self, sta, ap):
        """ Associate to an Access Point """ 
        wlan = sta.ifaceToAssociate
        self.cmd_associate(sta, wlan, ap)        
        
    @classmethod    
    def cmd_associate(self, sta, wlan, ap):
        sta.associatedAp = ap
        self.iwCommand(sta, wlan, ('connect %s' % ap.ssid))
        self.confirmInfraAssociation(sta, wlan, ap)
        sta.ifaceAssociatedToAp[wlan] = str(ap) 
        
    @classmethod    
    def adhoc(self, sta, **params):
        """ Adhoc mode """   
        sta.ifaceToAssociate += 1
        wlan = sta.ifaceToAssociate
        association.setAdhocParameters(sta, wlan)
        self.iwCommand(sta, wlan, 'set type ibss')
        self.iwCommand(sta, wlan, ('ibss join %s 2412' % sta.ssid))
        print "associating %s ..." % str(sta)
        interface = '%s-wlan%s' % (str(sta), wlan)
        self.confirmAdhocAssociation(sta, interface)
        
    @classmethod    
    def addMesh(self, sta, ipaddress=None, **params):
        """ Mesh mode """   
        sta.ifaceToAssociate += 1
        wlan = sta.ifaceToAssociate
        self.iwCommand(sta, wlan, ('interface add mp%s type mp' % wlan))
        sta.cmd('iw dev mp%s set %s' % (wlan, sta.channel))
        sta.cmd('ifconfig mp%s 192.168.10.%s up' % (wlan, ipaddress))
        sta.cmd('iw dev mp%s mesh join %s' % (wlan, sta.ssid))
        association.setAdhocParameters(sta, wlan)
        print "associating %s ..." % sta
        interface = '%s-wlan%s' % (sta, wlan)
        mpID = wlan
        self.confirmMeshAssociation(sta, interface, mpID)        
            
class accessPoint ( object ):    

    list = []
    number = 0
    exists = False   
    manual_apRange = -10   
    
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
    
    """
    def wds(self, ap1, int1, ap2, int2):
        os.system('iw dev %s set 4addr off' % int1)
        os.system('iw dev %s interface add wds.%s type managed 4addr on' % (int1, int1))
        os.system('iw dev %s set 4addr off' % int2)
        os.system('iw dev %s interface add wds.%s type managed 4addr on' % (int2, int2))
        os.system('ifconfig wds.%s down' % int1)
        os.system('ifconfig wds.%s down' % int2)
        os.system('ip link set dev wds.wlan2 addr 02:00:00:00:00:00')
        os.system('ip link set dev wds.wlan1 addr 02:00:00:00:01:00')
        os.system('ifconfig wds.%s up' % int1)
        os.system('ifconfig wds.%s up' % int2)
    """
    
    @classmethod
    def rename( self, intf, newname, wlan ):
        "Rename interface"
        newname = str(newname) + str(wlan)
        os.system('ifconfig %s down' % intf)
        os.system('ip link set %s name %s' % (intf, newname))
        os.system('ifconfig %s up' % newname)
            
    @classmethod
    def numberOfAssociatedStations( self, ap ):
        "Number of Associated Stations"
        cmd = 'iw dev %s-wlan0 station dump | grep Sta | grep -c ^' % ap     
        proc = subprocess.Popen(cmd,stdout=subprocess.PIPE,shell=True)   
        (out, err) = proc.communicate()
        output = out.rstrip('\n')
        ap.nAssociatedStations = int(output)
    
    @classmethod
    def start(self, ap, country_code=None, auth_algs=None, wpa=None, iface=None,
              wpa_key_mgmt=None, rsn_pairwise=None, wpa_passphrase=None, **params):
        """ Starts an Access Point """
        self.exists = True
        iface = str(ap.virtualWlan) + str(iface)
        self.cmd = ("echo \'")
        """General Configurations"""             
        self.cmd = self.cmd + ("interface=%s" % iface) # the interface used by the AP
        """Not using at the moment"""
        self.cmd = self.cmd + ("\ndriver=nl80211")
        self.cmd = self.cmd + ("\nssid=%s" % ap.ssid) # the name of the AP
        
        if ap.mode == 'n' or ap.mode == 'ac'or ap.mode == 'a':
            self.cmd = self.cmd + ("\nhw_mode=g") 
        else:
            self.cmd = self.cmd + ("\nhw_mode=%s" % ap.mode) 
        self.cmd = self.cmd + ("\nchannel=%s" % ap.channel) # the channel to use 
        #if(ap.mode=="ac" or ap.mode=='a'):
        #   self.cmd = self.cmd + ("\nieee80211ac=1")
        self.cmd = self.cmd + ("\nwme_enabled=1") 
        self.cmd = self.cmd + ("\nwmm_enabled=1") 
        #if(ap.mode=="n" or ap.mode=="a"):
        #   self.cmd = self.cmd + ("\nieee80211n=1")
        #if(ap.mode=="n"):
        #   self.cmd = self.cmd + ("\nht_capab=[HT40+][SHORT-GI-40][DSSS_CCK-40]")
        
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
        """ AP Bridge """  
        intf = str(ap)+'-'+str(iface[:4])+str(0)
        os.system("ovs-vsctl add-port %s %s" % (ap, intf))
        
    @classmethod
    def setBw(self, ap, wlan):
        """ Set bw to AP """  
        iface = str(ap.virtualWlan) + str(wlan)
        bandwidth = wifiParameters.set_bw(ap.mode)
        os.system("tc qdisc replace dev %s \
            root handle 2: netem rate %.2fmbit \
            latency 1ms \
            delay 0.1ms" % (iface, bandwidth))    
        #Reordering packets    
        os.system('tc qdisc add dev %s parent 2:1 pfifo limit 1000' % iface)
        #os.system("tc qdisc add dev %s root tbf rate %smbit latency 2ms burst 15k" % \
                 #(ap.virtualWlan, bandwidth))
        
class mobility ( object ):    
    """ Mobility """          
    plotap = {}
    plotsta = {}
    plottxt = {}
    nodesPlotted = []
    plotGraph = False
    cancelPlot = False
    ismobility = False
    leastLoadFirst = False
    DRAW = False
    MAX_X = 50
    MAX_Y = 50
    
    @classmethod 
    def closePlot(self):
        plt.close()    
    
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
        sta.speed = ((pos_x + pos_y + pos_z)/diffTime) 
        
        pos = '%.5f,%.5f,%.5f' % (pos_x/diffTime, pos_y/diffTime, pos_z/diffTime)
        pos = pos.split(',')
        return pos       
    
    @classmethod 
    def plotStation(self, sta, position, ax):
        plt.ion()
        self.plottxt[sta] = ax.annotate(sta, xy=(position[0],position[1]))
        self.plotsta[sta], = ax.plot(range(self.MAX_X), range(self.MAX_Y), \
                                     linestyle='', marker='.', ms=12, mfc='blue')
        self.nodesPlotted.append(sta)
        self.plotsta[sta].set_data(position[0],position[1])
        
    @classmethod 
    def plotAccessPoint(self, ap, position, ax):
        plt.ion()
        self.plotap[ap], = ax.plot(range(self.MAX_X), range(self.MAX_Y), \
                                   linestyle='', marker='.', ms=12, mfc='red')
        ax.add_patch(
            patches.Circle((position[0], position[1]),
            accessPoint.range(ap.mode), fill=True,  alpha=0.1
            )
        )
        self.plotap[ap].set_data(position[0], position[1])
        self.nodesPlotted.append(str(ap))
        plt.text(int(position[0]), int(position[1]), ap)

    @classmethod 
    def plot(self, src, dst, pos_src, pos_dst):
        """
            Plot Graph: Useful when the position is previously defined.
                        Not useful for Mobility Models
        """
        plt.ion()
        ax = plt.subplot(111)
        
        if dst.type == 'station' and str(dst) not in self.nodesPlotted:
            sta = str(dst)
            position = pos_dst
            self.plotStation(sta, position, ax)
        elif src.type == 'station' and str(src) not in self.nodesPlotted:
            sta = str(src)
            position = pos_src
            self.plotStation(sta, position, ax)
        
        if  src.type == 'station':
            self.plotsta[str(src)].set_data(pos_src[0],pos_src[1])
            self.plottxt[str(src)].xytext = (pos_src[0],pos_src[1])
        elif dst.type == 'station':
            self.plotsta[str(dst)].set_data(pos_dst[0],pos_dst[1])
            self.plottxt[str(dst)].xytext = (pos_dst[0],pos_dst[1])
             
        if dst.type == 'accessPoint' and str(dst) not in self.nodesPlotted:
            ap = dst
            position = pos_dst
        elif src.type == 'accessPoint' and str(src) not in self.nodesPlotted:
            ap = src
            position = pos_src
            
        if src.type == 'accessPoint' and str(src) not in self.nodesPlotted or dst.type == 'accessPoint'  and str(dst) not in self.nodesPlotted:
            self.plotAccessPoint(ap, position, ax)
        
        plt.title("Mininet-WiFi Graph")
        plt.draw()            
        
    @classmethod 
    def getDistance(self, src, dst):
        """ Get the distance between two points """
        pos_src = src.position
        pos_dst = dst.position
        if self.plotGraph and self.cancelPlot==False:
            self.plot(src, dst, pos_src, pos_dst)
        points = np.array([(pos_src[0], pos_src[1], pos_src[2]), (pos_dst[0], pos_dst[1], pos_dst[2])])
        dist = distance.pdist(points)
        return dist
    
    @classmethod 
    def printDistance(self, src, dst):
        """ Print the distance between two points """
        self.src = src
        self.dst = dst
        
        dist = self.getDistance(src, dst)
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
    def handover(self, sta, ap, wlan, distance, changeAP, reason=None, **params):
        
        if distance < ap.range: 
            if reason == 'llf' or reason == 'ssf':
                station.iwCommand(sta, wlan, 'disconnect')
                station.iwCommand(sta, wlan, ('connect %s' % ap.ssid))
                sta.ifaceAssociatedToAp[wlan] = str(ap)
            elif str(ap) not in sta.ifaceAssociatedToAp:
                #Useful for stations with more than one wifi iface
                if 'ap' not in sta.ifaceAssociatedToAp[wlan]:
                    station.iwCommand(sta, wlan, ('connect %s' % ap.ssid))
                    sta.ifaceAssociatedToAp[wlan] = str(ap)   
        elif distance > ap.range:
            if str(ap) == sta.ifaceAssociatedToAp[wlan]:
                station.iwCommand(sta, wlan, 'disconnect')
                sta.ifaceAssociatedToAp[wlan] = 'NoAssociated'  
            
    @classmethod   
    def models(self, wifiNodes=None, model=None, max_x=None, max_y=None, min_v=None, max_v=None, 
               manual_aprange=-10, n_staMov=None, ismobility=None, llf=False, seed=None,
               **mobilityparam):
        
        accessPoint.manual_apRange = manual_aprange
        self.modelName = model
        self.ismobility = ismobility
        self.leastLoadFirst = llf
        np.random.seed(seed)
        
        # set this to true if you want to plot node positions
        self.DRAW = self.plotGraph
        
        self.cancelPlot = True
        
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
                                                  linestyle='', marker='.', ms=12, mfc='blue')
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
                
        if model!='':
            try:
                for xy in mob:
                    if self.DRAW:
                        line.set_data(xy[:,0],xy[:,1])
                    for n in range (0,len(wifiNodes)):
                        node = wifiNodes[n]
                        if 'accessPoint' == node.type and str(node) not in oneTime:
                            ap = node
                            pos_zero = ap.startPosition[0]
                            pos_one = ap.startPosition[1]
                            ap.position = pos_zero, pos_one, 0     
                            if self.DRAW:
                                plt.plot([pos_zero], [pos_one], 'ro')
                                plt.text(int(pos_zero), int(pos_one), str(node))
                                ax.add_patch(
                                    patches.Circle((pos_zero, pos_one),
                                    ap.range, fill=True, alpha=0.1
                                    )
                                )
                            oneTime.append(str(wifiNodes[n]))
                        elif 'accessPoint' != node.type:
                            if str(node) not in station.fixedPosition:
                                pos_zero = xy[n][0]
                                pos_one = xy[n][1]
                                if self.DRAW:
                                    self.plottxt[str(node)].xytext = (pos_zero, pos_one)
                                node.position = pos_zero, pos_one, 0
                                for ap in accessPoint.list:
                                    sta = node
                                    distance = self.getDistance(sta, ap)
                                    association.setInfraParameters(sta, ap, distance, '')
                    if self.DRAW:
                        plt.title("Mininet-WiFi Graph")
                        plt.draw()
            except:
                print "Graph Stopped!"  
        
class wifiParameters ( object ):
    """
        WiFi Parameters 
    """
    
    @classmethod
    def get_rsi(self, sta, iface): 
        """ Get rsi info """
        sta.rsi = (sta.cmd('iwconfig %s | grep -o \'Signal.*\' | cut -f2- -d\'=\' | cut -c1-4'
                                            % iface)) 
    
    @classmethod
    def get_frequency(self, sta, iface): 
        """ Get frequency info **in development """
        freq = sta.cmd('iwconfig %s | grep -o \'Frequency.*z\' | cut -f2- -d\':\' | cut -c1-5'
                                            % iface)
        if freq!='':
            sta.frequency = float(freq) 
    
    @classmethod
    def get_tx_power(self, sta, iface): 
        """ Get tx_power info """
        sta.txpower = int(sta.cmd('iwconfig %s | grep -o \'Tx-Power.*\' | cut -f2- -d\'=\' | cut -c1-3'
                                         % iface))
    #@classmethod
    #def printNoiseInfo(self, host): 
    #    """
    #        Get noise info **in development**
    #    """
    #    print self.host.cmd('iw dev %s-wlan0 survey noise %d' % (host, int(str(host)[3:])+60))    
    
    @classmethod
    def latency(self, distance):        
        latency = 2 + distance
        return latency
        
    @classmethod
    def loss(self, distance, mode):  
        if distance!=0:
            loss =  0.1 * distance
        else:
            loss = 0.1
        return loss/10
    
    @classmethod
    def delay(self, distance, seconds):
        """"Based on RandomPropagationDelayModel (ns3)"""
        if seconds != 0:
            delay = distance/seconds
        else:
            delay = 1
        return delay        
    
    @classmethod
    def custom_step(self, mode):    
        """ only useful for bw """
        self.step = 0
        if (mode=='a' or mode=='g'):
            self.step = 3
        elif(mode=='b'):
            self.step = 5
        elif(mode=='n'):
            self.step = 5
        elif(mode=='ac'):
            self.step = 5
            
        return self.step
    
    @classmethod
    def custom_bw_step(self, mode):    
        """ only useful for bw """
        self.bw_step = 0
        if (mode=='a' or mode=='g'):
            self.bw_step = 5
        elif(mode=='b'):
            self.bw_step = 1.1
        elif(mode=='n'):
            self.bw_step = 42
        elif(mode=='ac'):
            self.bw_step = 338
            
        return self.bw_step
    
    @classmethod
    def bw(self, distance, sta, ap):
        mode = sta.mode
        signalRange = ap.range
        customStep = self.custom_step(mode)
        custombwStep = self.custom_bw_step(mode)
        if distance != 0: 
            bw = self.set_bw(mode)
            for n in range(0,signalRange+1):
                if n % customStep==0:
                    if n>=distance:
                        return bw
                    elif distance > signalRange:
                        return self.set_bw(mode)
                    bw = bw - custombwStep                    
        else:
            return self.set_bw(mode)        

    @classmethod
    def max_pathLoss(self, sta):
        """used to calculate the range."""  
        gains = 6
        losses = 6
        fademargin = 12
        maxpathloss = sta.txpower - sta.rsi + gains - losses - fademargin
        return maxpathloss
        
    @classmethod
    def friis_propagation_loss_model(self):   
        """
            power_r = Reception Power (W)
            power_t = Transmission Power (W)
            gain_r = Reception gain (unit-less)
            gain_t = Transmission gain (unit-less)
            wavelength = wavelength (m) => C/f => (m/s and Hz)
            distance = (m)
            systemLoss = system loss (unit-less)
        """
        frequency = 2412
        power_t = 20
        gain_r = 6
        gain_t = 6
        systemLoss = 1.
        wavelength = 299792458/frequency
        power_r = (power_t * gain_t * gain_r * math.pow(wavelength,2)) / \
                        math.pow((4 * math.pi * distance),2) * systemLoss
        return power_r
        
    @classmethod
    def pathLoss(self, exponent):
        """(pathloss) is the path loss in decibels
        (exponent) is the path loss exponent
        (distance) is the distance between the transmitter and the receiver (meters)
        and (constant) is a constant which accounts for system losses."""    
        #pathloss = 10 * exponent * math.log10(distance) + constant
    
    @classmethod
    def free_space_loss(self, sta):
        """Formula: Free Space Loss
        (distance) is the distance between the transmitter and the receiver (km)
        (frequency) signal frequency transmited(MegaHertz)."""  
        frequency = sta.frequency
        distance=10
        #32,44 is a constant and its value depends on the units for distance and frequency
        fsl = 32,44 - 20 * math.log10(distance) + 20 * math.log10(frequency) #fsl in dB
        return float('%.1f' % fsl)
    
    @classmethod
    def free_space_path_loss(self, sta):
        """Formula: Free Space Path Loss
        (distance) is the distance between the transmitter and the receiver (meters)
        (frequency) signal frequency transmited(Hertz)
        (constant) speed of light in a vacuum (metres per second)."""  
        constant = 3 * 10**8  
        frequency = sta.frequency * 10**9 # Using 10^9 to convert to Hz 
        distance = 1
        fspl = ((4 * math.pi * distance * frequency)/constant)**2
        return fspl
    
      
    @classmethod    
    def set_bw(self, mode):
        """ set maximum Bandwidth according Mode """
        self.bandwidth = 0
        if (mode=='a'):
            self.bandwidth = 20 # 54
        elif(mode=='b'):
            self.bandwidth = 6 #11
        elif(mode=='g'):
            self.bandwidth = 20 #54
        elif(mode=='n'):
            self.bandwidth = 48 # 600
        elif(mode=='ac'):
            self.bandwidth = 90 #6777
            
        return self.bandwidth