"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com
"""

from mininet.wifiChannel import channelParameters

class pairingAdhocNodes ( object ):
    
    ssid_ID = 0
    dist = 0
        
    def __init__( self, sta, wlan, stationList ):
        """ init """
        self.pairing(sta, wlan, stationList)           
    
    def pairing(self, sta, wlan, stationList):
        """Pairing nodes"""
        i=1
        ref_distance = 0
        self.dist = 0        
        
        par = []
        alreadyConn = []
        cont = True
        
        list = []   
        list.append(sta)     
        
        while cont:
            if len(par) != 0:
                sta = par[0]
                par.pop(0)
            for ref_sta in stationList:
                if ref_sta != sta:
                    dist = channelParameters.getDistance(sta, ref_sta)
                    if dist != 0.0:
                        totalRange = int(sta.params['range']) + int(ref_sta.params['range'])
                        if dist < totalRange:
                            cont= True
                            ref_distance = ref_distance + dist
                            ssid = sta.params['associatedTo'][wlan]
                            ref_ssid = ref_sta.params['associatedTo'][wlan]
                            i+=1
                            if ssid != ref_ssid or ssid == sta.ssid[wlan] or ssid == '' or ref_ssid == '':
                                if ref_sta.params['associatedTo'][wlan] != '' and ref_sta.params['associatedTo'][wlan] != ssid and sta not in alreadyConn:
                                    alreadyConn.append(sta)
                                    sta.params['associatedTo'][wlan] = ref_sta.params['associatedTo'][wlan]
                                    sta.pexec('iw dev %s ibss join %s 2412' % (sta.params['wlan'][wlan], sta.params['associatedTo'][wlan]))
                                    iface = sta.params['wlan']
                                    self.confirmAdhocAssociation(sta, iface, wlan)
                                else:
                                    if sta.params['associatedTo'][wlan] != sta.ssid[wlan] + str(self.ssid_ID) and sta not in alreadyConn: 
                                        alreadyConn.append(sta)
                                        sta.params['associatedTo'][wlan] = sta.ssid[wlan] + str(self.ssid_ID)
                                        sta.pexec('iw dev %s ibss join %s 2412' % (sta.params['wlan'][wlan], sta.params['associatedTo'][wlan]))   
                                        iface = sta.params['wlan'][wlan]
                                        print "associating %s to %s..." % (iface, sta.ssid[wlan] + str(self.ssid_ID))
                                        self.confirmAdhocAssociation(sta, iface, wlan)
                                    if ref_sta not in alreadyConn:
                                        alreadyConn.append(ref_sta)
                                        ref_sta.params['associatedTo'][wlan] = sta.ssid[wlan] + str(self.ssid_ID) 
                                        ref_sta.pexec('iw dev %s ibss join %s 2412' % (ref_sta.params['wlan'][wlan], ref_sta.params['associatedTo'][wlan]))
                                        iface = ref_sta.params['wlan'][wlan]
                                        print "associating %s to %s..." % (iface, sta.ssid[wlan] + str(self.ssid_ID))
                                        self.confirmAdhocAssociation(ref_sta, iface, wlan)
                            if ref_sta not in list:
                                par.append(ref_sta)
                                list.append(ref_sta)
            if len(par) == 0:
                cont = False
                self.ssid_ID+=1
        
        self.dist = ref_distance / i
        return self.dist
    
    def confirmAdhocAssociation(self, sta, iface, wlan):
        associated = ''
        while(associated == '' or len(associated) == 0):
            sta.sendCmd("iw dev %s scan ssid | grep %s" % (iface, sta.ssid[wlan]))
            associated = sta.waitOutput()
        sta.params['frequency'][wlan] = channelParameters.frequency(sta, wlan) 