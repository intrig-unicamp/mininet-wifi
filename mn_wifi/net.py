"""
    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)"""

import os
import random
import re
import sys
from sys import version_info as py_version_info
import select
import signal
from time import sleep
from itertools import chain, groupby
from math import ceil
from six import string_types

from mininet.cli import CLI
from mininet.term import cleanUpScreens, makeTerms
from mininet.net import Mininet
from mininet.node import (Node, Host, OVSKernelSwitch,
                          DefaultController, Controller)
from mininet.util import (quietRun, fixLimits, numCores, ensureRoot,
                          macColonHex, ipStr, ipParse, netParse, ipAdd,
                          waitListening)
from mininet.link import Link, Intf, TCLink, TCULink
from mininet.nodelib import NAT
from mininet.log import info, error, debug, output, warn

from mn_wifi.node import AccessPoint, AP, Station, Car, \
    OVSKernelAP, physicalAP
from mn_wifi.wmediumdConnector import w_starter, w_server, \
    error_prob, snr, interference
from mn_wifi.link import wirelessLink, wmediumd, Association, \
    _4address, TCWirelessLink, TCLinkWirelessStation, ITSLink, \
    wifiDirectLink, adhoc, mesh, physicalMesh, physicalWifiDirectLink
from mn_wifi.devices import GetRate, GetRange
from mn_wifi.mobility import mobility
from mn_wifi.plot import plot2d, plot3d, plotGraph
from mn_wifi.module import module
from mn_wifi.propagationModels import propagationModel
from mn_wifi.vanet import vanet
from mn_wifi.sixLoWPAN.net import Mininet_6LoWPAN as sixlowpan
from mn_wifi.sixLoWPAN.module import module as sixLoWPAN_module
from mn_wifi.sixLoWPAN.link import sixLoWPANLink

sys.path.append(str(os.getcwd()) + '/mininet/')

VERSION = "2.3"


