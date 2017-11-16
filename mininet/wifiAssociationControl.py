from mininet.log import debug

class associationControl (object):

    changeAP = False

    def __init__(self, sta, ap, wlan, ac, wirelessLink):
        self.customAssociationControl(sta, ap, wlan, ac, wirelessLink)

    def customAssociationControl(self, sta, ap, wlan, ac, wirelessLink):
        """Mechanisms that optimize the use of the APs
        llf: Least-loaded-first
        ssf: Strongest-signal-first"""
        if ac == "llf":
            apref = sta.params['associatedTo'][wlan]
            if apref != '':
                ref_llf = len(apref.params['associatedStations'])
                if len(ap.params['associatedStations']) + 2 < ref_llf:
                    debug('iw dev %s disconnect' % sta.params['wlan'][wlan])
                    sta.pexec('iw dev %s disconnect' % sta.params['wlan'][wlan])
                    self.changeAP = True
            else:
                self.changeAP = True
        elif ac == "ssf":
            distance = wirelessLink.getDistance(sta,
                                                sta.params['associatedTo'][wlan])
            RSSI = wirelessLink.setRSSI(sta, sta.params['associatedTo'][wlan],
                                        wlan, distance)
            refDistance = wirelessLink.getDistance(sta, ap)
            refRSSI = wirelessLink.setRSSI(sta, ap, wlan, refDistance)
            if float(refRSSI) > float(RSSI + 0.1):
                debug('iw dev %s disconnect' % sta.params['wlan'][wlan])
                sta.pexec('iw dev %s disconnect' % sta.params['wlan'][wlan])
                self.changeAP = True
        return self.changeAP
