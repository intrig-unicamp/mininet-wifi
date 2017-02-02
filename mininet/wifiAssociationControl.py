from mininet.wifiPropagationModels import propagationModel
from mininet.wifiChannel import setChannelParams
import time

class associationControl (object):

    changeAP = False

    def __init__(self, sta, ap, wlan, ac):
        self.customAssociationControl(sta, ap, wlan, ac)

    def customAssociationControl(self, sta, ap, wlan, ac):
        """Mechanisms that optimize the use of the APs
        llf: Least-loaded-first
        ssf: Strongest-signal-first"""
        if ac == "llf":
            apref = sta.params['associatedTo'][wlan]
            if apref != '':
                ref_llf = len(apref.params['associatedStations'])
                if len(ap.params['associatedStations']) + 2 < ref_llf:
                    self.changeAP = True
            else:
                self.changeAP = True
        elif ac == "ssf":
            refDistance = setChannelParams.getDistance(sta, ap)
            refRSSI = setChannelParams.setRSSI(sta, ap, wlan, refDistance)
            if float(refRSSI) > float(sta.params['rssi'][wlan] + 0.1):
                self.changeAP = True
        return self.changeAP
