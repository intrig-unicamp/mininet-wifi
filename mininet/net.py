"""

    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDN!

author: Bob Lantz (rlantz@cs.stanford.edu)
author: Brandon Heller (brandonh@stanford.edu)

Modified by Ramon Fontes (ramonrf@dca.fee.unicamp.br)

Mininet creates scalable OpenFlow test networks by using
process-based virtualization and network namespaces.

Simulated hosts/stations are created as processes in separate network
namespaces. This allows a complete OpenFlow network to be simulated on
top of a single Linux kernel.

Each host has:

A virtual console (pipes to a shell)
A virtual interface (half of a veth pair)
A parent shell (and possibly some child processes) in a namespace

Hosts have a network interface which is configured via ifconfig/ip
link/etc.

Each station has:

A virtual console (pipes to a shell)
A virtual wireless interface (half of a veth pair)
A parent shell (and possibly some child processes) in a namespace

This version supports both the kernel and user space datapaths
from the OpenFlow reference implementation (openflowswitch.org)
as well as OpenVSwitch (openvswitch.org.)

In kernel datapath mode, the controller and switches are simply
processes in the root namespace.

Kernel OpenFlow datapaths are instantiated using dpctl(8), and are
attached to the one side of a veth pair; the other side resides in the
host namespace. In this mode, switch processes can simply connect to the
controller via the loopback interface.

In user datapath mode, the controller and switches can be full-service
nodes that live in their own network namespaces and have management
interfaces and IP addresses on a control network (e.g. 192.168.123.1,
currently routed although it could be bridged.)

In addition to a management interface, user mode switches also have
several switch interfaces, halves of veth pairs whose other halves
reside in the host nodes that the switches are connected to.

Consistent, straightforward naming is important in addLinkorder to easily
identify hosts, switches and controllers, both from the CLI and
from program code. Interfaces are named to make it easy to identify
which interfaces belong to which node.

The basic naming scheme is as follows:

    Host nodes are named h1-hN
    Switch nodes are named s1-sN
    Controller nodes are named c0-cN
    Interfaces are named {nodename}-eth0 .. {nodename}-ethN

Note: If the network topology is created using mininet.topo, then
node numbers are unique among hosts and switches (e.g. we have
h1..hN and SN..SN+M) and also correspond to their default IP addresses
of 10.x.y.z/8 where x.y.z is the base-256 representation of N for
hN. This mapping allows easy determination of a node's IP
address from its name, e.g. h1 -> 10.0.0.1, h257 -> 10.0.1.1.

Note also that 10.0.0.1 can often be written as 10.1 for short, e.g.
"ping 10.1" is equivalent to "ping 10.0.0.1".

Currently we wrap the entire network in a 'mininet' object, which
constructs a simulated network based on a network topology created
using a topology object (e.g. LinearTopo) from mininet.topo or
mininet.topolib, and a Controller which the switches will connect
to. Several configuration options are provided for functions such as
automatically setting MAC addresses, populating the ARP table, or
even running a set of terminals to allow direct interaction with nodes.

After the network is created, it can be started using start(), and a
variety of useful tasks maybe performed, including basic connectivity
and bandwidth tests and running the mininet CLI.

Once the network is up and running, test code can easily get access
to host and switch objects which can then be used for arbitrary
experiments, typically involving running a series of commands on the
hosts.

After all desired tests or activities have been completed, the stop()
method may be called to shut down the network.

"""
import os
import re
import select
import signal
import random

from time import sleep
from itertools import chain, groupby
from math import ceil

from mininet.cli import CLI
from mininet.log import info, error, debug, output, warn
from mininet.node import (Node, Host, Station, Car, OVSKernelSwitch, OVSKernelAP,
                          DefaultController, Controller)
from mininet.nodelib import NAT
from mininet.link import Link, Intf, Association, WDSLink, TCIntfWireless
from mininet.util import (quietRun, fixLimits, numCores, ensureRoot,
                           macColonHex, ipStr, ipParse, netParse, ipAdd,
                           waitListening)
from mininet.term import cleanUpScreens, makeTerms
from mininet.wifiNet import mininetWiFi

from __builtin__ import True

# Mininet version: should be consistent with README and LICENSE
VERSION = "2.0r4"

