# author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

import re
import os


class manetProtocols(object):
    def __init__(self, intf, proto, **params):
        eval(proto)(intf, **params)


class batman(object):
    def __init__(self, intf, **params):
        self.load_module(intf)
        self.add_iface(intf)
        self.set_link_up(intf)

    def add_iface(self, intf):
        intf.node.cmd('batctl if add %s' % intf.name)

    def set_link_up(self, intf):
        intf.node.cmd('ip link set up dev bat0')
        self.setIP(intf)

    def setIP(self, intf):
        nums = re.findall(r'\d+', intf.node.name)
        id = hex(int(nums[0]))[2:]
        intf.node.cmd('ip addr add 192.168.123.%s/24 '
                      'dev bat0' % id)

    def load_module(self, intf):
        intf.node.cmd('modprobe batman-adv')


class olsr(object):
    def __init__(self, intf, **params):
        cmd = 'olsrd -i %s -d 0 ' % intf.name
        if 'flags' in params:
            cmd += params['flags']
        intf.node.cmd(cmd)


class babel(object):
    def __init__(self, intf, **params):
        pid = os.getpid()
        cmd = "babeld %s -I mn_%s_%s.staconf " % \
              (intf.name, intf.node, pid)
        if 'flags' in params:
            cmd += params['flags']
        intf.node.cmd(cmd + ' &')
