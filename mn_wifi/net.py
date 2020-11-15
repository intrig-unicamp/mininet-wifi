"""
    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)"""

import os
import socket
import random
import re
import sys
from sys import version_info as py_version_info
from threading import Thread as thread
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
                          waitListening, BaseString)
from mininet.link import Link, Intf, TCLink, TCULink
from mininet.nodelib import NAT
from mininet.log import info, error, debug, output, warn

from mn_wifi.node import AccessPoint, AP, Station, Car, \
    OVSKernelAP, physicalAP
from mn_wifi.wmediumdConnector import error_prob, snr, interference
from mn_wifi.link import wirelessLink, wmediumd, Association, \
    _4address, TCWirelessLink, TCLinkWirelessStation, ITSLink, \
    wifiDirectLink, adhoc, mesh, master, managed, physicalMesh, \
    physicalWifiDirectLink, _4addrClient, _4addrAP
from mn_wifi.clean import Cleanup as cleanup_mnwifi
from mn_wifi.energy import Energy
from mn_wifi.telemetry import parseData, telemetry as run_telemetry
from mn_wifi.mobility import tracked as trackedMob, model as mobModel, mobility as mob
from mn_wifi.plot import plot2d, plot3d, plotGraph
from mn_wifi.module import module
from mn_wifi.propagationModels import propagationModel
from mn_wifi.vanet import vanet
from mn_wifi.sixLoWPAN.net import Mininet_IoT
from mn_wifi.sixLoWPAN.link import sixLoWPAN
from mn_wifi.util import ipAdd6, netParse6


sys.path.append(str(os.getcwd()) + '/mininet/')

VERSION = "2.4.2"


