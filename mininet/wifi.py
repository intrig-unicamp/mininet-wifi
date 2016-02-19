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
import time
import numpy as np
import scipy.spatial.distance as distance 
import matplotlib.patches as patches
import matplotlib.pyplot as plt

from mininet.wifiMobility import gauss_markov, \
    truncated_levy_walk, random_direction, random_waypoint, random_walk, reference_point_group, tvc
    
from mininet.wifiDevices import deviceDataRate, deviceTxPower
from mininet.wifiPropagationModel import propagationModel

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
    wpa_supplicantIsRunning = False
        
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
        if self.wpa_supplicantIsRunning:
            os.system( 'pkill -f \'wpa_supplicant -B\'' )
        
    @classmethod
    def startEnvironment(self):
        self.physicalWlan = getWlan.physical()  #Get Phisical Wlan(s)
        self.isWiFi=True
        self._start_module(module.wifiRadios) #Initatilize WiFi Module
        phyInt.totalPhy = phyInt.getPhy() #Get Phy Interfaces                    
        
class association( object ):
    
    @classmethod    
    def setAdhocParameters(self, sta, iface, ref_distance):
        """ Set wifi AdHoc Parameters. Have to use models for loss, latency, bw... """
        if ref_distance != '':
            seconds = 1
            try:
                """Based on RandomPropagationDelayModel (ns3)"""
                seconds = abs(sta.speed)
            except: 
                pass
            latency = wifiParameters.latency(ref_distance)
            loss = wifiParameters.loss(ref_distance, sta.mode)
            bandwidth = wifiParameters.set_bw(sta.mode)
            delay = wifiParameters.delay(ref_distance, seconds)
            bw = wifiParameters.bw(ref_distance, sta, None, 0)
            sta.pexec("tc qdisc replace dev %s-wlan%s \
                root handle 1: netem rate %.2fmbit \
                loss %.1f%% \
                latency %.2fms \
                delay %.2fms" % (sta, iface, bw, loss, latency, delay))
        else:
            latency = 2
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
        
        if str(sta.associatedAp[wlan]) == str(ap):
            systemLoss = 1
            model = wifiParameters.propagationModel
            propagationModel(sta, ap, distance, wlan, model, systemLoss)
            latency = wifiParameters.latency(distance)
            loss = wifiParameters.loss(distance, ap.mode)
            delay = wifiParameters.delay(distance, seconds)
            bw = wifiParameters.bw(distance, sta, ap, wlan)
            
            sta.pexec("tc qdisc replace dev %s-wlan%s \
                root handle 1: netem rate %.2fmbit \
                loss %.1f%% \
                latency %.2fms \
                delay %.2fms" % (sta, wlan, bw, loss, latency, delay))    
            #Reordering packets    
            sta.pexec('tc qdisc replace dev %s-wlan%s parent 1:1 pfifo limit 10000' % (sta, wlan))
            #sta.pexec('tc qdisc del dev %s-wlan%s root' % (sta, wlan))
            
            associated = self.doAssociation(ap.mode, ap, distance) 
        else:
            if sta.ifaceAssociatedToAp[wlan] == 'NoAssociated':
                sta.rssi[wlan] = 0
            
            aps = 0
            for n in range(0,len(sta.ifaceAssociatedToAp)):
                if 'ap' in sta.ifaceAssociatedToAp[n]:
                    aps+=1
            if len(sta.ifaceAssociatedToAp) == aps:
                associated = True
            else:
                associated = False
        
        #Only if is a mobility environment
        if mobility.ismobility == True: 
            changeAP = False
            associationControl = dict ()
            
            """Association Control: mechanisms that optimize the use of the APs"""
            if mobility.associationControl != '':
                ac = mobility.associationControl
                changeAP = self.associationControl(ap, sta, wlan, ac)
                associationControl.setdefault( 'ac', ac )                
                    
            #Go to handover    
            if associated == False or changeAP == True:
                mobility.handover(sta, ap, wlan, distance, changeAP, **associationControl)
    
    @classmethod    
    def associationControl(self, ap, sta, wlan, ac):
        """Mechanisms that optimize the use of the APs"""
        changeAP = False
        
        """useful to llf (Least-loaded-first)"""
        if ac == "llf":
            llf = False
            for apref in accessPoint.list:
                if str(apref) == sta.ifaceAssociatedToAp[wlan]:
                    accessPoint.numberOfAssociatedStations(apref)
                    ref_llf = apref.nAssociatedStations
                    llf = True
            
            if llf == True:
                accessPoint.numberOfAssociatedStations(ap)
                if ap.nAssociatedStations+2 < ref_llf:
                    changeAP = True
        
        """useful to ssf (Strongest-signal-first)"""
        if ac == "ssf":
            for apref in accessPoint.list:
                if str(apref) == sta.ifaceAssociatedToAp[wlan]:
                    accessPoint.numberOfAssociatedStations(apref)
                    ref_Distance = mobility.getDistance(sta, apref)
                    #if ref_Distance > accessPoint.manual_apRange:
                    if ref_Distance < distance:
                        changeAP = True
            #if distance <= accessPoint.manual_apRange and changeAP == True: 
                #changeAP = True
        return changeAP     
                           
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
            for i in range(0, sta.nWlans):
                vwlan = module.virtualWlan.index(str(sta))
                #os.system('iw phy %s set rts 80' % phyInt.totalPhy[vwlan + i])
                os.system('iw phy %s set netns %s' % ( phyInt.totalPhy[vwlan + i], sta.pid ))
                sta.cmd('ip link set %s name %s-wlan%s' % (wlan[vwlan + i], str(sta), i))   
                sta.frequency.append(0)
                sta.txpower.append(0)
                sta.rssi.append(0)
                sta.ifaceAssociatedToAp.append(str(i))
                sta.associatedAp.append('NoAssociated')
                sta.antennaHeight.append(0.1)
                sta.antennaGain.append(1)
                             
    @classmethod
    def getWiFiParameters(self, sta, iface, wlan):
        wifiParameters.get_frequency(sta, iface, wlan)
        wifiParameters.get_tx_power(sta, iface, wlan)    
        #wifiParameters.get_rsi(sta, iface)          
        
    @classmethod    
    def confirmMeshAssociation(self, sta, iface, mpID, wlan):
        #associated = ''
        #while(associated == '' or len(associated) == 11):
        #cmd = 'ifconfig mp%s | grep -o \'TX b.*\' | cut -f2- -d\':\'' % mpID
        #sta.sendCmd(cmd)
        #associated = sta.waitOutput()
        self.getWiFiParameters(sta, iface, wlan)  
    
    @classmethod    
    def confirmAdhocAssociation(self, sta, iface, wlan):
        associated = ''
        while(associated == '' or len(associated) == 0):
            sta.sendCmd("iw dev %s scan ssid | grep %s" % (iface, sta.ssid))
            associated = sta.waitOutput()
        self.getWiFiParameters(sta, iface, wlan) 

    @classmethod    
    def confirmInfraAssociation(self, sta, ap, wlan):
        associated = ''
        if self.printCon:
            print "Associating %s to %s" % (sta, ap)
        while(associated == '' or len(associated[0]) == 15):
            associated = self.isAssociated(sta, wlan)
        iface = str(sta)+'-wlan%s' % wlan
        self.getWiFiParameters(sta, iface, wlan) 
        accessPoint.numberOfAssociatedStations(ap)
        sta.associatedAp[wlan] = ap
            
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
        if sta.passwd == None:
            self.iwCommand(sta, wlan, ('connect %s' % ap.ssid))
        elif sta.encrypt == 'wpa' or sta.encrypt == 'wpa2':
            self.associate_wpa(sta, wlan, ap.ssid, sta.passwd)
        elif sta.encrypt == 'wep':
            self.associate_wep(sta, wlan, ap.ssid, sta.passwd)
        self.confirmInfraAssociation(sta, ap, wlan)
        sta.ifaceAssociatedToAp[wlan] = str(ap) 
        
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
        association.setAdhocParameters(sta, wlan, '')
        self.iwCommand(sta, wlan, 'set type ibss')
        self.iwCommand(sta, wlan, ('ibss join %s 2412' % sta.ssid))
        print "associating %s ..." % str(sta)
        iface = '%s-wlan%s' % (str(sta), wlan)
        self.confirmAdhocAssociation(sta, iface, wlan)
        
    @classmethod    
    def addMesh(self, sta, **params):
        """ Mesh mode """   
        sta.ifaceToAssociate += 1
        wlan = sta.ifaceToAssociate
        self.iwCommand(sta, wlan, ('interface add %s-mp%s type mp' % (sta,wlan)))
        sta.cmd('iw dev %s-mp%s set %s' % (sta, wlan, sta.channel))
        sta.cmd('ifconfig %s-mp%s up' % (sta, wlan))
        sta.cmd('iw dev %s-mp%s mesh join %s' % (sta, wlan, sta.ssid))
        association.setAdhocParameters(sta, wlan, '')
        print "associating %s ..." % sta
        iface = '%s-wlan%s' % (sta, wlan)
        mpID = wlan
        self.confirmMeshAssociation(sta, iface, mpID, wlan)     

                 