class Mininet_wifi(Mininet):

    def __init__(self, topo=None, switch=OVSKernelSwitch,
                 accessPoint=OVSKernelAP, host=Host, station=Station,
                 car=Car, controller=DefaultController,
                 link=TCWirelessLink, intf=Intf, build=True, xterms=False,
                 cleanup=False, ipBase='10.0.0.0/8', inNamespace=False,
                 autoSetMacs=False, autoStaticArp=False, autoPinCpus=False,
                 listenPort=None, waitConnected=False, ssid="new-ssid",
                 mode="g", channel=1, wmediumd_mode=snr,
                 fading_coefficient=0, autoAssociation=True,
                 allAutoAssociation=True, driver='nl80211',
                 autoSetPositions=False, configureWiFiDirect=False,
                 configure4addr=False, noise_threshold=-91, cca_threshold=-90,
                 rec_rssi=False, disable_tcp_checksum=False, ifb=False,
                 bridge=False, plot=False, plot3d=False, docker=False,
                 container='mininet-wifi', ssh_user='alpha'):
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
           autoPinCpus: pin hosts to (real) cores (requires CPULimitedStation)?
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
        self.cleanup = cleanup
        self.ipBase = ipBase
        self.ipBaseNum, self.prefixLen = netParse(self.ipBase)
        self.nextIP = 1  # start for address allocation
        self.nextPos_sta = 1 # start for sta position allocation
        self.nextPos_ap = 1  # start for ap position allocation
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
        self.ssid = ssid
        self.mode = mode
        self.channel = channel
        self.wmediumd_mode = wmediumd_mode
        self.nameToNode = {}  # name to Node (Host/Switch) objects
        self.wmediumdMac = []
        self.aps = []
        self.controllers = []
        self.hosts = []
        self.links = []
        self.cars = []
        self.switches = []
        self.stations = []
        self.sixLP = []
        self.terms = []  # list of spawned xterm processes
        self.driver = driver
        self.autoAssociation = autoAssociation # does not include mobility
        self.allAutoAssociation = allAutoAssociation # includes mobility
        self.ppm_is_set = False
        self.DRAW = False
        self.docker = docker
        self.container = container
        self.ssh_user = ssh_user
        self.ifb = ifb #Support to Intermediate Functional Block (IFB) Devices
        self.isVanet = False
        self.bridge = bridge
        self.init_plot = plot
        self.init_plot3d = plot3d
        self.cca_threshold = cca_threshold
        self.configureWiFiDirect = configureWiFiDirect
        self.configure4addr = configure4addr
        self.fading_coefficient = fading_coefficient
        self.noise_threshold = noise_threshold
        self.mob_param = dict()
        self.alt_module = None
        self.rec_rssi = rec_rssi
        self.disable_tcp_checksum = disable_tcp_checksum
        self.plot = plot2d
        self.seed = 1
        self.n_radios = 0
        self.min_v = 1
        self.max_v = 10
        self.min_x = 0
        self.min_y = 0
        self.min_z = 0
        self.max_x = 100
        self.max_y = 100
        self.max_z = 0
        self.nroads = 0
        self.conn = {}
        self.wlinks = []
        Mininet_wifi.init()  # Initialize Mininet if necessary

        if self.rec_rssi:
            mobility.rec_rssi = True

        if not allAutoAssociation:
            self.autoAssociation = False
            mobility.allAutoAssociation = False

        self.built = False
        if topo and build:
            self.build()

    def waitConnected(self, timeout=None, delay=.5):
        """wait for each switch to connect to a controller,
           up to 5 seconds
           timeout: time to wait, or None to wait indefinitely
           delay: seconds to sleep per iteration
           returns: True if all switches are connected"""
        info('*** Waiting for switches/aps to connect\n')
        time = 0
        L2nodes = self.switches + self.aps
        remaining = list(L2nodes)
        while True:
            for switch in tuple(remaining):
                if switch.connected():
                    info('%s ' % switch)
                    remaining.remove(switch)
            if not remaining:
                info('\n')
                return True
            if timeout is not None and time > timeout:
                break
            sleep(delay)
            time += delay
        warn('Timed out after %d seconds\n' % time)
        for switch in remaining:
            if not switch.connected():
                warn('Warning: %s is not connected to a controller\n'
                     % switch.name)
            else:
                remaining.remove(switch)
        return not remaining

    def delNode(self, node, nodes=None):
        """Delete node
           node: node to delete
           nodes: optional list to delete from (e.g. self.hosts)"""
        if nodes is None:
            nodes = (self.hosts if node in self.hosts else
                     (self.switches if node in self.switches else
                      (self.controllers if node in self.controllers else [])))
        node.stop(deleteIntfs=True)
        node.terminate()
        nodes.remove(node)
        del self.nameToNode[node.name]

    def addStation(self, name, cls=None, **params):
        """Add Station.
           name: name of station to add
           cls: custom host class/constructor (optional)
           params: parameters for station
           returns: added station"""
        # Default IP and MAC addresses
        defaults = {'ip': ipAdd(self.nextIP,
                                ipBaseNum=self.ipBaseNum,
                                prefixLen=self.prefixLen) +
                          '/%s' % self.prefixLen,
                    'channel': self.channel,
                    'mode': self.mode
                   }
        defaults.update(params)

        if self.autoSetPositions:
            defaults['position'] = (round(self.nextPos_sta,2),0,0)
        if self.autoSetMacs:
            defaults['mac'] = macColonHex(self.nextIP)
        if self.autoPinCpus:
            defaults['cores'] = self.nextCore
            self.nextCore = (self.nextCore + 1) % self.numCores
        self.nextIP += 1
        self.nextPos_sta += 100

        if not cls:
            cls = self.station
        sta = cls(name, **defaults)

        self.addParameters(sta, self.autoSetMacs, **defaults)
        if 'sixlowpan' in params:
            sixlowpan.init(sta, **params)
            self.sixLP.append(sta)

        self.stations.append(sta)
        self.nameToNode[name] = sta
        return sta

    def add6LoWPAN(self, name, cls=None, **params):
        node = sixlowpan.add6LoWPAN(name, cls, **params)
        self.sixLP.append(node)
        self.nameToNode[name] = node
        return node

    def addCar(self, name, cls=None, **params):
        """Add Car.
           name: name of vehicle to add
           cls: custom host class/constructor
           params: parameters for car
           returns: added car"""
        # Default IP and MAC addresses
        defaults = {'ip': ipAdd(self.nextIP,
                                ipBaseNum=self.ipBaseNum,
                                prefixLen=self.prefixLen) +
                          '/%s' % self.prefixLen,
                    'channel': self.channel,
                    'mode': self.mode,
                    'ssid': self.ssid
                    }

        if self.autoSetMacs:
            defaults['mac'] = macColonHex(self.nextIP)
        if self.autoPinCpus:
            defaults['cores'] = self.nextCore
            self.nextCore = (self.nextCore + 1) % self.numCores
        defaults.update(params)

        self.nextIP += 1
        if not cls:
            cls = self.car
        car = cls(name, **defaults)

        self.addParameters(car, self.autoSetMacs, **defaults)
        self.cars.append(car)
        self.nameToNode[name] = car

        return car

    def addAccessPoint(self, name, cls=None, **params):
        """Add AccessPoint.
           name: name of accesspoint to add
           cls: custom switch class/constructor (optional)
           returns: added accesspoint
           side effect: increments listenPort var ."""
        defaults = {'listenPort': self.listenPort,
                    'inNamespace': self.inNamespace,
                    'ssid': self.ssid,
                    'channel': self.channel,
                    'mode': self.mode
                   }

        if self.bridge:
            defaults['isolate_clients'] = True
        defaults.update(params)
        if self.autoSetPositions:
            defaults['position'] = (round(self.nextPos_ap,2), 50, 0)
            self.nextPos_ap += 100

        wlan = None
        if cls == physicalAP:
            wlan = ("%s" % params.pop('phywlan', {}))
            cls = self.accessPoint
        if not cls:
            cls = self.accessPoint
        ap = cls(name, **defaults)

        if not self.inNamespace and self.listenPort:
            self.listenPort += 1

        if self.inNamespace or ('inNamespace' in params
                                and params['inNamespace'] is True):
            ap.params['inNamespace'] = True

        self.nameToNode[name] = ap

        if wlan:
            ap.params['phywlan'] = wlan

        self.addParameters(ap, self.autoSetMacs, node_mode='master', **defaults)
        if 'type' in params and params['type'] is 'mesh':
            ap.func[1] = 'mesh'
            ap.ifaceToAssociate = 1

        self.aps.append(ap)
        return ap

    def addNAT(self, name='nat0', connect=True, inNamespace=False,
               **params):
        """Add a NAT to the Mininet network
           name: name of NAT node
           connect: switch to connect to | True (s1) | None
           inNamespace: create in a network namespace
           params: other NAT node params, notably:
               ip: used as default gateway address"""
        nat = self.addHost(name, cls=NAT, inNamespace=inNamespace,
                           subnet=self.ipBase, **params)
        # find first ap and create link
        if connect:
            if not isinstance(connect, Node):
                # Use first ap if not specified
                connect = self.aps[0]
            # Connect the nat to the ap
            self.addLink(nat, connect)
            # Set the default route on stations
            natIP = nat.params['ip'].split('/')[0]
            for station in self.stations:
                if station.inNamespace:
                    station.setDefaultRoute('via %s' % natIP)
        return nat

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
                          self.stations, self.cars, self.aps, self.sixLP):
            yield node.name

    def __len__(self):
        "returns number of nodes in net"
        return (len(self.hosts) + len(self.switches) +
                len(self.controllers) + len(self.stations) +
                len(self.cars) + len(self.aps) + len(self.sixLP))

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
    def randMac():
        "Return a random, non-multicast MAC address"
        return macColonHex(random.randint(1, 2 ** 48 - 1) & 0xfeffffffffff |
                           0x020000000000)

    def setModule(self, moduleDir):
        "set an alternative module rather than mac80211_hwsim"
        self.alt_module = moduleDir

    def addMacToWmediumd(self, mac=None):
        self.wmediumdMac.append(mac)

    def addLink(self, node1, node2=None, port1=None, port2=None,
                cls=None, **params):
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
        node2 = node2 if not isinstance(node2, string_types) else self[node2]
        options = dict(params)

        self.conn.setdefault('src', [])
        self.conn.setdefault('dst', [])
        self.conn.setdefault('ls', [])

        cls = self.link if cls is None else cls

        if cls == mesh or cls == physicalMesh:
            isAP=False
            if isinstance(node1, AP):
                isAP=True
            cls(node=node1, isAP=isAP, **params)
        elif cls == adhoc:
            cls(node=node1, link=self.link, **params)
        elif cls == ITSLink:
            cls(node=node1, link=self.link, **params)
        elif cls == wifiDirectLink or cls == physicalWifiDirectLink:
            link = cls(node=node1, port=port1, **params)
            return link
        elif cls == sixLoWPANLink:
            link = cls(node=node1, port=port1, **params)
            self.links.append(link)
            return link
        elif cls == _4address:
            if 'position' in node1.params and 'position' in node2.params:
                self.conn['src'].append(node1)
                self.conn['dst'].append(node2)
                self.conn['ls'].append('--')

            if node1 not in self.aps:
                self.aps.append(node1)
            elif node2 not in self.aps:
                self.aps.append(node2)

            if self.wmediumd_mode == interference:
                link = cls(node1, node2, port1, port2)
                self.links.append(link)
                return link
            else:
                dist = node1.get_distance_to(node2)
                if dist <= node1.params['range'][0]:
                    link = cls(node1, node2)
                    self.links.append(link)
                    return link
        elif ((node1 in (self.stations or self.cars) and node2 in self.aps)
              or (node2 in (self.stations or self.cars) and node1 in self.aps)) and \
                        'link' not in options:
            self.infraAssociation(node1, node2, port1, port2, cls, **params)
        elif 'wifi' in params:
            self.infraAssociation(node1, node2, port1, port2, cls, **params)
        else:
            if 'link' in options:
                options.pop('link', None)

            if 'position' in node1.params and 'position' in node2.params:
                self.conn['src'].append(node1)
                self.conn['dst'].append(node2)
                self.conn['ls'].append('-')
            # Port is optional
            if port1 is not None:
                options.setdefault('port1', port1)
            if port2 is not None:
                options.setdefault('port2', port2)

            # Set default MAC - this should probably be in Link
            options.setdefault('addr1', self.randMac())
            options.setdefault('addr2', self.randMac())

            if not cls or cls == wmediumd or cls == TCWirelessLink:
                cls = TCLink
            if self.disable_tcp_checksum:
                cls = TCULink

            cls = self.link if cls is None else cls
            link = cls(node1, node2, **options)
            self.links.append(link)
            return link

    def infraAssociation(self, node1, node2, port1=None, port2=None,
                         cls=None, **params):
        sta = node2
        ap = node1
        sta_wlan = None
        ap_wlan = 0

        if port2:
            sta_wlan = port2
        if port1:
            ap_wlan = port1
        params['ap_wlan'] = ap_wlan

        if 'wifi' in params:
            if node2.name == params['wifi']:
                ap = node2
                sta = node1
        else:
            if node1 in self.stations and node2 in self.aps:
                sta = node1
                ap = node2
                if port1:
                    sta_wlan = port1
                if port2:
                    ap_wlan = port2

        wlan = sta.ifaceToAssociate
        if sta_wlan:
            wlan = sta_wlan
        params['wlan'] = wlan
        # If sta/ap have position
        doAssociation = True
        if 'position' in sta.params and 'position' in ap.params:
            dist = sta.get_distance_to(ap)
            if dist > ap.params['range'][ap_wlan]:
                doAssociation = False
        if doAssociation:
            sta.params['mode'][wlan] = ap.params['mode'][ap_wlan]
            sta.params['channel'][wlan] = ap.params['channel'][ap_wlan]
            enable_wmediumd = False
            enable_interference = False
            if self.link == wmediumd:
                enable_wmediumd = True
            if self.wmediumd_mode == interference:
                enable_interference = True
            if not self.topo:
                params['printCon'] = True
            params['doAssociation'] = True
            Association.associate(sta, ap, enable_wmediumd,
                                  enable_interference, **params)
            if 'TCWirelessLink' in str(self.link.__name__):
                if 'bw' not in params and 'bw' not in str(cls) and \
                        not self.ifb:
                    value = self.getDataRate(sta, ap, **params)
                    bw = value.rate
                    params['bw'] = bw
                # tc = True, this is useful only to apply tc configuration
                link = cls(name=sta.params['wlan'][wlan], node=sta,
                           tc=True, **params)
                # self.links.append(link)
                return link
        if self.link == wmediumd:
            if self.wmediumd_mode == error_prob:
                wmediumd.wlinks.append([sta, ap, params['error_prob']])
            elif self.wmediumd_mode != interference:
                wmediumd.wlinks.append([sta, ap])

    def delLink(self, link):
        "Remove a link from this network"
        link.delete()
        self.links.remove(link)

    def linksBetween(self, node1, node2):
        "Return Links between node1 and node2"
        return [link for link in self.links
                if (node1, node2) in (
                    (link.intf1.node, link.intf2.node),
                    (link.intf2.node, link.intf1.node))]

    def delLinkBetween(self, node1, node2, index=0, allLinks=False):
        """Delete link(s) between node1 and node2
           index: index of link to delete if multiple links (0)
           allLinks: ignore index and delete all such links (False)
           returns: deleted link(s)"""
        links = self.linksBetween(node1, node2)
        if not allLinks:
            links = [links[index]]
        for link in links:
            self.delLink(link)
        return links

    def configHosts(self):
        "Configure a set of nodes."
        hosts = self.hosts + self.stations
        for host in hosts:
            # info( host.name + ' ' )
            intf = host.defaultIntf()
            if intf:
                host.configDefault()
            else:
                # Don't configure nonexistent intf
                host.configDefault(ip=None, mac=None)
                # You're low priority, dude!
                # BL: do we want to do this here or not?
                # May not make sense if we have CPU lmiting...
                # quietRun( 'renice +18 -p ' + repr( host.pid ) )
                # This may not be the right place to do this, but
                # it needs to be done somewhere.
                # info( '\n' )

    def buildFromWirelessTopo(self, topo=None):
        """Build mininet from a topology object
           At the end of this function, everything should be connected
           and up."""
        info('*** Creating network\n')
        if not self.controllers and self.controller:
            # Add a default controller
            info('*** Adding controller\n')
            classes = self.controller
            if not isinstance(classes, list):
                classes = [classes]
            for i, cls in enumerate(classes):
                # Allow Controller objects because nobody understands partial()
                if isinstance(cls, Controller):
                    self.addController(cls)
                else:
                    self.addController('c%d' % i, cls)

        info('*** Adding stations:\n')
        for staName in topo.stations():
            self.addStation(staName, **topo.nodeInfo(staName))
            info(staName + ' ')

        info('\n*** Adding access points:\n')
        for apName in topo.aps():
            # A bit ugly: add batch parameter if appropriate
            params = topo.nodeInfo(apName)
            cls = params.get('cls', self.accessPoint)
            if hasattr(cls, 'batchStartup'):
                params.setdefault('batch', True)
            self.addAccessPoint(apName, **params)
            info(apName + ' ')

        info('\n*** Configuring wifi nodes...\n')
        self.configureWifiNodes()

        info('\n*** Adding link(s):\n')
        for srcName, dstName, params in topo.links(
                sort=True, withInfo=True):
            self.addLink(**params)
            info('(%s, %s) ' % (srcName, dstName))
        info('\n')

    def configureControlNetwork(self):
        "Control net config hook: override in subclass"
        raise Exception('configureControlNetwork: '
                        'should be overriden in subclass', self)

    def build(self):
        "Build mininet-wifi."
        if self.topo:
            self.buildFromWirelessTopo(self.topo)
            if self.init_plot or self.init_plot3d:
                max_z = 0
                if self.init_plot3d:
                    max_z = len(self.stations) * 100
                self.plotGraph(max_x=(len(self.stations) * 100),
                               max_y=(len(self.stations) * 100),
                               max_z=max_z)
        else:
            if not mobility.stations:
                for node in self.stations:
                    if 'position' in node.params:
                        mobility.stations.append(node)

        if (self.configure4addr or self.configureWiFiDirect
                or self.wmediumd_mode == error_prob) and self.link == wmediumd:
            wmediumd(self.fading_coefficient, self.noise_threshold,
                     self.stations, self.aps, self.cars, propagationModel,
                     self.wmediumdMac)
            for sta in self.stations:
                if self.wmediumd_mode != error_prob:
                    sta.set_pos_wmediumd(mob=False)
            for sta in self.stations:
                if sta in self.aps:
                    self.stations.remove(sta)

        if self.inNamespace:
            self.configureControlNetwork()
        info('*** Configuring nodes\n')
        self.configHosts()
        if self.xterms:
            self.startTerms()
        if self.autoStaticArp:
            self.staticArp()

        nodes = self.stations
        for node in nodes:
            for wlan in range(0, len(node.params['wlan'])):
                if not isinstance(node, AP) and node.func[0] != 'ap' and \
                        node.func[wlan] != 'mesh' and \
                                node.func[wlan] != 'adhoc' and \
                                node.func[wlan] != 'wifiDirect':
                    if isinstance(node, Station):
                        node.params['range'][wlan] = \
                            int(node.params['range'][wlan]) / 5

        if self.allAutoAssociation:
            if self.autoAssociation and not self.mob_param \
                    and not self.configureWiFiDirect:
                self.auto_association()
        if self.mob_param:
            if 'model' in self.mob_param or self.isVanet or self.nroads != 0:
                self.mob_param['nodes'] = self.getMobileNodes()
                self.start_mobility(**self.mob_param)
            else:
                self.mob_param['plotNodes'] = self.plot_nodes()
                mobility.stop(**self.mob_param)
        else:
            if self.DRAW:
                plotNodes = self.plot_nodes()
                self.plotCheck(plotNodes)
        self.built = True

    def plot_nodes(self):
        other_nodes = self.hosts + self.switches + self.controllers
        plotNodes = []
        for node in other_nodes:
            if hasattr(node, 'plotted') and node.plotted is True:
                plotNodes.append(node)
        return plotNodes

    def startTerms(self):
        "Start a terminal for each node."
        if 'DISPLAY' not in os.environ:
            error("Error starting terms: Cannot connect to display\n")
            return
        info("*** Running terms on %s\n" % os.environ['DISPLAY'])
        cleanUpScreens()
        self.terms += makeTerms(self.controllers, 'controller')
        self.terms += makeTerms(self.switches, 'switch')
        self.terms += makeTerms(self.hosts, 'host')
        self.terms += makeTerms(self.stations, 'station')
        self.terms += makeTerms(self.aps, 'ap')
        self.terms += makeTerms(self.sixLP, 'sixLP')

    def stopXterms(self):
        "Kill each xterm."
        for term in self.terms:
            os.kill(term.pid, signal.SIGKILL)
        cleanUpScreens()

    def staticArp(self):
        "Add all-pairs ARP entries to remove the need to handle broadcast."
        for src in self.hosts:
            for dst in self.hosts:
                if src != dst:
                    src.setARP(ip=dst.IP(), mac=dst.MAC())

    def start(self):
        "Start controller and switches."
        if not self.built:
            self.build()
        info('*** Starting controller(s)\n')
        for controller in self.controllers:
            info(controller.name + ' ')
            controller.start()
        info('\n')

        info('*** Starting switches and/or access points\n')
        nodesL2 = self.switches + self.aps
        for nodeL2 in nodesL2:
            info(nodeL2.name + ' ')
            nodeL2.start(self.controllers)

        started = {}
        if py_version_info < (3, 0):
            for swclass, switches in groupby(
                    sorted(nodesL2, key=type), type):
                switches = tuple(switches)
                if hasattr(swclass, 'batchStartup'):
                    success = swclass.batchStartup(switches)
                    started.update({s: s for s in success})
        else:
            for swclass, switches in groupby(
                    sorted(nodesL2, key=lambda x: str(type(x))), type):
                switches = tuple(switches)
                if hasattr(swclass, 'batchStartup'):
                    success = swclass.batchStartup(switches)
                    started.update({s: s for s in success})
        info('\n')
        if self.waitConn:
            self.waitConnected()

    def roads(self, nroads):
        "Number of roads"
        self.nroads = nroads

    def stop(self):
        'Stop Mininet-WiFi'
        self.stopGraphParams()
        info('*** Stopping %i controllers\n' % len(self.controllers))
        for controller in self.controllers:
            info(controller.name + ' ')
            controller.stop()
        info('\n')
        if self.terms:
            info('*** Stopping %i terms\n' % len(self.terms))
            self.stopXterms()
        info('*** Stopping %i links\n' % len(self.links))
        for link in self.links:
            info('.')
            link.stop()
        info('\n')
        info('*** Stopping switches/access points\n')
        stopped = {}
        nodesL2 = self.switches + self.aps
        if py_version_info < (3, 0):
            for swclass, switches in groupby(
                    sorted(nodesL2, key=type), type):
                switches = tuple(switches)
                if hasattr(swclass, 'batchShutdown'):
                    success = swclass.batchShutdown(switches)
                    stopped.update({s: s for s in success})
        else:
            for swclass, switches in groupby(
                    sorted(nodesL2, key=lambda x: str(type(x))), type):
                switches = tuple(switches)
                if hasattr(swclass, 'batchShutdown'):
                    success = swclass.batchShutdown(switches)
                    stopped.update({s: s for s in success})
        for switch in nodesL2:
            info(switch.name + ' ')
            if switch not in stopped:
                switch.stop()
            switch.terminate()
        info('\n')
        info('*** Stopping nodes\n')
        nodes = self.hosts + self.stations + self.sixLP
        for node in nodes:
            info(node.name + ' ')
            node.terminate()
        info('\n')
        if self.aps is not []:
            self.kill_hostapd()
        self.closeMininetWiFi()
        if self.sixLP:
            sixLoWPAN_module.stop()
        if self.link == wmediumd:
            self.kill_wmediumd()
        info('\n*** Done\n')

    def run(self, test, *args, **kwargs):
        "Perform a complete start/test/stop cycle."
        self.start()
        info('*** Running test\n')
        result = test(*args, **kwargs)
        self.stop()
        return result

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
            hosts = self.hosts + self.stations
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
        if self.sixLP:
            nodes = self.sixLP
            sixlowpan.ping6(nodes, timeout)
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
        sleep(3)
        return self.ping(timeout=timeout)

    def pingPair(self):
        """Ping between first two hosts, useful for testing.
           returns: ploss packet loss percentage"""
        sleep(3)
        nodes = self.hosts + self.stations
        hosts = [nodes[0], nodes[1]]
        return self.ping(hosts=hosts)

    def pingAllFull(self):
        """Ping between all hosts.
           returns: ploss packet loss percentage"""
        sleep(3)
        return self.pingFull()

    def pingPairFull(self):
        """Ping between first two hosts, useful for testing.
           returns: ploss packet loss percentage"""
        sleep(3)
        nodes = self.hosts + self.stations
        hosts = [nodes[0], nodes[1]]
        return self.pingFull(hosts=hosts)

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

    def runCpuLimitTest(self, cpu, duration=5):
        """run CPU limit test with 'while true' processes.
        cpu: desired CPU fraction of each host
        duration: test duration in seconds (integer)
        returns a single list of measured CPU fractions as floats.
        """
        cores = int(quietRun('nproc'))
        pct = cpu * 100
        info('*** Testing CPU %.0f%% bandwidth limit\n' % pct)
        stations = self.hosts
        cores = int(quietRun('nproc'))
        # number of processes to run a while loop on per host
        num_procs = int(ceil(cores * cpu))
        pids = {}
        for s in stations:
            pids[s] = []
            for _core in range(num_procs):
                s.cmd('while true; do a=1; done &')
                pids[s].append(s.cmd('echo $!').strip())
        outputs = {}
        time = {}
        # get the initial cpu time for each host
        for station in stations:
            outputs[station] = []
            with open('/sys/fs/cgroup/cpuacct/%s/cpuacct.usage'
                      % station, 'r') as f:
                time[station] = float(f.read())
        for _ in range(duration):
            sleep(1)
            for station in stations:
                with open('/sys/fs/cgroup/cpuacct/%s/cpuacct.usage'
                          % station, 'r') as f:
                    readTime = float(f.read())
                outputs[station].append(((readTime - time[station])
                                      / 1000000000) / cores * 100)
                time[station] = readTime
        for h, pids in pids.items():
            for pid in pids:
                h.cmd('kill -9 %s' % pid)
        cpu_fractions = []
        for _host, outputs in outputs.items():
            for pct in outputs:
                cpu_fractions.append(pct)
        output('*** Results: %s\n' % cpu_fractions)
        return cpu_fractions

    def get_distance(self, src, dst):
        """Gets the distance between two nodes

        :params src: source node
        :params dst: destination node
        :params nodes: list of nodes"""
        nodes = self.stations + self.cars + self.aps
        try:
            for host1 in nodes:
                if src == str(host1):
                    src = host1
                    for host2 in nodes:
                        if dst == str(host2):
                            dst = host2
                            dist = src.get_distance_to(dst)
                            info("The distance between %s and %s is %.2f "
                                 "meters\n" % (src, dst, float(dist)))
        except:
            info("node %s or/and node %s does not exist or there is no " \
                 "position defined" % (dst, src))

    def mobility(self, *args, **kwargs):
        "Configure mobility parameters"
        self.configureMobility(*args, **kwargs)

    def setMobilityModel(self, **kwargs):
        if 'seed' in kwargs:
            self.seed = kwargs['seed']
        self.setMobilityModelParams(**kwargs)

    def startMobility(self, **kwargs):
        if 'repetitions' in kwargs:
            self.repetitions = kwargs['repetitions']
        kwargs['init_time'] = kwargs['time']
        self.setMobilityParams(**kwargs)

    def getMobileNodes(self):
        nodes_ = []
        nodes = self.stations + self.aps + self.cars
        for node in nodes:
            if 'position' not in node.params:
                nodes_.append(node)
        return nodes_

    @staticmethod
    def setBgscan(module='simple', s_inverval=30, signal=-45,
                  l_interval=300,
                  database='/etc/wpa_supplicant/network1.bgscan'):
        """Set Background scanning
        :params module: module
        :params s_inverval: short bgscan interval in second
        :params signal: signal strength threshold
        :params l_interval: long interval
        :params database: database file name"""
        if module is 'simple':
            bgscan = 'bgscan=\"%s:%d:%d:%d\"' % \
                     (module, s_inverval, signal, l_interval)
        else:
            bgscan = 'bgscan=\"%s:%d:%d:%d:%s\"' % \
                     (module, s_inverval, signal, l_interval, database)

        Association.bgscan = bgscan

    def setPropagationModel(self, **kwargs):
        "Set Propagation Model"
        self.ppm_is_set = True
        kwargs['noise_threshold'] = self.noise_threshold
        kwargs['cca_threshold'] = self.cca_threshold
        self.propagation_model(**kwargs)

    def configWirelessLinkStatus(self, src, dst, status):

        sta = self.nameToNode[dst]
        ap = self.nameToNode[src]
        if isinstance(self.nameToNode[src], Station):
            sta = self.nameToNode[src]
            ap = self.nameToNode[dst]

        if status == 'down':
            for wlan in range(0, len(sta.params['wlan'])):
                if sta.params['associatedTo'][wlan]:
                    sta.cmd('iw dev %s disconnect' % sta.params['wlan'][wlan])
                    sta.params['associatedTo'][wlan] = ''
                    ap.params['associatedStations'].remove(sta)
        else:
            for wlan in range(0, len(sta.params['wlan'])):
                if not sta.params['associatedTo'][wlan]:
                    sta.pexec('iw dev %s connect %s %s'
                              % (sta.params['wlan'][wlan],
                                 ap.params['ssid'][0], ap.params['mac'][0]))
                    sta.params['associatedTo'][wlan] = ap
                    ap.params['associatedStations'].append(sta)

    # BL: I think this can be rewritten now that we have
    # a real link class.
    def configLinkStatus(self, src, dst, status):
        """Change status of src <-> dst links.
           src: node name
           dst: node name
           status: string {up, down}"""
        if src not in self.nameToNode:
            error('src not in network: %s\n' % src)
        elif dst not in self.nameToNode:
            error('dst not in network: %s\n' % dst)
        if isinstance(self.nameToNode[src], Station) \
                and isinstance(self.nameToNode[dst], AP) or \
                        isinstance(self.nameToNode[src], AP) \
                        and isinstance(self.nameToNode[dst], Station):
            self.configWirelessLinkStatus(src, dst, status)
        else:
            src = self.nameToNode[src]
            dst = self.nameToNode[dst]
            connections = src.connectionsTo(dst)
            if len(connections) is 0:
                error('src and dst not connected: %s %s\n' % (src, dst))
            for srcIntf, dstIntf in connections:
                result = srcIntf.ifconfig(status)
                if result:
                    error('link src status change failed: %s\n' % result)
                result = dstIntf.ifconfig(status)
                if result:
                    error('link dst status change failed: %s\n' % result)

    def interact(self):
        "Start network and run our simple CLI."
        self.start()
        result = CLI(self)
        self.stop()
        return result

    inited = False

    @classmethod
    def init(cls):
        "Initialize Mininet"
        if cls.inited:
            return
        ensureRoot()
        fixLimits()
        cls.inited = True

    def addParameters(self, node, autoSetMacs, node_mode='managed', **params):
        """adds parameters to wireless nodes
        node: node
        autoSetMacs: set MAC addrs automatically like IP addresses
        params: parameters
        defaults: Default IP and MAC addresses
        node_mode: if interface is running in managed or master mode"""
        node.params['wlan'] = []
        node.params['mac'] = []
        node.phyID = []

        array_ = ['passwd', 'scan_freq', 'freq_list', 'authmode',
                  'encrypt', 'radius_server', 'bw']
        for param in params:
            if param in array_:
                node.params[param] = []
                list = params[param].split(',')
                for value in list:
                    node.params[param].append(value)

        if node_mode == 'managed':
            node.params['apsInRange'] = []
            node.params['associatedTo'] = []
            if self.wmediumd_mode != interference:
                node.params['rssi'] = []
            node.ifaceToAssociate = 0

        array_ = ['speed', 'max_x', 'max_y', 'min_x', 'min_y',
                  'min_v', 'max_v', 'constantVelocity', 'constantDistance',
                  'min_speed', 'max_speed']
        for param in params:
            if param in array_:
                setattr(node, param, float(params[param]))

        # position
        if 'position' in params:
            position = params['position']
            if isinstance(position, string_types):
                position = position.split(',')
            node.params['position'] = (float(position[0]),
                                       float(position[1]),
                                       float(position[2]))
        else:
            if 'position' in node.params:
                position = node.params['position']
                if isinstance(position, string_types):
                    position = position.split(',')
                node.params['position'] = (float(position[0]),
                                           float(position[1]),
                                           float(position[2]))

        params['wlans'] = self.countWiFiIfaces(**params)

        for wlan in range(params['wlans']):
            node.func.append('none')
            node.phyID.append(0)
            if node_mode == 'managed':
                node.params['associatedTo'].append('')

            if node_mode == 'master':
                node.params['wlan'].append(node.name + '-wlan' + str(wlan + 1))
            else:
                node.params['wlan'].append(node.name + '-wlan' + str(wlan))
                if self.wmediumd_mode != interference:
                    node.params['rssi'].append(-60)
            node.params.pop("wlans", None)

        if node_mode == 'managed':
            self.add_mac_param(node, autoSetMacs, **params)
            self.add_ip_param(node, autoSetMacs, **params)

        array_ = ['antennaGain', 'antennaHeight', 'txpower',
                  'channel', 'mode', 'freq']

        for param in array_:
            node.params[param] = []
            if param in params:
                if isinstance(params[param], int):
                    params[param] = str(params[param])
                list = params[param].split(',')
                for value in list:
                    if param == 'mode' or param == 'channel':
                        node.params[param].append(value)
                    else:
                        node.params[param].append(float(value))
                len_ = len(node.params[param])
                if len != params['wlans']:
                    for _ in range(params['wlans']-len_):
                        node.params[param].append(value)
            else:
                for _ in range(params['wlans']):
                    if param == 'antennaGain':
                        value = 5.0
                    if param == 'antennaHeight':
                        value = 1.0
                    if param == 'txpower':
                        value = 14
                    if param == 'channel':
                        value = 1
                    if param == 'mode':
                        value = 'g'
                    if param == 'freq':
                        value = 2.412
                    node.params[param].append(value)
        self.add_range_param(node, **params)

        # Equipment Model
        model = ("%s" % params.pop('model', {}))
        if model != "{}":
            node.model = model

        if node_mode == 'master':
            node.params['associatedStations'] = []
            node.params['stationsInRange'] = {}

            node.params['mac'] = []
            node.params['mac'].append('')
            if 'mac' in params:
                node.params['mac'][0] = params['mac']

            if 'ssid' in params:
                node.params['ssid'] = []
                ssid_list = params['ssid'].split(',')
                for ssid in ssid_list:
                    node.params['ssid'].append(ssid)

            if 'config' in node.params:
                config = node.params['config'].split(',')
                for conf in config:
                    if 'wpa=' in conf or 'wep=' in conf:
                        node.params['encrypt'] = []
                        if 'wpa=' in conf:
                            node.params['encrypt'].append('wpa')
                        else:
                            node.params['encrypt'].append('wep')

    def add_range_param(self, node, wlans=0, **params):
        "Add Signal Range Param"
        node.params['range'] = []
        if 'range' in params:
            range_list = str(params['range']).split(',')
            for value in range_list:
                node.params['range'].append(float(value))
                node.setRange(float(value), intf=node.params['wlan'][0])
        else:
            for _ in range(0, wlans):
                if 'model' in node.params:
                    range_ = GetRange(node=node)
                    node.params['range'].append(range_.value)
                else:
                    node.params['range'].append(0)

    def add_ip_param(self, node, autoSetMacs=False, **params):
        "Add IP Param"
        node.params['ip'] = []
        if 'ip' in params:
            ip_list = params['ip'].split(',')
            for ip in ip_list:
                node.params['ip'].append(ip)
            if len(ip_list) != len(node.params['wlan']):
                for ip_list in range(len(ip_list),
                                     len(node.params['wlan'])):
                    node.params['ip'].append('0/0')
        elif autoSetMacs:
            for n in range(params['wlans']):
                node.params['ip'].append('0/0')
                node.params['ip'][n] = params[ 'ip' ]
        else:
            for _ in range(params['wlans']):
                node.params['ip'].append('')

    def add_mac_param(self, node, autoSetMacs=False, **params):
        "Add Mac Param"
        node.params['mac'] = []
        if 'mac' in params:
            mac_list = params['mac'].split(',')
            for mac in mac_list:
                node.params['mac'].append(mac)
            if len(mac_list) != params['wlans']:
                for _ in range(len(mac_list), params['wlans']):
                    node.params['mac'].append('')
        elif autoSetMacs:
            for n in range(params['wlans']):
                node.params['mac'].append('')
                node.params['mac'][n] = params[ 'mac' ]
        else:
            for _ in range(params['wlans']):
                node.params['mac'].append('')

    def countWiFiIfaces(self, **params):
        "Count the number of virtual wifi interfaces"
        if 'wlans' in params:
            self.n_radios += int(params['wlans'])
            wlans = int(params['wlans'])
        else:
            wlans = 1
            self.n_radios += 1
        return wlans

    def createVirtualIfaces(self, nodes):
        "Creates virtual wifi interfaces"
        for node in nodes:
            if 'nvif' in node.params:
                for vif_ in range(0, node.params['nvif']):
                    vif = node.params['wlan'][0] + str(vif_ + 1)
                    node.params['wlan'].append(vif)
                    node.params['associatedTo'].append('')
                    node.func.append('none')
                    node.phyID.append(0)
                    if self.wmediumd_mode != interference:
                        node.params['rssi'].append(-60)

                    new_mac = list(node.params['mac'][0])
                    new_mac[7] = str(vif_ + 1)
                    node.params['mac'].append("".join(new_mac))

                    array_ = ['range', 'txpower', 'channel', 'antennaGain',
                              'antennaHeight', 'mode', 'freq']
                    for param in array_:
                        node.params[param].append(node.params[param][0])

                    node.cmd('iw dev %s interface add %s type station'
                             % (node.params['wlan'][0], vif))
                    TCLinkWirelessStation(node, intfName1=vif)
                    self.configureMacAddr(node)

    @staticmethod
    def kill_hostapd():
        "Kill hostapd"
        module.kill_hostapd()
        sleep(0.1)

    @staticmethod
    def kill_wmediumd():
        "Kill wmediumd"
        info("\n*** Killing wmediumd")
        w_server.disconnect()
        w_starter.stop()
        sleep(0.1)

    @staticmethod
    def kill_mac80211_hwsim():
        "Kill mac80211_hwsim"
        module.kill_mac80211_hwsim()
        sleep(0.1)

    def configureWirelessLink(self):
        """Configure Wireless Link

        :param stations: list of stations
        :param aps: list of access points
        :param cars: list of cars"""
        nodes = self.stations + self.cars
        for node in nodes:
            for wlan in range(0, len(node.params['wlan'])):
                link = TCLinkWirelessStation(node,
                                             intfName1=node.params['wlan'][wlan])
                self.links.append(link)
            self.configureMacAddr(node)

    def plotGraph(self, **kwargs):
        "Plots Graph"
        self.DRAW = True
        for key in kwargs:
            setattr(self, key, kwargs[key])
        if 'max_z' in kwargs and kwargs['max_z'] != 0:
            self.plot = plot3d

    def checkDimension(self, nodes):
        try:
            for node in nodes:
                if hasattr(node, 'coord'):
                    node.params['position'] = node.coord[0].split(',')
            plotGraph(min_x=self.min_x, min_y=self.min_y, min_z=self.min_z,
                      max_x=self.max_x, max_y=self.max_y, max_z=self.max_z,
                      nodes=nodes, conn=self.conn)
            if not issubclass(self.plot, plot3d):
                self.plot.pause()
        except:
            info('Something went wrong with the GUI.\n')
            self.DRAW = False

    def start_mobility(self, **kwargs):
        "Starts Mobility"
        if 'model' in kwargs or self.isVanet or self.nroads != 0:
            nodes = []
            for node in kwargs['nodes']:
                if 'position' not in node.params \
                        or 'position' in node.params \
                                and node.params['position'] == (-1, -1, -1):
                    node.isStationary = False
                    nodes.append(node)
                    node.params['position'] = (0, 0, 0)
            kwargs['nodes'] = nodes
            self.setMobilityParams(**kwargs)
            if self.nroads == 0:
                mobility.start(**self.mob_param)
            else:
                self.mob_param['cars'] = self.cars
                vanet(**self.mob_param)

    def stopMobility(self, **kwargs):
        "Stops Mobility"
        if self.allAutoAssociation and \
                not self.configureWiFiDirect and not self.configure4addr:
            self.auto_association()
        kwargs['final_time'] = kwargs['time']
        self.setMobilityParams(**kwargs)

    def setAssociationCtrl(self, ac='ssf'):
        "set association control"
        mobility.ac = ac

    def setMobilityModelParams(self, **kwargs):
        "Set Mobility Parameters"

        args = ['min_x', 'min_y', 'min_z',
                'max_x', 'max_y', 'max_z',
                'min_v', 'max_v']
        for arg in args:
            if arg in kwargs:
                setattr(self, arg, float(kwargs[arg]))
                self.mob_param.setdefault(arg, getattr(self, arg))
            else:
                if arg in ['min_v', 'max_v']:
                    self.mob_param.setdefault(arg, 5.0)

        kwargs['nodes'] = self.getMobileNodes()
        if self.nroads != 0:
            self.mob_param.setdefault('nroads', self.nroads)

        if 'ac_method' in kwargs:
            self.mob_param.setdefault('ac_method', kwargs['ac_method'])

        if 'max_wt' in kwargs:
            self.mob_param.setdefault('max_wt', float(kwargs['max_wt']))
        else:
            self.mob_param.setdefault('max_wt', float(100))
        if 'min_wt' in kwargs:
            self.mob_param.setdefault('min_wt', float(kwargs['min_wt']))
        else:
            self.mob_param.setdefault('min_wt', float(0))

        args = ['seed', 'stations', 'aps', 'DRAW', 'conn']
        for arg in args:
            self.mob_param.setdefault(arg, getattr(self, arg))

        self.mob_param.setdefault('model', kwargs['model'])
        self.mob_param.setdefault('time', kwargs['time'])
        self.mob_param.setdefault('ppm', propagationModel.model)
        return self.mob_param

    def setMobilityParams(self, **kwargs):
        "Set Mobility Parameters"
        self.mob_param.update(**kwargs)

        args = ['min_x', 'min_y', 'min_z',
                  'max_x', 'max_y', 'max_z',
                  'stations', 'cars', 'aps',
                  'DRAW', 'conn', 'repetitions']
        for arg in args:
            self.mob_param.setdefault(arg, getattr(self, arg))

        if self.nroads != 0:
            self.mob_param.setdefault('nroads', self.nroads)
        if 'plotNodes' in kwargs:
            self.mob_param.setdefault('plotNodes', kwargs['plotNodes'])

        if 'nodes' in kwargs and kwargs['nodes']:
            self.mob_param.setdefault('nodes', kwargs['nodes'])
        if 'ac_method' in kwargs:
            self.mob_param.setdefault('ac_method', kwargs['ac_method'])
        self.mob_param.setdefault('ppm', propagationModel.model)

    def useExternalProgram(self, program, **kwargs):
        """Opens an external program

        :params program: only sumo is supported so far
        :params **params config_file: file configuration"""
        self.autoAssociation = False
        self.isVanet = True
        for car in self.cars:
            car.params['position'] = (0, 0, 0)
        program(self.cars, self.aps, **kwargs)

    @staticmethod
    def configureMacAddr(node):
        """Configure Mac Address

        :param node: node"""
        for wlan in range(0, len(node.params['wlan'])):
            iface = node.params['wlan'][wlan]
            if node.params['mac'][wlan] == '':
                node.params['mac'][wlan] = node.getMAC(iface)
            else:
                mac = node.params['mac'][wlan]
                node.setMAC(mac, iface)

    def configureWmediumd(self):
        "Configure Wmediumd"
        if self.autoSetPositions:
            self.wmediumd_mode = interference
        self.wmediumd_mode()
        if self.wmediumd_mode == interference:
            mobility.wmediumd_mode = 3
        else:
            mobility.wmediumd_mode = 1
        if not self.configureWiFiDirect and not self.configure4addr and \
            self.wmediumd_mode != error_prob:
            wmediumd(self.fading_coefficient, self.noise_threshold,
                     self.stations, self.aps, self.cars, propagationModel,
                     self.wmediumdMac)

    def configureWifiNodes(self):
        "Configure WiFi Nodes"
        if not self.ppm_is_set:
            self.setPropagationModel()
        params = {}
        if self.ifb:
            wirelessLink.ifb = True
            params['ifb'] = self.ifb
        if self.docker:
            params['docker'] = self.docker
            params['container'] = self.container
            params['ssh_user'] = self.ssh_user
        nodes = self.stations + self.aps + self.cars
        module.start(nodes, self.n_radios, self.alt_module, **params)
        if sixlowpan.n_wpans != 0:
            sixLoWPAN_module.start(self.sixLP, sixlowpan.n_wpans)
        self.configureWirelessLink()
        self.createVirtualIfaces(self.stations)
        AccessPoint(self.aps, self.driver, self.link)

        if self.link == wmediumd:
            self.configureWmediumd()

        setParam = True
        if self.wmediumd_mode == interference and not self.isVanet:
            setParam = False

        for node in nodes:
            for wlan in range(0, len(node.params['wlan'])):
                if int(node.params['range'][wlan]) == 0:
                    intf = node.params['wlan'][wlan]
                    node.params['range'][wlan] = node.getRange(intf=intf)
                else:
                    if 'model' not in node.params:
                        node.params['txpower'][wlan] = \
                            node.get_txpower_prop_model(wlan)
                if not self.configure4addr and not self.configureWiFiDirect:
                    node.setTxPower(node.params['txpower'][wlan],
                                    intf=node.params['wlan'][wlan],
                                    setParam=setParam)
                    node.setAntennaGain(node.params['antennaGain'][wlan],
                                        intf=node.params['wlan'][wlan],
                                        setParam=setParam)

        return self.stations, self.aps

    def plotCheck(self, plotNodes):
        "Check which nodes will be plotted"
        nodes = self.stations + self.aps + plotNodes + self.cars
        self.checkDimension(nodes)

    def plot_dynamic(self):
        "Check which nodes will be plotted dynamically at runtime"
        nodes = self.stations + self.aps + self.cars
        self.checkDimension(nodes)

        while True:
            for node in nodes:
                intf = node.params['wlan'][0]
                node.params['range'][0] = node.getRange(intf=intf)
                if self.DRAW:
                    if not issubclass(self.plot, plot3d):
                        self.plot.updateCircleRadius(node)
                    self.plot.update(node)
            self.plot.pause()
            sleep(0.5)

    def auto_association(self):
        "This is useful to make the users' life easier"
        isap = []
        for sta in self.stations:
            for wlan in range(0, len(sta.params['wlan'])):
                if ((sta.func[wlan] == 'ap' or
                             sta.func[wlan] == 'client') and sta not in self.aps):
                    if sta not in isap:
                        isap.append(sta)

        for sta in isap:
            self.aps.append(sta)
            self.stations.remove(sta)

        nodes = self.stations + self.aps

        ap = []
        for node in self.aps:
            if 'link' in node.params:
                ap.append(node)

        nodes = self.stations + ap

        if self.nroads == 0:
            for node in nodes:
                if 'position' in node.params and 'link' not in node.params:
                    mobility.aps = self.aps
                    mobility.configLinks(node)
                    if self.link == wmediumd and \
                                    self.wmediumd_mode == interference:
                        if sta.func[wlan] == 'adhoc':
                            node.set_pos_wmediumd_sleep(wlan=wlan)
                        else:
                            node.set_pos_wmediumd(mob=False)

            for sta in self.stations:
                for wlan in range(0, len(sta.params['wlan'])):
                    for ap in self.aps:
                        if 'position' in sta.params and 'position' in ap.params:
                            dist = sta.get_distance_to(ap)
                            if dist <= ap.params['range'][0]:
                                sleep(0.1)
                                mobility.handover(sta, ap, wlan, ap_wlan=0)
                                if self.rec_rssi:
                                    os.system('hwsim_mgmt -k %s %s >/dev/null 2>&1'
                                              % (sta.phyID[wlan],
                                                 abs(int(sta.params['rssi'][wlan]))))

    @staticmethod
    def propagation_model(**kwargs):
        "Propagation Model Attr"
        propagationModel.setAttr(**kwargs)

    @staticmethod
    def stop_simulation():
        "Pause the simulation"
        mobility.pause_simulation = True

    @staticmethod
    def start_simulation():
        "Start the simulation"
        mobility.pause_simulation = False

    @staticmethod
    def printDistance(src, dst, nodes):
        """Prints the distance between two points

        :params src: source node
        :params dst: destination node
        :params nodes: list of nodes"""
        try:
            for host1 in nodes:
                if src == str(host1):
                    src = host1
                    for host2 in nodes:
                        if dst == str(host2):
                            dst = host2
                            dist = src.get_distance_to(dst)
                            info("The distance between %s and %s is %.2f "
                                 "meters\n" % (src, dst, float(dist)))
        except:
            info("node %s or/and node %s does not exist or there is no " \
                 "position defined\n" % (dst, src))

    @staticmethod
    def configureMobility(*args, **kwargs):
        "Configure mobility parameters"
        args[0].isStationary = False
        mobility.configure(*args, **kwargs)

    @staticmethod
    def getDataRate(sta=None, ap=None, **params):
        "Set the rate"
        value = GetRate(sta=sta, ap=ap, **params)
        return value

    @staticmethod
    def setChannelEquation(**params):
        """Set Channel Equation. The user may change the equation defined in
        wifiChannel.py by any other.

        :params bw: bandwidth (mbps)
        :params delay: delay (ms)
        :params latency: latency (ms)
        :params loss: loss (%)"""
        if 'bw' in params:
            wirelessLink.equationBw = params['bw']
        if 'delay' in params:
            wirelessLink.equationDelay = params['delay']
        if 'latency' in params:
            wirelessLink.equationLatency = params['latency']
        if 'loss' in params:
            wirelessLink.equationLoss = params['loss']

    @staticmethod
    def stopGraphParams():
        "Stop the graph"
        if mobility.thread_:
            mobility.thread_._keep_alive = False
        sleep(0.5)

    def closeMininetWiFi(self):
        "Close Mininet-WiFi"
        self.plot.closePlot()
        module.stop()  # Stopping WiFi Module


