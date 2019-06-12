"""
    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)"""

import re
from time import sleep
from six import string_types

from mininet.net import Mininet
from mininet.node import (DefaultController)
from mininet.util import (fixLimits, numCores, ensureRoot,
                          macColonHex, waitListening)
from mn_wifi.sixLoWPAN.util import ipAdd6, netParse
from mininet.link import Intf
from mininet.log import info, error, debug, output

from mn_wifi.sixLoWPAN.node import sixLoWPan
from mn_wifi.sixLoWPAN.module import module
from mn_wifi.sixLoWPAN.link import sixLoWPANLink


class Mininet_6LoWPAN(Mininet):

    topo=None
    sixLoWPan=sixLoWPan
    controller=DefaultController
    link=sixLoWPANLink
    intf=Intf
    ipBase='2001:0:0:0:0:0:0:0/64'
    inNamespace=False
    autoSetMacs=False
    autoStaticArp=False
    autoPinCpus=False
    listenPort=None
    waitConnected=False
    autoSetPositions=False
    ipBaseNum, prefixLen = netParse(ipBase)
    nextIP = 1  # start for address allocation
    nextPosition = 1
    inNamespace = inNamespace
    numCores = numCores()
    nextCore = 0  # next core for pinning hosts to CPUs
    nameToNode = {}  # name to Node (Host/Switch) objects
    links = []
    sixLP = []
    terms = []  # list of spawned xterm processes
    n_wpans = 0
    connections = {}
    wlinks = []


    @classmethod
    def init(self, node, **params):
        node.wpanports = -1
        self.n_wpans = self.n_wpans + params['sixlowpan']
        self.addParameters(node, **params)

    @classmethod
    def addParameters(self, node, node_mode='managed', **params):
        """adds parameters to wireless nodes
        node: node
        autoSetMacs: set MAC addrs automatically like IP addresses
        params: parameters
        defaults: Default IP and MAC addresses
        node_mode: if interface is running in managed or master mode"""
        node.params['wpan'] = []
        node.wpanPhyID = []

        wpans = self.count6LoWPANIfaces(**params)

        for wpan in range(wpans):
            self.addParamsToNode(node)
            if node_mode == 'managed':
                self.appendAssociatedTo(node)
                self.add_ip_param(node, wpans, **params)

            node.params['wpan'].append(node.name + '-wpan' + str(wpan))
            node.params.pop("wpans", None)

    @staticmethod
    def add_ip_param(node, wpans, autoSetMacs=False, **params):
        "Add IP Param"
        node.params['wpan_ip'] = []
        if 'wpan_ip' in params:
            ip_list = params['wpan_ip'].split(',')
            for ip in ip_list:
                node.params['wpan_ip'].append(ip)
            if len(ip_list) != len(node.params['wpan']):
                for ip_list in range(len(ip_list),
                                     len(node.params['wpan'])):
                    node.params['wpan_ip'].append('0/0')
        elif autoSetMacs:
            for n in range(wpans):
                node.params['wpan_ip'].append('0/0')
                node.params['wpan_ip'][n] = params['wpan_ip']
        else:
            for _ in range(wpans):
                node.params['wpan_ip'].append('')

    @staticmethod
    def appendAssociatedTo(node):
        "Add associatedTo param"
        node.params['associatedTo'].append('')


    @classmethod
    def add6LoWPAN(self, name, cls=None, **params):
        """Add 6LoWPAN node.
           name: name of station to add
           cls: custom 6LoWPAN class/constructor (optional)
           params: parameters for 6LoWPAN
           returns: added station"""
        # Default IP and MAC addresses
        defaults = {'wpan_ip': ipAdd6(self.nextIP,
                                ipBaseNum=self.ipBaseNum,
                                prefixLen=self.prefixLen) +
                          '/%s' % self.prefixLen
                   }
        defaults.update(params)

        if self.autoSetPositions:
            defaults['position'] = ('%s,0,0' % self.nextPosition)
        if self.autoSetMacs:
            defaults['mac'] = macColonHex(self.nextIP)
        if self.autoPinCpus:
            defaults['cores'] = self.nextCore
            self.nextCore = (self.nextCore + 1) % self.numCores
        self.nextIP += 1
        self.nextPosition += 1

        if not cls:
            cls = self.sixLoWPan
        node = cls(name, **defaults)

        self.addParameters(node, defaults)

        self.sixLP.append(node)
        self.nameToNode[name] = node

        return node

    # BL: We now have four ways to look up nodes
    # This may (should?) be cleaned up in the future.
    def getNodeByName(self, *args):
        "Return node(s) with given name(s)"
        if len(args) is 1:
            return self.nameToNode[args[0]]
        return [self.nameToNode[n] for n in args]

    def get(self, *args):
        "Convenience alias for getNodeByName"
        return self.getNodeByName(*args)

    # Even more convenient syntax for node lookup and iteration
    def __getitem__(self, key):
        "net[ name ] operator: Return node with given name"
        return self.nameToNode[key]

    def __delitem__(self, key):
        "del net[ name ] operator - delete node with given name"
        self.delNode(self.nameToNode[key])

    def __contains__(self, item):
        "returns True if net contains named node"
        return item in self.nameToNode

    def keys(self):
        "return a list of all node names or net's keys"
        return list(self)

    def values(self):
        "return a list of all nodes or net's values"
        return [self[name] for name in self]

    def items(self):
        "return (key,value) tuple list for every node in net"
        return zip(self.keys(), self.values())

    @staticmethod
    def _parsePing(pingOutput):
        "Parse ping output and return packets sent, received."
        # Check for downed link
        if 'connect: Network is unreachable' in pingOutput:
            return 1, 0
        r = r'(\d+) packets transmitted, (\d+)( packets)? received'
        m = re.search(r, pingOutput)
        if m is None:
            error('*** Error: could not parse ping output: %s\n' %
                  pingOutput)
            return 1, 0
        sent, received = int(m.group(1)), int(m.group(2))
        return sent, received

    @classmethod
    def ping6(self, hosts=None, timeout=None):
        """Ping6 between all specified hosts.
           hosts: list of hosts
           timeout: time to wait for a response, as string
           returns: ploss packet loss percentage"""
        # should we check if running?
        packets = 0
        lost = 0
        ploss = None
        if not hosts:
            hosts = self.sixLP
            output('*** Ping: testing ping reachability\n')
        for node in hosts:
            output('%s -> ' % node.name)
            for dest in hosts:
                if node != dest:
                    opts = ''
                    if timeout:
                        opts = '-W %s' % timeout
                    if dest.intfs:
                        result = node.cmdPrint('ping6 -c1 %s %s'
                                               % (opts, dest.IP()))
                        sent, received = self._parsePing(result)
                    else:
                        sent, received = 0, 0
                    packets += sent
                    if received > sent:
                        error('*** Error: received too many packets')
                        error('%s' % result)
                        node.cmdPrint('route')
                        exit(1)
                    lost += sent - received
                    output(('%s ' % dest.name) if received else 'X ')
            output('\n')
        if packets > 0:
            ploss = 100.0 * lost / packets
            received = packets - lost
            output("*** Results: %i%% dropped (%d/%d received)\n" %
                   (ploss, received, packets))
        else:
            ploss = 0
            output("*** Warning: No packets sent\n")
        return ploss

    @staticmethod
    def _parseFull(pingOutput):
        "Parse ping output and return all data."
        errorTuple = (1, 0, 0, 0, 0, 0)
        # Check for downed link
        r = r'[uU]nreachable'
        m = re.search(r, pingOutput)
        if m is not None:
            return errorTuple
        r = r'(\d+) packets transmitted, (\d+)( packets)? received'
        m = re.search(r, pingOutput)
        if m is None:
            error('*** Error: could not parse ping output: %s\n' %
                  pingOutput)
            return errorTuple
        sent, received = int(m.group(1)), int(m.group(2))
        r = r'rtt min/avg/max/mdev = '
        r += r'(\d+\.\d+)/(\d+\.\d+)/(\d+\.\d+)/(\d+\.\d+) ms'
        m = re.search(r, pingOutput)
        if m is None:
            if received is 0:
                return errorTuple
            error('*** Error: could not parse ping output: %s\n' %
                  pingOutput)
            return errorTuple
        rttmin = float(m.group(1))
        rttavg = float(m.group(2))
        rttmax = float(m.group(3))
        rttdev = float(m.group(4))
        return sent, received, rttmin, rttavg, rttmax, rttdev

    def pingFull(self, hosts=None, timeout=None):
        """Ping between all specified hosts and return all data.
           hosts: list of hosts
           timeout: time to wait for a response, as string
           returns: all ping data; see function body."""
        # should we check if running?
        # Each value is a tuple: (src, dsd, [all ping outputs])
        all_outputs = []
        if not hosts:
            hosts = self.hosts
            output('*** Ping: testing ping reachability\n')
        for node in hosts:
            output('%s -> ' % node.name)
            for dest in hosts:
                if node != dest:
                    opts = ''
                    if timeout:
                        opts = '-W %s' % timeout
                    result = node.cmd('ping -c1 %s %s' % (opts, dest.IP()))
                    outputs = self._parsePingFull(result)
                    sent, received, rttmin, rttavg, rttmax, rttdev = outputs
                    all_outputs.append((node, dest, outputs))
                    output(('%s ' % dest.name) if received else 'X ')
            output('\n')
        output("*** Results: \n")
        for outputs in all_outputs:
            src, dest, ping_outputs = outputs
            sent, received, rttmin, rttavg, rttmax, rttdev = ping_outputs
            output(" %s->%s: %s/%s, " % (src, dest, sent, received))
            output("rtt min/avg/max/mdev %0.3f/%0.3f/%0.3f/%0.3f ms\n" %
                   (rttmin, rttavg, rttmax, rttdev))
        return all_outputs

    def pingAll(self, timeout=None):
        """Ping between all hosts.
           returns: ploss packet loss percentage"""
        return self.ping6(timeout=timeout)

    @staticmethod
    def _parseIperf(iperfOutput):
        """Parse iperf output and return bandwidth.
           iperfOutput: string
           returns: result string"""
        r = r'([\d\.]+ \w+/sec)'
        m = re.findall(r, iperfOutput)
        if m:
            return m[-1]
        else:
            # was: raise Exception(...)
            error('could not parse iperf output: ' + iperfOutput)
            return ''

    def iperf(self, hosts=None, l4Type='TCP', udpBw='10M', fmt=None,
              seconds=5, port=5001):
        """Run iperf between two hosts.
           hosts: list of hosts; if None, uses first and last hosts
           l4Type: string, one of [ TCP, UDP ]
           udpBw: bandwidth target for UDP test
           fmt: iperf format argument if any
           seconds: iperf time to transmit
           port: iperf port
           returns: two-element array of [ server, client ] speeds
           note: send() is buffered, so client rate can be much higher than
           the actual transmission rate; on an unloaded system, server
           rate should be much closer to the actual receive rate"""
        sleep(2)
        nodes = self.sixLP
        hosts = hosts or [nodes[0], nodes[-1]]
        assert len(hosts) is 2
        client, server = hosts
        output('*** Iperf: testing', l4Type, 'bandwidth between',
               client, 'and', server, '\n')
        server.cmd('killall -9 iperf')
        iperfArgs = 'iperf -p %d ' % port
        bwArgs = ''
        if l4Type is 'UDP':
            iperfArgs += '-u '
            bwArgs = '-b ' + udpBw + ' '
        elif l4Type != 'TCP':
            raise Exception('Unexpected l4 type: %s' % l4Type)
        if fmt:
            iperfArgs += '-f %s ' % fmt
        server.sendCmd(iperfArgs + '-s')
        if l4Type is 'TCP':
            if not waitListening(client, server.IP(), port):
                raise Exception('Could not connect to iperf on port %d'
                                % port)
        cliout = client.cmd(iperfArgs + '-t %d -c ' % seconds +
                            server.IP() + ' ' + bwArgs)
        debug('Client output: %s\n' % cliout)
        servout = ''
        # We want the last *b/sec from the iperf server output
        # for TCP, there are two of them because of waitListening
        count = 2 if l4Type is 'TCP' else 1
        while len(re.findall('/sec', servout)) < count:
            servout += server.monitor(timeoutms=5000)
        server.sendInt()
        servout += server.waitOutput()
        debug('Server output: %s\n' % servout)
        result = [self._parseIperf(servout), self._parseIperf(cliout)]
        if l4Type is 'UDP':
            result.insert(0, udpBw)
        output('*** Results: %s\n' % result)
        return result

    @classmethod
    def addParamsToNode(self, node):
        "Add func and wpanPhyID"
        node.func.append('none')
        node.wpanPhyID.append(0)

    @classmethod
    def count6LoWPANIfaces(self, **params):
        "Count the number of virtual 6LoWPAN interfaces"
        if 'wpans' in params:
            self.n_wpans += int(params['wpans'])
            wpans = int(params['wpans'])
        else:
            wpans = 1
            self.n_wpans += 1
        return wpans

    def kill_fakelb(self):
        "Kill fakelb"
        module.fakelb()
        sleep(0.1)

    def configureIface(self, node, wlan):
        intf = module.wlan_list[0]
        module.wlan_list.pop(0)
        node.renameIface(intf, node.params['wpan'][wlan])

    def closeMininetWiFi(self):
        "Close Mininet-WiFi"
        module.stop()
