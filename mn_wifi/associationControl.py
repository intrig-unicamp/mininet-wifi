"""Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
   author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)"""


class associationControl(object):
    "Mechanisms that optimize the use of the APs"

    changeAP = False

    def __init__(self, intf, ap_intf, ac):
        if ac in dir(self):
            self.__getattribute__(ac)(intf, ap_intf)

    def disconnect(self, intf):
        return 'iw dev %s disconnect' % intf.name

    def llf(self, intf, ap_intf):
        #llf: Least loaded first
        apref = intf.associatedTo
        if apref != '':
            ref_llf = len(apref.associatedStations)
            if len(ap_intf.associatedStations) + 2 < ref_llf:
                intf.node.pexec(self.disconnect(intf))
                self.changeAP = True
        else:
            self.changeAP = True
        return self.changeAP

    def ssf(self, intf, ap_intf):
        #ssf: Strongest signal first
        distance = intf.node.get_distance_to(intf.associatedTo)
        rssi = intf.node.get_rssi(intf, ap_intf, distance)
        ref_dist = intf.node.get_distance_to(ap_intf.node)
        ref_rssi = intf.node.get_rssi(intf, ap_intf, ref_dist)
        if float(ref_rssi) > float(rssi + 0.1):
            intf.node.pexec(self.disconnect(intf))
            self.changeAP = True
        return self.changeAP
