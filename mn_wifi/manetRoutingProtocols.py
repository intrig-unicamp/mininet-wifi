# author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

import re
import os


class manetProtocols(object):
    def __init__(self, protocol, node, wlan=0, **params):
        eval(protocol)(node, wlan, **params)


class batman(object):
    def __init__(self, node, wlan=0, **params):
        self.load_module(node)
        self.add_iface(node, wlan)
        self.set_link_up(node, wlan)

    def add_iface(self, node, wlan=0):
        iface = node.params['wlan'][wlan]
        node.cmd('batctl if add %s' % iface)

    def set_link_up(self, node, wlan=0):
        node.cmd('ip link set up dev bat%s' % wlan)
        self.setIP(node, wlan)

    def setIP(self, node, wlan):
        nums = re.findall(r'\d+', node.name)
        id = hex(int(nums[0]))[2:]
        node.cmd('ip addr add 192.168.123.%s/24 '
                 'dev bat%s' % (id, wlan))

    def load_module(self, node):
        node.cmd('modprobe batman-adv')


class olsr(object):
    def __init__(self, node, wlan=0, **params):
        cmd = 'olsrd -i %s -d 0 ' % node.params['wlan'][wlan]
        if 'flags' in params:
            cmd += params['flags']
        node.cmd(cmd)


class babel(object):
    def __init__(self, node, wlan=0, **params):
        pid = os.getpid()
        cmd = "babeld %s -I mn_%s_%s.staconf " % \
              (node.params['wlan'][wlan], node, pid)
        if 'flags' in params:
            cmd += params['flags']
        node.cmd(cmd + ' &')
