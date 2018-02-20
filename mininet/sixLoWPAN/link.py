"author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)"

from mininet.log import info, error, debug
from mininet.link import Intf


class sixLoWPANLink(object):

    def __init__(self, node, intf=Intf, **params):
        """Create 6LoWPAN link to another node.
           node: node
           intf: default interface class/constructor"""
        self.configIface(node, **params)

    @classmethod
    def configIface(cls, node, **params):
        node.cmd('ip link set lo up')
        node.cmd('ip link set %s down' % node.params['wlan'][0])
        node.cmd('iwpan dev %s set pan_id "%s"' % (node.params['wlan'][0], params['panid']))
        node.cmd('ip link add link %s name %s-lowpan type lowpan'
                 % (node.params['wlan'][0], node.name))
        node.cmd('ip link set %s up' % node.params['wlan'][0])
        node.cmd('ip link set %s-lowpan up' % node.name)