"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

from mininet.wifiMobilityModels import distance

class listNodes ( object ):
    
    nodesX = []
    nodesY = []
    
    @classmethod  
    def clearList(self):
        del self.nodesX[:]
        del self.nodesY[:]
    
    @classmethod  
    def printList(self):
        print self.nodesX
        print self.nodesY
    
    @classmethod   
    def pairingNodes(self, node, wlan, stationList, **params):
        i=1
        ref_distance = 0
        sta = node
        controlMeshMac = []
        miss_sta = []
        self.dist = 0
        
        for n in stationList:
            miss_sta.append(n)
        miss_sta.remove(node)
        for ref_sta in miss_sta:
            #To see...
            if ref_sta.position != 0 and sta.position !=0:
                d = distance(sta, ref_sta)
                dist = d.dist
            else:
                dist = 1
                    
            totalRange = sta.range + ref_sta.range
            if dist < totalRange:
                ref_distance = ref_distance + dist
                if sta.ssid[wlan] == ref_sta.ssid[wlan]:
                    mac = ref_sta.meshMac[wlan]    
                    listNodes.nodesX.append(sta)
                    listNodes.nodesY.append(ref_sta)                 
                    if mac not in controlMeshMac:
                        controlMeshMac.append(mac)
                    if ref_sta not in sta.staAssociated:
                        sta.staAssociated.append(ref_sta)
                i+=1
        self.dist = ref_distance / i
        return self.dist
    
    
class meshRouting ( object ):   
    """Mesh Routing"""
    
    @classmethod   
    def customMeshRouting(self, node, wlan, stationList, **params):
       
        sta = node        
        
        if wlan < sta.nWlans:
            associate = False
            controlMeshMac = []
            command = ''
            iface = 'mp'
           
            for ref_sta in stationList:
                if ref_sta != sta:
                    #To see...
                    if ref_sta.position != 0 and sta.position !=0:
                        d = distance(sta, ref_sta)
                        dist = d.dist
                    else:
                        dist = 1
                            
                    totalRange = sta.range + ref_sta.range
                    if dist < totalRange:
                        for w in range(ref_sta.nWlans):
                            if ref_sta.func[w] == 'mesh':
                                if sta.ssid[wlan] == ref_sta.ssid[w]:                                
                                    associate = True
            
            """Mesh Join""" 
            if associate == True and sta.isAssociated[wlan] == False:
                sta.pexec('iw dev %s-%s%s mesh join %s' % (sta, iface, wlan, sta.ssid[wlan]))   
                sta.isAssociated[wlan] = True
            
            """Adding all reached target paths"""
            if associate:
                sta.isAssociated[wlan] = True
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
                            command = 'iw dev %s-%s%s mpath new %s next_hop %s' % (sta, iface, wlan, y.meshMac[wlan], y.meshMac[wlan])
                            sta.pexec(command)
                            exist.append(y)
                            controlMeshMac.append(y.meshMac[wlan])
                            sta_ref.append(y)
                        elif x == newsta and y not in exist:
                            command = 'iw dev %s-%s%s mpath new %s next_hop %s' % (sta, iface, wlan, y.meshMac[wlan], x.meshMac[wlan])
                            sta.pexec(command)
                            exist.append(y)
                            controlMeshMac.append(y.meshMac[wlan])
                            sta_ref.append(y)
                    if newsta in sta_ref:
                        sta_ref.remove(newsta)
            
            """delete all unknown paths"""
            if associate:
                for y in stationList:
                    for w in range(y.nWlans):
                        if y.meshMac[w] not in controlMeshMac:
                            sta.pexec('iw dev %s-%s%s mpath del %s' % (sta, iface, wlan, y.meshMac[w]))
            
                #sta.cmd('%s' % (command[:-3]))
                sta.isAssociated[wlan] = True
                
            """mesh leave"""
            if associate == False:
                sta.pexec('iw dev %s-%s%s mesh leave' % (sta, iface, wlan))
                sta.isAssociated[wlan] = False
                
            