"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""
from mininet.wifiParameters import wifiParameters
from mininet.wifiMobility import mobility


class association ( object ):
    """Association Class"""
    
    printCon = True
    isCode = False
    
    @classmethod 
    def confirmInfraAssociation(self, node1, node2, wlan):
        associated = ''
        if self.printCon:
            print "Associating %s to %s" % (node1, node2)
        while(associated == '' or len(associated[0]) == 15):
            associated = self.isAssociated(node1, wlan)
        iface = str(node1)+'-wlan%s' % wlan
        wifiParameters.getWiFiParameters(node1, wlan, iface) 
        mobility.numberOfAssociatedStations(node2)
        node1.associatedAp[wlan] = node2
        mobility.getAPsInRange(node1)
            
    @classmethod    
    def isAssociated(self, sta, iface):
        associated = sta.pexec("iw dev %s-wlan%s link" % (sta, iface))
        return associated
            
    @classmethod 
    def cmd_associate(self, sta, ap, wlan):
        if sta.passwd == None:
            #sta.pexec('iw dev %s-wlan%s connect %s' % (sta, wlan, ap.ssid[0]))
            sta.pexec('iwconfig %s-wlan%s essid %s ap %s' % (sta, wlan, ap.ssid[0], ap.mac))
        elif sta.encrypt == 'wpa' or sta.encrypt == 'wpa2':
            self.associate_wpa(sta, wlan, ap.ssid[0], sta.passwd)
        elif sta.encrypt == 'wep':
            self.associate_wep(sta, wlan, ap.ssid[0], sta.passwd)
        self.confirmInfraAssociation(sta, ap, wlan)
        sta.associatedAp[wlan] = ap 
        ap.associatedStations.append(sta)
        sta.ssid[wlan] = ap.ssid[0]
        sta.wlanToAssociate += 1
    
    @classmethod 
    def associate_wpa(self, sta, wlan, ssid, passwd):
        sta.cmd("wpa_supplicant -B -D nl80211 -i %s-wlan%s -c <(wpa_passphrase \"%s\" \"%s\")" \
                % (sta, wlan, ssid, passwd))
    
    @classmethod 
    def associate_wep(self, sta, wlan, ssid, passwd):    
        sta.cmd('iw dev %s-wlan%s connect %s key 0:%s' \
                % (sta, wlan, ssid, passwd))