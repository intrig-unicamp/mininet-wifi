"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com
"""

from mininet.wifiChannel import channelParameters

class pairingNodes ( object ):
    
    nodesX = []
    nodesY = []
    ssid_ID = 0
    
    @classmethod  
    def clearList(self):
        del self.nodesX[:]
        del self.nodesY[:]
        
    @classmethod   
    def pairing(self, sta, wlan, stationList, **params):
        """Pairing nodes"""
        i=1
        ref_distance = 0
        self.dist = 0
        self.ssid_ID+=1
        
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
                if ref_sta != sta and ref_sta.func[wlan] == 'mesh' :
                    dist = channelParameters.getDistance(sta, ref_sta)
                    if dist != 0.0:
                        totalRange = int(sta.params['range']) + int(ref_sta.params['range'])
                        if dist < totalRange:
                            cont= True
                            ref_distance = ref_distance + dist
                            pairingNodes.nodesX.append(sta)
                            pairingNodes.nodesY.append(ref_sta)  
                            ssid = sta.params['associatedTo'][wlan]
                            ref_ssid = ref_sta.params['associatedTo'][wlan]
                            if ssid != ref_ssid or ssid == sta.ssid[wlan] or ssid == '' or ref_ssid == '':
                                if ref_sta.params['associatedTo'][wlan] != '' and \
                                        ref_sta.params['associatedTo'][wlan] != ssid and sta not in alreadyConn:
                                    alreadyConn.append(sta)
                                    sta.params['associatedTo'][wlan] = ref_sta.params['associatedTo'][wlan]
                                    sta.pexec('iw dev %s mesh join %s' % (sta.params['wlan'][wlan], sta.params['associatedTo'][wlan]))
                                else:
                                    if sta.params['associatedTo'][wlan] != sta.ssid[wlan] + str(self.ssid_ID) and \
                                                                                                sta not in alreadyConn: 
                                        alreadyConn.append(sta)
                                        sta.params['associatedTo'][wlan] = sta.ssid[wlan] + str(self.ssid_ID)
                                        sta.pexec('iw dev %s mesh leave' % sta.params['wlan'][wlan])
                                        sta.pexec('iw dev %s mesh join %s' % (sta.params['wlan'][wlan], sta.params['associatedTo'][wlan]))
                                    if ref_sta not in alreadyConn:
                                        alreadyConn.append(ref_sta)
                                        ref_sta.pexec('iw dev %s mesh leave' % sta.params['wlan'][wlan])
                                        ref_sta.params['associatedTo'][wlan] = sta.ssid[wlan] + str(self.ssid_ID)                   
                                        ref_sta.pexec('iw dev %s mesh join %s' % (ref_sta.params['wlan'][wlan], ref_sta.params['associatedTo'][wlan]))
                            if ref_sta not in list:
                                par.append(ref_sta)
                                list.append(ref_sta)
                            i+=1
            if len(par) == 0:
                cont = False
                self.ssid_ID+=1
        self.dist = ref_distance / i
        return self.dist    
    
    @classmethod   
    def pairingNodes_(self, sta, wlan, stationList, **params):
        i=1
        ref_distance = 0
        self.dist = 0
        
        for ref_sta in stationList:
            if ref_sta != sta:
                dist = channelParameters.getDistance(sta, ref_sta)
                totalRange = int(sta.params['range']) + int(ref_sta.params['range'])
                if dist < totalRange:
                    ref_distance = ref_distance + dist
                    if sta.ssid[wlan] == ref_sta.ssid[wlan]:
                        pairingNodes.nodesX.append(sta)
                        pairingNodes.nodesY.append(ref_sta)                                 
                    i+=1
        self.dist = ref_distance / i
        return self.dist    
    
class meshRouting ( object ):   
    """Mesh Routing"""    
    routing = 'custom'
    
    def __init__( self, sta, wlan, stationList ):
        """ init """
        self.customMeshRouting(sta, wlan, stationList)      
    
    def customMeshRouting(self, sta, wlan, stationList ):
        """Custom Mesh Routing"""            
        associate = False
        controlMeshMac = []
        command = ''
        for ref_sta in stationList:
            if ref_sta != sta and ref_sta.func[wlan] == 'mesh' :
                dist = channelParameters.getDistance(sta, ref_sta)                            
                totalRange = int(sta.params['range']) + int(ref_sta.params['range'])
                if dist < totalRange:
                    for w in range(len(ref_sta.params['wlan'])):
                        if ref_sta.func[w] == 'mesh':
                            if sta.params['associatedTo'][wlan] == ref_sta.params['associatedTo'][w]:                                
                                associate = True
        
        """Adding all reached target paths"""
        if associate:
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
                
                for x, y in zip(pairingNodes.nodesX, pairingNodes.nodesY):
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
            
        """mesh leave"""
        if associate == False:
            sta.pexec('iw dev %s mesh leave' % sta.params['wlan'][wlan])
            sta.params['associatedTo'][wlan] = ''