class accessPoint ( object ):    

    list = []
    number = 0
    exists = False   
    manual_apRange = -10   
    
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
        try:
            ap.nAssociatedStations = int(output)
        except:
            pass
    
    @classmethod
    def start(self, ap, country_code=None, auth_algs=None, wpa=None, iface=None,
              wpa_key_mgmt=None, rsn_pairwise=None, wpa_passphrase=None, encrypt=None, 
              wep_key0=None, **params):
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
        
        if encrypt == 'wpa':
            module.wpa_supplicantIsRunning = True
            self.cmd = self.cmd + ("\nauth_algs=%s" % auth_algs)
            self.cmd = self.cmd + ("\nwpa=%s" % wpa)
            self.cmd = self.cmd + ("\nwpa_key_mgmt=%s" % wpa_key_mgmt ) 
            self.cmd = self.cmd + ("\nwpa_passphrase=%s" % wpa_passphrase)                        
        elif encrypt == 'wpa2':
            module.wpa_supplicantIsRunning = True
            self.cmd = self.cmd + ("\nauth_algs=%s" % auth_algs)
            self.cmd = self.cmd + ("\nwpa=%s" % wpa)
            self.cmd = self.cmd + ("\nwpa_key_mgmt=%s" % wpa_key_mgmt ) 
            self.cmd = self.cmd + ("\nrsn_pairwise=%s" % rsn_pairwise)  
            self.cmd = self.cmd + ("\nwpa_passphrase=%s" % wpa_passphrase)   
        elif encrypt == 'wep':
            self.cmd = self.cmd + ("\nauth_algs=%s" % auth_algs)
            self.cmd = self.cmd + ("\nwep_default_key=%s" % 0) 
            self.cmd = self.cmd + ("\nwep_key0=%s" % wep_key0)                         
                
        #Not used yet!
        if(country_code!=None):
            self.cmd = self.cmd + ("\ncountry_code=%s" % country_code) # the country code
        
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
        
        if ap.equipmentModel == None:
            bw = wifiParameters.set_bw(ap.mode)
        else: 
            r = deviceDataRate(ap, None, wlan)
            bw = r.rate
        
        os.system("tc qdisc replace dev %s \
            root handle 2: netem rate %.2fmbit \
            latency 1ms \
            delay 0.1ms" % (iface, bw))   
        #Reordering packets    
        os.system('tc qdisc add dev %s parent 2:1 pfifo limit 1000' % iface)

                 
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
            color = 'blue'
            mobility.plttxt[node] = ax.annotate(node, xy=(node.position[0], node.position[1]))
        elif node.type == 'accessPoint':
            color = 'red'
            self.plotCircle(node, ax)
            plt.text(int(node.position[0]), int(node.position[1]), node)  
        
        mobility.pltNode[node], = ax.plot(range(mobility.MAX_X), range(mobility.MAX_Y), \
                                     linestyle='', marker='.', ms=12, mfc=color)
        mobility.pltNode[node].set_data(node.position[0], node.position[1])
        mobility.nodesPlotted.append(node)
        
    def plotUpdate(self, node):
        """Update Draw"""
        mobility.pltNode[node].set_data(node.position[0], node.position[1])
        mobility.plttxt[node].xytext = (node.position[0], node.position[1])
     
    def plotCircle(self, node, ax):
        """Plot Circle"""
        mobility.pltCircle[node] = ax.add_patch(
            patches.Circle((node.position[0], node.position[1]),
            node.range, fill=True, alpha=0.1
            )
        )
        
    def startPlot(self, src, dst):
        """Start"""
        plt.ion()
        ax = plt.subplot(111)
        
        if src.type == 'station' and dst.type == 'station' and src not in mobility.nodesPlotted:
            self.plotNode(src, ax)
            self.plotCircle(src, ax)
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
    ismobility = False
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
        sta.speed = ((pos_x + pos_y + pos_z)/diffTime) 
        
        pos = '%.5f,%.5f,%.5f' % (pos_x/diffTime, pos_y/diffTime, pos_z/diffTime)
        pos = pos.split(',')
        return pos       
        
    @classmethod 
    def getDistance(self, src, dst):
        """ Get the distance between two points """
        pos_src = src.position
        pos_dst = dst.position
        
        points = np.array([(pos_src[0], pos_src[1], pos_src[2]), (pos_dst[0], pos_dst[1], pos_dst[2])])
        dist = distance.pdist(points)
        return dist
  
    @classmethod 
    def printDistance(self, src, dst):
        """ Print the distance between two points """
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
    def handover(self, sta, ap, wlan, distance, changeAP, ac=None, **params):
        
        if distance < ap.range: 
            if ac == 'llf' or ac == 'ssf':
                station.iwCommand(sta, wlan, 'disconnect')
                station.iwCommand(sta, wlan, ('connect %s' % ap.ssid))
                sta.ifaceAssociatedToAp[wlan] = str(ap)
                sta.associatedAp[wlan] = ap
                iface = str(sta)+'-wlan%s' % wlan
                station.getWiFiParameters(sta, iface, wlan)
            elif str(ap) not in sta.ifaceAssociatedToAp:
                #Useful for stations with more than one wifi iface
                if 'ap' not in sta.ifaceAssociatedToAp[wlan]:
                    station.iwCommand(sta, wlan, ('connect %s' % ap.ssid))
                    sta.ifaceAssociatedToAp[wlan] = str(ap)   
                    sta.associatedAp[wlan] = ap
                    iface = str(sta)+'-wlan%s' % wlan
                    station.getWiFiParameters(sta, iface, wlan)
        elif distance > ap.range:
            if str(ap) == sta.ifaceAssociatedToAp[wlan]:
                station.iwCommand(sta, wlan, 'disconnect')
                sta.ifaceAssociatedToAp[wlan] = 'NoAssociated'  
                sta.associatedAp[wlan] = 'NoAssociated'
                sta.txpower[wlan] = 0
                sta.rssi[wlan] = 0
        accessPoint.numberOfAssociatedStations(ap)
            
    @classmethod   
    def models(self, wifiNodes=None, model=None, max_x=None, max_y=None, min_v=None, max_v=None, 
               manual_aprange=-10, n_staMov=None, ismobility=None, AC=None, seed=None, plot=False,
               **mobilityparam):
        
        accessPoint.manual_apRange = manual_aprange
        self.modelName = model
        self.ismobility = ismobility
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
                                ap.range, fill=True, alpha=0.1
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
                            for ap in accessPoint.list:
                                sta = node
                                distance = self.getDistance(sta, ap)
                                association.setInfraParameters(sta, ap, distance, '')                                
                if self.DRAW:
                    plt.title("Mininet-WiFi Graph")
                    plt.draw()
                    
         
