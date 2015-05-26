"""
wifi setups for Mininet.

startWiFi: 

module:

phyInterface:

station:

accessPoint:

"""

import os

import socket
import struct
import fcntl
import fileinput
import subprocess

from mininet.log import  info


class startWiFi ( object ):
    
    def __init__( self, storeMacAddress ):
        self.storeMacAddress = storeMacAddress
        self.checkNetworkManager()   
        
    
    def checkNetworkManager(self):      
        self.printMac = False   
        unmatch = ""
        if(os.path.exists('/etc/NetworkManager/NetworkManager.conf')):
            if(os.path.isfile('/etc/NetworkManager/NetworkManager.conf')):
                self.resultIface = open('/etc/NetworkManager/NetworkManager.conf')
                lines=self.resultIface
        else:
            os.makedirs("/etc/NetworkManager/")
            os.system("touch /etc/NetworkManager/NetworkManager.conf")
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
            echo = "unmanaged-devices="
        
        for n in range(len(self.storeMacAddress)): 
            if self.storeMacAddress[n] not in unmatch:
                echo = echo + "mac:"
                echo = echo + self.storeMacAddress[n] + ";"
                self.printMac = True
            
        if(self.printMac):
            for line in fileinput.input('/etc/NetworkManager/NetworkManager.conf', inplace=1): 
                line.replace(unmatch, echo)
    

class module( object ):
    
    @classmethod    
    def _start_module(self, wirelessRadios):
        info( "*** Enabling Wireless Module\n" )
        os.system( 'modprobe mac80211_hwsim radios=%s' % wirelessRadios )
                
    @classmethod
    def _stop_module(self):
        #info( "*** Removing Wireless Module\n" )
        if(os.path.exists('ap.conf')):
            os.system( 'rm ap.conf' )
            
        os.system( 'rmmod mac80211_hwsim' )
        os.system( 'killall -9 hostapd' )


