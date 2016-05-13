"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

import glob
import os
import subprocess
from mininet.log import debug
from mininet.wifiMobility import mobility
from mininet.wifiAccessPoint import accessPoint

class module( object ):
    """ Start and Stop mac80211_hwsim module """ 
    
    @classmethod
    def loadModule(self, wifiRadios):
        """ Start wireless Module """
        os.system( 'modprobe mac80211_hwsim radios=%s' % wifiRadios )
        debug( 'Loading %s virtual interfaces\n' % wifiRadios)
    
    @classmethod       
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
        
        if mobility.apList!=[]:
            os.system( 'killall -9 hostapd' )
        if accessPoint.wpa_supplicantIsRunning:
            os.system( 'pkill -f \'wpa_supplicant -B\'' )
            
    @classmethod    
    def start(self, wifiRadios):
        """Starting environment"""
        physicalWlan = self.getWlanList()  #Get Phisical Wlan(s)
        self.loadModule(wifiRadios) #Initatilize WiFi Module
        totalPhy = self.getPhy() #Get Phy Interfaces  
        return physicalWlan, totalPhy
        
    @classmethod
    def getWlanList(self):
        self.wlans = []        
        self.wlans = (subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",
                                                      shell=True)).split('\n')
        self.wlans.pop()
        return self.wlans
    
    @classmethod
    def getPhy(self):
        """ Get phy """ 
        phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name hwsim | cut -d/ -f 6 | sort", 
                                                             shell=True).split("\n")
        phy.pop()
        return phy
    
    @classmethod    
    def assingIface(self, stations, virtualWlan, physicalWlan, totalPhy):
        wlan = self.getWlanIface(physicalWlan)
        for sta in stations:
            for i in range(0, sta.nWlans):
                w = virtualWlan.index(str(sta))
                os.system('iw phy %s set netns %s' % ( totalPhy[w + i], sta.pid ))
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
    def getWlanIface(self, physicalWlan):
        self.newapif=[]
        self.apif = subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",
                                            shell=True).split('\n')
        for apif in self.apif:
            if apif not in physicalWlan and apif!="":
                self.newapif.append(apif)
        self.newapif = sorted(self.newapif)
        self.newapif.sort(key=len, reverse=False)
        return self.newapif