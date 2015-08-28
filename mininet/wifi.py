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
import time
import glob

from mininet.log import info
import numpy as np
import scipy.spatial.distance as distance 


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
        
        
class association( object ):
    
    @classmethod    
    def set_bw(self, mode):
        """
            set the Bandwidth  
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
    
    @classmethod    
    def getDistance(self, mode, distance):
        """
            Get Distance for association or not    
        """
        doAssociation = True
        
        if (mode=='a' and distance > 33):
            doAssociation = False
        elif(mode=='b' and distance > 50):
            doAssociation = False
        elif(mode=='g' and distance > 33):
            doAssociation = False
        elif(mode=='n' and distance > 70):
            doAssociation = False
        elif(mode=='ac' and distance > 100):
            doAssociation = False        
        
        return doAssociation
    
    
    @classmethod    
    def setAdhocParameters(self, selfhost, host, mode, **params):
        """
            Set wifi AdHoc Parameters. Have to use models for loss, latency, bw...    
        """
        self.mode = mode
        rate = 0
        latency = 10
        newIface = False
        
        try:
            options = dict( params )
            self.interface = options[ 'interface' ]
            newIface = True
        except:
            newIface = False
            self.host = selfhost
        
        if newIface:            
            if (self.mode=="a"):
                rate = 54
            elif (self.mode=="b"):
                rate = 11
            elif (self.mode=="g"):
                rate = 54
            elif (self.mode=="n"):
                rate = 600
            elif (self.mode=="ac"):
                rate = 6777
            self.host.cmd(host, "tc qdisc add dev %s-%s root htb rate %smbit latency %sms" % (host, self.interface, rate, latency)) 
        else:
            if (self.mode=="a"):
                rate = 54
            elif (self.mode=="b"):
                rate = 11
            elif (self.mode=="g"):
                rate = 54
            elif (self.mode=="n"):
                rate = 600
            elif (self.mode=="ac"):
                rate = 6777
            self.host.cmd(host, "tc qdisc add dev %s-wlan0 root htb rate %smbit latency %sms burst 15k" % (host, rate, latency))
            
    
    @classmethod    
    def setInfraParameters(self, host, mode, distance):
        """
            Set wifi Infrastrucure Parameters. Have to use models for loss, latency, bw...    
        """
        self.host = host
        self.mode = mode
        rate = 0
        latency = 10 + distance
        #loss = 0.01 + distance/10
        delay = 5 * distance
        disassociate = False
        
        if (self.mode=="a"):
            rate = 54 - distance/2
        elif (self.mode=="b"):
            rate = 11 - distance/2
        elif (self.mode=="g"):
            rate = 54 - distance/2 
        elif (self.mode=="n"):
            rate = 600 - distance/2
        elif (self.mode=="ac"):
            rate = 6777 - distance/2
        self.host.cmd("tc qdisc replace dev %s-wlan0 root netem rate %.2fmbit latency %.2fms delay %.2fms" % (self.host, rate, latency, delay)) 
        #self.host.cmd("tc qdisc replace dev %s-wlan0 root netem rate %.2fmbit loss %.1f%% latency %.2fms delay %.2fms" % (self.host, rate, loss, latency, delay)) 
        #self.host.cmd("tc qdisc replace dev %s-wlan0 root tbf rate %.2fmbit latency %.2fms burst 15k" % (self.host, rate, latency)) 
        
        if (mode=='a' and distance > 33):
            disassociate = True
        elif(mode=='b' and distance > 50):
            disassociate = True
        elif(mode=='g' and distance > 33):
            disassociate = True
        elif(mode=='n' and distance > 70):
            disassociate = True
        elif(mode=='ac' and distance > 100):
            disassociate = True     
        if disassociate:
            self.host.cmd("iw dev %s-wlan0 disconnect" % (self.host))
        

class module( object ):
    
    @classmethod    
    def _start_module(self, wirelessRadios):
        """
             Start wireless Module 
        """
        info( "*** Enabling Wireless Module\n" )
        os.system( 'modprobe mac80211_hwsim radios=%s' % wirelessRadios )
        #os.system( 'insmod mac80211_hwsim.ko radios=%s' % wirelessRadios )
     
    @classmethod
    def _stop_module(self):
        """
            Stop wireless Module 
        """   
        if glob.glob("*.conf"):
            os.system( 'rm *.conf' )
        
        if glob.glob("*.txt"):
            os.system( 'rm *.txt' )
       
        os.system( 'rmmod mac80211_hwsim' )
        #os.system( 'rmmod mac80211_hwsim.ko' )
        os.system( 'killall -9 hostapd' )


class phyInterface ( object ):
    
    phy = {}
    nextIface = 0
    
    @classmethod 
    def setNextIface(self):
        """
            Go to next Interface
        """   
        self.nextIface+=1
     
    @classmethod
    def getPhyInterfaces(self):
        """
            Get phy Interface
        """ 
        phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name hwsim | cut -d/ -f 6 | sort", 
                                                             shell=True).split("\n")
        phy.pop()
        self.setNextIface()
        return phy
    
    @classmethod
    def phyInt(self):
        """
            get Wireless Interface
        """   
        return subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'", shell=True).split("\n")
    
        
class station ( object ):
    
    nextWlan = {}    
  
    @classmethod    
    def associate(self, selfHost, host, ssid):
        """
            Station Associate to an Access Point
        """   
        self.host = selfHost
        self.ssid = ssid
        self.host.cmd(host, "iw dev %s-wlan0 connect %s" % (host, self.ssid))
        time.sleep(0.1)
    
          
    @classmethod    
    def adhoc(self, selfhost, host, ssid, mode, waitTime, **params):
        """
            Adhoc mode
        """   
        self.ssid = ssid
        self.mode = mode
        hasIface = False   
        
        try:
            options = dict( params )
            self.interface = options[ 'interface' ]
            hasIface = True
        except:
            hasIface = False
            self.host = selfhost
        
        if hasIface: # if new iface
            association.setAdhocParameters(selfhost, host, mode, **params)
            self.host.cmd(host, "iw dev %s-%s set type ibss" % (host, self.interface))
            self.host.cmd(host, "iw dev %s-%s ibss join %s 2412" % (host, self.interface, self.ssid))
            print "connecting %s ..." % host
            time.sleep(waitTime)
        else: # if not
            association.setAdhocParameters(selfhost, host, mode, **params)
            self.host.cmd(host, "iw dev %s-wlan0 set type ibss" % (host))
            self.host.cmd(host, "iw dev %s-wlan0 ibss join %s 2412" % (host, self.ssid))
            print "connecting %s ..." % host
            time.sleep(waitTime)
        
    @classmethod    
    def addIface(self, station):
        """
            Add phy Interface to Stations
        """ 
        phy = phyInterface.getPhyInterfaces()
        phyInterface.phy[station] = phy[phyInterface.nextIface-1][3:]
        os.system("iw phy phy%s set netns %s" % (phyInterface.phy[station], station.pid)) 
        wif = station.cmd("iwconfig 2>&1 | grep IEEE | awk '{print $1}'").split("\n")
        wif.pop()
        for iface in wif:
            if iface[:4]=="wlan":
                try:
                    self.nextWlan[station] += 1
                except:
                    self.nextWlan[station] = 1
                netxWlan = self.nextWlan[station] 
                self.renameIface(station, netxWlan, iface)
     
    @classmethod    
    def renameIface(self, station, nextWlan, iface):
        """
            Rename wireless interface if necessary
        """
        iface = iface[:-1]
        station.cmd('ip link set dev %s name %s-wlan%s' % (iface, station, nextWlan ))
        station.cmd('ifconfig %s-wlan%s up' % (station, nextWlan))    
                        
            
class accessPoint ( object ):    
    """
        Starts an Access Point
    """
    @classmethod
    def start(self, interfaceID, nextIface, ssid, mode, channel, 
              country_code, auth_algs, wpa, wpa_key_mgmt, rsn_pairwise, wpa_passphrase):
        self.cmd = ("echo \'")
        """General Configurations"""             
        if(interfaceID!=None):
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
        self.mode = mode
        if (self.mode=="a"):
            rate = 54
        elif (self.mode=="b"):
            rate = 11
        elif (self.mode=="g"):
            rate = 54
        elif (self.mode=="n"):
            rate = 600
        elif (self.mode=="ac"):
            rate = 6777
        os.system("tc qdisc add dev %s root tbf rate %smbit latency 2ms burst 15k" % (self.newapif, rate))   
        

class mobility ( object ):    
    """
        Mobility 
    """            
    @classmethod   
    def move(self, node, diffTime, speed, startposition, endposition):      
        """
            Moving nodes
            diffTime: important to calculate the speed of moving  
        """
        pos_x = float(endposition[0]) - float(startposition[0])
        pos_y = float(endposition[1]) - float(startposition[1])
        pos_z = float(endposition[2]) - float(startposition[2])
        
        pos = '%.5f,%.5f,%.5f' % (pos_x/diffTime, pos_y/diffTime, pos_z/diffTime)
        pos = pos.split(',')
        return pos       
        
        
    @classmethod 
    def getDistance(self, src, dst, pos_src, pos_dst):
        """
            Get the distance between two points  
        """
        points = np.array([(pos_src[0], pos_src[1], pos_src[2]), (pos_dst[0], pos_dst[1], pos_dst[2])])
        dist = distance.pdist(points)
        return dist
    
    
    @classmethod 
    def printDistance(self, src, dst, pos_src, pos_dst):
        """
            Print the distance between two points
        """
        dist = self.getDistance(src, dst, pos_src, pos_dst)
        print ("The distance between %s and %s is %.2f meters\n" % (src, dst, float(dist)))
    
        
    @classmethod   
    def printPosition(self, src, position):
        """
            Print position of STAs and APs  
        """
        self.pos_x = position[0]
        self.pos_y = position[1]
        self.pos_z = position[2]        
        print "----------------\nPosition of %s\n----------------\nPosition X: %.2f\nPosition Y: %.2f\nPosition Z: %.2f\n" % (src, self.pos_x, self.pos_y, self.pos_z)
        
    
class wifiParameters ( object ):
    """
        WiFi Parameters 
    """
    @classmethod
    def getNoiseInfo(self, host): 
        """
            Only get noise info **in development**
        """
        self.host = host
        print self.host.cmd('iw dev %s-wlan0 survey noise %d' % (host, int(str(self.host)[3:])+60))    