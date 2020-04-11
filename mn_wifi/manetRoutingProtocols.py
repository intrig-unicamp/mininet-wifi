# author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

import re
import os

from mininet.log import info


class manetProtocols(object):
    def __init__(self, intf, proto_args):
        eval(intf.proto)(intf, proto_args)


class batmand(object):
    def __init__(self, intf, proto_args):
        self.set_batmand_iface(intf, proto_args)

    def set_batmand_iface(self, intf, proto_args):
        intf.cmd('batmand {} {}'.format(proto_args, intf.name))


class batman_adv(object):
    def __init__(self, intf, proto_args):
        self.load_module(intf)
        self.add_iface(intf)
        self.set_link_up(intf)

    def add_iface(self, intf):
        intf.cmd('batctl if add %s' % intf.name)

    def set_link_up(self, intf):
        intf.cmd('ip link set up dev bat0')
        self.setIP(intf)

    def setIP(self, intf):
        nums = re.findall(r'\d+', intf.node.name)
        id = hex(int(nums[0]))[2:]
        intf.cmd('ip addr add 192.168.123.%s/24 '
                 'dev bat0' % id)

    def load_module(self, intf):
        intf.cmd('modprobe batman-adv')


class olsr(object):
    def __init__(self, intf, proto_args):
        cmd = 'olsrd -i %s -d 0 ' % intf.name
        cmd += proto_args
        intf.cmd(cmd)
        info('Starting olsrd in {}...\n'.format(intf.name))


class babel(object):
    def __init__(self, intf, proto_args):
        pid = os.getpid()
        cmd = "babeld %s -I mn_%s_%s.staconf " % \
              (intf.name, intf.node, pid)
        cmd += proto_args
        intf.cmd(cmd + ' &')
        info('Starting babeld in {}...\n'.format(intf.name))
