"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com
"""

from mininet.wifiChannel import setChannelParams
from mininet.wmediumdConnector import WmediumdServerConn, WmediumdSNRLink
from mininet.log import debug 

class listNodes (object):

    nodesX = []
    nodesY = []
    ssid_ID = 0
    list = []

    @classmethod
    def clearList(self):
        del self.nodesX[:]
        del self.nodesY[:]

    @classmethod
    def pairingNodes(self, sta, wlan, nodes, **params):
        """Pairing nodes"""
        i = 1
        ref_distance = 0
        self.ssid_ID += 1
        cont = True
        par = []
        alreadyConn = []
        self.list = []
        self.list.append(sta)
        currentSta = sta

        while cont:
            if len(par) != 0:
                sta = par[0]
                par.pop(0)
            for ref_sta in nodes:
                if ref_sta.type == 'vehicle':
                    car = ref_sta
                    ref_sta = ref_sta.params['carsta']
                    ref_sta.params['position'] = car.params['position']
                    ref_sta.params['range'] = car.params['range']
                if ref_sta != sta and ref_sta.func[wlan] == 'mesh' :
                    dist = setChannelParams.getDistance(sta, ref_sta)
                    if dist >= 0.1:
                        totalRange = int(sta.params['range'])
                        ref_totalRange = int(ref_sta.params['range'])
                        if ref_totalRange > totalRange:
                            totalRange = ref_totalRange
                        if dist <= totalRange:
                            cont = True
                            ref_distance = ref_distance + dist
                            if currentSta == sta:
                                self.nodesX.append(sta)
                                self.nodesY.append(ref_sta)
                            ssid = sta.params['associatedTo'][wlan]
                            ref_ssid = ref_sta.params['associatedTo'][wlan]
                            if ssid != ref_ssid or ssid == sta.params['ssid'][wlan] or ssid == '':
                                if sta not in alreadyConn:
                                    if ref_ssid != '' and ref_ssid != ssid:
                                        alreadyConn.append(sta)
                                        sta.params['associatedTo'][wlan] = ref_sta.params['associatedTo'][wlan]
                                        self.meshAssociation(sta, wlan)
                                    elif ssid != sta.params['ssid'][wlan] + str(self.ssid_ID):
                                        alreadyConn.append(sta)
                                        sta.params['associatedTo'][wlan] = sta.params['ssid'][wlan] + str(self.ssid_ID)
                                        self.meshAssociation(sta, wlan)
                                if ref_sta not in alreadyConn:
                                    alreadyConn.append(ref_sta)
                                    ref_sta.params['associatedTo'][wlan] = sta.params['ssid'][wlan] + str(self.ssid_ID)
                                    self.meshAssociation(ref_sta, wlan)
                            if ref_sta not in self.list:
                                par.append(ref_sta)
                                self.list.append(ref_sta)
                            i += 1
            if len(par) == 0:
                cont = False
                self.ssid_ID += 1
        dist = ref_distance / i
        return dist
    
    
    @classmethod
    def meshAssociation(self, sta, wlan):
        debug("associating %s to %s...\n" % (sta.params['wlan'][wlan], sta.params['associatedTo'][wlan]))
        sta.pexec('iw dev %s mesh join %s' % (sta.params['wlan'][wlan], sta.params['associatedTo'][wlan]))


class meshRouting (object):
    """Mesh Routing"""
    routing = ''

    def __init__(self, stations):
        
        for node in stations:
            for wlan in range(0, len(node.params['wlan'])):
                if node.func[wlan] == 'mesh':
                    """Mesh Routing"""
                    if node.type == 'vehicle':
                        node = node.params['carsta']
                        wlan = 0
                    self.customMeshRouting(node, wlan, stations)
        listNodes.clearList()
        listNodes.ssid_ID = 0

    def customMeshRouting(self, sta, wlan, stations):
        """Custom Mesh Routing"""
        associate = False
        controlMeshMac = []
        command = ''
        for ref_sta in stations:
            if ref_sta.type == 'vehicle':
                ref_sta = ref_sta.params['carsta']
            for ref_wlan in range(len(ref_sta.params['wlan'])):
                if ref_sta != sta and ref_sta.func[ref_wlan] == 'mesh' and 'position' in sta.params:
                    dist = setChannelParams.getDistance(sta, ref_sta)
                    totalRange = int(sta.params['range'])
                    ref_totalRange = int(ref_sta.params['range'])
                    if ref_totalRange > totalRange:
                        totalRange = ref_totalRange
                    if dist <= totalRange:
                        if ref_sta.func[ref_wlan] == 'mesh':
                            if sta.params['associatedTo'][wlan] == ref_sta.params['associatedTo'][ref_wlan]:
                                associate = True
                                if not WmediumdServerConn.connected:
                                    setChannelParams(sta=sta, wlan=wlan, dist=dist)
                elif 'position' not in sta.params:
                    associate = True

        """Adding all reached target paths"""
        if associate:
            exist = []
            sta_ref = []
            sta_ref.append(sta)
            j = 0
            exist.append(sta)
            while j < len(stations):
                j += 1
                if sta_ref == []:
                    break
                else:
                    newsta = sta_ref[0]
                for x, y in zip(listNodes.nodesX, listNodes.nodesY):
                    if x == sta and y not in exist:
                        if WmediumdServerConn.connected:
                            WmediumdServerConn.send_snr_update(WmediumdSNRLink(sta.wmediumdIface, y.wmediumdIface, sta.params['snr'][wlan]))
                        else:
                            command = 'iw dev %s mpath new %s next_hop %s' % (sta.params['wlan'][wlan], \
                                                                              y.meshMac[wlan], y.meshMac[wlan])
                            debug('\n'+command)
                            sta.pexec(command)
                        exist.append(y)
                        controlMeshMac.append(y.meshMac[wlan])
                        sta_ref.append(y)
                    elif x == newsta and y not in exist:
                        if WmediumdServerConn.connected:
                            pass
                        else:
                            command = 'iw dev %s mpath new %s next_hop %s' % (sta.params['wlan'][wlan], \
                                                                              y.meshMac[wlan], x.meshMac[wlan])
                            debug('\n'+command)
                            sta.pexec(command)
                        exist.append(y)
                        controlMeshMac.append(y.meshMac[wlan])
                        sta_ref.append(y)
                if newsta in sta_ref:
                    sta_ref.remove(newsta)

        """delete unknown paths"""
        if associate:
            for ref_sta in stations:
                for ref_wlan in range(len(ref_sta.params['wlan'])):
                    if ref_sta != sta and ref_sta.func[ref_wlan] == 'mesh' :
                        if ref_sta.type == 'vehicle':
                            ref_wlan = 0
                            ref_sta = ref_sta.params['carsta']
                        if ref_sta.meshMac[ref_wlan] not in controlMeshMac:
                            sta.pexec('iw dev %s mpath del %s' % (sta.params['wlan'][wlan], ref_sta.meshMac[ref_wlan]))

        """mesh leave"""
        if associate == False:
            debug('\niw dev %s mesh leave' % sta.params['wlan'][wlan])
            sta.pexec('iw dev %s mesh leave' % sta.params['wlan'][wlan])
            sta.params['associatedTo'][wlan] = ''