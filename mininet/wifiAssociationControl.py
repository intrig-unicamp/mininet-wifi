from mininet.wifiPropagationModels import propagationModel_
from mininet.wifiChannel import channelParameters

class associationControl ( object ):
    
    changeAP = False
    
    def __init__( self, node1, node2, wlan, ac ):
        self.customAssociationControl(node1, node2, wlan, ac)
           
    def customAssociationControl(self, node1, node2, wlan, ac):
        """Mechanisms that optimize the use of the APs"""        
        if ac == "llf": #useful for llf (Least-loaded-first)
            apref = node1.associatedAp[wlan]
            if apref != 'NoAssociated':
                ref_llf = apref.nAssociatedStations
                if node2.nAssociatedStations+2 < ref_llf:
                    self.changeAP = True
            else:
                self.changeAP = True
        elif ac == "ssf": #useful for ssf (Strongest-signal-first)
            if propagationModel_.model == '':
                propagationModel_.model = 'friisPropagationLossModel'
            refDistance = channelParameters.getDistance(node1, node2)
            refValue = propagationModel_(node1, node2, refDistance, wlan)
            if refValue.rssi > float(node1.params['rssi'][wlan] + 0.1):
                self.changeAP = True
        return self.changeAP  