class Mininet(object):
    "Network emulation with hosts spawned in network namespaces."

    def __init__(self, topo=None, switch=OVSKernelSwitch, accessPoint=OVSKernelAP, host=Host, station=Station,
                  car=Car, controller=DefaultController, isWiFi=False, link=Link, intf=Intf,
                  build=True, xterms=False, cleanup=False, ipBase='10.0.0.0/8',
                  inNamespace=False, autoSetMacs=False, autoStaticArp=False, autoPinCpus=False,
                  listenPort=None, waitConnected=False, ssid="new-ssid", mode="g", channel="6", rec_rssi=False,
                  useWmediumd=False, enable_interference=False, disableAutoAssociation=False):
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
        self.inNamespace = inNamespace
        self.xterms = xterms
        self.cleanup = cleanup
        self.autoSetMacs = autoSetMacs
        self.autoStaticArp = autoStaticArp
        self.autoPinCpus = autoPinCpus
        self.numCores = numCores()
        self.nextCore = 0  # next core for pinning hosts to CPUs
        self.listenPort = listenPort
        self.waitConn = waitConnected
        self.routing = ''
        self.ssid = ssid
        self.mode = mode
        self.channel = channel
        self.nameToNode = {}  # name to Node (Host/Switch) objects
        self.accessPoints = []
        self.controllers = []
        self.hosts = []
        self.links = []
        self.cars = []
        self.switches = []
        self.stations = []
        self.vehicles = []
        self.vehiclesSTA = []
        self.walls = []
        self.terms = []  # list of spawned xterm processes
        self.useWmediumd = useWmediumd
        self.disableAutoAssociation = disableAutoAssociation

        mininetWiFi.isWiFi = isWiFi
        mininetWiFi.enable_interference = enable_interference
        mininetWiFi.rec_rssi = rec_rssi
        Mininet.init()  # Initialize Mininet if necessary

        self.built = False
        if topo and build:
            self.build()

    def waitConnected(self, timeout=None, delay=.5):
        """wait for each switch to connect to a controller,
           up to 5 seconds
           timeout: time to wait, or None to wait indefinitely
           delay: seconds to sleep per iteration
           returns: True if all switches are connected"""
        info('*** Waiting for switches to connect\n')
        time = 0
        remaining = list(self.switches)
        while True:
            for switch in tuple(remaining):
                if switch.connected():
                    info('%s ' % switch)
                    remaining.remove(switch)
            if not remaining:
                info('\n')
                return True
            if time > timeout and timeout is not None:
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

    def addHost(self, name, cls=None, **params):
        """Add host.
           name: name of host to add
           cls: custom host class/constructor (optional)
           params: parameters for host
           returns: added host"""
        # Default IP and MAC addresses
        defaults = { 'ip': ipAdd(self.nextIP,
                                  ipBaseNum=self.ipBaseNum,
                                  prefixLen=self.prefixLen) +
                                  '/%s' % self.prefixLen }
        if self.autoSetMacs:
            defaults[ 'mac' ] = macColonHex(self.nextIP)
        if self.autoPinCpus:
            defaults[ 'cores' ] = self.nextCore
            self.nextCore = (self.nextCore + 1) % self.numCores
        self.nextIP += 1
        defaults.update(params)
        if not cls:
            cls = self.host
        h = cls(name, **defaults)
        h.type = 'host'
        self.hosts.append(h)
        self.nameToNode[ name ] = h
        return h

    def delNode(self, node, nodes=None):
        """Delete node
           node: node to delete
           nodes: optional list to delete from (e.g. self.hosts)"""
        if nodes is None:
            nodes = (self.hosts if node in self.hosts else
                      (self.switches if node in self.switches else
                        (self.controllers if node in self.controllers else
                          [])))
        node.stop(deleteIntfs=True)
        node.terminate()
        nodes.remove(node)
        del self.nameToNode[ node.name ]

    def delHost(self, host):
        "Delete a host"
        self.delNode(host, nodes=self.hosts)

    def addStation(self, name, cls=None, **params):
        """Add Station.
           name: name of station to add
           cls: custom host class/constructor (optional)
           params: parameters for station
           returns: added station"""
        # Default IP and MAC addresses
        defaults = { 'ip': ipAdd(self.nextIP,
                                  ipBaseNum=self.ipBaseNum,
                                  prefixLen=self.prefixLen) +
                                  '/%s' % self.prefixLen,
                     'channel': self.channel,
                     'mode': self.mode
                                  }
        defaults.update(params)

        if self.autoSetMacs:
            defaults[ 'mac' ] = macColonHex(self.nextIP)
        if self.autoPinCpus:
            defaults[ 'cores' ] = self.nextCore
            self.nextCore = (self.nextCore + 1) % self.numCores
        self.nextIP += 1

        if not cls:
            cls = self.station
        sta = cls(name, **defaults)
        sta.type = 'station'

        self.hosts.append(sta)
        self.stations.append(sta)
        self.nameToNode[ name ] = sta

        mininetWiFi.addParameters(sta, self.autoSetMacs, defaults)
        return sta

    def addAPAdhoc(self, name, cls=None, **params):
        """Add Station with AP capabilities.
           name: name of station ap to add
           cls: custom host class/constructor (optional)
           params: parameters for station
           returns: added station"""
        # Default IP and MAC addresses
        defaults = { 'ip': ipAdd(self.nextIP,
                                  ipBaseNum=self.ipBaseNum,
                                  prefixLen=self.prefixLen) +
                                  '/%s' % self.prefixLen,
                     'channel': self.channel,
                     'mode': self.mode}

        if self.autoSetMacs:
            defaults[ 'mac' ] = macColonHex(self.nextIP)
        if self.autoPinCpus:
            defaults[ 'cores' ] = self.nextCore
            self.nextCore = (self.nextCore + 1) % self.numCores
        self.nextIP += 1
        defaults.update(params)
        
        if not cls:
            cls = self.station
        sta = cls(name, **defaults)
        sta.type = 'station'

        if 'ssid' in params:
            sta.params['ssid'] = []
            sta.params['ssid'].append('')
            sta.params['ssid'][0] = params['ssid']

        self.hosts.append(sta)
        self.stations.append(sta)
        self.nameToNode[ name ] = sta

        mininetWiFi.addParameters(sta, self.autoSetMacs, defaults)
        sta.func[0] = 'ap'
        return sta

    def addCar(self, name, cls=None, **params):
        """Add Car.
           name: name of vehicle to add
           cls: custom host class/constructor
           params: parameters for car
           returns: added car"""
        # Default IP and MAC addresses
        defaults = { 'ip': ipAdd(self.nextIP,
                                  ipBaseNum=self.ipBaseNum,
                                  prefixLen=self.prefixLen) +
                                  '/%s' % self.prefixLen,
                     'channel': self.channel,
                     'mode': self.mode,
                     'ssid': self.ssid}

        if self.autoSetMacs:
            defaults[ 'mac' ] = macColonHex(self.nextIP)
        if self.autoPinCpus:
            defaults[ 'cores' ] = self.nextCore
            self.nextCore = (self.nextCore + 1) % self.numCores
        defaults.update(params)

        self.nextIP += 1
        if not cls:
            cls = self.car
        car = cls(name, **defaults)
        self.hosts.append(car)

        self.nameToNode[ name ] = car
        car.type = 'vehicle'
        mininetWiFi.addParameters(car, self.autoSetMacs, defaults)

        carsta = self.addStation(name + 'STA')
        carsta.params['range'] = car.params['range']

        car.params['carsta'] = carsta
        self.vehiclesSTA.append(carsta)
        switchName = car.name + 'SW'
        carsw = self.addSwitch(switchName, inband=True)
        self.vehicles.append(carsw)
        self.cars.append(car)

        car.func.append('mesh')
        self.addLink(carsta, carsw)
        self.addLink(car, carsw)
        return car

    def addAccessPoint(self, name, cls=None, **params):
        """Add AccessPoint.
           name: name of accesspoint to add
           cls: custom switch class/constructor (optional)
           returns: added acesspoint
           side effect: increments listenPort ivar ."""
        defaults = { 'listenPort': self.listenPort,
                     'inNamespace': self.inNamespace,
                     'ssid': self.ssid,
                     'channel': self.channel,
                     'mode': self.mode
                     }

        defaults.update(params)

        if not cls:
            cls = self.accessPoint
        ap = cls(name, **defaults)

        if not self.inNamespace and self.listenPort:
            self.listenPort += 1

        if self.inNamespace or ('inNamespace' in params and params['inNamespace'] == True):
            ap.params['inNamespace'] = True

        self.nameToNode[ name ] = ap
        ap.type = 'accessPoint'

        mininetWiFi.addParameters(ap, self.autoSetMacs, defaults, mode='master')
        self.switches.append(ap)
        self.accessPoints.append(ap)
        return ap

    def addWirelessMeshAP(self, name, cls=None, **params):
        """Add AP with Wireless Mesh capabilities.
           name: name of accesspoint to add
           cls: custom switch class/constructor (optional)
           returns: added acesspoint
           side effect: increments listenPort ivar ."""
        defaults = { 'ip': ipAdd(self.nextIP,
                                  ipBaseNum=self.ipBaseNum,
                                  prefixLen=self.prefixLen) +
                                  '/%s' % self.prefixLen,
                     'listenPort': self.listenPort,
                     'inNamespace': self.inNamespace,
                     'channel': self.channel,
                     'mode': self.mode
                     }

        defaults.update(params)

        if not cls:
            cls = self.accessPoint
        node = cls(name, **defaults)

        if not self.inNamespace and self.listenPort:
            self.listenPort += 1

        if self.inNamespace or ('inNamespace' in params and params['inNamespace'] == True):
            node.params['inNamespace'] = True

        self.nameToNode[ name ] = node
        node.type = 'station'
        self.nextIP += 1

        mininetWiFi.addParameters(node, self.autoSetMacs, defaults)
        self.switches.append(node)
        self.stations.append(node)
        return node

    def addPhysicalBaseStation(self, name, cls=None, **params):
        """Add BaseStation.
           name: name of basestation to add
           cls: custom switch class/constructor (optional)
           returns: added physical basestation
           side effect: increments listenPort ivar ."""
        defaults = { 'listenPort': self.listenPort,
                     'inNamespace': self.inNamespace,
                     'channel': self.channel,
                     'mode': self.mode,
                     'ssid': self.ssid
                     }

        defaults.update(params)

        if not cls:
            cls = self.switch
        bs = cls(name, **defaults)
        if not self.inNamespace and self.listenPort:
            self.listenPort += 1

        self.nameToNode[ name ] = bs
        bs.type = 'accessPoint'

        wlan = ("%s" % params.pop('phywlan', {}))
        bs.params['phywlan'] = wlan
        self.switches.append(bs)
        self.accessPoints.append(bs)

        mininetWiFi.addParameters(bs, self.autoSetMacs, defaults, mode='master')
        return bs

    def addSwitch(self, name, cls=None, **params):
        """Add switch.
           name: name of switch to add
           cls: custom switch class/constructor (optional)
           returns: added switch
           side effect: increments listenPort ivar ."""
        defaults = { 'listenPort': self.listenPort,
                     'inNamespace': self.inNamespace }

        defaults.update(params)

        if not cls:
            cls = self.switch

        sw = cls(name, **defaults)
        sw.type = 'switch'

        if not self.inNamespace and self.listenPort:
            self.listenPort += 1
        self.switches.append(sw)
        self.nameToNode[ name ] = sw
        return sw

    def delSwitch(self, switch):
        "Delete a switch"
        self.delNode(switch, nodes=self.switches)

    # Still in development
    def addWall(self, name, cls=None, **params):
        """in development"""
        defaults = {}
        defaults.update(params)

        if not cls:
            cls = self.switch

        wall = cls(name, **defaults)
        wall.type = 'wall'
        self.walls.append(wall)

        initPos = ("%s" % params.pop('initPos', {}))
        if(initPos != "{}"):
            initPos = initPos.split(',')
            wall.params['initPos'] = initPos

        finalPos = ("%s" % params.pop('finalPos', {}))
        if(finalPos != "{}"):
            finalPos = finalPos.split(',')
            wall.params['finalPos'] = finalPos

        width = ("%s" % params.pop('width', {}))
        if(width != "{}"):
            wall.params['width'] = width
        else:
            wall.params['width'] = 3

        self.nameToNode[ name ] = wall
        return wall

    def addController(self, name='c0', controller=None, **params):
        """Add controller.
           controller: Controller class"""
        # Get controller class
        if not controller:
            controller = self.controller
        # Construct new controller if one is not given
        if isinstance(name, Controller):
            controller_new = name

            # Pylint thinks controller is a str()
            # pylint: disable=maybe-no-member
            name = controller_new.name
            # pylint: enable=maybe-no-member
        else:
            controller_new = controller(name, **params)
        # Add new controller to net
        if controller_new:  # allow controller-less setups
            self.controllers.append(controller_new)
            self.nameToNode[ name ] = controller_new
        return controller_new

    def delController(self, controller):
        """Delete a controller
           Warning - does not reconfigure switches, so they
           may still attempt to connect to it!"""
        self.delNode(controller)

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
        # find first switch and create link
        if connect:
            if not isinstance(connect, Node):
                # Use first switch if not specified
                connect = self.switches[ 0 ]
            # Connect the nat to the switch
            self.addLink(nat, connect)
            # Set the default route on hosts
            natIP = nat.params[ 'ip' ].split('/')[ 0 ]
            for host in self.hosts:
                if host.inNamespace:
                    host.setDefaultRoute('via %s' % natIP)
        return nat

    # BL: We now have four ways to look up nodes
    # This may (should?) be cleaned up in the future.
    def getNodeByName(self, *args):
        "Return node(s) with given name(s)"
        if len(args) == 1:
            return self.nameToNode[ args[ 0 ] ]
        return [ self.nameToNode[ n ] for n in args ]

    def get(self, *args):
        "Convenience alias for getNodeByName"
        return self.getNodeByName(*args)

    # Even more convenient syntax for node lookup and iteration
    def __getitem__(self, key):
        "net[ name ] operator: Return node with given name"
        return self.nameToNode[ key ]

    def __delitem__(self, key):
        "del net[ name ] operator - delete node with given name"
        self.delNode(self.nameToNode[ key ])

    def __iter__(self):
        "return iterator over node names"
        for node in chain(self.hosts, self.switches, self.controllers):
            yield node.name

    def __len__(self):
        "returns number of nodes in net"
        return (len(self.hosts) + len(self.switches) +
                 len(self.controllers))

    def __contains__(self, item):
        "returns True if net contains named node"
        return item in self.nameToNode

    def keys(self):
        "return a list of all node names or net's keys"
        return list(self)

    def values(self):
        "return a list of all nodes or net's values"
        return [ self[name] for name in self ]

    def items(self):
        "return (key,value) tuple list for every node in net"
        return zip(self.keys(), self.values())

    @staticmethod
    def randMac():
        "Return a random, non-multicast MAC address"
        return macColonHex(random.randint(1, 2 ** 48 - 1) & 0xfeffffffffff |
                            0x020000000000)

    def runAlternativeModule(self, moduleDir):
        "Run an alternative module rather than mac80211_hwsim"
        mininetWiFi.alternativeModule = moduleDir

    def addMesh(self, node, cls=None, **params):
        """
        Configure wireless mesh
        
        node: name of the node
        cls: custom association class/constructor
        params: parameters for node
        """
        params['nextIP'] = self.nextIP
        params['ipBaseNum'] = self.ipBaseNum
        params['prefixLen'] = self.prefixLen
        mininetWiFi.addMesh(node, cls=None, **params)

    def addHoc(self, node, cls=None, **params):
        """
        Configure AdHoc
        
        node: name of the node
        cls: custom association class/constructor
        params: parameters for station
           
        """
        params['nextIP'] = self.nextIP
        params['ipBaseNum'] = self.ipBaseNum
        params['prefixLen'] = self.prefixLen
        mininetWiFi.addHoc(node, cls=None, **params)

    def wifiDirect(self, sta, cls=None, **params):
        """
        Configure wifidirect
        
        sta: name of the station
        cls: custom association class/constructor
        params: parameters for station
        """

        wlan = sta.ifaceToAssociate
        sta.func[wlan] = 'wifiDirect'

        options = { 'ip': ipAdd(self.nextIP,
                                  ipBaseNum=self.ipBaseNum,
                                  prefixLen=self.prefixLen) +
                                  '/%s' % self.prefixLen}
        options.update(params)

        cmd = ("echo \'")
        cmd = cmd + 'ctrl_interface=/var/run/wpa_supplicant\
            \nap_scan=1\
            \np2p_go_ht40=1\
            \ndevice_name=%s\
            \ndevice_type=1-0050F204-1\
            \np2p_no_group_iface=1' % (sta)
        confname = "mn%d_%s_wifiDirect.conf" % (os.getpid(), sta)
        cmd = cmd + ("\' > %s" % confname)
        os.system(cmd)
        sta.cmd('wpa_supplicant -B -Dnl80211 -c%s -i%s -d' % (confname, sta.params['wlan'][wlan]))

        value = mininetWiFi.setDataRate(sta=sta, wlan=wlan)
        self.bw = value.rate

        options['sta'] = sta
        options.update(params)
        # Set default MAC - this should probably be in Link
        options.setdefault('addr1', self.randMac())

    def useIFB(self):
        "Support to Intermediate Functional Block (IFB) Devices"
        mininetWiFi.ifb = True

    def addLink(self, node1, node2, port1=None, port2=None,
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
        node1 = node1 if not isinstance(node1, basestring) else self[ node1 ]
        node2 = node2 if not isinstance(node2, basestring) else self[ node2 ]
        options = dict(params)

        # If AP and STA
        if((((node1.type == 'station' or node1.type == 'vehicle') and ('ssid' in node2.params and 'apsInRange' in node1.params)) \
            or ((node2.type == 'station' or node2.type == 'vehicle') and ('ssid' in node1.params and 'apsInRange' in node2.params)))
            and 'link' not in options):

            if (node1.type == 'station' or node1.type == 'vehicle') and 'ssid' in node2.params:
                sta = node1
                ap = node2
            else:
                sta = node2
                ap = node1

            if 'intf' in params:
                for intf_ in sta.params['wlan']:
                    if params['intf'] == intf_:
                        wlan = sta.params['wlan'].index(intf_)
            else:
                wlan = sta.ifaceToAssociate

            sta.params['mode'][wlan] = ap.params['mode'][0]
            sta.params['channel'][wlan] = ap.params['channel'][0]

            # If sta/ap have defined position
            if 'position' in sta.params and 'position' in ap.params:
                dist = mininetWiFi.getDistance(sta, ap)
                if dist > ap.params['range']:
                    doAssociation = False
                else:
                    doAssociation = True
            else:
                doAssociation = True

            if(doAssociation):
                cls = Association
                cls.associate(sta, ap, self.useWmediumd)
                if 'bw' not in params and 'mininet.util.TCIntfWireless' not in str(self.link):
                    value = mininetWiFi.setDataRate(sta, ap, wlan)
                    self.bw = value.rate
                    params['bw'] = self.bw

                if 'mininet.util.TCIntfWireless' in str(self.link):
                    cls = self.link
                else:
                    cls = TCIntfWireless

                if 'mininet.util.TCIntfWireless' in str(self.link) or 'bw' in params and params['bw'] != 0 \
                    and 'position' not in sta.params:
                    # tc = True, this is useful only to apply tc configuration
                    cls(name=sta.params['wlan'][wlan], node=sta,
                                  link=None, tc=True, **params)

                if self.useWmediumd:
                    mininetWiFi.wlinks.append([sta, ap])

        elif (node1.type == 'accessPoint' and node2.type == 'accessPoint' and 'link' in options and options['link'] == 'wds'):
            # If sta/ap have defined position
            if 'position' in node1.params and 'position' in node2.params:
                mininetWiFi.srcConn.append(node1)
                mininetWiFi.dstConn.append(node2)

                dist = mininetWiFi.getDistance(node1, node2)
                if dist > node1.params['range']:
                    doAssociation = False
                else:
                    doAssociation = True
            else:
                doAssociation = True

            if(doAssociation):
                cls = WDSLink
                cls(node1, node2)
        else:
            if 'link' in options:
                options.pop('link', None)

            if 'position' in node1.params and 'position' in node2.params:
                mininetWiFi.srcConn.append(node1)
                mininetWiFi.dstConn.append(node2)
            # Port is optional
            if port1 is not None:
                options.setdefault('port1', port1)
            if port2 is not None:
                options.setdefault('port2', port2)

            # Set default MAC - this should probably be in Link
            options.setdefault('addr1', self.randMac())
            options.setdefault('addr2', self.randMac())

            cls = self.link if cls is None else cls
            link = cls(node1, node2, **options)
            self.links.append(link)
            return link

    def autoAssociation(self):
        "This is useful to make the users' life easier"
        stations = self.stations
        accessPoints = self.accessPoints
        mininetWiFi.autoAssociation(stations, accessPoints)

    def configureWifiNodes(self):
        "Configure WiFi Nodes"
        params = dict()
        params.setdefault('stations', self.stations)
        params.setdefault('accessPoints', self.accessPoints)
        params.setdefault('cars', self.cars)
        params.setdefault('switches', self.switches)
        params.setdefault('nextIP', self.nextIP)
        params.setdefault('ipBaseNum', self.ipBaseNum)
        params.setdefault('prefixLen', self.prefixLen)
        params.setdefault('useWmediumd', self.useWmediumd)
        self.stations, self.accessPoints = mininetWiFi.configureWifiNodes(**params)

    def delLink(self, link):
        "Remove a link from this network"
        link.delete()
        self.links.remove(link)

    def linksBetween(self, node1, node2):
        "Return Links between node1 and node2"
        return [ link for link in self.links
                 if (node1, node2) in (
                    (link.intf1.node, link.intf2.node),
                    (link.intf2.node, link.intf1.node)) ]

    def delLinkBetween(self, node1, node2, index=0, allLinks=False):
        """Delete link(s) between node1 and node2
           index: index of link to delete if multiple links (0)
           allLinks: ignore index and delete all such links (False)
           returns: deleted link(s)"""
        links = self.linksBetween(node1, node2)
        if not allLinks:
            links = [ links[ index ] ]
        for link in links:
            self.delLink(link)
        return links

    def configHosts(self):
        "Configure a set of hosts."
        for host in self.hosts:
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

    def buildFromTopo(self, topo=None):
        """Build mininet from a topology object
           At the end of this function, everything should be connected
           and up."""
        cls = Association
        cls.printCon = False
        # Possibly we should clean up here and/or validate
        # the topo
        if self.cleanup:
            pass

        info('*** Creating network\n')
        if not self.controllers and self.controller:
            # Add a default controller
            info('*** Adding controller\n')
            classes = self.controller
            if not isinstance(classes, list):
                classes = [ classes ]
            for i, cls in enumerate(classes):
                # Allow Controller objects because nobody understands partial()
                if isinstance(cls, Controller):
                    self.addController(cls)
                else:
                    self.addController('c%d' % i, cls)

        info('*** Adding hosts and stations:\n')
        for hostName in topo.hosts():
            if 'sta' in str(hostName):
                self.addStation(hostName, **topo.nodeInfo(hostName))
            else:
                self.addHost(hostName, **topo.nodeInfo(hostName))
            info(hostName + ' ')

        info('\n*** Adding switches and access point(s):\n')
        for switchName in topo.switches():
            # A bit ugly: add batch parameter if appropriate
            params = topo.nodeInfo(switchName)
            cls = params.get('cls', self.switch)
            if hasattr(cls, 'batchStartup'):
                params.setdefault('batch', True)
            if 'ap' in str(switchName):
                self.addAccessPoint(switchName, **params)
            else:
                self.addSwitch(switchName, **params)
            info(switchName + ' ')

        if mininetWiFi.isWiFi:
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
        "Build mininet."
        if self.topo:
            self.buildFromTopo(self.topo)
        if self.useWmediumd:
            mininetWiFi.configureWmediumd(self.stations, self.accessPoints)

        if self.inNamespace:
            self.configureControlNetwork()
        info('*** Configuring hosts\n')
        self.configHosts()
        if self.xterms:
            self.startTerms()
        if self.autoStaticArp:
            self.staticArp()
        self.built = True
        if self.useWmediumd:
            mininetWiFi.wmediumdConnect()

    def startTerms(self):
        "Start a terminal for each node."
        if 'DISPLAY' not in os.environ:
            error("Error starting terms: Cannot connect to display\n")
            return
        info("*** Running terms on %s\n" % os.environ[ 'DISPLAY' ])
        cleanUpScreens()
        self.terms += makeTerms(self.controllers, 'controller')
        self.terms += makeTerms(self.switches, 'switch')
        self.terms += makeTerms(self.hosts, 'host')

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
        for switch in self.switches:
            info(switch.name + ' ')
            switch.start(self.controllers)

        started = {}
        for swclass, switches in groupby(
                sorted(self.switches, key=type), type):
            switches = tuple(switches)
            if hasattr(swclass, 'batchStartup'):
                success = swclass.batchStartup(switches)
                started.update({ s: s for s in success })
        info('\n')
        if self.waitConn:
            self.waitConnected()

    def meshRouting(self, routing):
        """
        Defined the mesh routing
        
        :params routing: the mesh routing (default: custom)
        """
        if routing != '':
            mininetWiFi.meshRouting(routing)

    def seed(self, seed):
        "Seed"
        mininetWiFi.seed_ = seed

    def roads(self, nroads):
        "Number of roads"
        mininetWiFi.isVanet = True
        mininetWiFi.nroads = nroads

    def stop(self):
        "Stop the controller(s), switches and hosts"
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
        info('*** Stopping switches and/or access points\n')
        stopped = {}
        for swclass, switches in groupby(
                sorted(self.switches, key=type), type):
            switches = tuple(switches)
            if hasattr(swclass, 'batchShutdown'):
                success = swclass.batchShutdown(switches)
                stopped.update({ s: s for s in success })
        for switch in self.switches:
            info(switch.name + ' ')
            if switch not in stopped:
                switch.stop()
            switch.terminate()
        info('\n')

        info('*** Stopping hosts and/or stations\n')
        for host in self.hosts:
            info(host.name + ' ')
            host.terminate()
        info('\n')
        if(mininetWiFi.isWiFi):
            "Stop Graph"
            mininetWiFi.closeMininetWiFi()
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
        h1 = hosts[ 0 ]  # so we can call class method fdToNode
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
            hosts = self.hosts
            output('*** Ping: testing ping reachability\n')
        for node in hosts:
            output('%s -> ' % node.name)
            for dest in hosts:
                if node != dest:
                    opts = ''
                    if timeout:
                        opts = '-W %s' % timeout
                    if dest.intfs:
                        result = node.cmdPrint('ping -c1 %s %s' %
                                           (opts, dest.IP()))
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
    def _parsePingFull(pingOutput):
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
            if received == 0:
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
        return self.ping(timeout=timeout)

    def pingPair(self):
        """Ping between first two hosts, useful for testing.
           returns: ploss packet loss percentage"""
        hosts = [ self.hosts[ 0 ], self.hosts[ 1 ] ]
        return self.ping(hosts=hosts)

    def pingAllFull(self):
        """Ping between all hosts.
           returns: ploss packet loss percentage"""
        return self.pingFull()

    def pingPairFull(self):
        """Ping between first two hosts, useful for testing.
           returns: ploss packet loss percentage"""
        hosts = [ self.hosts[ 0 ], self.hosts[ 1 ] ]
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
        hosts = hosts or [ self.hosts[ 0 ], self.hosts[ -1 ] ]
        assert len(hosts) == 2
        client, server = hosts

        conn1 = 0
        conn2 = 0
        if client.type == 'station' or server.type == 'station':
            if client.type == 'station':
                while conn1 == 0:
                    conn1 = int(client.cmd('iwconfig %s-wlan0 | grep -ic \'Link Quality\'' % client))
            if server.type == 'station':
                while conn2 == 0:
                    conn2 = int(server.cmd('iwconfig %s-wlan0 | grep -ic \'Link Quality\'' % server))
        output('*** Iperf: testing', l4Type, 'bandwidth between',
                client, 'and', server, '\n')
        server.cmd('killall -9 iperf')
        iperfArgs = 'iperf -p %d ' % port
        bwArgs = ''
        if l4Type == 'UDP':
            iperfArgs += '-u '
            bwArgs = '-b ' + udpBw + ' '
        elif l4Type != 'TCP':
            raise Exception('Unexpected l4 type: %s' % l4Type)
        if fmt:
            iperfArgs += '-f %s ' % fmt
        server.sendCmd(iperfArgs + '-s')
        if l4Type == 'TCP':
            if not waitListening(client, server.IP(), port):
                raise Exception('Could not connect to iperf on port %d'
                                 % port)
        cliout = client.cmd(iperfArgs + '-t %d -c ' % seconds +
                             server.IP() + ' ' + bwArgs)
        debug('Client output: %s\n' % cliout)
        servout = ''
        # We want the last *b/sec from the iperf server output
        # for TCP, there are two of them because of waitListening
        count = 2 if l4Type == 'TCP' else 1
        while len(re.findall('/sec', servout)) < count:
            servout += server.monitor(timeoutms=5000)
        server.sendInt()
        servout += server.waitOutput()
        debug('Server output: %s\n' % servout)
        result = [ self._parseIperf(servout), self._parseIperf(cliout) ]
        if l4Type == 'UDP':
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
        hosts = self.hosts
        cores = int(quietRun('nproc'))
        # number of processes to run a while loop on per host
        num_procs = int(ceil(cores * cpu))
        pids = {}
        for h in hosts:
            pids[ h ] = []
            for _core in range(num_procs):
                h.cmd('while true; do a=1; done &')
                pids[ h ].append(h.cmd('echo $!').strip())
        outputs = {}
        time = {}
        # get the initial cpu time for each host
        for host in hosts:
            outputs[ host ] = []
            with open('/sys/fs/cgroup/cpuacct/%s/cpuacct.usage' %
                       host, 'r') as f:
                time[ host ] = float(f.read())
        for _ in range(duration):
            sleep(1)
            for host in hosts:
                with open('/sys/fs/cgroup/cpuacct/%s/cpuacct.usage' %
                           host, 'r') as f:
                    readTime = float(f.read())
                outputs[ host ].append(((readTime - time[ host ])
                                        / 1000000000) / cores * 100)
                time[ host ] = readTime
        for h, pids in pids.items():
            for pid in pids:
                h.cmd('kill -9 %s' % pid)
        cpu_fractions = []
        for _host, outputs in outputs.items():
            for pct in outputs:
                cpu_fractions.append(pct)
        output('*** Results: %s\n' % cpu_fractions)
        return cpu_fractions

    def mobility(self, *args, **kwargs):
        "Configure mobility parameters"
        mininetWiFi.configureMobility(*args, **kwargs)

    def startMobility(self, **kwargs):
        "Starts Mobility"
        stations = self.stations
        accessPoints = self.accessPoints
        mininetWiFi.startMobility(stations, accessPoints, **kwargs)

    def stopMobility(self, **kwargs):
        """Stops Mobility"""

        stations = self.stations
        accessPoints = self.accessPoints
        mininetWiFi.stopMobility(stations, accessPoints, **kwargs)

    def useExternalProgram(self, program, **params):
        """
        Opens an external program
        
        :params program: any program (useful for SUMO)
        :params **params config_file: file configuration
        """
        params['stations'] = self.stations
        params['aps'] = self.accessPoints
        params['cars'] = self.cars
        params['program'] = program
        mininetWiFi.useExternalProgram(**params)

    def getCurrentDistance(self, src, dst):
        """ 
        Gets the distance between two nodes
        
        :params src: source node
        :params dst: destination node
        """
        nodes = self.stations + self.cars + self.accessPoints
        mininetWiFi.printDistance(src, dst, nodes)

    def plotNode(self, node, position):
        """ 
        Useful for plotting switches and hosts 
        
        :params node: the node
        :params position: position of the node (x,y,z)
        """
        node.params['position'] = position.split(',')
        node.params['range'] = 0
        mininetWiFi.plotNodes.append(node)

    def plotGraph(self, max_x=0, max_y=0, max_z=0):
        """ 
        Plots Graph 
        
        :params max_x: maximum X
        :params max_y: maximum Y
        :params max_z: maximum Z
        """
        mininetWiFi.plotGraph(max_x, max_y, max_z)

    def startGraph(self):
        mininetWiFi.startGraph()

    def setChannelEquation(self, **params):
        """ 
        Set Channel Equation. The user may change the equation defined in wifiChannel.py by any other.
        
        :params bw: bandwidth (mbps)
        :params delay: delay (ms)
        :params latency: latency (ms)
        :params loss: loss (%)
        """
        mininetWiFi.setChannelEquation(**params)

    def propagationModel(self, model, exp=2, sL=1, lF=0, pL=0, nFloors=0, gRandom=0):
        """ 
        Attributes for Propagation Model 
        
        :params model: propagation model
        :params exp: exponent
        :params sL: system Loss
        :params lF: floor penetration loss factor
        :params pL: power Loss Coefficient
        :params nFloors: number of floors
        :params gRandom: gaussian random variable
        """
        mininetWiFi.propagationModel(self.stations, self.accessPoints, model, exp, sL, lF, pL, nFloors, gRandom)

    def associationControl(self, ac):
        """
        Defines an association control
        
        :params ac: the association control
        """
        mininetWiFi.associationControl(ac)

    def configWirelessLinkStatus(self, src, dst, status):
        if status == 'down':
            if self.nameToNode[ src ].type == 'station':
                sta = self.nameToNode[ src ]
                ap = self.nameToNode[ dst ]
            else:
                sta = self.nameToNode[ dst ]
                ap = self.nameToNode[ src ]
            for wlan in range(0, len(sta.params['wlan'])):
                if sta.params['associatedTo'][wlan] != '':
                    sta.cmd('iw dev %s disconnect' % sta.params['wlan'][wlan])
                    sta.params['associatedTo'][wlan] = ''
                    ap.params['associatedStations'].remove(sta)
        else:
            if self.nameToNode[ src ].type == 'station':
                sta = self.nameToNode[ src ]
                ap = self.nameToNode[ dst ]
            else:
                sta = self.nameToNode[ dst ]
                ap = self.nameToNode[ src ]
            for wlan in range(0, len(sta.params['wlan'])):
                if sta.params['associatedTo'][wlan] == '':
                    sta.pexec('iwconfig %s essid %s ap %s' % (sta.params['wlan'][wlan], ap.params['ssid'][0], ap.params['mac'][0]))
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
        if self.nameToNode[ src ].type == 'station' and self.nameToNode[ dst ].type == 'accessPoint' or \
            self.nameToNode[ src ].type == 'accessPoint' and self.nameToNode[ dst ].type == 'station':
            self.configWirelessLinkStatus(src, dst, status)
        else:
            src = self.nameToNode[ src ]
            dst = self.nameToNode[ dst ]
            connections = src.connectionsTo(dst)
            if len(connections) == 0:
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


class MininetWithControlNet(Mininet):

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
        controller = self.controllers[ 0 ]
        info(controller.name + ' <->')
        cip = ip
        snum = ipParse(ip)
        for switch in self.switches:
            info(' ' + switch.name)
            link = self.link(switch, controller, port1=0)
            sintf, cintf = link.intf1, link.intf2
            switch.controlIntf = sintf
            snum += 1
            while snum & 0xff in [ 0, 255 ]:
                snum += 1
            sip = ipStr(snum)
            cintf.setIP(cip, prefixLen)
            sintf.setIP(sip, prefixLen)
            controller.setHostRoute(sip, cintf)
            switch.setHostRoute(cip, sintf)
        info('\n')
        info('*** Testing control network\n')
        while not cintf.isUp():
            info('*** Waiting for', cintf, 'to come up\n')
            sleep(1)
        for switch in self.switches:
            while not sintf.isUp():
                info('*** Waiting for', sintf, 'to come up\n')
                sleep(1)
            if self.ping(hosts=[ switch, controller ]) != 0:
                error('*** Error: control network test failed\n')
                exit(1)
        info('\n')
