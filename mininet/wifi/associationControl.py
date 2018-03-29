"""

    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

"""

from mininet.log import debug

class associationControl(object):
    "Mechanisms that optimize the use of the APs"

    changeAP = False

    def __init__(self, sta, ap, wlan, ac):
        if ac in dir(self):
            self.__getattribute__(ac)(sta=sta, ap=ap, wlan=wlan)

    def llf(self, sta, ap, wlan):
        #llf: Least loaded first
        apref = sta.params['associatedTo'][wlan]
        if apref != '':
            ref_llf = len(apref.params['associatedStations'])
            if len(ap.params['associatedStations']) + 2 < ref_llf:
                debug('iw dev %s disconnect\n' % sta.params['wlan'][wlan])
                sta.pexec('iw dev %s disconnect' % sta.params['wlan'][wlan])
                self.changeAP = True
        else:
            self.changeAP = True
        return self.changeAP

    def ssf(self, sta, ap, wlan):
        #ssf: Strongest signal first
        distance = sta.get_distance_to(sta.params['associatedTo'][wlan])
        rssi = sta.set_rssi(sta.params['associatedTo'][wlan],
                            wlan, distance)
        ref_dist = sta.get_distance_to(ap)
        ref_rssi = sta.set_rssi(ap, wlan, ref_dist)
        if float(ref_rssi) > float(rssi + 0.1):
            debug('iw dev %s disconnect\n' % sta.params['wlan'][wlan])
            sta.pexec('iw dev %s disconnect' % sta.params['wlan'][wlan])
            self.changeAP = True
        return self.changeAP
