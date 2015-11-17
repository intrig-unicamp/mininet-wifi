"""

    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDN!

author: Bob Lantz (rlantz@cs.stanford.edu)
author: Brandon Heller (brandonh@stanford.edu)

Modified by Ramon Fontes (ramonrf@dca.fee.unicamp.br)

Mininet creates scalable OpenFlow test networks by using
process-based virtualization and network namespaces.

Simulated hosts are created as processes in separate network
namespaces. This allows a complete OpenFlow network to be simulated on
top of a single Linux kernel.

Each host has:

A virtual console (pipes to a shell)
A virtual interfaces (half of a veth pair)
A parent shell (and possibly some child processes) in a namespace

Hosts have a network interface which is configured via ifconfig/ip
link/etc.

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
import time
import threading

from time import sleep
from itertools import chain, groupby
from math import ceil

from mininet.cli import CLI
from mininet.log import info, error, debug, output, warn
from mininet.node import ( Node, Host, OVSKernelSwitch, DefaultController,
                           Controller )
from mininet.nodelib import NAT
from mininet.link import Link, Intf, TCLink
from mininet.util import ( quietRun, fixLimits, numCores, ensureRoot,
                           macColonHex, ipStr, ipParse, netParse, ipAdd,
                           waitListening )
from mininet.term import cleanUpScreens, makeTerms
from mininet.wifi import checkNM, module, accessPoint, station, wifiParameters, association, mobility, getWlan
from __builtin__ import True

# Mininet version: should be consistent with README and LICENSE
VERSION = "1.6"

class Mininet( object ):
    "Network emulation with hosts spawned in network namespaces."

    def __init__( self, topo=None, switch=OVSKernelSwitch, host=Host,
                  controller=DefaultController, link=Link, intf=Intf,
                  build=True, xterms=False, cleanup=False, ipBase='10.0.0.0/8',
                  inNamespace=False, autoSetMacs=False, autoStaticArp=False, autoPinCpus=False,
                  listenPort=None, waitConnected=False, 
                  ssid="my-ssid", mode="g", channel="6" ):
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
        self.thread = threading.Thread()
        self.topo = topo
        self.switch = switch
        self.baseStation = switch
        self.host = host
        self.controller = controller
        self.link = link
        self.intf = intf
        self.ipBase = ipBase
        self.ipBaseNum, self.prefixLen = netParse( self.ipBase )
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
        self.meshIP = 0    
        self.start_time = 0 #start mobility time
        self.set_seed = 10
        self.model = ''
        self.firstAssociation = True
        self.ssid = ssid        
        self.mode = mode
        self.channel = channel
        self.nameToNode = {}  # name to Node (Host/Switch) objects
        self.newapif = []
        self.apexists = []
        self.missingStations = []
        self.sta_inMov = []
        self.wifiNodes = []
        self.hosts = []
        self.switches = []
        self.stations = []
        self.accessPoints = []
        self.controllers = []
        self.links = []
        self.terms = []  # list of spawned xterm processes
        Mininet.init()  # Initialize Mininet if necessary
                
        self.built = False
        if topo and build:
            self.build()

    def waitConnected( self, timeout=None, delay=.5 ):
        """wait for each switch to connect to a controller,
           up to 5 seconds
           timeout: time to wait, or None to wait indefinitely
           delay: seconds to sleep per iteration
           returns: True if all switches are connected"""
        info( '*** Waiting for switches to connect\n' )
        time = 0
        remaining = list( self.switches )
        while True:
            for switch in tuple( remaining ):
                if switch.connected():
                    info( '%s ' % switch )
                    remaining.remove( switch )
            if not remaining:
                info( '\n' )
                return True
            if time > timeout and timeout is not None:
                break
            sleep( delay )
            time += delay
        warn( 'Timed out after %d seconds\n' % time )
        for switch in remaining:
            if not switch.connected():
                warn( 'Warning: %s is not connected to a controller\n'
                      % switch.name )
            else:
                remaining.remove( switch )
        return not remaining

    def addHost( self, name, cls=None, **params ):
        """Add host.
           name: name of host to add
           cls: custom host class/constructor (optional)
           params: parameters for host
           returns: added host"""      
        # Default IP and MAC addresses                
        defaults = { 'ip': ipAdd( self.nextIP,
                                  ipBaseNum=self.ipBaseNum,
                                  prefixLen=self.prefixLen ) +
                                  '/%s' % self.prefixLen }
        if self.autoSetMacs:
            defaults[ 'mac' ] = macColonHex( self.nextIP )
        if self.autoPinCpus:
            defaults[ 'cores' ] = self.nextCore
            self.nextCore = ( self.nextCore + 1 ) % self.numCores
        self.nextIP += 1
        defaults.update( params )
        if not cls:
            cls = self.host
        h = cls( name, **defaults )  
        h.type = 'host'    
        self.hosts.append( h )
        self.nameToNode[ name ] = h
        return h

    def addStation( self, name, cls=None, **params ):
        """Add Station.
           name: name of station to add
           cls: custom host class/constructor (optional)
           params: parameters for host
           returns: added host"""
        #Default IP and MAC addresses
        defaults = { 'ip': ipAdd( self.nextIP,
                                  ipBaseNum=self.ipBaseNum,
                                  prefixLen=self.prefixLen ) +
                                  '/%s' % self.prefixLen}
        
        if self.autoSetMacs:
            defaults[ 'mac' ] = macColonHex( self.nextIP )
        if self.autoPinCpus:
            defaults[ 'cores' ] = self.nextCore
            self.nextCore = ( self.nextCore + 1 ) % self.numCores        
        defaults.update( params )        
        if not cls:
            cls = self.host
        sta = cls( name, **defaults )      
        
        mac = ("%s" % params.pop('mac', {}))
        if(mac!="{}"):        
            mac = mac.split(',')
            sta.mac = str(mac[0])
        elif self.autoSetMacs:
            sta.mac = defaults[ 'mac' ]
        
        self.hosts.append( sta )
        self.stations.append(sta)
        self.wifiNodes.append(sta)
        self.nameToNode[ name ] = sta        
        self.missingStations.append(sta)
        sta.type = 'station'
        
        position = ("%s" % params.pop('position', {}))
        if(position!="{}"):        
            position = position.split(',')
            sta.startPosition = position
            sta.position = position
            station.fixedPosition.append(sta)
        else:
            sta.startPosition = 0
            sta.position = 0            
        
        channel = ("%s" % params.pop('channel', {}))
        if(channel!="{}"): 
            sta.channel = channel
        else:
            sta.channel = 1
      
        mode = ("%s" % params.pop('mode', {}))
        if(mode!="{}"): 
            sta.mode = mode
        else:
            sta.mode = "g"
                
        wifi = ("%s" % params.pop('wlans', {}))
        if(wifi!="{}"):        
            module.wifiRadios = module.wifiRadios + int(wifi)
            for n in range(int(wifi)):
                module.virtualWlan.append(name)
        else:
            module.wifiRadios+=1
            wifi = 1
            module.virtualWlan.append(name)
        sta.nWlans = int(wifi)
        
        self.nextIP += 1        
        return sta

    def addBaseStation( self, name, cls=None, **params ):
        """Add BaseStation.
           name: name of basestation to add
           cls: custom switch class/constructor (optional)
           returns: added switch
           side effect: increments listenPort ivar ."""
        defaults = { 'listenPort': self.listenPort,
                     'inNamespace': self.inNamespace, 
                     'channel': self.channel,
                     'mode': self.mode,
                     'ssid': self.ssid                 
                     }
        defaults.update( params )        
        
        if not cls:
            cls = self.baseStation
        bs = cls( name, **defaults )
        if not self.inNamespace and self.listenPort:
            self.listenPort += 1
        self.wifiNodes.append(bs)
        self.nameToNode[ name ] = bs
        bs.type = 'accessPoint'
        
        position = ("%s" % params.pop('position', {}))
        if(position!="{}"):        
            position =  position.split(',')
            bs.startPosition = position
            bs.position = position
        else:
            bs.startPosition = 0
            bs.position = 0            
      
        channel = ("%s" % params.pop('channel', {}))
        if(channel!="{}"):
            bs.channel = channel
        else:
            bs.channel = self.channel 
      
        mode = ("%s" % params.pop('mode', {}))
        if(mode!="{}"):
            bs.mode = mode
        else:
            bs.mode = self.mode
                      
        ssid = ("%s" % params.pop('ssid', {}))
        if(ssid!="{}"):
            bs.ssid = ssid
        else:
            bs.ssid = self.ssid
            
        self.range = ("%s" % params.pop('range', {}))
        if(self.range!="{}"):
            bs.range = int(self.range)
        else:
            bs.range = accessPoint.range(bs.mode)
            
        wifi = ("%s" % params.pop('wlans', {}))
        if(wifi!="{}"):        
            module.wifiRadios = module.wifiRadios + int(wifi)
            for n in range(int(wifi)):
                module.virtualWlan.append(str(name)+str(n))
        else:
            module.wifiRadios+=1
            wifi = 1
            module.virtualWlan.append(str(name)+str(0))
        bs.nWlans = int(wifi)
        
        accessPoint.list.append(bs)
        self.switches.append( bs )          
        self.accessPoints.append( bs ) 
        
        return bs
     
    def addSwitch( self, name, cls=None, **params ):
        """Add switch.
           name: name of switch to add
           cls: custom switch class/constructor (optional)
           returns: added switch
           side effect: increments listenPort ivar ."""
        defaults = { 'listenPort': self.listenPort,
                     'inNamespace': self.inNamespace }
        
        defaults.update( params )
        
        if not cls:
            cls = self.switch
        
        sw = cls( name, **defaults )
        sw.type = 'switch'
        
        if not self.inNamespace and self.listenPort:
            self.listenPort += 1
        self.switches.append( sw )
        self.nameToNode[ name ] = sw
        return sw

    def addController( self, name='c0', controller=None, **params ):
        """Add controller.
           controller: Controller class"""
        # Get controller class
        if not controller:
            controller = self.controller
        # Construct new controller if one is not given
        if isinstance( name, Controller ):
            controller_new = name
            # Pylint thinks controller is a str()
            # pylint: disable=maybe-no-member
            name = controller_new.name
            # pylint: enable=maybe-no-member
        else:
            controller_new = controller( name, **params )
        # Add new controller to net
        if controller_new:  # allow controller-less setups
            self.controllers.append( controller_new )
            self.nameToNode[ name ] = controller_new
        return controller_new

    def addNAT( self, name='nat0', connect=True, inNamespace=False,
                **params):
        """Add a NAT to the Mininet network
           name: name of NAT node
           connect: switch to connect to | True (s1) | None
           inNamespace: create in a network namespace
           params: other NAT node params, notably:
               ip: used as default gateway address"""
        nat = self.addHost( name, cls=NAT, inNamespace=inNamespace,
                            subnet=self.ipBase, **params )
        # find first switch and create link
        if connect:
            if not isinstance( connect, Node ):
                # Use first switch if not specified
                connect = self.switches[ 0 ]
            # Connect the nat to the switch
            self.addLink( nat, self.switches[ 0 ] )
            # Set the default route on hosts
            natIP = nat.params[ 'ip' ].split('/')[ 0 ]
            for host in self.hosts:
                if host.inNamespace:
                    host.setDefaultRoute( 'via %s' % natIP )
        return nat

    # BL: We now have four ways to look up nodes
    # This may (should?) be cleaned up in the future.
    def getNodeByName( self, *args ):
        "Return node(s) with given name(s)"
        if len( args ) == 1:
            return self.nameToNode[ args[ 0 ] ]
        return [ self.nameToNode[ n ] for n in args ]

    def get( self, *args ):
        "Convenience alias for getNodeByName"
        return self.getNodeByName( *args )

    # Even more convenient syntax for node lookup and iteration
    def __getitem__( self, key ):
        """net [ name ] operator: Return node(s) with given name(s)"""
        return self.nameToNode[ key ]

    def __iter__( self ):
        "return iterator over node names"
        for node in chain( self.hosts, self.switches, self.controllers ):
            yield node.name

    def __len__( self ):
        "returns number of nodes in net"
        return ( len( self.hosts ) + len( self.switches ) +
                 len( self.controllers ) )

    def __contains__( self, item ):
        "returns True if net contains named node"
        return item in self.nameToNode

    def keys( self ):
        "return a list of all node names or net's keys"
        return list( self )

    def values( self ):
        "return a list of all nodes or net's values"
        return [ self[name] for name in self ]

    def items( self ):
        "return (key,value) tuple list for every node in net"
        return zip( self.keys(), self.values() )

    @staticmethod
    def randMac():
        "Return a random, non-multicast MAC address"
        return macColonHex( random.randint(1, 2**48 - 1) & 0xfeffffffffff |
                            0x020000000000 )
    
    def addWlan (self, wifiRadios):
        module.wifiRadios+=wifiRadios
    
    def addMesh( self, sta, cls=None, **params ):
        
        if self.firstAssociation:
            self.configureWifiNodes(False)
                
        node = sta if not isinstance( sta, basestring ) else self[ sta ]
        options = dict()
        
        channel = ("%s" % params.pop('channel', {}))
        if(channel!="{}"): 
            sta.channel = channel
        else:
            sta.channel = 1
            
        ssid = ("%s" % params.pop('ssid', {}))
        if(ssid!="{}"):
            sta.ssid = ssid
        else:
            sta.ssid = 'meshNetwork'
            
        mode = ("%s" % params.pop('mode', {}))
        if(mode!="{}"):
            sta.mode = mode
        else:
            sta.mode = 'g'
        
        self.meshIP+=1
        
        options.setdefault( 'ipaddress', self.meshIP )        
        # Set default MAC - this should probably be in Link
        options.setdefault( 'addr1', self.randMac() )
        
        cls = self.link if cls is None else cls
        link = cls( node, 'onlyOneDevice', **options )
        
        for sta in self.hosts:
            if (sta == node):
                station.addMesh(sta, **options)
                if sta in self.missingStations:
                    self.missingStations.remove(sta)
        
        return link    
    
    def addHoc( self, sta, cls=None, **params ):
        
        if self.firstAssociation:
            self.configureWifiNodes(False)
            
        node = sta if not isinstance( sta, basestring ) else self[ sta ]
        options = dict( )
        
        channel = ("%s" % params.pop('channel', {}))
        if(channel!="{}"): 
            sta.channel = channel
        else:
            sta.channel = 1
            
        ssid = ("%s" % params.pop('ssid', {}))
        if(ssid!="{}"):
            sta.ssid = ssid
        else:
            sta.ssid = 'meshNetwork'
            
        mode = ("%s" % params.pop('mode', {}))
        if(mode!="{}"):
            sta.mode = mode
        else:
            sta.mode = 'g'
        
        # Set default MAC - this should probably be in Link
        options.setdefault( 'addr1', self.randMac() )
        
        cls = self.link if cls is None else cls
        link = cls( sta, 'onlyOneDevice', **options )
        
        for sta in self.hosts:
            if (sta == node):
                station.adhoc(sta, **options)
                if sta in self.missingStations:
                    self.missingStations.remove(sta)
        return link
      
    def configureAP(self):
        
        for ap in self.accessPoints:
            ap.virtualWlan = str(ap)+'-'+str('wlan')
            
            for wlan in range(0,ap.nWlans):
       
                wifiparam = dict()
                intf = self.newapif[module.virtualWlan.index(str(ap)+str(wlan))]
                newname = str(ap)+'-'+str('wlan')
                accessPoint.rename(intf, newname, wlan)
                checkNM.getMacAddress(ap, wlan)         
                accessPoint.setBw(ap, wlan)
                
                self.wpa_key_mgmt = None
                self.country_code = None
                self.rsn_pairwise = None
                self.wpa_passphrase = None
                self.wpa = None
                self.auth_algs = None
                self.wmm_enabled = None        
                
                wifiparam.setdefault( 'country_code', self.country_code )
                wifiparam.setdefault( 'auth_algs', self.auth_algs )
                wifiparam.setdefault( 'wpa', self.wpa )
                wifiparam.setdefault( 'wpa_key_mgmt', self.wpa_key_mgmt )
                wifiparam.setdefault( 'rsn_pairwise', self.rsn_pairwise )
                wifiparam.setdefault( 'wpa_passphrase', self.wpa_passphrase )
                wifiparam.setdefault( 'iface', wlan )
               
                cmd = accessPoint.start(ap, **wifiparam)   
                checkNM.APfile(cmd, ap, wlan) 
                 
    def addMissingSTAs(self, sta):
        
        options = dict( )
        self.bw = wifiParameters.set_bw(sta.mode)
        options.setdefault( 'bw', self.bw )
        options.setdefault( 'use_hfsc', True )
        
        # Set default MAC - this should probably be in Link
        options.setdefault( 'addr1', self.randMac() )
       
        cls = None
        cls = self.link if cls is None else cls
        cls( sta, 'onlyOneDevice', **options )
        
        if sta.mac != '':
            station.setMac(sta)        
        
        #necessary if does not exist link between sta and other device
        sta.associate = False
            
    """    
    def wds( self, ap1, ap2, cls=None, **params ):
        
        if('ap' in str(ap1) and 'ap' in str(ap2)):
            
            if self.firstAssociation:
                module.startEnvironment()
                self.link = TCLink
                self.newapif = getWlan.virtual()  #Get Virtual Wlans      
                self.firstAssociation = False
                station.assingIface(self.hosts)
                
            node1 = ap1 if not isinstance( ap1, basestring ) else self[ ap1 ]
            node2 = ap2 if not isinstance( ap2, basestring ) else self[ ap2 ]
        
            ap1 = str(node1)
            self.apexists.append(ap1) 
            ap2 = str(node2)
            self.apexists.append(ap2)
            
            int1 = self.newapif[module.virtualWlan.index(ap1)]
            int2 = self.newapif[module.virtualWlan.index(ap2)]
            accessPoint.wds(ap1, int1, ap2, int2)
            
            #configure AP
            self.configureAP(ap1)
            self.configureAP(ap2)
                
            self.bw = wifiParameters.set_bw(self.mode)
    """
    
    def activeMultipleWlans(self):
        """Useful when stations have multiple interfaces"""
        for st in self.stations:
            for z in range(0, st.nWlans):
                st.ifaceAssociatedToAp.append(str(z))
                
    def configureWifiNodes(self, hasAP=True):
        module.startEnvironment()
        self.link = TCLink
        self.newapif = getWlan.virtual()  #Get Virtual Wlans      
        self.firstAssociation = False
        station.assingIface(self.hosts)
        if hasAP:
            self.configureAP() #configure AP
            #Useful when stations have multiple interfaces
            self.activeMultipleWlans()
        
            
    def addLink( self, node1, node2, port1=None, port2=None, 
                 cls=None, **params ):
        
        # Accept node objects or names
        node1 = node1 if not isinstance( node1, basestring ) else self[ node1 ]
        node2 = node2 if not isinstance( node2, basestring ) else self[ node2 ]
        options = dict( params )
        
        #If AP and STA
        if((node1.type =='station' and node2.type == 'accessPoint') \
            or ( node2.type =='station' and node1.type == 'accessPoint')):
            
            if self.firstAssociation:
                self.configureWifiNodes(True)
                
            #Only if AP
            if node1.type == 'accessPoint' and str(node1) not in self.apexists \
                or node2.type == 'accessPoint' and str(node2) not in self.apexists:
        
                if node1.type == 'accessPoint':
                    ap = node1
                    self.apexists.append(str(node1)) 
                else:
                    ap = node2
                    self.apexists.append(str(node2))
                
            if (node1.type =='station' or node2.type =='station'):
                if node1.type =='station':
                    sta = node1
                    ap = node2
                else:
                    sta = node2
                    ap = node1
                
                if sta in self.missingStations:
                    self.missingStations.remove(sta)
                self.bw = wifiParameters.set_bw(sta.mode)
                options.setdefault( 'bw', self.bw )
                options.setdefault( 'use_hfsc', True )
                
                # Set default MAC - this should probably be in Link
                options.setdefault( 'addr1', self.randMac() )
                #options.setdefault( 'addr2', self.randMac() )
                
                cls = self.link if cls is None else cls
                link = cls( sta, 'onlyOneDevice', **options )
                
                if sta.mac != '':
                    station.setMac(sta)        
                
                #If sta/ap have defined position 
                if sta.startPosition !=0 and ap.startPosition !=0:
                    distance = mobility.getDistance(sta, ap)
                    doAssociation = association.doAssociation(sta.mode, ap, distance)
                #if not
                else:
                    doAssociation = True
                    distance = 0                
                
                sta.doAssociation = doAssociation
                if(doAssociation):
                    sta.ifaceToAssociate+=1
                    wlan = sta.ifaceToAssociate
                    station.associate(sta, ap)
                    association.setInfraParameters(sta, ap, distance, wlan)   
            return link
        
        else:
            """"Add a link from node1 to node2
                node1: source node (or name)
                node2: dest node (or name)
                port1: source port (optional)
                port2: dest port (optional)
                cls: link class (optional)
                params: additional link params (optional)
                returns: link object"""
            
            #Only if AP
            if node1.type == 'accessPoint' and node2.type == 'accessPoint' :   
                if self.firstAssociation:
                    self.configureWifiNodes(True)              
                
                listap = []
                if str(node1) not in self.apexists:
                    listap.append(str(node1))
                if str(node2) not in self.apexists:
                    listap.append(str(node2))
                    
                if str(node1) not in self.apexists:
                    self.apexists.append(str(node1))
                if str(node2) not in self.apexists:
                    self.apexists.append(str(node2))
                        
            elif node1.type == 'accessPoint' and str(node1) not in self.apexists \
                or node2.type == 'accessPoint' and str(node2) not in self.apexists: 
                
                if self.firstAssociation:
                    self.configureWifiNodes(True)
                
                if node1.type == 'accessPoint':
                    ap = str(node1)
                    self.apexists.append(str(node1))
                else:
                    ap = str(node2)
                    self.apexists.append(str(node2))
                
            #necessary if does not exist link between sta and other device
            if node1 in self.missingStations:
                self.missingStations.remove(node1)
                node1.mode = 'g'
                node1.associate = False
            if node2 in self.missingStations:
                self.missingStations.remove(node2)
                node2.mode = 'g'
                node2.associate = False
            
            # Port is optional
            if port1 is not None:
                options.setdefault( 'port1', port1 )
            if port2 is not None:
                options.setdefault( 'port2', port2 )
            # Set default MAC - this should probably be in Link
            options.setdefault( 'addr1', self.randMac() )
            options.setdefault( 'addr2', self.randMac() )
            
            cls = self.link if cls is None else cls
            link = cls( node1, node2, **options )
            self.links.append( link )
            return link
    
    def configHosts( self ):
        "Configure a set of hosts."
        for host in self.hosts:
            #info( host.name + ' ' )
            intf = host.defaultIntf()
            if intf:
                host.configDefault()
            else:
                # Don't configure nonexistent intf
                host.configDefault( ip=None, mac=None )
           
            # You're low priority, dude!
            # BL: do we want to do this here or not?
            # May not make sense if we have CPU lmiting...
            # quietRun( 'renice +18 -p ' + repr( host.pid ) )
            # This may not be the right place to do this, but
            # it needs to be done somewhere.
        #info( '\n' )

    def buildFromTopo( self, topo=None ):
        """Build mininet from a topology object
           At the end of this function, everything should be connected
           and up."""
        station.printCon = False
        # Possibly we should clean up here and/or validate
        # the topo
        if self.cleanup:
            pass

        info( '*** Creating network\n' )
        if not self.controllers and self.controller:
            # Add a default controller
            info( '*** Adding controller\n' )
            classes = self.controller
            if not isinstance( classes, list ):
                classes = [ classes ]
            for i, cls in enumerate( classes ):
                # Allow Controller objects because nobody understands partial()
                if isinstance( cls, Controller ):
                    self.addController( cls )
                else:
                    self.addController( 'c%d' % i, cls )
                    
        info( '*** Adding hosts and stations:\n' )
        for hostName in topo.hosts():
            if 'sta' in str(hostName):
                self.addStation( hostName, **topo.nodeInfo( hostName ) )
            else:
                self.addHost( hostName, **topo.nodeInfo( hostName ) )
            info( hostName + ' ' )  
        
        info( '\n*** Adding switches and access point(s):\n' )
        for switchName in topo.switches():
            # A bit ugly: add batch parameter if appropriate
            params = topo.nodeInfo( switchName)
            cls = params.get( 'cls', self.switch )
            if hasattr( cls, 'batchStartup' ):
                params.setdefault( 'batch', True )
            if 'ap' in str(switchName):
                self.addBaseStation( switchName, **params )
            else:    
                self.addSwitch( switchName, **params )
            info( switchName + ' ' )
            
        info( '\n*** Adding links and associating station(s):\n' )
        for srcName, dstName, params in topo.links(
                sort=True, withInfo=True ):
            self.addLink( **params )
            info( '(%s, %s) ' % ( srcName, dstName ) )   
        info( '\n' )

    def configureControlNetwork( self ):
        "Control net config hook: override in subclass"
        raise Exception( 'configureControlNetwork: '
                         'should be overriden in subclass', self )

    def build( self ):
        module.isCode=True
        #useful if there no link between sta and any other device
        for s in self.missingStations:
            self.addMissingSTAs(s)
        
        "Build mininet."
        if self.topo:
            self.buildFromTopo( self.topo )
        if self.inNamespace:
            self.configureControlNetwork()
            info( '*** Configuring hosts\n' )       
        self.configHosts()
        if self.xterms:
            self.startTerms()
        if self.autoStaticArp:
            self.staticArp()
        self.built = True
            
    def startTerms( self ):
        "Start a terminal for each node."
        if 'DISPLAY' not in os.environ:
            error( "Error starting terms: Cannot connect to display\n" )
            return
        info( "*** Running terms on %s\n" % os.environ[ 'DISPLAY' ] )
        cleanUpScreens()
        self.terms += makeTerms( self.controllers, 'controller' )
        self.terms += makeTerms( self.switches, 'switch' )
        self.terms += makeTerms( self.hosts, 'host' )

    def stopXterms( self ):
        "Kill each xterm."
        for term in self.terms:
            os.kill( term.pid, signal.SIGKILL )
        cleanUpScreens()

    def staticArp( self ):
        "Add all-pairs ARP entries to remove the need to handle broadcast."
        for src in self.hosts:
            for dst in self.hosts:
                if src != dst:
                    src.setARP( ip=dst.IP(), mac=dst.MAC() )

    def start( self ):
        module.isCode = False
        "Start controller and switches."
        if not self.built:
            self.build()
        info( '*** Starting controller(s)\n' )
        for controller in self.controllers:
            info( controller.name + ' ')
            controller.start()
        info( '\n' )
        
        info( '*** Starting switches and access points\n' )
        for switch in self.switches:
            info( switch.name + ' ')
            switch.start( self.controllers )
            if 'ap' in switch.name:  
                os.system('ovs-vsctl add-br %s' % switch.name)
          
        started = {}
        for swclass, switches in groupby(
                sorted( self.switches, key=type ), type ):
            switches = tuple( switches )
            if hasattr( swclass, 'batchStartup' ):
                success = swclass.batchStartup( switches )
                started.update( { s: s for s in success } )
        
        #It is necessary to make a bridge between ap and wlan interface
        for switch in self.switches:
            if 'ap' in switch.name:  
                for iface in range(0, switch.nWlans):
                    interface = self.newapif[module.virtualWlan.index(switch.name+str(iface))]
                    accessPoint.apBridge(switch.name, interface)
        
        info( '\n' )
        if self.waitConn:
            self.waitConnected()

    def seed( self, seed ):
        "Seed"
        self.set_seed = seed

    def stop( self ):
        "Stop plotting"
        mobility.cancelPlot = True
        mobility.plotGraph = False
        mobility.DRAW = False
        mobility.ismobility = False
        try:
            mobility.closePlot()
        except:
            pass
        
        "Stop the controller(s), switches and hosts"
        info( '*** Stopping %i controllers\n' % len( self.controllers ) )
        for controller in self.controllers:
            info( controller.name + ' ' )
            controller.stop()
        info( '\n' )
                
        if self.terms:
            info( '*** Stopping %i terms\n' % len( self.terms ) )
            self.stopXterms()
        info( '*** Stopping %i links\n' % len( self.links ) )
        for link in self.links:
            info( '.' )
            link.stop()
        info( '\n' )
        info( '*** Stopping switches and access points\n' )
        stopped = {}
        for swclass, switches in groupby(
                sorted( self.switches, key=type ), type ):
            switches = tuple( switches )
            if hasattr( swclass, 'batchShutdown' ):
                success = swclass.batchShutdown( switches )
                stopped.update( { s: s for s in success } )
        for switch in self.switches:
            info( switch.name + ' ' )
            if switch not in stopped:
                switch.stop()
            switch.terminate()
        info( '\n' )
        info( '*** Stopping hosts and stations\n' )
        for host in self.hosts:
            info( host.name + ' ' )
            host.terminate()
        info( '\n' )
        if(module.isWiFi):
            module._stop_module() #Stopping WiFi Module
        info( '\n*** Done\n' )

    def run( self, test, *args, **kwargs ):
        "Perform a complete start/test/stop cycle."
        self.start()
        info( '*** Running test\n' )
        result = test( *args, **kwargs )
        self.stop()
        return result

    def monitor( self, hosts=None, timeoutms=-1 ):
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
            poller.register( host.stdout )
        while True:
            ready = poller.poll( timeoutms )
            for fd, event in ready:
                host = h1.fdToNode( fd )
                if event & select.POLLIN:
                    line = host.readline()
                    if line is not None:
                        yield host, line
            # Return if non-blocking
            if not ready and timeoutms >= 0:
                yield None, None

    @staticmethod
    def _parsePing( pingOutput ):
        "Parse ping output and return packets sent, received."
        # Check for downed link
        if 'connect: Network is unreachable' in pingOutput:
            return 1, 0
        r = r'(\d+) packets transmitted, (\d+) received'
        m = re.search( r, pingOutput )
        if m is None:
            error( '*** Error: could not parse ping output: %s\n' %
                   pingOutput )
            return 1, 0
        sent, received = int( m.group( 1 ) ), int( m.group( 2 ) )
        return sent, received

    def ping( self, hosts=None, timeout=None ):
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
            output( '*** Ping: testing ping reachability\n' )
        for node in hosts:
            output( '%s -> ' % node.name )
            for dest in hosts:
                if node != dest:
                    opts = ''
                    if timeout:
                        opts = '-W %s' % timeout
                    if dest.intfs:
                        result = node.cmd( 'ping -c1 %s %s' %
                                           (opts, dest.IP()) )
                        sent, received = self._parsePing( result )
                    else:
                        sent, received = 0, 0
                    packets += sent
                    if received > sent:
                        error( '*** Error: received too many packets' )
                        error( '%s' % result )
                        node.cmdPrint( 'route' )
                        exit( 1 )
                    lost += sent - received
                    output( ( '%s ' % dest.name ) if received else 'X ' )
            output( '\n' )
        if packets > 0:
            ploss = 100.0 * lost / packets
            received = packets - lost
            output( "*** Results: %i%% dropped (%d/%d received)\n" %
                    ( ploss, received, packets ) )
        else:
            ploss = 0
            output( "*** Warning: No packets sent\n" )
        return ploss

    @staticmethod
    def _parsePingFull( pingOutput ):
        "Parse ping output and return all data."
        errorTuple = (1, 0, 0, 0, 0, 0)
        # Check for downed link
        r = r'[uU]nreachable'
        m = re.search( r, pingOutput )
        if m is not None:
            return errorTuple
        r = r'(\d+) packets transmitted, (\d+) received'
        m = re.search( r, pingOutput )
        if m is None:
            error( '*** Error: could not parse ping output: %s\n' %
                   pingOutput )
            return errorTuple
        sent, received = int( m.group( 1 ) ), int( m.group( 2 ) )
        r = r'rtt min/avg/max/mdev = '
        r += r'(\d+\.\d+)/(\d+\.\d+)/(\d+\.\d+)/(\d+\.\d+) ms'
        m = re.search( r, pingOutput )
        if m is None:
            if received == 0:
                return errorTuple
            error( '*** Error: could not parse ping output: %s\n' %
                   pingOutput )
            return errorTuple
        rttmin = float( m.group( 1 ) )
        rttavg = float( m.group( 2 ) )
        rttmax = float( m.group( 3 ) )
        rttdev = float( m.group( 4 ) )
        return sent, received, rttmin, rttavg, rttmax, rttdev

    def pingFull( self, hosts=None, timeout=None ):
        """Ping between all specified hosts and return all data.
           hosts: list of hosts
           timeout: time to wait for a response, as string
           returns: all ping data; see function body."""
        # should we check if running?
        # Each value is a tuple: (src, dsd, [all ping outputs])
        all_outputs = []
        if not hosts:
            hosts = self.hosts
            output( '*** Ping: testing ping reachability\n' )
        for node in hosts:
            output( '%s -> ' % node.name )
            for dest in hosts:
                if node != dest:
                    opts = ''
                    if timeout:
                        opts = '-W %s' % timeout
                    result = node.cmd( 'ping -c1 %s %s' % (opts, dest.IP()) )
                    outputs = self._parsePingFull( result )
                    sent, received, rttmin, rttavg, rttmax, rttdev = outputs
                    all_outputs.append( (node, dest, outputs) )
                    output( ( '%s ' % dest.name ) if received else 'X ' )
            output( '\n' )
        output( "*** Results: \n" )
        for outputs in all_outputs:
            src, dest, ping_outputs = outputs
            sent, received, rttmin, rttavg, rttmax, rttdev = ping_outputs
            output( " %s->%s: %s/%s, " % (src, dest, sent, received ) )
            output( "rtt min/avg/max/mdev %0.3f/%0.3f/%0.3f/%0.3f ms\n" %
                    (rttmin, rttavg, rttmax, rttdev) )
        return all_outputs

    def pingAll( self, timeout=None ):
        """Ping between all hosts.
           returns: ploss packet loss percentage"""
        return self.ping( timeout=timeout )

    def pingPair( self ):
        """Ping between first two hosts, useful for testing.
           returns: ploss packet loss percentage"""
        hosts = [ self.hosts[ 0 ], self.hosts[ 1 ] ]
        return self.ping( hosts=hosts )

    def pingAllFull( self ):
        """Ping between all hosts.
           returns: ploss packet loss percentage"""
        return self.pingFull()

    def pingPairFull( self ):
        """Ping between first two hosts, useful for testing.
           returns: ploss packet loss percentage"""
        hosts = [ self.hosts[ 0 ], self.hosts[ 1 ] ]
        return self.pingFull( hosts=hosts )

    @staticmethod
    def _parseIperf( iperfOutput ):
        """Parse iperf output and return bandwidth.
           iperfOutput: string
           returns: result string"""
        r = r'([\d\.]+ \w+/sec)'
        m = re.findall( r, iperfOutput )
        if m:
            return m[-1]
        else:
            # was: raise Exception(...)
            error( 'could not parse iperf output: ' + iperfOutput )
            return ''

    def iperf( self, hosts=None, l4Type='TCP', udpBw='10M', fmt=None,
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
        assert len( hosts ) == 2
        client, server = hosts
        output( '*** Iperf: testing', l4Type, 'bandwidth between',
                client, 'and', server, '\n' )
        server.cmd( 'killall -9 iperf' )
        iperfArgs = 'iperf -p %d ' % port
        bwArgs = ''
        if l4Type == 'UDP':
            iperfArgs += '-u '
            bwArgs = '-b ' + udpBw + ' '
        elif l4Type != 'TCP':
            raise Exception( 'Unexpected l4 type: %s' % l4Type )
        if fmt:
            iperfArgs += '-f %s ' % fmt
        server.sendCmd( iperfArgs + '-s' )
        if l4Type == 'TCP':
            if not waitListening( client, server.IP(), port ):
                raise Exception( 'Could not connect to iperf on port %d'
                                 % port )
        cliout = client.cmd( iperfArgs + '-t %d -c ' % seconds +
                             server.IP() + ' ' + bwArgs )
        debug( 'Client output: %s\n' % cliout )
        server.sendInt()
        servout = server.waitOutput()
        debug( 'Server output: %s\n' % servout )
        result = [ self._parseIperf( servout ), self._parseIperf( cliout ) ]
        if l4Type == 'UDP':
            result.insert( 0, udpBw )
        output( '*** Results: %s\n' % result )
        return result

    def runCpuLimitTest( self, cpu, duration=5 ):
        """run CPU limit test with 'while true' processes.
        cpu: desired CPU fraction of each host
        duration: test duration in seconds (integer)
        returns a single list of measured CPU fractions as floats.
        """
        cores = int( quietRun( 'nproc' ) )
        pct = cpu * 100
        info( '*** Testing CPU %.0f%% bandwidth limit\n' % pct )
        hosts = self.hosts
        cores = int( quietRun( 'nproc' ) )
        # number of processes to run a while loop on per host
        num_procs = int( ceil( cores * cpu ) )
        pids = {}
        for h in hosts:
            pids[ h ] = []
            for _core in range( num_procs ):
                h.cmd( 'while true; do a=1; done &' )
                pids[ h ].append( h.cmd( 'echo $!' ).strip() )
        outputs = {}
        time = {}
        # get the initial cpu time for each host
        for host in hosts:
            outputs[ host ] = []
            with open( '/sys/fs/cgroup/cpuacct/%s/cpuacct.usage' %
                       host, 'r' ) as f:
                time[ host ] = float( f.read() )
        for _ in range( duration ):
            sleep( 1 )
            for host in hosts:
                with open( '/sys/fs/cgroup/cpuacct/%s/cpuacct.usage' %
                           host, 'r' ) as f:
                    readTime = float( f.read() )
                outputs[ host ].append( ( ( readTime - time[ host ] )
                                        / 1000000000 ) / cores * 100 )
                time[ host ] = readTime
        for h, pids in pids.items():
            for pid in pids:
                h.cmd( 'kill -9 %s' % pid )
        cpu_fractions = []
        for _host, outputs in outputs.items():
            for pct in outputs:
                cpu_fractions.append( pct )
        output( '*** Results: %s\n' % cpu_fractions )
        return cpu_fractions
         
    def mobility(self, *args, **kwargs):
        """ Mobility Parameters """
        self.node = args[0]
        self.stage = args[1]
        self.speed = 0
       
        for n in self.stations:
            if self.node == str(n):
                sta = n            
       
        if 'position' in kwargs:
            if(self.stage == 'stop'):
                self.position = kwargs['position']
                sta.endPosition = self.position.split(',')
            if(self.stage == 'start'):
                self.position = kwargs['position']
                sta.startPosition = self.position.split(',')    
                self.sta_inMov.append(self.node)
        if 'time' in kwargs:
            self.time = kwargs['time']
        if 'speed' in kwargs:
            sta.speed = kwargs['speed'][:-2]
            
        mobility.ismobility = True
        
        if(self.stage == 'start'):
            sta.startTime = self.time        
        elif(self.stage == 'stop'):
            sta.endTime = self.time 
            diffTime = sta.endTime - sta.startTime
            sta.moveSta = mobility.move(sta, diffTime, sta.speed, sta.startPosition, sta.endPosition)
        
    def startMobility(self, **kwargs):
        """ Starts Mobility """
        mobilityparam = dict()      
        if 'model' in kwargs:
            mobilityparam.setdefault( 'model', kwargs['model'] )
            self.model = kwargs['model']
        if 'max_x' in kwargs:
            mobilityparam.setdefault( 'max_x', kwargs['max_x'] )
        if 'max_y' in kwargs:
            mobilityparam.setdefault( 'max_y', kwargs['max_y'] )
        if 'min_v' in kwargs:
            mobilityparam.setdefault( 'min_v', kwargs['min_v'] )
        if 'max_v' in kwargs:
            mobilityparam.setdefault( 'max_v', kwargs['max_v'] )
        if 'aprange' in kwargs:
            mobilityparam.setdefault( 'manual_aprange', kwargs['aprange'] )
        if 'llf' in kwargs: #Least Loaded First
            mobilityparam.setdefault( 'llf', kwargs['llf'] )
        
        mobilityparam.setdefault( 'ismobility', True )
        mobilityparam.setdefault( 'seed', self.set_seed )
     
        if self.model != '':
            mobilityparam.setdefault( 'wifiNodes', self.wifiNodes )
            staMov = []
            for n in self.stations:
                if n not in station.fixedPosition:
                    staMov.append(n)
            mobilityparam.setdefault( 'n_staMov', len(staMov) )
            self.thread = threading.Thread(name='mobilityModel', target=mobility.models, kwargs=dict(mobilityparam,))
            self.thread.daemon = True
            self.thread.start()
           
        if 'startTime' in kwargs:
            self.start_time = kwargs['startTime']
        print "Mobility started at %s second(s)" % kwargs['startTime']
        
    def stopMobility(self, **kwargs):
        """ Stop Mobility """
        if 'stopTime' in kwargs:
            stop_time = kwargs['stopTime']
        self.thread = threading.Thread(name='onGoingMobility', target=self.onGoingMobility, args=(stop_time,))
        self.thread.daemon = True
        self.thread.start()
                
    def onGoingMobility(self, stop_time):
        """ ongoing Mobility """        
        t_end = time.time() + stop_time
        t_start = time.time() + self.start_time
        currentTime = time.time()
        i=1
        
        try:
            while time.time() < t_end and time.time() > t_start:
                if time.time() - currentTime >= i:
                    for sta in self.hosts:
                        if sta.type == 'station':
                            if str(sta) in self.sta_inMov:
                                if time.time() - currentTime >= sta.startTime and time.time() - currentTime <= sta.endTime:
                                    sta.startPosition[0] = float(sta.startPosition[0]) + float(sta.moveSta[0])
                                    sta.startPosition[1] = float(sta.startPosition[1]) + float(sta.moveSta[1])
                                    sta.startPosition[2] = float(sta.startPosition[2]) + float(sta.moveSta[2])
                            else:
                                sta.startPosition[0] = float(sta.startPosition[0])
                                sta.startPosition[1] = float(sta.startPosition[1])
                                sta.startPosition[2] = float(sta.startPosition[2])
                            sta.position = sta.startPosition
                            for ap in self.accessPoints:
                                distance = mobility.getDistance(sta, ap)
                                association.setInfraParameters(sta, ap, distance, '')
                    i+=1
        except:
            print 'Error! Mobility stopped!'
                   
    def plotGraph(self, **kwargs):
        """ Plot Graph """
        if 'max_x' in kwargs:
            mobility.MAX_X = kwargs['max_x']
        if 'max_y' in kwargs:
            mobility.MAX_Y = kwargs['max_y']
        mobility.plotGraph = True
        
    def getCurrentPosition(self, node):
        """ Get Current Position """ 
        try:
            for host in self.wifiNodes:
                if node == str(host):
                    mobility.printPosition(host)
        except:
            print ("Position was not defined")
                        
    def getCurrentDistance(self, src, dst):
        """ Get current Distance """ 
        existSrc = False
        existDst = False
        for host1 in self.wifiNodes:
            if src == str(host1):
                existSrc = True
                for host2 in self.wifiNodes:
                    if dst == str(host2):
                        existDst = True
                        mobility.printDistance(src, dst)
        try:               
            if(existSrc==False and existDst==False):
                print ("WiFi nodes %s and %s do not exist" % (src, dst))
            elif(existSrc==True and existDst==False):
                print ("WiFi node %s or %s does not exist" % (src, dst))
        except:
            print ("Station or Access Point does not exist!")
        
    # BL: I think this can be rewritten now that we have
    # a real link class.
    def configLinkStatus( self, src, dst, status ):
        """Change status of src <-> dst links.
           src: node name
           dst: node name
           status: string {up, down}"""
        if src not in self.nameToNode:
            error( 'src not in network: %s\n' % src )
        elif dst not in self.nameToNode:
            error( 'dst not in network: %s\n' % dst )
        else:
            if isinstance( src, basestring ):
                src = self.nameToNode[ src ]
            if isinstance( dst, basestring ):
                dst = self.nameToNode[ dst ]
            connections = src.connectionsTo( dst )
            if len( connections ) == 0:
                error( 'src and dst not connected: %s %s\n' % ( src, dst) )
            for srcIntf, dstIntf in connections:
                result = srcIntf.ifconfig( status )
                if result:
                    error( 'link src status change failed: %s\n' % result )
                result = dstIntf.ifconfig( status )
                if result:
                    error( 'link dst status change failed: %s\n' % result )

    def interact( self ):
        "Start network and run our simple CLI."        
        self.start()
        result = CLI( self )
        self.stop()
        return result

    inited = False

    @classmethod
    def init( cls ):
        "Initialize Mininet"
        if cls.inited:
            return
        ensureRoot()
        fixLimits()
        cls.inited = True


class MininetWithControlNet( Mininet ):

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

    def configureControlNetwork( self ):
        "Configure control network."
        self.configureRoutedControlNetwork()

    # We still need to figure out the right way to pass
    # in the control network location.

    def configureRoutedControlNetwork( self, ip='192.168.123.1',
                                       prefixLen=16 ):
        """Configure a routed control network on controller and switches.
           For use with the user datapath only right now."""
        controller = self.controllers[ 0 ]
        info( controller.name + ' <->' )
        cip = ip
        snum = ipParse( ip )
        for switch in self.switches:
            info( ' ' + switch.name )
            link = self.link( switch, controller, port1=0 )
            sintf, cintf = link.intf1, link.intf2
            switch.controlIntf = sintf
            snum += 1
            while snum & 0xff in [ 0, 255 ]:
                snum += 1
            sip = ipStr( snum )
            cintf.setIP( cip, prefixLen )
            sintf.setIP( sip, prefixLen )
            controller.setHostRoute( sip, cintf )
            switch.setHostRoute( cip, sintf )
        info( '\n' )
        info( '*** Testing control network\n' )
        while not cintf.isUp():
            info( '*** Waiting for', cintf, 'to come up\n' )
            sleep( 1 )
        for switch in self.switches:
            while not sintf.isUp():
                info( '*** Waiting for', sintf, 'to come up\n' )
                sleep( 1 )
            if self.ping( hosts=[ switch, controller ] ) != 0:
                error( '*** Error: control network test failed\n' )
                exit( 1 )
        info( '\n' )
