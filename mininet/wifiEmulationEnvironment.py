"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

import subprocess

class emulationEnvironment ( object ):
    
    propagation_Model = ''
    wpa_supplicantIsRunning = False
    associationControlMethod = False
    isWiFi = False
    isCode = False
    continue_ = True
    physicalWlan = []
    apList = []
    staList = []
    wifiRadios = 0 
    DRAW = False
    totalPhy = []
    
    
    @classmethod
    def numberOfAssociatedStations( self, ap ):
        "Number of Associated Stations"
        cmd = 'iw dev %s-wlan0 station dump | grep Sta | grep -c ^' % ap     
        proc = subprocess.Popen("exec " + cmd, stdout=subprocess.PIPE,shell=True)   
        (out, err) = proc.communicate()
        output = out.rstrip('\n')
        ap.nAssociatedStations = int(output)        
    
    @classmethod
    def getPhy(self):
        """ Get phy """ 
        phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name hwsim | cut -d/ -f 6 | sort", 
                                                             shell=True).split("\n")
        phy.pop()
        return phy
    
    