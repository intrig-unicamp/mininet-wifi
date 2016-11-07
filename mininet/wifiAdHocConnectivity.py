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

        for ref_sta in stationList:
            if ref_sta != sta:
                dist = channelParams.getDistance(sta, ref_sta)
                if dist != 0.0:
                    totalRange = int(sta.params['range']) + int(ref_sta.params['range'])
                    if dist < totalRange:
                        ref_distance = ref_distance + dist
                        #ssid = sta.params['associatedTo'][wlan]
                        #ref_ssid = ref_sta.params['associatedTo'][wlan]
                        i += 1
                        sta.params['frequency'][wlan] = channelParams.frequency(sta, wlan)
                        if sta.params['ssid'][wlan] == ref_sta.params['ssid'][wlan]:
                            if sta.params['cell'][wlan] == '':
                                alreadyConn.append(sta)
                                sta.params['associatedTo'][wlan] = sta.params['ssid'][wlan]
                                sta.params['cell'][wlan] = ('02:CA:FF:EE:BA:0%s' % self.ssid_ID)
                                iface = sta.params['wlan'][wlan]
                                print "associating %s to %s..." % (iface, sta.params['ssid'][wlan])
                                sta.pexec('iwconfig %s essid %s ap 02:CA:FF:EE:BA:0%s' % (sta.params['wlan'][wlan], sta.params['associatedTo'][wlan], self.ssid_ID))
                                # sta.pexec('iw dev %s ibss join %s 2412' % (sta.params['wlan'][wlan], \
                                #                                           sta.params['associatedTo'][wlan]))
                                sta.params['frequency'][wlan] = channelParams.frequency(sta, wlan)
                            if sta.params['cell'][wlan] != ref_sta.params['cell'][wlan]:
                                if ref_sta.params['associatedTo'][wlan] == sta.params['ssid'][wlan]:
                                    if ref_sta.params['cell'][wlan] == '' and sta in alreadyConn:
                                        alreadyConn.append(ref_sta)
                                        iface = ref_sta.params['wlan'][wlan]
                                        ref_sta.params['associatedTo'][wlan] = sta.params['ssid'][wlan]
                                        ref_sta.params['cell'][wlan] = ('02:CA:FF:EE:BA:0%s' % self.ssid_ID)
                                        print "associating %s to %s..." % (iface, sta.params['ssid'][wlan])
                                        ref_sta.pexec('iwconfig %s essid %s ap 02:CA:FF:EE:BA:0%s' % (ref_sta.params['wlan'][wlan], ref_sta.params['associatedTo'][wlan], self.ssid_ID))
                                        # ref_sta.pexec('iw dev %s ibss join %s 2412' % (ref_sta.params['wlan'][wlan], \
                                        #                                               ref_sta.params['associatedTo'][wlan]))
                                        ref_sta.params['frequency'][wlan] = channelParams.frequency(ref_sta, wlan)
        if alreadyConn != [] and len(alreadyConn) != 1:
            for ref_sta in stationList:
                if ref_sta not in alreadyConn:
                    if ref_sta.params['cell'][wlan] == sta.params['cell'][wlan]:
                        ref_sta.params['cell'][wlan] = ''
        self.ssid_ID += 1

        self.dist = ref_distance / i
        return self.dist

    # def confirmAdhocAssociation(self, sta, iface, wlan):
    #    associated = ''
    #    while(associated == '' or len(associated) == 0):
    #        sta.sendCmd("iw dev %s scan ssid | grep %s" % (iface, sta.ssid[wlan]))
    #        associated = sta.waitOutput()
    #    sta.params['frequency'][wlan] = channelParameters.frequency(sta, wlan)
