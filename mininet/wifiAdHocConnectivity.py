"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com
"""

from mininet.wifiChannel import channelParams

class pairingAdhocNodes (object):

    ssid_ID = 0
    dist = 0

    def __init__(self, sta, wlan, stationList):
        """ init """
        self.pairing(sta, wlan, stationList)

    def pairing(self, sta, wlan, stationList):
        """Pairing nodes"""
        i = 1
        ref_distance = 0
        self.dist = 0

        alreadyConn = []

        for sta_ref in stationList:
            if sta_ref != sta:
                dist = channelParams.getDistance(sta, sta_ref)
                if dist != 0.0:
                    totalRange = min(sta.params['range'], sta_ref.params['range'])
                    if dist < totalRange:
                        ref_distance = ref_distance + dist
                        i += 1
                        for wlan_ref in range(len(sta_ref.params['wlan'])):
                            if sta.params['ssid'][wlan] == sta_ref.params['ssid'][wlan_ref]:
                                if sta.params['cell'][wlan] == '':
                                    alreadyConn.append(sta)
                                    sta.params['associatedTo'][wlan] = sta.params['ssid'][wlan]
                                    sta.params['cell'][wlan] = ('02:CA:FF:EE:BA:0%s' % self.ssid_ID)
                                    iface = sta.params['wlan'][wlan]
                                    print "associating %s to %s..." % (iface, sta.params['ssid'][wlan])
                                    sta.pexec('iwconfig %s essid %s ap 02:CA:FF:EE:BA:0%s' % (sta.params['wlan'][wlan], sta.params['associatedTo'][wlan], self.ssid_ID))
                                    sta.params['frequency'][wlan] = channelParams.frequency(sta, wlan)
                                if sta.params['cell'][wlan] != sta_ref.params['cell'][wlan_ref]:
                                    if sta_ref.params['associatedTo'][wlan_ref] == sta.params['ssid'][wlan]:
                                        if sta_ref.params['cell'][wlan_ref] == '' and sta in alreadyConn:
                                            alreadyConn.append(sta_ref)
                                            iface = sta_ref.params['wlan'][wlan_ref]
                                            sta_ref.params['associatedTo'][wlan_ref] = sta.params['ssid'][wlan]
                                            sta_ref.params['cell'][wlan_ref] = ('02:CA:FF:EE:BA:0%s' % self.ssid_ID)
                                            print "associating %s to %s..." % (iface, sta.params['ssid'][wlan])
                                            sta_ref.pexec('iwconfig %s essid %s ap 02:CA:FF:EE:BA:0%s' % (sta_ref.params['wlan'][wlan_ref], sta_ref.params['associatedTo'][wlan_ref], self.ssid_ID))
                                            sta_ref.params['frequency'][wlan_ref] = channelParams.frequency(sta_ref, wlan_ref)
        if alreadyConn != [] and len(alreadyConn) != 1:
            for sta_ref in stationList:
                if sta_ref not in alreadyConn:
                    if sta_ref.params['cell'][wlan_ref] == sta.params['cell'][wlan]:
                        sta_ref.params['cell'][wlan_ref] = ''
        self.ssid_ID += 1

        self.dist = ref_distance / i
        return self.dist

    # def confirmAdhocAssociation(self, sta, iface, wlan):
    #    associated = ''
    #    while(associated == '' or len(associated) == 0):
    #        sta.sendCmd("iw dev %s scan ssid | grep %s" % (iface, sta.ssid[wlan]))
    #        associated = sta.waitOutput()
    #    sta.params['frequency'][wlan] = channelParameters.frequency(sta, wlan)
