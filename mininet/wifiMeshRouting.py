"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com
"""

from mininet.wifiChannel import channelParameters
from mininet.log import debug

class listNodes (object):

    nodesX = []
    nodesY = []
    ssid_ID = 0

    @classmethod
    def clearList(self):
        del self.nodesX[:]
        del self.nodesY[:]

    @classmethod
    def pairingNodes(self, sta, wlan, stationList, **params):
        """Pairing nodes"""
        i = 1
        ref_distance = 0
        self.dist = 0
        self.ssid_ID += 1

        par = []
        alreadyConn = []
        cont = True

        list = []
        list.append(sta)
        currentSta = sta

        while cont:
            if len(par) != 0:
                sta = par[0]
                par.pop(0)
            for ref_sta in stationList:
                if ref_sta.type == 'vehicle':
                    car = ref_sta
                    ref_sta = ref_sta.params['carsta']
                    ref_sta.params['position'] = car.params['position']
                if ref_sta != sta and ref_sta.func[wlan] == 'mesh' :
                    dist = channelParameters.getDistance(sta, ref_sta)
                    if dist >= 0.1:
                        totalRange = int(sta.params['range'])
                        if dist <= totalRange:
                            cont = True
                            ref_distance = dist
                            if currentSta == sta:
                                listNodes.nodesX.append(sta)
                                listNodes.nodesY.append(ref_sta)
                            ssid = sta.params['associatedTo'][wlan]
                            ref_ssid = ref_sta.params['associatedTo'][wlan]
                            if ssid != ref_ssid or ssid == sta.params['ssid'][wlan] or ssid == '' or ref_ssid == '':
                                if ref_sta.params['associatedTo'][wlan] != '' and \
                                        ref_sta.params['associatedTo'][wlan] != ssid and sta not in alreadyConn:
                                    alreadyConn.append(sta)
                                    sta.params['associatedTo'][wlan] = ref_sta.params['associatedTo'][wlan]
                                    debug('\niw dev %s mesh join %s' % (sta.params['wlan'][wlan], \
                                                                          sta.params['associatedTo'][wlan]))
                                    sta.pexec('iw dev %s mesh join %s' % (sta.params['wlan'][wlan], \
                                                                          sta.params['associatedTo'][wlan]))
                                else:
                                    if sta.params['associatedTo'][wlan] != sta.params['ssid'][wlan] + str(self.ssid_ID) and \
                                                                                                sta not in alreadyConn:
                                        alreadyConn.append(sta)
                                        sta.params['associatedTo'][wlan] = sta.params['ssid'][wlan] + str(self.ssid_ID)
                                        debug('\niw dev %s mesh join %s' % (sta.params['wlan'][wlan], \
                                                                              sta.params['associatedTo'][wlan]))
                                        sta.pexec('iw dev %s mesh join %s' % (sta.params['wlan'][wlan], \
                                                                              sta.params['associatedTo'][wlan]))
                                if ref_sta not in alreadyConn:
                                    alreadyConn.append(ref_sta)
                                    ref_sta.params['associatedTo'][wlan] = sta.params['ssid'][wlan] + str(self.ssid_ID)
                                    debug('\niw dev %s mesh join %s' % (ref_sta.params['wlan'][wlan], 
                                                                              ref_sta.params['associatedTo'][wlan]))
                                    ref_sta.pexec('iw dev %s mesh join %s' % (ref_sta.params['wlan'][wlan], 
                                                                              ref_sta.params['associatedTo'][wlan]))
                            if ref_sta not in list:
                                par.append(ref_sta)
                                list.append(ref_sta)
                            i += 1
            if len(par) == 0:
                cont = False
                self.ssid_ID += 1
        self.dist = ref_distance / i
        return self.dist


class meshRouting (object):
    """Mesh Routing"""
    routing = ''

    @classmethod
    def customMeshRouting(self, sta, wlan, stationList, **params):
        """Custom Mesh Routing"""
        associate = False
        controlMeshMac = []
        command = ''
        for ref_sta in stationList:
            if ref_sta.type == 'vehicle':
                car = ref_sta
                ref_sta = ref_sta.params['carsta']
                ref_sta.params['position'] = car.params['position']
            for ref_wlan in range(len(ref_sta.params['wlan'])):
                if ref_sta != sta and ref_sta.func[ref_wlan] == 'mesh' and 'position' in sta.params:
                    dist = channelParameters.getDistance(sta, ref_sta)
                    totalRange = int(sta.params['range'])
                    if dist <= totalRange:
                        if ref_sta.func[ref_wlan] == 'mesh':
                            if sta.params['associatedTo'][wlan] == ref_sta.params['associatedTo'][ref_wlan]:
                                associate = True
                elif 'position' not in sta.params:
                    associate = True

        """Adding all reached target paths"""
        if associate:
            exist = []
            sta_ref = []
            sta_ref.append(sta)
            j = 0
            exist.append(sta)
            while j < len(stationList):
                j += 1
                if sta_ref == []:
                    break
                else:
                    newsta = sta_ref[0]
                for x, y in zip(listNodes.nodesX, listNodes.nodesY):
                    if x == sta and y not in exist:
                        command = 'iw dev %s mpath new %s next_hop %s' % (sta.params['wlan'][wlan], \
                                                                          y.meshMac[wlan], y.meshMac[wlan])
                        debug('\n'+command)
                        sta.pexec(command)
                        exist.append(y)
                        controlMeshMac.append(y.meshMac[wlan])
                        sta_ref.append(y)
                    elif x == newsta and y not in exist:
                        command = 'iw dev %s mpath new %s next_hop %s' % (sta.params['wlan'][wlan], \
                                                                          y.meshMac[wlan], x.meshMac[wlan])
                        debug('\n'+command)
                        sta.pexec(command)
                        exist.append(y)
                        controlMeshMac.append(y.meshMac[wlan])
                        sta_ref.append(y)
                if newsta in sta_ref:
                    sta_ref.remove(newsta)

        """delete all unknown paths"""
        if associate:
            for ref_sta in stationList:
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