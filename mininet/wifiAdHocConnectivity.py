"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com
"""

from mininet.wifiLink import link
from mininet.log import debug

class pairingAdhocNodes (object):

    ssid_ID = 0
    dist = 0

    def __init__(self, node, wlan, nodes):
        self.pairing(node, wlan, nodes)

    def pairing(self, node, wlan, nodes):
        """Pairing nodes"""
        i = 1
        ref_distance = 0
        self.dist = 0
        alreadyConn = []

        for sta_ref in nodes:
            if sta_ref != node:
                dist = link.getDistance(node, sta_ref)
                if dist != 0.0:
                    totalRange = min(node.params['range'], sta_ref.params['range'])
                    if dist < totalRange:
                        ref_distance = ref_distance + dist
                        i += 1
                        for wlan_ref in range(len(sta_ref.params['wlan'])):
                            if node.params['ssid'][wlan] == sta_ref.params['ssid'][wlan_ref]:
                                if node.params['cell'][wlan] == '':
                                    alreadyConn.append(node)
                                    node.params['associatedTo'][wlan] = node.params['ssid'][wlan]
                                    node.params['cell'][wlan] = ('02:CA:FF:EE:BA:0%s' % self.ssid_ID)
                                    iface = node.params['wlan'][wlan]
                                    debug("\nassociating %s to %s..." % (iface, node.params['ssid'][wlan]))
                                    debug('iwconfig %s essid %s ap 02:CA:FF:EE:BA:0%s' % \
                                          (node.params['wlan'][wlan], node.params['associatedTo'][wlan], self.ssid_ID))
                                    node.pexec('iwconfig %s essid %s ap 02:CA:FF:EE:BA:0%s' % \
                                          (node.params['wlan'][wlan], node.params['associatedTo'][wlan], self.ssid_ID))
                                    node.params['frequency'][wlan] = link.frequency(node, wlan)
                                if node.params['cell'][wlan] != sta_ref.params['cell'][wlan_ref]:
                                    if sta_ref.params['associatedTo'][wlan_ref] == node.params['ssid'][wlan]:
                                        if sta_ref.params['cell'][wlan_ref] == '' and node in alreadyConn:
                                            alreadyConn.append(sta_ref)
                                            iface = sta_ref.params['wlan'][wlan_ref]
                                            sta_ref.params['associatedTo'][wlan_ref] = node.params['ssid'][wlan]
                                            sta_ref.params['cell'][wlan_ref] = ('02:CA:FF:EE:BA:0%s' % self.ssid_ID)
                                            debug("\nassociating %s to %s..." % (iface, node.params['ssid'][wlan]))
                                            debug('iwconfig %s essid %s ap 02:CA:FF:EE:BA:0%s' % \
                                                          (sta_ref.params['wlan'][wlan_ref], sta_ref.params['associatedTo'][wlan_ref], self.ssid_ID))
                                            sta_ref.pexec('iwconfig %s essid %s ap 02:CA:FF:EE:BA:0%s' % \
                                                          (sta_ref.params['wlan'][wlan_ref], sta_ref.params['associatedTo'][wlan_ref], self.ssid_ID))
                                            sta_ref.params['frequency'][wlan_ref] = link.frequency(sta_ref, wlan_ref)
        if alreadyConn != [] and len(alreadyConn) != 1:
            for sta_ref in nodes:
                if sta_ref not in alreadyConn:
                    if sta_ref.params['cell'][wlan_ref] == node.params['cell'][wlan]:
                        sta_ref.params['cell'][wlan_ref] = ''
        self.ssid_ID += 1

        self.dist = ref_distance / i
        return self.dist