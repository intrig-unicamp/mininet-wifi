"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com
"""

from mininet.wifiChannel import channelParameters

class listNodes ( object ):
    
    nodesX = []
    nodesY = []
    ssid_ID = 0
    
    @classmethod  
    def clearList(self):
        del self.nodesX[:]
        del self.nodesY[:]
        
    @classmethod   
    def pairingNodes_(self, sta, wlan, stationList, **params):
        """Pairing nodes"""
        i=1
        ref_distance = 0
        self.dist = 0
        self.ssid_ID+=1
        for ref_sta in stationList:
            if ref_sta != sta:
                dist = channelParameters.getDistance(sta, ref_sta)
                if dist != 0.0:
                    totalRange = sta.range + ref_sta.range
                    if dist < totalRange:
                        ref_distance = ref_distance + dist
                        listNodes.nodesX.append(sta)
                        listNodes.nodesY.append(ref_sta)  
                        ssid = sta.params['associatedTo'][wlan]
                        ref_ssid = ref_sta.params['associatedTo'][wlan]
                        if ssid != ref_ssid or ssid == sta.ssid[wlan] or ssid == '' or ref_ssid == '':
                            sta.params['associatedTo'][wlan] = sta.ssid[wlan] + str(self.ssid_ID)
                            ref_sta.params['associatedTo'][wlan] = sta.ssid[wlan] + str(self.ssid_ID)  
                        i+=1
        self.dist = ref_distance / i
        return self.dist
    
    
    @classmethod   
    def pairingNodes(self, sta, wlan, stationList, **params):
        i=1
        ref_distance = 0
        self.dist = 0
        
        for ref_sta in stationList:
            if ref_sta != sta:
                dist = channelParameters.getDistance(sta, ref_sta)
                totalRange = sta.range + ref_sta.range
                if dist < totalRange:
                    ref_distance = ref_distance + dist
                    if sta.ssid[wlan] == ref_sta.ssid[wlan]:
                        listNodes.nodesX.append(sta)
                        listNodes.nodesY.append(ref_sta)                                 
                    i+=1
        self.dist = ref_distance / i
        return self.dist
    
    
class meshRouting ( object ):   
    """Mesh Routing"""    
    routing = ''
    
    @classmethod   
    def customMeshRouting(self, sta, wlan, stationList, **params):
        """Custom Mesh Routing"""            
        associate = False
        controlMeshMac = []
        command = ''
        
        for ref_sta in stationList:
            if ref_sta != sta:
                dist = channelParameters.getDistance(sta, ref_sta)                            
                totalRange = sta.range + ref_sta.range
                if dist < totalRange:
                    for w in range(len(ref_sta.params['wlan'])):
                        if ref_sta.func[w] == 'mesh':
                            if sta.params['associatedTo'][wlan] == ref_sta.params['associatedTo'][w]:                                
                                associate = True
        
        """Mesh Join""" 
        if associate == True and sta.params['associatedTo'][wlan] == '':                
            sta.pexec('ifconfig %s up' % sta.params['wlan'][wlan])
            sta.pexec('iw dev %s mesh join %s' % (sta.params['wlan'][wlan], sta.params['associatedTo'][wlan]))   
            sta.params['associatedTo'][wlan] = 'mesh'
        
        """Adding all reached target paths"""
        if associate:
            sta.pexec('ifconfig %s up' % sta.params['wlan'][wlan])        
            sta.params['associatedTo'][wlan] = 'mesh'
            exist = []
            sta_ref = []
            sta_ref.append(sta)
            j = 0
            exist.append(sta)
            while j < len(stationList):
                j+=1         
                if sta_ref == []:
                    break       
                else:
                    newsta = sta_ref[0]
                
                for x, y in zip(listNodes.nodesX, listNodes.nodesY):
                    if x == sta and y not in exist:
                        command = 'iw dev %s mpath new %s next_hop %s' % (sta.params['wlan'][wlan], y.meshMac[wlan], y.meshMac[wlan])
                        sta.pexec(command)
                        exist.append(y)
                        controlMeshMac.append(y.meshMac[wlan])
                        sta_ref.append(y)
                    elif x == newsta and y not in exist:
                        command = 'iw dev %s mpath new %s next_hop %s' % (sta.params['wlan'][wlan], y.meshMac[wlan], x.meshMac[wlan])
                        sta.pexec(command)
                        exist.append(y)
                        controlMeshMac.append(y.meshMac[wlan])
                        sta_ref.append(y)
                if newsta in sta_ref:
                    sta_ref.remove(newsta)
        
        """delete all unknown paths"""
        if associate:
            for y in stationList:
                for w in range(len(y.params['wlan'])):
                    if y.meshMac[w] not in controlMeshMac:
                        sta.pexec('iw dev %s mpath del %s' % (sta.params['wlan'][wlan], y.meshMac[w]))        
            sta.params['associatedTo'][wlan] = 'mesh'
            
        """mesh leave"""
        if associate == False:
            sta.pexec('iw dev %s mesh leave' % sta.params['wlan'][wlan])            
            sta.pexec('ifconfig %s down' % sta.params['wlan'][wlan])
            sta.params['associatedTo'][wlan] = ''