class wifiParameters ( object ):
    """WiFi Parameters""" 
       
    propagationModel = ''
    rate = 0
   
    @classmethod
    def get_rsi(self, sta, iface): 
        """ Get rsi info """
        sta.rsi = (sta.cmd('iwconfig %s | grep -o \'Signal.*\' | cut -f2- -d\'=\' | cut -c1-4'
                                            % iface)) 
    
    @classmethod
    def get_frequency(self, device, iface, wlan): 
        """ Get frequency info **in development """
        try:
            freq = device.cmd('iwconfig %s | grep -o \'Frequency.*z\' | cut -f2- -d\':\' | cut -c1-5'
                                                % iface)
            if freq!='':
                device.frequency[wlan] = float(freq) 
        except:
            pass
    
    @classmethod
    def get_tx_power(self, device, iface, wlan): 
        """ Get tx_power info """
        try:
            if device.equipmentModel == None:
                device.txpower[wlan] = int(device.cmd('iwconfig %s | grep -o \'Tx-Power.*\' | cut -f2- -d\'=\' | cut -c1-3'
                                                 % iface))
            else:
                deviceTxPower(device.equipmentModel, device)
        except:
            pass
            
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
    def signal_to_noise_ratio(self, sta):
        """RSSI (Received Signal Strength Indicator): 
                - Value between 0 and -120
                - The closer this value to 0 (zero), stronger the signal
           NOISE:
                - value between 0 and -120
                - The closer this value to -120, better."""  
        #channelNoise = 1
        #snr = sta.rssi - channelNoise      
                  
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
    def range(self, node):
        
        mode = node.mode
        
        if (mode=='a'):
            self.value = 33
        elif(mode=='b'):
            self.value = 50
        elif(mode=='g'):
            self.value = 33 
        elif(mode=='n'):
            self.value = 70
        elif(mode=='ac'):
            self.value = 100 
            
        return self.value   
    
    @classmethod
    def custom_bw(self, mode):    
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
    def bw(self, distance, sta, ap, wlan):
        if ap == None:
            mode = sta.mode
            signalRange = sta.range
            customStep = self.custom_step(mode)
            custombw = self.custom_bw(mode)
            bw = self.set_bw_node_moving(mode)
            for n in range(0,signalRange+1):
                sta.rssi[wlan] = -50 - distance
                if n % customStep==0:
                    if n>=distance:
                        return bw
                    elif distance > signalRange:
                        return 0
                    bw = bw - custombw
        else:
            mode = ap.mode
            signalRange = ap.range
            customStep = self.custom_step(mode)
            custombw = self.custom_bw(mode)
            if distance != 0: 
                if ap.equipmentModel == None:
                    bw = self.set_bw_node_moving(mode)
                    for n in range(0,signalRange+1):
                        sta.rssi[wlan] = -50 - distance
                        if n % customStep==0:
                            if n>=distance:
                                return bw
                            elif distance > signalRange:
                                return 0
                            bw = bw - custombw 
                elif ap.equipmentModel != None and self.propagationModel == '':
                    sta.rssi[wlan] = -50 - distance
                    if sta.associatedAp[wlan] != 'NoAssociated':
                        r = deviceDataRate(ap, sta, wlan)
                        self.rate = r.rate
                    else:
                        self.rate = 0
                    return self.rate
                else:
                    if sta.associatedAp[wlan] != 'NoAssociated':
                        r = deviceDataRate(ap, sta, wlan)
                        self.rate = r.rate
                    else:
                        self.rate = 0
                    return self.rate
            else:
                return self.set_bw(mode)        
    
    @classmethod    
    def set_bw_node_moving(self, mode):
        """ set maximum Bandwidth according Mode - Useful when there is mobility """
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
      
    @classmethod    
    def set_bw(self, mode):
        """ set maximum Bandwidth according Mode - Useful when nodes are stopped"""
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