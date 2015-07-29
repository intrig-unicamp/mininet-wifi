"""
wifi setups for Mininet-WiFi.

"""

import os
import socket
import struct
import fcntl
import fileinput
import subprocess
import time
import glob

from mininet.log import  info

class checkNM ( object ):
    
    @classmethod 
    def checkNetworkManager(self, storeMacAddress): 
        self.storeMacAddress = storeMacAddress     
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
                if line.__contains__('unmanaged-devices'): 
                    print line.replace(unmatch, echo)
                else:
                    print line.rstrip()
                
    @classmethod 
    def getMacAddress(self, wlanInterface):
        self.wlanInterface = wlanInterface
        self.storeMacAddress=[]
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', '%s'[:15]) % str(self.wlanInterface))
        self.storeMacAddress.append(''.join(['%02x:' % ord(char) for char in info[18:24]])[:-1])
        return self.storeMacAddress
    
    @classmethod   
    def APfile(self, apcommand, ap):
        self.apcommand = apcommand + ("\' > %s.conf" % ap)  
        os.system(self.apcommand)
        self.cmd = ("hostapd -f apdebug.txt -B %s.conf" % ap)
        os.system(self.cmd)
    

class module( object ):
    
    @classmethod    
    def _start_module(self, wirelessRadios):
        info( "*** Enabling Wireless Module\n" )
        os.system( 'modprobe mac80211_hwsim radios=%s' % wirelessRadios )
                
    @classmethod
    def _stop_module(self):
        #info( "*** Removing Wireless Module\n" )
        if glob.glob("*.conf"):
            os.system( 'rm *.conf' )
        
        if glob.glob("*.txt"):
            os.system( 'rm *.txt' )
       
        os.system( 'rmmod mac80211_hwsim' )
        os.system( 'killall -9 hostapd' )


class phyInterface ( object ):
    
    phy = {}
    
    @classmethod
    def getPhyInterfaces(self):
        phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name hwsim | cut -d/ -f 6 | sort", 
                                                             shell=True).split("\n")
        phy.pop()
        return phy
    
    @classmethod
    def phyInt(self):
        return subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",shell=True).split("\n")
    
    
    
class station ( object ):
        
    @classmethod    
    def tcmode(self, newapif, mode):
        self.newapif = newapif
        self.mode = mode
        if (self.mode=="a"):
            os.system("tc qdisc add dev %s root tbf rate 54mbit latency 10ms burst 1540" % (self.newapif)) 
        elif(self.mode=="b"):
            os.system("tc qdisc add dev %s root tbf rate 11mbit latency 10ms burst 1540" % (self.newapif)) 
        elif(self.mode=="g"):
            os.system("tc qdisc add dev %s root tbf rate 54mbit latency 10ms burst 1540" % (self.newapif)) 
        elif(self.mode=="n"):
            os.system("tc qdisc add dev %s root tbf rate 600mbit latency 10ms burst 1540" % (self.newapif))   
    
    @classmethod    
    def associate(self, selfHost, host, ssid, isNode1):
        self.host = selfHost
        self.ssid = ssid
        self.isNode1 = isNode1
        if self.isNode1:
            self.host.cmd(host, "iw dev %s-wlan0 connect %s" % (host, self.ssid))
        else:
            self.host.cmd(host, "iw dev %s-wlan0 connect %s" % (host, self.ssid))
            
    @classmethod    
    def adhoc(self, selfHost, host, ssid, mode, waitTime):
        self.host = selfHost
        self.ssid = ssid
        self.mode = mode
        if (self.mode=="g"):
            self.host.cmd(host, "tc qdisc add dev %s-wlan0 root tbf rate 54mbit latency 10ms burst 1540" % (host)) 
        self.host.cmd(host, "iw dev %s-wlan0 set type ibss" % (host))
        #self.host.cmd(host, "ifconfig %s-wlan0 down" % (host))
        #self.host.cmd(host, "ifconfig %s-wlan0 up" % (host))
        self.host.cmd(host, "iw dev %s-wlan0 ibss join %s 2412" % (host, self.ssid))
        print "connecting %s ..." % host
        time.sleep(waitTime)
        
    @classmethod    
    def isWifi(self, isWiFi):
        return isWiFi
    
class wlanIface ( object ):
    
    @classmethod    
    def numberOfCurrentIfaces(self):
        return subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",shell=True)
                        
            
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
        
    @classmethod
    def apBridge(self, ap, iface):
        os.system("ovs-vsctl add-port %s %s" % (ap, iface))
        
    @classmethod   
    def getAPIface(self):
        return subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",shell=True)
