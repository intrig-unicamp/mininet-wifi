from mininet.wifiEmulationEnvironment import emulationEnvironment
from mininet.wifiPropagationModels import propagationModel
from mininet.wifiChannel import channelParameters

class associationControl ( object ):
    
    systemLoss = 1    
    changeAP = False
    
    def __init__( self, node1, node2, wlan, ac ):
        self.customAssociationControl(node1, node2, wlan, ac)
           
    def customAssociationControl(self, node1, node2, wlan, ac):
        """Mechanisms that optimize the use of the APs"""        
        if ac == "llf": #useful for llf (Least-loaded-first)
            apref = node1.associatedAp[wlan]
            if apref != 'NoAssociated':
                #accessPoint.numberOfAssociatedStations(apref)
                ref_llf = apref.nAssociatedStations
                if node2.nAssociatedStations+2 < ref_llf:
                    self.changeAP = True
            else:
                self.changeAP = True
        elif ac == "ssf": #useful for ssf (Strongest-signal-first)
            if emulationEnvironment.propagation_Model == '':
                emulationEnvironment.propagation_Model = 'friisPropagationLossModel'
            refDistance = channelParameters.getDistance(node1, node2)
            refValue = propagationModel(node1, node2, refDistance, wlan, emulationEnvironment.propagation_Model, self.systemLoss)
            if refValue.rssi > float(node1.rssi[wlan] + 0.1):
                self.changeAP = True
        return self.changeAP  