class MininetWithControlWNet(Mininet_wifi):
    """Control network support:
       Create an explicit control network. Currently this is only
       used/usable with the user datapath.
       Notes:
       1. If the controller and switches are in the same (e.g. root)
          namespace, they can just use the loopback connection.
       2. If we can get unix domain sockets to work, we can use them
          instead of an explicit control network.
       3. Instead of routing, we could bridge or use 'in-band' control.
       4. Even if we dispense with this in general, it could still be
          useful for people who wish to simulate a separate control
          network (since real networks may need one!)
       5. Basically nobody ever used this code, so it has been moved
          into its own class.
       6. Ultimately we may wish to extend this to allow us to create a
          control network which every node's control interface is
          attached to."""

    def configureControlNetwork(self):
        "Configure control network."
        self.configureRoutedControlNetwork()

    # We still need to figure out the right way to pass
    # in the control network location.

    def configureRoutedControlNetwork(self, ip='192.168.123.1',
                                      prefixLen=16):
        """Configure a routed control network on controller and switches.
           For use with the user datapath only right now."""
        controller = self.controllers[0]
        info(controller.name + ' <->')
        cip = ip
        snum = ipParse(ip)
        nodesL2 = self.switches + self.aps
        for nodeL2 in nodesL2:
            info(' ' + nodeL2.name)
            if self.link == wmediumd:
                link = Link(nodeL2, controller, port1=0)
            else:
                self.link = Link
                link = self.link(nodeL2, controller, port1=0)
            sintf, cintf = link.intf1, link.intf2
            nodeL2.controlIntf = sintf
            snum += 1
            while snum & 0xff in [0, 255]:
                snum += 1
            sip = ipStr(snum)
            cintf.setIP(cip, prefixLen)
            sintf.setIP(sip, prefixLen)
            controller.setHostRoute(sip, cintf)
            nodeL2.setHostRoute(cip, sintf)
        info('\n')
        info('*** Testing control network\n')
        while not cintf.isUp():
            info('*** Waiting for', cintf, 'to come up\n')
            sleep(1)
        for nodeL2 in nodesL2:
            while not sintf.isUp():
                info('*** Waiting for', sintf, 'to come up\n')
                sleep(1)
            if self.ping(hosts=[nodeL2, controller]) != 0:
                error('*** Error: control network test failed\n')
                exit(1)
        info('\n')