class Mininet_wifi(Mininet):

    def __init__(self, topo=None, switch=OVSKernelSwitch,
                 accessPoint=OVSKernelAP, host=Host, station=Station,
                 car=Car, controller=DefaultController,
                 link=TCWirelessLink, intf=Intf, build=True, xterms=False,
                 cleanup=False, ipBase='10.0.0.0/8', ip6Base='2001:0:0:0:0:0:0:0/64',
                 inNamespace=False, autoSetMacs=False, autoStaticArp=False,
                 autoPinCpus=False, listenPort=None, waitConnected=False,
                 ssid="new-ssid", mode="g", channel=1, wmediumd_mode=snr, roads=0,
                 fading_cof=0, autoAssociation=True, allAutoAssociation=True,
                 autoSetPositions=False, configWiFiDirect=False,
                 config4addr=False, noise_th=-91, cca_th=-90,
                 disable_tcp_checksum=False, ifb=False, bridge=False, plot=False,
                 plot3d=False, docker=False, container='mininet-wifi', ssh_user='alpha',
                 set_socket_ip=None, set_socket_port=12345, iot_module='mac802154_hwsim',
                 docker_host='172.17.0.1'):
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
        self.ip6Base = ip6Base
        self.ipBaseNum, self.prefixLen = netParse(self.ipBase)
        self.ip6BaseNum, self.prefixLen6 = netParse6(self.ip6Base)
        self.nextIP = 1  # start for address allocation
        self.nextIP6 = 1  # start for address allocation
        self.nextPos_sta = 1  # start for sta position allocation
        self.nextPos_ap = 1  # start for ap position allocation
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
        self.sensors = []
        self.terms = []  # list of spawned xterm processes
        self.autoAssociation = autoAssociation  # does not include mobility
        self.allAutoAssociation = allAutoAssociation  # includes mobility
        self.ppm_is_set = False
        self.draw = False
        self.isReplaying = False
        self.wmediumd_started = False
        self.mob_check = False
        self.isVanet = False
        self.alt_module = None
        self.set_socket_ip = set_socket_ip
        self.set_socket_port = set_socket_port
        self.docker = docker
        self.container = container
        self.ssh_user = ssh_user
        self.docker_host = docker_host
        self.ifb = ifb   # Support to Intermediate Functional Block (IFB) Devices
        self.bridge = bridge
        self.init_plot = plot
        self.init_plot3d = plot3d
        self.cca_th = cca_th
        self.configWiFiDirect = configWiFiDirect
        self.config4addr = config4addr
        self.fading_cof = fading_cof
        self.noise_th = noise_th
        self.mob_param = dict()
        self.disable_tcp_checksum = disable_tcp_checksum
        self.plot = plot2d
        self.roads = roads
        self.iot_module = iot_module
        self.seed = 1
        self.min_v = 1
        self.max_v = 10
        self.min_x = 0
        self.min_y = 0
        self.min_z = 0
        self.max_x = 100
        self.max_y = 100
        self.max_z = 0
        self.conn = {}
        self.wlinks = []
        Mininet_wifi.init()  # Initialize Mininet-WiFi if necessary

        if autoSetPositions and link == wmediumd:
            self.wmediumd_mode = interference

        if not allAutoAssociation:
            self.autoAssociation = False
            mob.allAutoAssociation = False

        self.built = False
        if topo and build:
            self.build()

    def start_socket_server(self):
        self.server()

    def server(self):
        thread(target=self.start_socket).start()

    def start_socket(self):
        host = self.set_socket_ip
        port, cleanup_mnwifi.socket_port = self.set_socket_port, self.set_socket_port

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((host, port))
        s.listen(1)

        while True:
            conn, addr = s.accept()
            try:
                thread(target=self.get_socket_data, args=(conn, addr)).start()
            except:
                print("Thread did not start.\n")

    def get_socket_data(self, conn, addr):
        while True:
            try:
                data = conn.recv(1024).decode('utf-8').split('.')
                if data[0] == 'set':
                    node = self.getNodeByName(data[1])
                    if len(data) < 4:
                        data = 'usage: set.node.method.value'
                    else:
                        if hasattr(node, data[2]):
                            method_to_call = getattr(node, data[2])
                            method_to_call(data[3])
                            data = 'command accepted!'
                        else:
                            data = 'unrecognized method!'
                elif data[0] == 'get':
                    node = self.getNodeByName(data[1])
                    if len(data) < 3:
                        data = 'usage: get.node.param'
                    else:
                        data = getattr(node, data[2])
                else:
                    try:
                        cmd = ''
                        for d in range(1, len(data)):
                            cmd = cmd + data[d] + ' '
                        node = self.getNodeByName(data[0])
                        node.pexec(cmd)
                        data = 'command accepted!'
                    except:
                        data = 'unrecognized option %s:' % data[0]
                conn.send(str(data).encode('utf-8'))
                break
            except:
                conn.close()

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

    def pos_to_array(self, node):
        pos = node.params['position']
        if isinstance(pos, string_types):
            pos = pos.split(',')
        node.position = [float(pos[0]), float(pos[1]), float(pos[2])]
        node.params.pop('position', None)

    def count_ifaces(self):
        "Count the number of virtual wifi interfaces"
        nodes = self.stations + self.cars + self.aps
        nradios = 0
        for node in nodes:
            nradios += len(node.params['wlan'])
        return nodes, nradios

    def addWlans(self, node):
        node.params['wlan'] = []
        wlans = 1
        if 'wlans' in node.params:
            wlans = node.params['wlans']
        for wlan in range(wlans):
            if isinstance(node, AP):
                node.params['wlan'].append(node.name + '-wlan' + str(wlan + 1))
            else:
                node.params['wlan'].append(node.name + '-wlan' + str(wlan))
        node.params.pop("wlans", None)

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
                    'ip6': ipAdd6(self.nextIP6,
                                  ipBaseNum=self.ip6BaseNum,
                                  prefixLen=self.prefixLen6) +
                           '/%s' % self.prefixLen6,
                    'channel': self.channel,
                    'mode': self.mode
                   }
        defaults.update(params)

        if self.autoSetPositions:
            defaults['position'] = [round(self.nextPos_sta, 2), 0, 0]
        if self.autoSetMacs:
            defaults['mac'] = macColonHex(self.nextIP)
        if self.autoPinCpus:
            defaults['cores'] = self.nextCore
            self.nextCore = (self.nextCore + 1) % self.numCores
        self.nextIP += 1
        self.nextIP6 += 1
        self.nextPos_sta += 2

        if not cls:
            cls = self.station
        sta = cls(name, **defaults)

        if 'position' in params:
            self.pos_to_array(sta)

        self.addWlans(sta)
        self.stations.append(sta)
        self.nameToNode[name] = sta
        return sta

    def addSensor(self, name, cls=None, **params):
        node = Mininet_IoT.addSensor(name, cls, **params)
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
                    'ip6': ipAdd6(self.nextIP,
                                  ipBaseNum=self.ipBaseNum,
                                  prefixLen=self.prefixLen) +
                           '/%s' % self.prefixLen,
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

        if 'position' in params:
            self.pos_to_array(car)

        self.addWlans(car)
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
            defaults['position'] = [round(self.nextPos_ap, 2), 50, 0]
            self.nextPos_ap += 100
        wlan = None
        if cls and isinstance(cls, physicalAP):
            wlan = ("%s" % params.pop('phywlan', {}))
            cls = self.accessPoint
        if not cls:
            cls = self.accessPoint
        ap = cls(name, **defaults)
        if not self.inNamespace and self.listenPort:
            self.listenPort += 1
        self.nameToNode[name] = ap
        if wlan:
            ap.params['phywlan'] = wlan
        if 'position' in params:
            self.pos_to_array(ap)
        self.addWlans(ap)
        self.aps.append(ap)
        return ap

    def setStaticRoute(self, node, ip=None, **params):
        """Set the static route to go through intf.
           net: subnet address"""
        # Note setParam won't call us if intf is none
        if isinstance(ip, BaseString) and ' ' in ip:
            params = ip
        else:
            natIP = ip.split('/')[0]
            params = '%s via %s' % (params['net'], natIP)
        # Do this in one line in case we're messing with the root namespace
        node.cmd('ip route add', params)

    def addNAT(self, name='nat0', connect=True, inNamespace=False,
               linkTo=None, **params):
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
                if linkTo:
                    nodes = self.switches + self.aps
                    for node in nodes:
                        if linkTo == node.name:
                            connect = node
                else:
                    if self.switches:
                        connect = self.switches[0]
            # Connect the nat to the ap
            self.addLink(nat, connect)
            # Set the default route on stations
            natIP = nat.params['ip'].split('/')[0]
            nodes = self.stations + self.hosts + self.cars
            if 'net' in params:
                for node in nodes:
                    if node.inNamespace:
                        self.setStaticRoute(node, '%s via %s' % (params['net'], natIP))
            else:
                for node in nodes:
                    if node.inNamespace:
                        node.setDefaultRoute('via %s' % natIP)
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
                          self.stations, self.cars, self.aps, self.sensors):
            yield node.name

    def __len__(self):
        "returns number of nodes in net"
        return (len(self.hosts) + len(self.switches) +
                len(self.controllers) + len(self.stations) +
                len(self.cars) + len(self.aps) + len(self.sensors))

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

    def infra_wmediumd_link(self, node1, node2, **params):
        ap = node1 if node1 in self.aps else node2
        sta = node1 if node2 in self.aps else node2

        if self.wmediumd_mode == error_prob:
            wmediumd.wlinks.append([sta, ap, params['error_prob']])
        elif self.wmediumd_mode != interference:
            wmediumd.wlinks.append([sta, ap])

    def infra_tc(self, node1, node2, port1=None, port2=None,
                 cls=None, **params):

        ap = node1 if node1 in self.aps else node2
        sta = node1 if node2 in self.aps else node2

        wlan = 0
        ap_wlan = 0
        if port1 and port2:
            ap_wlan = port1 if node1 in self.aps else port2
            wlan = port1 if node2 in self.aps else port2

        params['ap_wlan'] = ap_wlan
        params['wlan'] = wlan

        intf = sta.wintfs[wlan]
        ap_intf = ap.wintfs[ap_wlan]

        # If sta/ap have position
        doAssociation = True
        if hasattr(sta, 'position') and hasattr(ap, 'position'):
            dist = sta.get_distance_to(ap)
            if dist > ap.wintfs[ap_wlan].range:
                doAssociation = False
        if doAssociation:
            Association.associate(intf, ap_intf)

            if 'bw' not in params and 'bw' not in str(cls):
                if hasattr(sta, 'position'):
                    params['bw'] = intf.getCustomRate()
                else:
                    params['bw'] = intf.getRate()
            # tc = True, this is useful for tc configuration
            link = cls(name=sta.params['wlan'][wlan], node=sta,
                       **params)
            # self.links.append(link)
            return link

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

        modes = [mesh, physicalMesh, adhoc, ITSLink,
                 wifiDirectLink, physicalWifiDirectLink]
        if cls in modes:
            cls(node=node1, **params)
        elif cls == sixLoWPAN:
            link = cls(node=node1, port=port1, **params)
            self.links.append(link)
            return link
        elif cls == _4address:
            if hasattr(node1, 'position') and hasattr(node2, 'position'):
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
                if dist <= node1.wintfs[0].range:
                    link = cls(node1, node2)
                    self.links.append(link)
                    return link
        elif ((node1 in self.stations and node2 in self.aps)
              or (node2 in self.stations and node1 in self.aps)) and cls != TCLink:
            if cls == wmediumd:
                self.infra_wmediumd_link(node1, node2, **params)
            else:
                self.infra_tc(node1, node2, port1, port2, cls, **params)
        else:
            if hasattr(node1, 'position') and hasattr(node2, 'position'):
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
        nodes = self.hosts + self.stations + self.cars + self.sensors
        for node in nodes:
            # info( host.name + ' ' )
            intf = node.defaultIntf()
            if intf:
                node.configDefault()
            else:
                # Don't configure nonexistent intf
                node.configDefault(ip=None, mac=None)
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

        if topo.hosts():
            info('*** Adding hosts:\n')
            for hostName in topo.hosts():
                self.addHost(hostName, **topo.nodeInfo(hostName))
                info(hostName + ' ')

        info('\n*** Adding access points:\n')
        for apName in topo.aps():
            # A bit ugly: add batch parameter if appropriate
            params = topo.nodeInfo(apName)
            cls = params.get('cls', self.accessPoint)
            if hasattr(cls, 'batchStartup'):
                params.setdefault('batch', True)
            self.addAccessPoint(apName, **params)
            info(apName + ' ')

        if topo.switches():
            info('\n*** Adding switches:\n')
            for switchName in topo.switches():
                # A bit ugly: add batch parameter if appropriate
                params = topo.nodeInfo(switchName)
                cls = params.get('cls', self.switch)
                if hasattr(cls, 'batchStartup'):
                    params.setdefault('batch', True)
                self.addSwitch(switchName, **params)
                info(switchName + ' ')

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

    def check_if_mob(self):
        if self.mob_param:
            self.get_mob_stat_nodes()
            self.mob_param['mnNodes'] = self.plot_mininet_nodes()
            if 'model' in self.mob_param or self.isVanet or self.roads:
                self.start_mobility(**self.mob_param)
            else:
                trackedMob(**self.mob_param)
            self.mob_check = True
        else:
            if self.draw and not self.isReplaying:
                mnNodes = self.plot_mininet_nodes()
                self.plotCheck(mnNodes)

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
            if not mob.stations:
                for node in self.stations:
                    if hasattr(node, 'position'):
                        mob.stations.append(node)

        if not self.wmediumd_started:
            self.init_wmediumd()

        if self.inNamespace:
            self.configureControlNetwork()

        info('*** Configuring nodes\n')
        self.configHosts()
        if self.xterms:
            self.startTerms()
        if self.autoStaticArp:
            self.staticArp()

        for node in self.stations:
            for wlan, intf in enumerate(node.wintfs.values()):
                if not isinstance(intf, master) and not isinstance(intf, adhoc) \
                        and not isinstance(intf, mesh) \
                        and not isinstance(intf, wifiDirectLink):
                    if isinstance(node, Station) and not hasattr(node, 'range'):
                        intf.range = int(intf.range)

        if self.allAutoAssociation:
            if self.autoAssociation and not self.configWiFiDirect:
                self.auto_association()

        if not self.mob_check:
            self.check_if_mob()

        nodes = self.stations + self.aps + self.cars
        battery_nodes = []
        for node in nodes:
            if 'battery' in node.params:
                battery_nodes.append(node)
        if battery_nodes:
            Energy(battery_nodes)

        self.built = True

    def plot_mininet_nodes(self):
        from mn_wifi.node import Node_wifi
        nodes = self.hosts + self.switches + self.controllers
        mnNodes = []
        for node in nodes:
            if hasattr(node, 'position'):
                self.pos_to_array(node)
                node.wintfs = {0 : Node_wifi}
                node.wintfs[0].range = 0
                mnNodes.append(node)
        return mnNodes

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
        self.terms += makeTerms(self.sensors, 'sensor')

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

    def telemetry(self, **kwargs):
        run_telemetry(**kwargs)

    def start(self):
        "Start controller and switches."
        if not self.built:
            self.build()

        if not self.mob_check:
            self.check_if_mob()

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
        nodes = self.hosts + self.stations + self.sensors
        for node in nodes:
            info(node.name + ' ')
            node.terminate()
        info('\n')
        self.closeMininetWiFi()
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
            hosts = self.hosts + self.stations
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
                                           % client.wintfs[0].name))
            if isinstance(server, Station):
                while conn2 is 0:
                    conn2 = int(server.cmd('iw dev %s link | grep -ic '
                                           '\'Connected\''
                                           % server.wintfs[0].name))
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
                            info("The distance between %s and %s is %s "
                                 "meters\n" % (src, dst, dist))
        except:
            info("node %s or/and node %s does not exist or there is no " \
                 "position defined" % (dst, src))

    def mobility(self, *args, **kwargs):
        "Configure mobility parameters"
        mob.configure(*args, **kwargs)

    def get_mob_stat_nodes(self):
        mob_nodes = []
        stat_nodes = []
        nodes = self.stations + self.aps + self.cars
        for node in nodes:
            if hasattr(node, 'position') and 'initPos' not in node.params:
                stat_nodes.append(node)
            else:
                mob_nodes.append(node)
        self.mob_param['stat_nodes'] = stat_nodes
        self.mob_param['mob_nodes'] = mob_nodes

    def setPropagationModel(self, **kwargs):
        "Set Propagation Model"
        self.ppm_is_set = True
        kwargs['noise_th'] = self.noise_th
        kwargs['cca_th'] = self.cca_th
        self.propagation_model(**kwargs)

    def configWirelessLinkStatus(self, src, dst, status):

        sta = self.nameToNode[dst]
        ap = self.nameToNode[src]
        if isinstance(self.nameToNode[src], Station):
            sta = self.nameToNode[src]
            ap = self.nameToNode[dst]

        if status == 'down':
            for wlan, intf in enumerate(sta.wintfs.values()):
                if intf.associatedTo:
                    sta.cmd('iw dev %s disconnect' % intf.name)
                    intf.associatedTo = ''
                    ap.wintfs[0].associatedStations.remove(sta)
        else:
            for wlan, intf in enumerate(sta.wintfs.values()):
                if not intf.associatedTo:
                    sta.pexec('iw dev %s connect %s %s'
                              % (intf.name, ap.wintfs[0].ssid, ap.wintfs[0].mac))
                    intf.associatedTo = ap
                    ap.wintfs[0].associatedStations.append(sta)

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

    def createVirtualIfaces(self, nodes):
        "Creates virtual wifi interfaces"
        for node in nodes:
            if 'nvif' in node.params:
                for vif_ in range(0, node.params['nvif']):
                    vif = node.params['wlan'][0] + str(vif_ + 1)
                    node.params['wlan'].append(vif)
                    mac = str(node.wintfs[0].mac)
                    new_mac = '{}{}{}'.format(mac[:4], vif_+1, mac[5:])

                    node.cmd('iw dev %s interface add %s type station'
                             % (node.params['wlan'][0], vif))
                    TCLinkWirelessStation(node, intfName=vif)
                    managed(node, wlan=vif_+1)

                    node.wintfs[vif_+1].mac = new_mac
                    for intf in node.wintfs.values():
                        intf.configureMacAddr()

    def configure6LowPANLink(self):
        for sensor in self.sensors:
            for wpan in range(0, len(sensor.params['wpan'])):
                sixLoWPAN(sensor, wpan)
                #managed(node, wlan)

    def configIFB(self):
        nodes = self.stations + self.cars
        ifbID = 0
        for node in nodes:
            for wlan in range(0, len(node.params['wlan'])):
                if self.ifb:
                    node.configIFB(wlan, ifbID)  # Adding Support to IFB
                    node.wintfs[wlan].ifb = 'ifb' + str(wlan + 1)
                    ifbID += 1

    def configWirelessLink(self):
        "Configure Wireless Link"
        nodes = self.stations + self.cars
        for node in nodes:
            for wlan in range(0, len(node.params['wlan'])):
                intf = node.params['wlan'][wlan]
                link = TCLinkWirelessStation(node, intfName1=intf)
                managed(node, wlan)
                self.links.append(link)
            for intf in node.wintfs.values():
                intf.configureMacAddr()
        self.configIFB()

    def plotGraph(self, **kwargs):
        "Plots Graph"
        self.draw = True
        for key in kwargs:
            setattr(self, key, kwargs[key])
        if 'max_z' in kwargs and kwargs['max_z'] != 0:
            self.plot = plot3d
        cleanup_mnwifi.plot = self.plot

    def checkDimension(self, nodes):
        try:
            for node in nodes:
                if hasattr(node, 'coord'):
                    node.position = node.coord[0].split(',')
            plotGraph(min_x=self.min_x, min_y=self.min_y, min_z=self.min_z,
                      max_x=self.max_x, max_y=self.max_y, max_z=self.max_z,
                      nodes=nodes, conn=self.conn)
            if not issubclass(self.plot, plot3d):
                self.plot.pause()
        except:
            info('Something went wrong with the GUI.\n')
            self.draw = False

    def start_mobility(self, mob_nodes, **kwargs):
        "Starts Mobility"
        for node in mob_nodes:
            node.position = (0, 0, 0)
            node.pos = (0, 0, 0)
        self.setMobilityParams(**kwargs)
        if self.roads:
            vanet(**self.mob_param)
        else:
            mobModel(**self.mob_param)

    def setMobilityModel(self, **kwargs):
        self.setMobilityParams(**kwargs)

    def startMobility(self, **kwargs):
        if 'repetitions' not in kwargs:
            kwargs['repetitions'] = 1
        kwargs['init_time'] = kwargs['time']
        self.setMobilityParams(**kwargs)

    def stopMobility(self, **kwargs):
        "Stops Mobility"
        if self.allAutoAssociation and \
                not self.configWiFiDirect and not self.config4addr:
            self.auto_association()
        kwargs['end_time'] = kwargs['time']
        self.setMobilityParams(**kwargs)

    def setMobilityParams(self, **kwargs):
        "Set Mobility Parameters"
        self.mob_param.update(**kwargs)

        float_args = ['min_x', 'min_y', 'min_z',
                      'max_x', 'max_y', 'max_z',
                      'min_v', 'max_v', 'min_wt', 'max_wt']
        args = ['stations', 'cars', 'aps', 'draw', 'conn', 'seed']
        args += float_args
        for arg in args:
            if arg != 'min_wt' and arg != 'max_wt':
                self.mob_param.setdefault(arg, getattr(self, arg))
            if arg in kwargs and arg in float_args:
                if arg != 'min_wt' and arg != 'max_wt':
                    setattr(self, arg, float(kwargs[arg]))

        args = ['mnNodes', 'ac_method', 'model', 'time']
        for arg in args:
            if arg in kwargs:
                self.mob_param.setdefault(arg, kwargs[arg])

        if self.roads:
            self.mob_param.setdefault('roads', self.roads)

        self.mob_param.setdefault('ppm', propagationModel.model)

    def setAssociationCtrl(self, ac='ssf'):
        "set association control"
        mob.ac = ac  # backwards compatibility

    def useExternalProgram(self, program, **kwargs):
        """Opens an external program

        :params program: only sumo is supported so far
        :params **params config_file: file configuration"""
        self.isVanet = True
        for car in self.cars:
            if hasattr(car, 'position'):
                car.pos = car.position
            else:
                car.position = (0, 0, 0)
                car.pos = (0, 0, 0)
        program(self.cars, self.aps, **kwargs)

    def configWmediumd(self):
        "Configure Wmediumd"
        if self.autoSetPositions:
            self.wmediumd_mode = interference
        self.wmediumd_mode()

        if not self.configWiFiDirect and not self.config4addr and \
            self.wmediumd_mode != error_prob:
            wmediumd(self.fading_cof, self.noise_th,
                     self.stations, self.aps, self.cars, propagationModel,
                     self.wmediumdMac)

    def init_wmediumd(self):
        if (self.config4addr or self.configWiFiDirect
                or self.wmediumd_mode == error_prob) and self.link == wmediumd:
            wmediumd(self.fading_cof, self.noise_th,
                     self.stations, self.aps, self.cars, propagationModel,
                     self.wmediumdMac)
            for sta in self.stations:
                if self.wmediumd_mode != error_prob:
                    sta.set_pos_wmediumd(sta.position)
            for sta in self.stations:
                if sta in self.aps:
                    self.stations.remove(sta)
        self.wmediumd_started = True

    def addSensors(self, sensors):
        for sensor in sensors:
            self.nameToNode[sensor.name] = sensor

    def configureWifiNodes(self):
        "Configure WiFi Nodes"
        if not self.ppm_is_set:
            self.setPropagationModel()
        params = {}
        if self.docker:
            params['docker'] = self.docker
            params['container'] = self.container
            params['ssh_user'] = self.ssh_user
            params['ip'] = self.docker_host

        nodes, nradios = self.count_ifaces()
        if nodes:
            module(nodes, nradios, self.alt_module, **params)

        if self.ifb:
            module.load_ifb(nradios)

        if Mininet_IoT.nwpans:
            self.sensors = Mininet_IoT.init_module(iot_module=self.iot_module)
            self.addSensors(self.sensors)
            self.configure6LowPANLink()

        self.configWirelessLink()
        self.createVirtualIfaces(self.stations)
        AccessPoint(self.aps, check_nm=True)
        if self.link == wmediumd:
            self.configWmediumd()
        AccessPoint(self.aps)

        for node in nodes:
            for wlan, intf in enumerate(node.wintfs.values()):
                if int(intf.range) == 0:
                    intf.range = node.getRange(intf)
                else:
                    if 'model' not in node.params:
                        intf.txpower = node.get_txpower_prop_model(intf)
                if not self.config4addr and not self.configWiFiDirect:
                    node.setTxPower(intf.txpower, intf=intf.name)
                    node.setAntennaGain(intf.antennaGain, intf=intf.name)

    def plotCheck(self, mnNodes):
        "Check which nodes will be plotted"
        nodes = self.stations + self.aps + mnNodes + self.cars
        self.checkDimension(nodes)

    def plot_dynamic(self):
        "Check which nodes will be plotted dynamically at runtime"
        nodes = self.stations + self.aps + self.cars
        self.checkDimension(nodes)

        while True:
            for node in nodes:
                node.wintfs[0].range = node.getRange(node.wintfs[0])
                if self.draw:
                    if not issubclass(self.plot, plot3d):
                        self.plot.updateCircleRadius(node)
                    self.plot.update(node)
            self.plot.pause()
            sleep(0.5)

    def auto_association(self):
        "This is useful to make the users' life easier"
        isap = []
        for node in self.stations:
            for intf in node.wintfs.values():
                if isinstance(intf, master) or \
                        isinstance(intf, _4addrClient or isinstance(intf, _4addrAP)):
                    if node not in self.aps and node not in isap:
                        isap.append(node)

        for sta in isap:
            self.aps.append(sta)
            self.stations.remove(sta)
            if sta in mob.stations:
                mob.stations.remove(sta)

        mob.aps = self.aps
        nodes = self.aps + self.stations + self.cars
        for node in nodes:
            if hasattr(node, 'position'):
                if isinstance(node, Car) and node.position == (0, 0, 0):
                    pass
                else:
                    for intf in node.wintfs.values():
                        pos = node.position
                        if isinstance(intf, adhoc):
                            info('%s ' % node)
                            sleep(1)
                    if not isinstance(node, AP):
                        node.pos = (0, 0, 0)
                        mob.configLinks(node)
                    # we need this cause wmediumd is struggling
                    # with some associations e.g. wpa
                    if self.wmediumd_mode == interference:
                        sleep(0.1)
                        pos_x = float(pos[0]) + 1
                        pos = (pos_x, pos[1], pos[2])
                        node.set_pos_wmediumd(pos)
                        sleep(0.1)
                        pos_x = float(pos[0]) - 1
                        pos = (pos_x, pos[1], pos[2])
                        node.set_pos_wmediumd(pos)

    @staticmethod
    def propagation_model(**kwargs):
        "Propagation Model Attr"
        propagationModel.setAttr(**kwargs)

    @staticmethod
    def stop_simulation():
        "Pause the simulation"
        mob.pause_simulation = True

    @staticmethod
    def start_simulation():
        "Start the simulation"
        mob.pause_simulation = False

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
        if parseData.thread_:
            parseData.thread_._keep_alive = False
        if mob.thread_:
            mob.thread_._keep_alive = False
        sleep(0.5)

    @classmethod
    def closeMininetWiFi(self):
        "Close Mininet-WiFi"
        cleanup_mnwifi.kill_mod_proc()


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
