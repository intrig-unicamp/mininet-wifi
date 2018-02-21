"""
    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)"""

import os
import random
import re
import sys
import select
import signal
from time import sleep
from itertools import chain, groupby
from math import ceil
from six import string_types

from mininet.cli import CLI
from mininet.term import cleanUpScreens, makeTerms
from mininet.wifi.net import Mininet_wifi
from mininet.node import (Node, Host, OVSKernelSwitch,
                          DefaultController, Controller)
from mininet.util import (quietRun, fixLimits, numCores, ensureRoot,
                          macColonHex, ipParse, waitListening)
from mininet.sixLoWPAN.util import ipAdd6, netParse
from mininet.link import Link, Intf
from mininet.nodelib import NAT
from mininet.log import info, error, debug, output, warn

from mininet.wifi.node import AccessPoint, AP, Station, Car, OVSKernelAP
from mininet.wifi.devices import deviceDataRate
from mininet.wifi.mobility import mobility
from mininet.wifi.plot import plot2d, plot3d, plotGraph
from mininet.sixLoWPAN.module import module
from mininet.sixLoWPAN.link import sixLoWPANLink
from mininet.wifi.propagationModels import propagationModel
from mininet.wifi.vanet import vanet

sys.path.append(str(os.getcwd()) + '/mininet/')
from mininet.sumo.runner import sumo