class phyInterface ( object ):
    
    def __init__(self, wlanInterface, phyInterfaces):
        
        self.wlanInterface = wlanInterface
        self.phyInterfaces = phyInterfaces 
        self.storeMacAddress=[]
        self.getMacAddress()
        self.nextWiphyIface = 0
        
    def getMacAddress(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', 'wlan%s'[:15]) % str(self.wlanInterface))
        self.storeMacAddress.append(''.join(['%02x:' % ord(char) for char in info[18:24]])[:-1])
        return self.storeMacAddress
    
    @classmethod
    def getPhyInterfaces(self):
        phy = (subprocess.check_output("find /sys/kernel/debug/ieee80211 -name hwsim | cut -d/ -f 6 | sort", 
                                                             shell=True)).split("\n")
        return phy
    
    @classmethod
    def phyInt(self):
        nPhy = (subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",shell=True)).split("\n")
        return nPhy
    
 
class station ( object ):
    
    def __init__(self, host):
        self.host = host
        
    @classmethod    
    def tcmode(self, host, mode, hostname):
        self.host = host
        self.mode = mode
        self.hostname = hostname
        if (self.mode=="a"):
            self.host.cmd(self.hostname,"tc qdisc add dev %s-wlan0 root tbf rate 54mbit latency 10ms burst 1540" % (self.hostname)) 
        elif(self.mode=="b"):
            self.host.cmd(self.hostname,"tc qdisc add dev %s-wlan0 root tbf rate 11mbit latency 10ms burst 1540" % (self.hostname)) 
        elif(self.mode=="g"):
            self.host.cmd(self.hostname,"tc qdisc add dev %s-wlan0 root tbf rate 54mbit latency 10ms burst 1540" % (self.hostname)) 
        elif(self.mode=="n"):
            self.host.cmd(self.hostname,"tc qdisc add dev %s-wlan0 root tbf rate 600mbit latency 10ms burst 1540" % (self.hostname))   
            
                        
            
class accessPoint ( object ):
    
    def __init__(self, baseStationName, interfaceID, phyInterfaces, nextIface, mode, channel, 
                 ieee80211d, country_code, wmm_enabled, ssid, auth_algs, wpa, wpa_key_mgmt, 
                 rsn_pairwise, wpa_passphrase, countAP):
        
        self.baseStationName = baseStationName
        self.interfaceID = interfaceID
        self.phyInterfaces = phyInterfaces
        self.nextIface = nextIface
        self.mode = mode
        self.channel = channel
        self.ieee80211d = ieee80211d
        self.country_code = country_code
        self.wmm_enabled = wmm_enabled
        self.ssid = ssid
        self.auth_algs = auth_algs
        self.wpa = wpa
        self.wpa_key_mgmt = wpa_key_mgmt
        self.rsn_pairwise = rsn_pairwise
        self.wpa_passphrase = wpa_passphrase
        self.countAP = countAP
        self.apcommand = ""
        
    def addAccessPoint(self):
        if(len(self.baseStationName)==1):            
            self.cmd = ("echo \"")
            """General Configurations"""             
            if(self.interfaceID!=None):
                if(self.phyInterfaces[0][:4]!="wlan"):
                    self.cmd = self.cmd + ("interface=wlan%s" % (self.nextIface)) # the interface used by the AP
                else:
                    self.cmd = self.cmd + ("interface=wlan%s" % (self.nextIface+len(self.phyInterfaces))) # the interface used by the AP
                    
            """Not using at the moment"""
            #self.cmd = self.cmd + ("\ndriver=nl80211")
            if(self.mode!=None):
                self.cmd = self.cmd + ("\nhw_mode=%s" % self.mode) # g simply means 2.4GHz
            if(self.channel!=None):
                self.cmd = self.cmd + ("\nchannel=%s" % self.channel) # the channel to use 
            if(self.ieee80211d!=None):
                self.cmd = self.cmd + ("\nieee80211d=%s" % self.ieee80211d) # limit the frequencies used to those allowed in the country
            if(self.country_code!=None):
                self.cmd = self.cmd + ("\ncountry_code=%s" % self.country_code) # the country code
            #self.cmd = self.cmd + ("\nieee8021self.apcommand = ""1n=1") # 802.11n support
            if(self.wmm_enabled!=None):
                self.cmd = self.cmd + ("\nwmm_enabled=%s" % self.wmm_enabled) # QoS support
            """Not using at the moment"""
            #self.cmd = self.cmd + ("\nmacaddr_acl=0\nauth_algs=1\nignore_broadcast_ssid=0")
                
            self.cmd = self.cmd + "\n"    
            """AP1"""    
            if(self.ssid!=None):
                self.cmd = self.cmd + ("\nssid=%s" % self.ssid) # the name of the AP
            if(self.auth_algs!=None):
                self.cmd = self.cmd + ("\nauth_algs=%s" % self.auth_algs) # 1=wpa, 2=wep, 3=both
            if(self.wpa!=None):
                self.cmd = self.cmd + ("\nwpa=%s" % self.wpa) # WPA2 only
            if(self.wpa_key_mgmt!=None):
                self.cmd = self.cmd + ("\nwpa_key_mgmt=%s" % self.wpa_key_mgmt ) 
            if(self.rsn_pairwise!=None):
                self.cmd = self.cmd + ("\nrsn_pairwise=%s" % self.rsn_pairwise)  
            if(self.wpa_passphrase!=None):
                self.cmd = self.cmd + ("\nwpa_passphrase=%s" % self.wpa_passphrase)                  
        
        elif(len(self.baseStationName)>self.countAP and len(self.baseStationName) != 1):
            """From AP2"""
            self.cmd = self.apcommand
            self.cmd = self.cmd + "\n"
            self.cmd = self.cmd + ("\nbss=wlan%s" % self.nextIface) # the interface used by the AP
            if(self.ssid!=None):
                self.cmd = self.cmd + ("\nssid=teste3") # the name of the AP
                #self.cmd = self.cmd + ("\nssid=%s" % self.ssid) # the name of the AP
            if(self.auth_algs!=None):
                self.cmd = self.cmd + ("\nauth_algs=%s" % self.auth_algs) # 1=wpa, 2=wep, 3=both
            if(self.wpa!=None):
                self.cmd = self.cmd + ("\nwpa=%s" % self.wpa) # WPA2 only
            if(self.wpa_key_mgmt!=None):
                self.cmd = self.cmd + ("\nwpa_key_mgmt=%s" % self.wpa_key_mgmt ) 
            if(self.rsn_pairwise!=None):
                self.cmd = self.cmd + ("\nrsn_pairwise=%s" % self.rsn_pairwise)  
            if(self.wpa_passphrase!=None):
                self.cmd = self.cmd + ("\nwpa_passphrase=%s" % self.wpa_passphrase)  
            self.countAP = len(self.baseStationName)
            self.apcommand = ""
        
        self.apcommand = self.apcommand + self.cmd
        return self.apcommand
    
    @classmethod
    def apBridge(self, ap, iface):
        os.system("ovs-vsctl add-port %s wlan%s" % (ap, iface))
        #subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",shell=True)
        
    @classmethod   
    def APfile(self, apcommand):
        self.apcommand = apcommand + ("\" > ap.conf")
        os.system(self.apcommand)
        self.cmd = ("hostapd -B ap.conf")
        os.system(self.cmd)
        
