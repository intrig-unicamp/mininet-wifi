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
from netlink.util import Rate

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
             
    
    """
    get Mac Address of any Interface   
    """
    @classmethod 
    def getMacAddress(self, wlanInterface):
        self.wlanInterface = wlanInterface
        self.storeMacAddress=[]
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', '%s'[:15]) % str(self.wlanInterface))
        self.storeMacAddress.append(''.join(['%02x:' % ord(char) for char in info[18:24]])[:-1])
        return self.storeMacAddress
    
    """
    run an Access Point and create the file  
    """
    @classmethod   
    def APfile(self, apcommand, ap):
        self.apcommand = apcommand + ("\' > %s.conf" % ap)  
        os.system(self.apcommand)
        self.cmd = ("hostapd -f apdebug.txt -B %s.conf" % ap)
        os.system(self.cmd)
        
        
class association( object ):
    
    @classmethod    
    def bw(self, mode):
        
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
    def checkDistance(self, mode, distance):
        
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
    def setAdhocParameters(self, selfHost, host, mode, newIface, **params):
        
        self.host = selfHost
        self.mode = mode
        rate = 0
        latency = 10
        
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
            Set wifi Infrastrucure Parameters. Have to use models to loss, latency, bw...    
        """
        self.host = host
        self.mode = mode
        rate = 0
        latency = 10 + distance
        
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
        self.host.cmd("tc qdisc replace dev %s-wlan0 root tbf rate %.2fmbit latency %.2fms burst 15k" % (self.host, rate, latency))   
        

class module( object ):
    
    """
    Start wireless Module 
    """
    @classmethod    
    def _start_module(self, wirelessRadios):
        info( "*** Enabling Wireless Module\n" )
        os.system( 'modprobe mac80211_hwsim radios=%s' % wirelessRadios )
        #os.system( 'insmod mac80211_hwsim.ko radios=%s' % wirelessRadios )
     
    """
    Stop wireless Module 
    """           
    @classmethod
    def _stop_module(self):
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
    
    """
    Go to next Interface
    """   
    @classmethod 
    def setNextIface(self):
        self.nextIface+=1
    
    """
    Get phy Interface
    """   
    @classmethod
    def getPhyInterfaces(self):
        phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name hwsim | cut -d/ -f 6 | sort", 
                                                             shell=True).split("\n")
        phy.pop()
        self.setNextIface()
        return phy
    
    """
    get Wireless Interface
    """   
    @classmethod
    def phyInt(self):
        return subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",shell=True).split("\n")
    
        
class station ( object ):
    
    nextWlan = {}    
  
    """
    Station Associate to an Access Point
    """   
    @classmethod    
    def associate(self, selfHost, host, ssid):
        self.host = selfHost
        self.ssid = ssid
        self.host.cmd(host, "iw dev %s-wlan0 connect %s" % (host, self.ssid))
        time.sleep(0.1)
    
          
    """
    Adhoc mode
    """   
    @classmethod    
    def adhoc(self, selfHost, host, ssid, mode, waitTime, **params):
        
        self.host = selfHost
        self.ssid = ssid
        self.mode = mode
        
        try: # if new iface
            options = dict( params )
            self.interface = options[ 'interface' ]
            association.setAdhocParameters(selfHost, host, mode, newIface=True, **params)
            self.host.cmd(host, "iw dev %s-%s set type ibss" % (host, self.interface))
            self.host.cmd(host, "iw dev %s-%s ibss join %s 2412" % (host, self.interface, self.ssid))
            print "connecting %s ..." % host
            time.sleep(waitTime)
        except: # if not
            association.setAdhocParameters(selfHost, host, mode, newIface=False, **params)
            self.host.cmd(host, "iw dev %s-wlan0 set type ibss" % (host))
            self.host.cmd(host, "iw dev %s-wlan0 ibss join %s 2412" % (host, self.ssid))
            print "connecting %s ..." % host
            time.sleep(waitTime)
        
    """
    Add phy Interface to Stations
    """ 
    @classmethod    
    def addIface(self, station):
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
     
    """
    Rename wireless interface if necessary
    """
    @classmethod    
    def renameIface(self, station, nextWlan, iface):
        station.cmd('ip link set dev %s name %s-wlan%s' % (iface[:5], station, nextWlan ))
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
        os.system("ovs-vsctl add-port %s %s" % (ap, iface))
        
        
    @classmethod
    def setBw(self, newapif, mode):
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
        
            
class wifiParameters ( object ):
    
    @classmethod
    def noise(self, host): 
        self.host = host
        print self.host.cmd('iw dev %s-wlan0 survey noise %d' % (host, int(str(self.host)[3:])+60))
        
    @classmethod 
    def mobility(self):
        """
        In development    
        """
        
    @classmethod 
    def getDistance(self, src, dst, pos_src, pos_dst):
        """
        In development  
        """
        points = np.array([(pos_src[0], pos_src[1], pos_src[2]), (pos_dst[0], pos_dst[1], pos_dst[2])])
        dist = distance.pdist(points)
        return dist
    
    @classmethod 
    def printDistance(self, src, dst, pos_src, pos_dst):
        dist = self.getDistance(src, dst, pos_src, pos_dst)
        print ("The distance between %s and %s is %.2f meters\n" % (src, dst, float(dist)))
        
        
    @classmethod   
    def position(self, src, position):
        """
        In development (x,y,z)   
        """
        self.pos_x = position[0]
        self.pos_y = position[1]
        self.pos_z = position[2]        
        print "----------------\nPosition of %s\n----------------\nPosition X: %s\nPosition Y: %s\nPosition Z: %s\n" % (src, self.pos_x, self.pos_y, self.pos_z)
        
        
        