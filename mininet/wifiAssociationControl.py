from mininet.wifiPropagationModels import propagationModel_
from mininet.wifiChannel import channelParameters

class associationControl ( object ):
    
    changeAP = False
    
    def __init__( self, sta, ap, wlan, ac ):
        self.customAssociationControl(sta, ap, wlan, ac)
           
    def customAssociationControl(self, sta, ap, wlan, ac):
        """Mechanisms that optimize the use of the APs"""        
        if ac == "llf": #useful for llf (Least-loaded-first)
            apref = sta.params['associatedTo'][wlan]
            if apref != 'NoAssociated':
                ref_llf = len(apref.associatedStations)
                if len(ap.associatedStations)+2 < ref_llf:
                    self.changeAP = True
            else:
                self.changeAP = True
        elif ac == "ssf": #useful for ssf (Strongest-signal-first)
            if propagationModel_.model == '':
                propagationModel_.model = 'friisPropagationLossModel'
            refDistance = channelParameters.getDistance(sta, ap)
            refValue = propagationModel_(sta, ap, refDistance, wlan)
            if refValue.rssi > float(sta.params['rssi'][wlan] + 1):
                self.changeAP = True
        return self.changeAP  