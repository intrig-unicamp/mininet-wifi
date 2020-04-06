# author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

import re
import os


class manetProtocols(object):
    def __init__(self, intf, proto, proto_args):
        eval(proto)(intf, proto_args)


class batman(object):
    def __init__(self, intf, proto_args):
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
    def __init__(self, intf, proto_args):
        cmd = 'olsrd -i %s -d 0 ' % intf.name
        cmd += proto_args
        intf.node.cmd(cmd)


class babel(object):
    def __init__(self, intf, proto_args):
        pid = os.getpid()
        cmd = "babeld %s -I mn_%s_%s.staconf " % \
              (intf.name, intf.node, pid)
        cmd += proto_args
        intf.node.cmd(cmd + ' &')