class Mininet_sixLoWPAN(Mininet_wifi):

    def __init__(self, topo=None, switch=OVSKernelSwitch,
                 accessPoint=OVSKernelAP, host=Host, station=Station,
                 car=Car, controller=DefaultController,
                 link=Link, intf=Intf, build=True, xterms=False,
                 ipBase='2001:0:0:0:0:0:0:0/64', inNamespace=False,
                 autoSetMacs=False, autoStaticArp=False, autoPinCpus=False,
                 listenPort=None, waitConnected=False, ssid="new-ssid",
                 mode="g", channel="1", enable_wmediumd=False,
                 enable_interference=False, enable_spec_prob_link=False,
                 enable_error_prob=False, fading_coefficient=0,
                 disableAutoAssociation=False,
                 driver='nl80211', autoSetPositions=False,
                 configureWiFiDirect=False, configure4addr=False,
                 defaultGraph=False, noise_threshold=-91, cca_threshold=-90,
                 rec_rssi=False):
        """Create Mininet object.
           topo: Topo (topology) object or None
           switch: default Switch class
           host: default Host class/constructor
           controller: default Controller class/constructor
           link: default Link class/constructor
           intf: default Intf class/constructor
           ipBase: base IP address for hosts,
           build: build now from topo?
           xterms: if build now, spawn xterms?
           cleanup: if build now, cleanup before creating?
           inNamespace: spawn switches and controller in net namespaces?
           autoSetMacs: set MAC addrs automatically like IP addresses?
           autoStaticArp: set all-pairs static MAC addrs?
           autoPinCpus: pin hosts to (real) cores (requires CPULimitedHost)?
           listenPort: base listening port to open; will be incremented for
               each additional switch in the net if inNamespace=False"""
        self.topo = topo
        self.switch = switch
        self.host = host
        self.station = station
        self.accessPoint = accessPoint
        self.car = car
        self.controller = controller
        self.link = link
        self.intf = intf
        self.ipBase = ipBase
        self.ipBaseNum, self.prefixLen = netParse(self.ipBase)
        self.nextIP = 1  # start for address allocation
        self.nextPosition = 1 # start for position allocation
        self.repetitions = 1 # mobility: number of repetitions
        self.inNamespace = inNamespace
        self.xterms = xterms
        self.autoSetMacs = autoSetMacs
        self.autoSetPositions = autoSetPositions
        self.autoStaticArp = autoStaticArp
        self.autoPinCpus = autoPinCpus
        self.numCores = numCores()
        self.nextCore = 0  # next core for pinning hosts to CPUs
        self.listenPort = listenPort
        self.waitConn = waitConnected
        self.routing = ''
        self.ssid = ssid
        self.mode = mode
        self.wmediumd_mode = ''
        self.channel = channel
        self.nameToNode = {}  # name to Node (Host/Switch) objects
        self.aps = []
        self.controllers = []
        self.hosts = []
        self.links = []
        self.cars = []
        self.carsSW = []
        self.carsSTA = []
        self.switches = []
        self.stations = []
        self.sixLP = []
        self.terms = []  # list of spawned xterm processes
        self.driver = driver
        self.disableAutoAssociation = disableAutoAssociation
        self.mobilityKwargs = ''
        self.isMobilityModel = False
        self.isMobility = False
        self.ppm_is_set = False
        self.alreadyPlotted = False
        self.DRAW = False
        self.ifb = False
        self.isVanet = False
        self.noise_threshold = noise_threshold
        self.cca_threshold = cca_threshold
        self.configureWiFiDirect = configureWiFiDirect
        self.configure4addr = configure4addr
        self.enable_wmediumd = enable_wmediumd
        self.enable_error_prob = enable_error_prob
        self.fading_coefficient = fading_coefficient
        self.enable_interference = enable_interference
        self.enable_spec_prob_link = enable_spec_prob_link
        self.mobilityparam = dict()
        self.AC = ''
        self.alternativeModule = ''
        self.rec_rssi = rec_rssi
        self.plot = plot2d
        self.n_wpans = 0
        self.n_radios = 0
        self.min_x = 0
        self.min_y = 0
        self.min_z = 0
        self.max_x = 0
        self.max_y = 0
        self.max_z = 0
        self.nroads = 0
        self.connections = {}
        self.wlinks = []
        Mininet_sixLoWPAN.init()  # Initialize Mininet if necessary

        self.built = False
        if topo and build:
            self.build()

    def addParameters(self, node, autoSetMacs, params, mode='managed'):
        """adds parameters to wireless nodes
        node: node
        autoSetMacs: set MAC addrs automatically like IP addresses
        params: parameters
        defaults: Default IP and MAC addresses
        mode: if interface is running in managed or master mode"""
        node.params['frequency'] = []
        node.params['channel'] = []
        node.params['mode'] = []
        node.params['wlan'] = []
        node.params['mac'] = []
        node.phyID = []
        node.autoTxPower = False

        if 'passwd' in params:
            node.params['passwd'] = []
            passwd_list = params['passwd'].split(',')
            for passwd in passwd_list:
                node.params['passwd'].append(passwd)

        if 'encrypt' in params:
            node.params['encrypt'] = []
            encrypt_list = params['encrypt'].split(',')
            for encrypt in encrypt_list:
                node.params['encrypt'].append(encrypt)

        if mode == 'managed':
            node.params['apsInRange'] = []
            node.params['associatedTo'] = []
            if not self.enable_interference:
                node.params['rssi'] = []
            node.ifaceToAssociate = 0
            node.max_x = 0
            node.max_y = 0
            node.min_x = 0
            node.min_y = 0
            node.max_v = 0
            node.min_v = 0

            # max_speed
            if 'max_speed' in params:
                node.max_speed = int(params['max_speed'])
            else:
                node.max_speed = 10

            # min_speed
            if 'min_speed' in params:
                node.min_speed = int(params['min_speed'])
            else:
                node.min_speed = 1

        # speed
        if 'speed' in params:
            node.speed = int(params['speed'])

        # max_x
        if 'max_x' in params:
            node.max_x = int(params['max_x'])

        # max_y
        if 'max_y' in params:
            node.max_y = int(params['max_y'])

        # min_x
        if 'min_x' in params:
            node.min_x = int(params['min_x'])

        # min_y
        if 'min_y' in params:
            node.min_y = int(params['min_y'])

        # min_v
        if 'min_v' in params:
            node.min_v = int(params['min_v'])

        # max_v
        if 'max_v' in params:
            node.max_v = int(params['max_v'])

        # constantVelocity
        if 'constantVelocity' in params:
            node.constantVelocity = int(params['constantVelocity'])
        else:
            node.constantVelocity = 1

        # constantDistance
        if 'constantDistance' in params:
            node.constantDistance = int(params['constantDistance'])
        else:
            node.constantDistance = 1

        # position
        if 'position' in params:
            position = params['position']
            position = position.split(',')
            node.params['position'] = [float(position[0]),
                                       float(position[1]),
                                       float(position[2])]
        else:
            if 'position' in node.params:
                position = node.params['position']
                position = position.split(',')
                node.params['position'] = [float(position[0]),
                                           float(position[1]),
                                           float(position[2])]

        wpans = self.count6LoWPANIfaces(params)

        for wlan in range(wpans):
            self.addParamsToNode(node)
            if mode == 'managed':
                self.appendAssociatedTo(node)

            if mode == 'master':
                if 'phywlan' in node.params and wlan == 0:
                    node.params['wlan'].append(node.params['phywlan'])
                else:
                    node.params['wlan'].append(node.name + '-wpan' + str(wlan + 1))
                if 'link' in params and params['link'] == 'mesh':
                    self.appendRSSI(node)
                    self.appendAssociatedTo(node)
            else:
                node.params['wlan'].append(node.name + '-wpan' + str(wlan))
                self.appendRSSI(node)
            node.params.pop("wlans", None)

        if mode == 'managed':
            self.addMacParamToNode(node, wpans, autoSetMacs, params)
            self.addIpParamToNode(node, wpans, autoSetMacs, params)

        self.addAntennaGainParamToNode(node, wpans, params)
        self.addAntennaHeightParamToNode(node, wpans, params)
        self.addTxPowerParamToNode(node, wpans, params)
        self.addChannelParamToNode(node, wpans, params)
        self.addModeParamToNode(node, wpans, params)
        self.addRangeParamToNode(node, wpans, params)

        # Equipment Model
        equipmentModel = ("%s" % params.pop('equipmentModel', {}))
        if equipmentModel != "{}":
            node.equipmentModel = equipmentModel

        if mode == 'master' or 'ssid' in node.params:
            node.params['associatedStations'] = []
            node.params['stationsInRange'] = {}
            node._4addr = False

            if 'config' in node.params:
                config = node.params['config']
                if config != []:
                    config = node.params['config'].split(',')
                    for conf in config:
                        if 'wpa=' in conf or 'wep=' in conf:
                            node.params['encrypt'] = []
                        if 'wpa=' in conf:
                            node.params['encrypt'].append('wpa')
                        if 'wep=' in conf:
                            node.params['encrypt'].append('wep')

            if mode == 'master':
                node.params['mac'] = []
                node.params['mac'].append('')
                if 'mac' in params:
                    node.params['mac'][0] = params[ 'mac' ]

                if 'ssid' in params:
                    node.params['ssid'] = []
                    ssid_list = params['ssid'].split(',')
                    for ssid in ssid_list:
                        node.params['ssid'].append(ssid)

    def add6LoWPAN(self, name, cls=None, **params):
        """Add 6LoWPAN node.
           name: name of station to add
           cls: custom 6LoWPAN class/constructor (optional)
           params: parameters for 6LoWPAN
           returns: added station"""
        # Default IP and MAC addresses
        defaults = {'ip': ipAdd6(self.nextIP,
                                ipBaseNum=self.ipBaseNum,
                                prefixLen=self.prefixLen) +
                          '/%s' % self.prefixLen,
                    'channel': self.channel,
                    'mode': self.mode
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
            cls = self.station
        node = cls(name, **defaults)

        self.addParameters(node, self.autoSetMacs, defaults)

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

    def __iter__(self):
        "return iterator over node names"
        for node in chain(self.hosts, self.switches, self.controllers,
                          self.stations, self.carsSTA, self.aps, self.sixLP):
            yield node.name

    def __len__(self):
        "returns number of nodes in net"
        return (len(self.hosts) + len(self.switches) +
                len(self.controllers) + len(self.stations) +
                len(self.carsSTA) + len(self.aps) + len(self.sixLP))

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

    def addLink(self, node1, port1=None, cls=None, **params):
        """"Add a link from node1 to node2
            node1: source node (or name)
            node2: dest node (or name)
            port1: source port (optional)
            port2: dest port (optional)
            cls: link class (optional)
            params: additional link params (optional)
            returns: link object"""
        # Accept node objects or names
        node1 = node1 if not isinstance(node1, string_types) else self[node1]
        options = dict(params)

        # Port is optional
        if port1 is not None:
            options.setdefault('port1', port1)

        # Set default MAC - this should probably be in Link
        options.setdefault('addr1', self.randMac())

        cls = sixLoWPANLink
        cls(name=node1.params['wlan'][0], node=node1, **params)


    def monitor(self, hosts=None, timeoutms=-1):
        """Monitor a set of hosts (or all hosts by default),
           and return their output, a line at a time.
           hosts: (optional) set of hosts to monitor
           timeoutms: (optional) timeout value in ms
           returns: iterator which returns host, line"""
        if hosts is None:
            hosts = self.hosts
        poller = select.poll()
        h1 = hosts[0]  # so we can call class method fdToNode
        for host in hosts:
            poller.register(host.stdout)
        while True:
            ready = poller.poll(timeoutms)
            for fd, event in ready:
                host = h1.fdToNode(fd)
                if event & select.POLLIN:
                    line = host.readline()
                    if line is not None:
                        yield host, line
            # Return if non-blocking
            if not ready and timeoutms >= 0:
                yield None, None

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

    def ping(self, hosts=None, timeout=None):
        """Ping between all specified hosts.
           hosts: list of hosts
           timeout: time to wait for a response, as string
           returns: ploss packet loss percentage"""
        # should we check if running?
        packets = 0
        lost = 0
        ploss = None
        if not hosts:
            hosts = self.hosts + self.stations + self.sixLP
            output('*** Ping: testing ping reachability\n')
        for node in hosts:
            output('%s -> ' % node.name)
            for dest in hosts:
                if node != dest:
                    opts = ''
                    if timeout:
                        opts = '-W %s' % timeout
                    if dest.intfs:
                        result = node.cmdPrint('ping -c1 %s %s'
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
        hosts = self.hosts + self.stations + self.sixLP
        all_outputs = []
        packets = 0
        lost = 0
        ploss = None
        output('*** Ping: testing ping reachability\n')
        for node in hosts:
            output('%s -> ' % node.name)
            if timeout:
                opts = '-W %s' % timeout
            result = node.cmdPrint('ping6 -c1 ff02::1%%%s-lowpan'
                          % (node.name))
            sent, received = self._parsePing(result)
            packets += sent
            if received > sent:
                error('*** Error: received too many packets')
                error('%s' % result)
                node.cmdPrint('route')
                exit(1)
            lost += sent - received
            #output(('%s ' % dest.name) if received else 'X ')
        output('\n')
        if packets > 0:
            ploss = 100.0 * lost / packets
            received = packets - lost
            output("*** Results: %i%% dropped (%d/%d received)\n" %
                   (ploss, received, packets))
        else:
            ploss = 0
            output("*** Warning: No packets sent\n")
        return 1

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
        sleep(3)
        nodes = self.hosts + self.stations
        hosts = hosts or [nodes[0], nodes[-1]]
        assert len(hosts) is 2
        client, server = hosts

        conn1 = 0
        conn2 = 0
        if isinstance(client, Station) or isinstance(server, Station):
            if isinstance(client, Station):
                while conn1 is 0:
                    conn1 = int(client.cmd('iw dev %s link '
                                           '| grep -ic \'Connected\''
                                           % client.params['wlan'][0]))
            if isinstance(server, Station):
                while conn2 is 0:
                    conn2 = int(server.cmd('iw dev %s link | grep -ic '
                                           '\'Connected\''
                                           % server.params['wlan'][0]))
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
    def init(cls):
        "Initialize Mininet"
        if cls.inited:
            return
        ensureRoot()
        fixLimits()
        cls.inited = True

    def count6LoWPANIfaces(self, params):
        "Count the number of virtual 6LoWPAN interfaces"
        if 'wlans' in params:
            self.n_wpans += int(params['wlans'])
            wlans = int(params['wlans'])
        else:
            wlans = 1
            self.n_wpans += 1
        return wlans

    def kill_fakelb(self):
        "Kill fakelb"
        module.fakelb()
        sleep(0.1)

    def configureIface(self, node, wlan):
        intf = module.wlan_list[0]
        module.wlan_list.pop(0)
        node.renameIface(intf, node.params['wlan'][wlan])

    def configureSixLoWPANNodes(self):
        "Configure WiFi Nodes"
        nodes = self.stations + self.aps + self.cars + self.sixLP
        module.start(nodes, self.n_wpans, self.alternativeModule)
        self.configureWifiNodes()
        #self.configNodes()

    def configNodes(self):
        "Configure a set of nodes."
        for node in self.sixLP:
            ip = node.params['ip'][0]
            sixLoWPANLink.setIP(ip)

    def closeMininetWiFi(self):
        "Close Mininet-WiFi"
        self.plot.closePlot()
        module.stop()  # Stopping WiFi Module
