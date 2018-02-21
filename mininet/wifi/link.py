"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

import os
import re
import subprocess
from six import string_types

from mininet.log import info, error, debug
from mininet.link import Intf

from mininet.wifi.devices import deviceDataRate
from mininet.wifi.wmediumdConnector import DynamicWmediumdIntfRef, \
    WmediumdStarter, WmediumdSNRLink, WmediumdTXPower, WmediumdPosition, \
    WmediumdConstants, WmediumdServerConn, WmediumdERRPROBLink


class IntfWireless(object):

    "Basic interface object that can configure itself."

    def __init__(self, name, node=None, port=None, link=None,
                 mac=None, tc=False, **params):
        """name: interface name (e.g. h1-eth0)
           node: owning node (where this intf most likely lives)
           link: parent link if we're part of a link
           other arguments are passed to config()"""
        self.node = node
        self.name = name
        self.link = link
        self.port = port
        self.mac = mac
        self.ip, self.prefixLen = None, None
        # if interface is lo, we know the ip is 127.0.0.1.
        # This saves an ip link/addr command per node
        if self.name == 'lo':
            self.ip = '127.0.0.1'
        # Add to node (and move ourselves if necessary )
        moveIntfFn = params.pop('moveIntfFn', None)

        if tc is False:
            if moveIntfFn:
                node.addIntf(self, port=port, moveIntfFn=moveIntfFn)
            else:
                node.addIntf(self, port=port)

        # Save params for future reference
        self.params = params
        self.config(**params)

    def cmd(self, *args, **kwargs):
        "Run a command in our owning node"
        return self.node.cmd(*args, **kwargs)

    def ipAddr(self, *args):
        "Configure ourselves using ip link/addr"
        if self.name not in self.node.params['wlan']:
            self.cmd('ip addr flush ', self.name)
            return self.cmd('ip addr add', args[0], 'dev', self.name)
        else:
            if len(args) == 0:
                return self.cmd('ip addr show', self.name)
            else:
                self.cmd('ip addr flush ', self.name)
                return self.cmd('ip addr add', args[0], 'dev', self.name)

    def ipLink(self, *args):
        "Configure ourselves using ip link"
        return self.cmd('ip link set', self.name, *args)

    def setIP(self, ipstr, prefixLen=None):
        """Set our IP address"""
        # This is a sign that we should perhaps rethink our prefix
        # mechanism and/or the way we specify IP addresses
        if '/' in ipstr:
            self.ip, self.prefixLen = ipstr.split('/')
            return self.ipAddr(ipstr)
        else:
            if prefixLen is None:
                raise Exception('No prefix length set for IP address %s'
                                % (ipstr,))
            self.ip, self.prefixLen = ipstr, prefixLen
            return self.ipAddr('%s/%s' % (ipstr, prefixLen))

    def setMAC(self, macstr):
        """Set the MAC address for an interface.
           macstr: MAC address as string"""
        self.mac = macstr
        return (self.ipLink('down') +
                self.ipLink('address', macstr) +
                self.ipLink('up'))

    _ipMatchRegex = re.compile(r'\d+\.\d+\.\d+\.\d+')
    _macMatchRegex = re.compile(r'..:..:..:..:..:..')

    def updateIP(self):
        "Return updated IP address based on ip addr"
        # use pexec instead of node.cmd so that we dont read
        # backgrounded output from the cli.
        ipAddr, _err, _exitCode = self.node.pexec(
            'ip addr show %s' % self.name)
        ips = self._ipMatchRegex.findall(ipAddr)
        self.ip = ips[ 0 ] if ips else None
        return self.ip

    def updateMAC(self):
        "Return updated MAC address based on ip addr"
        ipAddr = self.ipAddr()
        macs = self._macMatchRegex.findall(ipAddr)
        self.mac = macs[ 0 ] if macs else None
        return self.mac

    # Instead of updating ip and mac separately,
    # use one ipAddr call to do it simultaneously.
    # This saves an ipAddr command, which improves performance.

    def updateAddr(self):
        "Return IP address and MAC address based on ip addr."
        ipAddr = self.ipAddr()
        ips = self._ipMatchRegex.findall(ipAddr)
        macs = self._macMatchRegex.findall(ipAddr)
        self.ip = ips[ 0 ] if ips else None
        self.mac = macs[ 0 ] if macs else None
        return self.ip, self.mac

    def IP(self):
        "Return IP address"
        return self.ip

    def MAC(self):
        "Return MAC address"
        return self.mac

    def isUp(self, setUp=False):
        "Return whether interface is up"
        if setUp:
            cmdOutput = self.ipLink('up')
            # no output indicates success
            if cmdOutput:
                # error( "Error setting %s up: %s " % ( self.name, cmdOutput ) )
                return False
            else:
                return True
        else:
            return "UP" in self.ipAddr()

    def rename(self, newname):
        "Rename interface"
        self.ipLink('down')
        result = self.cmd('ip link set', self.name, 'name', newname)
        self.name = newname
        self.ipLink('up')
        return result

    # The reason why we configure things in this way is so
    # That the parameters can be listed and documented in
    # the config method.
    # Dealing with subclasses and superclasses is slightly
    # annoying, but at least the information is there!

    def setParam(self, results, method, **param):
        """Internal method: configure a *single* parameter
           results: dict of results to update
           method: config method name
           param: arg=value (ignore if value=None)
           value may also be list or dict"""
        name, value = list(param.items())[ 0 ]
        f = getattr(self, method, None)
        if not f or value is None:
            return
        if isinstance(value, list):
            result = f(*value)
        elif isinstance(value, dict):
            result = f(**value)
        else:
            result = f(value)
        results[ name ] = result
        return result

    def config(self, mac=None, ip=None, ipAddr=None, up=True, **_params):
        """Configure Node according to (optional) parameters:
           mac: MAC address
           ip: IP address
           ipAddr: arbitrary interface configuration
           Subclasses should override this method and call
           the parent class's config(**params)"""
        # If we were overriding this method, we would call
        # the superclass config method here as follows:
        # r = Parent.config( **params )
        r = {}
        self.setParam(r, 'setMAC', mac=mac)
        self.setParam(r, 'setIP', ip=ip)
        self.setParam(r, 'isUp', up=up)
        self.setParam(r, 'ipAddr', ipAddr=ipAddr)

        return r

    def delete(self):
        "Delete interface"
        self.cmd('ip link del ' + self.name)
        # We used to do this, but it slows us down:
        # if self.node.inNamespace:
        # Link may have been dumped into root NS
        # quietRun( 'ip link del ' + self.name )
        self.node.delIntf(self)
        self.link = None

    def status(self):
        "Return intf status as a string"
        links, _err, _result = self.node.pexec('ip link show')
        if self.name in links:
            return "OK"
        else:
            return "MISSING"

    def __repr__(self):
        return '<%s %s>' % (self.__class__.__name__, self.name)

    def __str__(self):
        return self.name


class TCWirelessLink(IntfWireless):
    """Interface customized by tc (traffic control) utility
       Allows specification of bandwidth limits (various methods)
       as well as delay, loss and max queue length"""

    # The parameters we use seem to work reasonably up to 1 Gb/sec
    # For higher data rates, we will probably need to change them.
    bwParamMax = 1000

    def bwCmds(self, bw=None, speedup=0, use_hfsc=False, use_tbf=False,
               latency_ms=None, enable_ecn=False, enable_red=False):
        "Return tc commands to set bandwidth"
        cmds, parent = [], ' root '
        if bw and (bw < 0 or bw > self.bwParamMax):
            error('Bandwidth limit', bw, 'is outside supported range 0..%d'
                  % self.bwParamMax, '- ignoring\n')
        elif bw is not None:
            # BL: this seems a bit brittle...
            if speedup > 0:
                bw = speedup
            # This may not be correct - we should look more closely
            # at the semantics of burst (and cburst) to make sure we
            # are specifying the correct sizes. For now I have used
            # the same settings we had in the mininet-hifi code.
            if use_hfsc:
                cmds += [ '%s qdisc add dev %s root handle 5:0 hfsc default 1',
                          '%s class add dev %s parent 5:0 classid 5:1 hfsc sc '
                          + 'rate %fMbit ul rate %fMbit' % (bw, bw) ]
            elif use_tbf:
                if latency_ms is None:
                    latency_ms = 15 * 8 / bw
                cmds += [ '%s qdisc add dev %s root handle 5: tbf ' +
                          'rate %fMbit burst 15000 latency %fms' %
                          (bw, latency_ms) ]
            else:
                cmds += [ '%s qdisc add dev %s root handle 5:0 htb default 1',
                          '%s class add dev %s parent 5:0 classid 5:1 htb ' +
                          'rate %fMbit burst 15k' % bw ]
            parent = ' parent 5:1 '
            # ECN or RED
            if enable_ecn:
                cmds += [ '%s qdisc add dev %s' + parent +
                          'handle 6: red limit 1000000 ' +
                          'min 30000 max 35000 avpkt 1500 ' +
                          'burst 20 ' +
                          'bandwidth %fmbit probability 1 ecn' % bw ]
                parent = ' parent 6: '
            elif enable_red:
                cmds += [ '%s qdisc add dev %s' + parent +
                          'handle 6: red limit 1000000 ' +
                          'min 30000 max 35000 avpkt 1500 ' +
                          'burst 20 ' +
                          'bandwidth %fmbit probability 1' % bw ]
                parent = ' parent 6: '

        return cmds, parent

    @staticmethod
    def delayCmds(parent, delay=None, jitter=None,
                  loss=None, max_queue_size=None):
        "Internal method: return tc commands for delay and loss"
        cmds = []
        if delay:
            delay_ = int(delay.replace("ms", ""))
        if delay and delay_ < 0:
            error( 'Negative delay', delay, '\n' )
        elif jitter and jitter < 0:
            error('Negative jitter', jitter, '\n')
        elif loss and (loss < 0 or loss > 100):
            error('Bad loss percentage', loss, '%%\n')
        else:
            # Delay/jitter/loss/max queue size
            netemargs = '%s%s%s%s' % (
                'delay %s ' % delay if delay is not None else '',
                '%s ' % jitter if jitter is not None else '',
                'loss %.5f ' % loss if loss is not None else '',
                'limit %d' % max_queue_size if max_queue_size is not None
                else '')
            if netemargs:
                cmds = [ '%s qdisc add dev %s ' + parent +
                         ' handle 10: netem ' +
                         netemargs ]
                parent = ' parent 10:1 '
        return cmds, parent

    def tc(self, cmd, tc='tc'):
        "Execute tc command for our interface"
        c = cmd % (tc, self)  # Add in tc command and our name
        debug(" *** executing command: %s\n" % c)
        return self.cmd(c)

    def config(self, bw=None, delay=None, jitter=None, loss=None,
               gro=False, speedup=0, use_hfsc=False, use_tbf=False,
               latency_ms=None, enable_ecn=False, enable_red=False,
               max_queue_size=None, **params):
        """Configure the port and set its properties.
            bw: bandwidth in b/s (e.g. '10m')
            delay: transmit delay (e.g. '1ms' )
            jitter: jitter (e.g. '1ms')
            loss: loss (e.g. '1%' )
            gro: enable GRO (False)
            txo: enable transmit checksum offload (True)
            rxo: enable receive checksum offload (True)
            speedup: experimental switch-side bw option
            use_hfsc: use HFSC scheduling
            use_tbf: use TBF scheduling
            latency_ms: TBF latency parameter
            enable_ecn: enable ECN (False)
            enable_red: enable RED (False)
            max_queue_size: queue limit parameter for netem"""

        # Support old names for parameters
        gro = not params.pop('disable_gro', not gro)

        result = IntfWireless.config(self, **params)

        def on(isOn):
            "Helper method: bool -> 'on'/'off'"
            return 'on' if isOn else 'off'

        # Set offload parameters with ethool
        self.cmd('ethtool -K', self,
                 'gro', on(gro))

        # Optimization: return if nothing else to configure
        # Question: what happens if we want to reset things?
        if (bw is None and not delay and not loss
                and max_queue_size is None):
            return

        # Clear existing configuration
        tcoutput = self.tc('%s qdisc show dev %s')
        if "priomap" not in tcoutput and "noqueue" not in tcoutput:
            cmds = [ '%s qdisc del dev %s root' ]
        else:
            cmds = []
        # Bandwidth limits via various methods
        bwcmds, parent = self.bwCmds(bw=bw, speedup=speedup,
                                     use_hfsc=use_hfsc, use_tbf=use_tbf,
                                     latency_ms=latency_ms,
                                     enable_ecn=enable_ecn,
                                     enable_red=enable_red)
        cmds += bwcmds

        # Delay/jitter/loss/max_queue_size using netem
        delaycmds, parent = self.delayCmds(delay=delay, jitter=jitter,
                                           loss=loss,
                                           max_queue_size=max_queue_size,
                                           parent=parent)
        cmds += delaycmds

        # Execute all the commands in our node
        debug("at map stage w/cmds: %s\n" % cmds)
        tcoutputs = [ self.tc(cmd) for cmd in cmds ]
        for output in tcoutputs:
            if output != '':
                error("*** Error: %s" % output)
        debug("cmds:", cmds, '\n')
        debug("outputs:", tcoutputs, '\n')
        result[ 'tcoutputs'] = tcoutputs
        result[ 'parent' ] = parent

        return result

class _4addrLink(object):

    def __init__(self, node1, node2, intf=Intf):
        """Create 4addr link to another node.
           node1: first node
           node2: second node
           intf: default interface class/constructor
           """
        intf1 = None
        intf2 = None
        cls = intf

        ap = node1
        client = node2
        client_intfName = '%s.wds' % client.name

        if node1.params['_4addr'] == 'client':
            client = node1
            ap = node2
            client_intfName = '%s.wds' % node1.name

        if client_intfName not in client.params['wlan']:
            self.add4addrIface(client, client_intfName)
            client.params['mac'].append(client.params['mac'][0][:3] +
                                        '09' + client.params['mac'][0][5:])
            self.setMAC(client)
            self.bring4addrIfaceUP(client)
            client.func.append('client')
            ap.func.append('ap')

            ap.params['mode'].append(ap.params['mode'][0])
            ap.params['channel'].append(ap.params['channel'][0])
            ap.params['frequency'].append(ap.params['frequency'][0])
            ap.params['txpower'].append(14)
            ap.params['antennaGain'].append(ap.params['antennaGain'][0])

            client.params['mode'].append(node1.params['mode'][0])
            client.params['channel'].append(client.params['channel'][0])
            client.params['frequency'].append(client.params['frequency'][0])
            client.params['txpower'].append(14)
            client.params['antennaGain'].append(client.params['antennaGain'][0])
            client.params['wlan'].append(client_intfName)
            client.cmd('iw dev %s connect %s %s'
                       % (client.params['wlan'][1],
                          ap.params['ssid'][0], ap.params['mac'][0]))

            params1 = {}
            params2 = {}
            params1[ 'port' ] = client.newPort()
            params2[ 'port' ] = ap.newPort()
            intf1 = cls(name=client_intfName, node=client, link=self, **params1)
            if hasattr(ap, 'wds'):
                ap.wds += 1
            else:
                ap.wds = 1
            intfName2 = ap.params['wlan'][0] + '.sta%s' % ap.wds
            intf2 = cls(name=intfName2, node=ap, link=self, **params2)
            ap.params['wlan'].append(intfName2)
            ap.params['mac'].append(ap.params['mac'][0])

        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2

    @classmethod
    def bring4addrIfaceUP(cls, node):
        node.cmd('ip link set dev %s.wds up' % node.name)

    @classmethod
    def setMAC(cls, node):
        node.cmd('ip link set dev %s.wds addr %s'
                 % (node.name, node.params['mac'][1]))

    @classmethod
    def add4addrIface(cls, node, intfName):
        node.cmd('iw dev %s interface add %s type managed 4addr on'
                 % (node.params['wlan'][0], intfName))

class WirelessLinkAP(object):

    """A basic link is just a veth pair.
       Other types of links could be tunnels, link emulators, etc.."""

    # pylint: disable=too-many-branches
    def __init__(self, node1, port1=None,
                 intfName1=None, addr1=None,
                 intf=Intf, cls1=None, params1=None):
        """Create veth link to another node, making two new interfaces.
           node1: first node
           port1: node1 port number (optional)
           intf: default interface class/constructor
           cls1: optional interface-specific constructors
           intfName1: node1 interface name (optional)
           params1: parameters for interface 1"""
        # This is a bit awkward; it seems that having everything in
        # params is more orthogonal, but being able to specify
        # in-line arguments is more convenient! So we support both.

        if params1 is None:
            params1 = {}

        if port1 is not None:
            params1[ 'port' ] = port1

        ifacename = 'wlan'

        if 'port' not in params1:
            if intfName1 is None:
                nodelen = int(len(node1.params['wlan']))
                currentlen = node1.wlanports
                if nodelen > currentlen + 1:
                    params1[ 'port' ] = node1.newPort()
                else:
                    params1[ 'port' ] = currentlen
                intfName1 = self.wlanName(node1, ifacename, params1[ 'port' ])
                intf1 = cls1(name=intfName1, node=node1,
                             link=self, mac=addr1, **params1)
            else:
                params1[ 'port' ] = node1.newPort()
                node1.newPort()
                intf1 = cls1(name=intfName1, node=node1,
                             link=self, mac=addr1, **params1)
        else:
            intfName1 = self.wlanName(node1, ifacename, params1[ 'port' ])
            intf1 = cls1(name=intfName1, node=node1,
                         link=self, mac=addr1, **params1)

        if not intfName1:
            intfName1 = self.wlanName(node1, ifacename, node1.newWlanPort())

        if not cls1:
            cls1 = intf

        intf2 = 'wireless'
        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2
    # pylint: enable=too-many-branches

    @staticmethod
    def _ignore(*args, **kwargs):
        "Ignore any arguments"
        pass

    def wlanName(self, node, ifacename, n):
        "Construct a canonical interface name node-ethN for interface n."
        # Leave this as an instance method for now
        assert self
        return node.name + '-' + ifacename + repr(n)

    def delete(self):
        "Delete this link"
        self.intf1.delete()
        self.intf1 = None
        self.intf2.delete()
        self.intf2 = None

    def stop(self):
        "Override to stop and clean up link as needed"
        self.delete()

    def status(self):
        "Return link status as a string"
        return "(%s %s)" % (self.intf1.status(), self.intf2.status())

    def __str__(self):
        return '%s<->%s' % (self.intf1, self.intf2)


class WirelessLinkStation(object):

    """A basic link is just a veth pair.
       Other types of links could be tunnels, link emulators, etc.."""

    # pylint: disable=too-many-branches
    def __init__(self, node1, port1=None,
                 intfName1=None, addr1=None,
                 intf=Intf, cls1=None, params1=None):
        """Create veth link to another node, making two new interfaces.
           node1: first node
           port1: node1 port number (optional)
           intf: default interface class/constructor
           cls1: optional interface-specific constructors
           intfName1: node1 interface name (optional)
           params1: parameters for interface 1"""
        # This is a bit awkward; it seems that having everything in
        # params is more orthogonal, but being able to specify
        # in-line arguments is more convenient! So we support both.

        if params1 is None:
            params1 = {}

        if port1 is not None:
            params1[ 'port' ] = port1

        if 'port' not in params1:
            params1[ 'port' ] = node1.newPort()

        if not intfName1:
            ifacename = 'wlan'
            intfName1 = self.wlanName(node1, ifacename, node1.newWlanPort())

        if not cls1:
            cls1 = intf

        intf1 = cls1(name=intfName1, node=node1,
                     link=self, mac=addr1, **params1)
        intf2 = 'wireless'
        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2
    # pylint: enable=too-many-branches

    @staticmethod
    def _ignore(*args, **kwargs):
        "Ignore any arguments"
        pass

    def wlanName(self, node, ifacename, n):
        "Construct a canonical interface name node-ethN for interface n."
        # Leave this as an instance method for now
        assert self
        return node.name + '-' + ifacename + repr(n)

    def delete(self):
        "Delete this link"
        self.intf1.delete()
        self.intf1 = None
        self.intf2.delete()
        self.intf2 = None

    def stop(self):
        "Override to stop and clean up link as needed"
        self.delete()

    def status(self):
        "Return link status as a string"
        return "(%s %s)" % (self.intf1.status(), self.intf2.status())

    def __str__(self):
        return '%s<->%s' % (self.intf1, self.intf2)


class TCLinkWirelessStation(WirelessLinkStation):
    "Link with symmetric TC interfaces configured via opts"
    def __init__(self, node1, port1=None, intfName1=None,
                 addr1=None, **params):
        WirelessLinkStation.__init__(self, node1, port1=port1,
                                     intfName1=intfName1,
                                     cls1=TCWirelessLink,
                                     addr1=addr1,
                                     params1=params)

class TCLinkWirelessAP(WirelessLinkAP):
    "Link with symmetric TC interfaces configured via opts"
    def __init__(self, node1, port1=None, intfName1=None,
                 addr1=None, **params):
        WirelessLinkAP.__init__(self, node1, port1=port1,
                                intfName1=intfName1,
                                cls1=TCWirelessLink,
                                addr1=addr1,
                                params1=params)


class wmediumd(TCWirelessLink):
    "Wmediumd Class"
    wlinks = []
    links = []
    txpowers = []
    positions = []
    nodes = []

    def __init__(cls, wmediumd_mode, fading_coefficient, stations,
                 aps, propagation_model):

        cls.configureWmediumd(wmediumd_mode, fading_coefficient, stations,
                               aps, propagation_model)

    @classmethod
    def configureWmediumd(cls, wmediumd_mode, fading_coefficient, stations,
                          aps, propagation_model):
        "Configure wmediumd"
        from mininet.wifi.node import AP, Car

        intfrefs = []
        isnodeaps = []
        cars = []
        mode = wmediumd_mode
        fading_coefficient = fading_coefficient

        for node in stations:
            if 'carsta' in node.params:
                cars.append(node.params['carsta'])

        cls.nodes = stations + aps + cars
        for node in cls.nodes:
            node.wmIface = []
            if isinstance(node, Car):
                wlans = 1
            elif '_4addr' in node.params and node.params['_4addr'] == 'ap':
                wlans = 1
            else:
                wlans = len(node.params['wlan'])
            for wlan in range(0, wlans):
                node.wmIface.append(wlan)
                node.wmIface[wlan] = DynamicWmediumdIntfRef(node, intf=wlan)
                intfrefs.append(node.wmIface[wlan])
                if (node.func[wlan] == 'ap' or (isinstance(node, AP)
                                                and node.func[wlan] is not 'client')):
                    isnodeaps.append(1)
                else:
                    isnodeaps.append(0)

        if mode == 'interference':
            set_interference()
            mode = WmediumdConstants.INTERFERENCE_MODE
        elif mode == 'spec_prob_link':
            spec_prob_link()
            mode = WmediumdConstants.SPECPROB_MODE
        elif mode == 'error_prob':
            set_error_prob()
            mode = WmediumdConstants.ERRPROB_MODE
        else:
            set_snr()
            mode = WmediumdConstants.SNR_MODE
        start_wmediumd(intfrefs, wmediumd.links, mode, wmediumd.positions,
                       fading_coefficient, wmediumd.txpowers, isnodeaps,
                       propagation_model)


class start_wmediumd(object):
    def __init__(cls, intfrefs, links, mode, positions,
                 fading_coefficient, txpowers, isnodeaps,
                 propagation_model):

        WmediumdStarter.start(intfrefs, links, mode=mode, positions=positions,
                              fading_coefficient=fading_coefficient,
                              txpowers=txpowers, isnodeaps=isnodeaps,
                              ppm=propagation_model)


class set_interference(object):

    def __init__(cls):
        cls.interference()

    @classmethod
    def interference(cls):

        from mininet.wifi.node import Car
        for node in wmediumd.nodes:
            if 'position' not in node.params:
                posX = 0
                posY = 0
                posZ = 0
            else:
                posX = node.params['position'][0]
                posY = node.params['position'][1]
                posZ = node.params['position'][2]
            node.lastpos = [posX, posY, posZ]

            if isinstance(node, Car):
                wlans = 1
            elif '_4addr' in node.params and node.params['_4addr'] == 'ap':
                wlans = 1
            else:
                wlans = len(node.params['wlan'])

            for wlan in range(0, wlans):
                wmediumd.positions.append(WmediumdPosition(node.wmIface[wlan],
                                                           [posX, posY, posZ]))
                wmediumd.txpowers.append(WmediumdTXPower(
                    node.wmIface[wlan], float(node.params['txpower'][wlan])))


class spec_prob_link(object):
    "wmediumd: spec prob link"
    def __init__(cls):
        'do nothing'


class set_error_prob(object):
    "wmediumd: set error prob"
    def __init__(cls):
        cls.error_prob()

    @classmethod
    def error_prob(cls):
        "wmediumd: error prob"
        for node in wmediumd.wlinks:
            wmediumd.links.append(WmediumdERRPROBLink(node[0].wmIface[0],
                                                      node[1].wmIface[0], node[2]))
            wmediumd.links.append(WmediumdERRPROBLink(node[1].wmIface[0],
                                                      node[0].wmIface[0], node[2]))


class set_snr(object):
    "wmediumd: set snr"
    def __init__(cls):
        cls.snr()

    @classmethod
    def snr(cls):
        "wmediumd: snr"
        for node in wmediumd.wlinks:
            wmediumd.links.append(WmediumdSNRLink(node[0].wmIface[0], node[1].wmIface[0],
                                                  node[0].params['rssi'][0] - (-91)))
            wmediumd.links.append(WmediumdSNRLink(node[1].wmIface[0], node[0].wmIface[0],
                                                  node[0].params['rssi'][0] - (-91)))


class wirelessLink (object):

    dist = 0
    noise = 0
    equationLoss = '(dist * 2) / 1000'
    equationDelay = '(dist / 10) + 1'
    equationLatency = '(dist / 10)/2'
    equationBw = ' * (1.01 ** -dist)'
    ifb = False

    def __init__(self, sta=None, ap=None, wlan=0, ap_wlan=0, dist=0):
        """"
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        :param dist: distance between source and destination
        """

        delay_ = self.setDelay(dist)
        latency_ = self.setLatency(dist)
        loss_ = self.setLoss(dist)
        bw_ = self.setBW(sta=sta, ap=ap, dist=dist, wlan=wlan)
        self.tc(sta, wlan, bw_, loss_, latency_, delay_)

    @classmethod
    def setDelay(cls, dist):
        """"Based on RandomPropagationDelayModel

        :param dist: distance between source and destination
        """
        return eval(cls.equationDelay)

    @classmethod
    def setLatency(cls, dist):
        """
        :param dist: distance between source and destination
        """
        return eval(cls.equationLatency)

    @classmethod
    def setLoss(cls, dist):
        """
        :param dist: distance between source and destination
        """
        return eval(cls.equationLoss)

    @classmethod
    def setBW(cls, sta=None, ap=None, wlan=0, dist=0):
        """set BW
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        :param dist: distance between source and destination
        """
        value = deviceDataRate(sta, ap, wlan)
        custombw = value.rate
        rate = eval(str(custombw) + cls.equationBw)

        if rate <= 0.0:
            rate = 0.1
        return rate

    @classmethod
    def delete(cls, node):
        "Delete interfaces"
        from mininet.wifi.node import Car

        for wlan in node.params['wlan']:
            if isinstance(node, Car) and node.params['wlan'].index(wlan) == 1:
                node = node.params['carsta']
                wlan = node.params['wlan'][0]
            if isinstance(wlan, string_types):
                node.cmd('iw dev ' + wlan + ' del')
                node.delIntf(wlan)
                node.intf = None

    @classmethod
    def tc(cls, sta, wlan, bw, loss, latency, delay):
        """Applies TC

        :param sta: station
        :param wlan: wlan ID
        :param bw: bandwidth (mbps)
        :param loss: loss (%)
        :param latency: latency (ms)
        :param delay: delay (ms)"""
        # delay is not being used

        if cls.ifb:
            sta.pexec("tc qdisc replace dev ifb%s \
                        root handle 2: netem rate %.2fmbit \
                        loss %.1f%% \
                        latency %.2fms " % (sta.ifb[wlan], bw, loss, latency))
        if 'encrypt' in sta.params['associatedTo'][wlan].params:
            """tbf is applied to encrypt, cause we have noticed troubles 
            with wpa_supplicant and netem"""
            tc = 'tc qdisc replace dev %s root handle 1: tbf ' \
                 'rate %.2fmbit ' \
                 'burst 15000b ' \
                 'lat %.2fms' % (sta.params['wlan'][wlan], bw, latency)
            sta.pexec(tc)
        else:
            tc = "tc qdisc replace dev %s " \
                 "root handle 2: netem " \
                 "rate %.4fmbit " \
                 "loss %.1f%% " \
                 "latency %.2fms " % (sta.params['wlan'][wlan], bw, loss,
                                      latency)
            sta.pexec(tc)

class Association(object):

    printCon = True
    bgscan = ''

    @classmethod
    def setSNRWmediumd(cls, sta, ap, snr):
        "Send SNR to wmediumd"
        WmediumdServerConn.send_snr_update(WmediumdSNRLink(
            sta.wmIface[0], ap.wmIface[0], snr))
        WmediumdServerConn.send_snr_update(WmediumdSNRLink(
            ap.wmIface[0], sta.wmIface[0], snr))

    @classmethod
    def configureWirelessLink(cls, sta, ap, enable_wmediumd=False,
                              wmediumd_mode=False, ap_wlan=0):
        """Updates RSSI and Others...

        :param sta: station
        :param ap: access point
        :param ap_wlan: AP wlan ID"""

        dist = sta.get_distance_to(ap)
        if dist <= ap.params['range'][0]:
            for wlan in range(0, len(sta.params['wlan'])):
                if wmediumd_mode is not 'interference':
                    if sta.params['rssi'][wlan] == 0:
                        cls.updateParams(sta, ap, wlan)
                if sta.params['associatedTo'][wlan] == '' \
                        and ap not in sta.params['associatedTo']:
                    Association.associate_infra(sta, ap, wlan, ap_wlan)
                    if not enable_wmediumd:
                        if dist >= 0.01:
                            wirelessLink(sta, ap, wlan, ap_wlan, dist)
                    if sta not in ap.params['associatedStations']:
                        ap.params['associatedStations'].append(sta)
                if wmediumd_mode is not 'interference':
                    rssi = sta.set_rssi(ap, wlan, dist)
                    sta.params['rssi'][wlan] = rssi
            if ap not in sta.params['apsInRange']:
                sta.params['apsInRange'].append(ap)
                if wmediumd_mode is not 'interference':
                    ap.params['stationsInRange'][sta] = rssi

    @classmethod
    def updateParams(cls, sta, ap, wlan):
        """
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """

        sta.params['frequency'][wlan] = ap.get_freq(0)
        sta.params['channel'][wlan] = ap.params['channel'][0]
        sta.params['mode'][wlan] = ap.params['mode'][0]

    @classmethod
    def associate(cls, sta, ap, enable_wmediumd, wmediumd_mode, wlan=0, ap_wlan=0):
        "Associate to Access Point"
        if wlan == 0:
            wlan = sta.ifaceToAssociate
        if 'position' in sta.params:
            cls.configureWirelessLink(sta, ap, enable_wmediumd,
                                      wmediumd_mode, ap_wlan=0)
        else:
            cls.associate_infra(sta, ap, wlan=0, ap_wlan=0)
            sta.params['associatedTo'][wlan] = ap
            ap.params['associatedStations'].append(sta)
        sta.ifaceToAssociate += 1

    @classmethod
    def associate_noEncrypt(cls, sta, ap, wlan, ap_wlan):
        """Association when there is no encrypt

        :param sta: station
        :param ap: access point
        :param wlan: wlan ID"""
        #debug('iw dev %s connect %s %s\n'
        #      % (sta.params['wlan'][wlan], ap.params['ssid'][0], ap.params['mac'][0]))
        #sta.pexec('iw dev %s connect %s %s'
        #          % (sta.params['wlan'][wlan], ap.params['ssid'][0], ap.params['mac'][0]))
        #iwconfig is still necessary, since iw doesn't include essid like iwconfig does.
        debug('iwconfig %s essid %s ap %s\n' % (
            sta.params['wlan'][wlan], ap.params['ssid'][ap_wlan], ap.params['mac'][ap_wlan]))
        sta.pexec('iwconfig %s essid %s ap %s' % (
            sta.params['wlan'][wlan], ap.params['ssid'][ap_wlan], ap.params['mac'][ap_wlan]))

    @classmethod
    def associate_infra(cls, sta, ap, wlan, ap_wlan):
        """Association when infra

        :param sta: station
        :param ap: access point
        :param wlan: wlan ID"""
        if 'ieee80211r' in ap.params and ap.params['ieee80211r'] == 'yes':
            if sta.params['associatedTo'][wlan] == '':
                command = ('ps -aux | grep %s | wc -l' % sta.params['wlan'][wlan])
                np = int(subprocess.check_output(command, shell=True))
                if np == 2:
                    cls.associate_wpa(sta, ap, wlan, ap_wlan)
                else:
                    cls.handover_ieee80211r(sta, ap, wlan, ap_wlan)
            else:
                cls.handover_ieee80211r(sta, ap, wlan, ap_wlan)
        elif 'encrypt' not in ap.params:
            cls.associate_noEncrypt(sta, ap, wlan, ap_wlan)
        else:
            if sta.params['associatedTo'][wlan] == '':
                if ap.params['encrypt'][ap_wlan] == 'wpa' \
                        or ap.params['encrypt'][ap_wlan] == 'wpa2':
                    cls.associate_wpa(sta, ap, wlan, ap_wlan)
                elif ap.params['encrypt'][ap_wlan] == 'wep':
                    cls.associate_wep(sta, ap, wlan, ap_wlan)
        if cls.printCon:
            iface = sta.params['wlan'][wlan]
            info("Associating %s to %s\n" % (iface, ap))
        sta.params['frequency'][wlan] = ap.get_freq(0)
        sta.params['associatedTo'][wlan] = ap

    @classmethod
    def wpaFile(cls, sta, ap, wlan, ap_wlan):
        """creates wpa config file

        :param sta: station
        :param ap: access point
        :param wlan: wlan ID"""
        if 'config' not in ap.params or 'config' not in sta.params:
            if 'authmode' not in ap.params:
                if 'passwd' not in sta.params:
                    passwd = ap.params['passwd'][ap_wlan]
                else:
                    passwd = sta.params['passwd'][wlan]

        cmd = 'ctrl_interface=/var/run/wpa_supplicant\nnetwork={\n'

        if 'config' in sta.params:
            config = sta.params['config']
            if config is not []:
                config = sta.params['config'].split(',')
                sta.params.pop("config", None)
                for conf in config:
                    cmd = cmd + "   " + conf + "\n"
        else:
            cmd = cmd + '   ssid=\"%s\"\n' % ap.params['ssid'][ap_wlan]
            if 'authmode' not in ap.params:
                cmd = cmd + '   psk=\"%s\"\n' % passwd
                cmd = cmd + '   proto=%s\n' % ap.params['encrypt'][ap_wlan].upper()
                cmd = cmd + '   pairwise=%s\n' % ap.rsn_pairwise
                if 'active_scan' in sta.params and sta.params['active_scan'] == 1:
                    cmd = cmd + '   scan_ssid=1\n'
                if 'scan_freq' in sta.params:
                    cmd = cmd + '   scan_freq=%s\n' % sta.params['scan_freq'][wlan]
            cmd = cmd + '   key_mgmt=%s\n' % ap.wpa_key_mgmt
            if cls.bgscan != '':
                cmd = cmd + '   %s\n' % cls.bgscan
            if 'authmode' in ap.params and ap.params['authmode'] == '8021x':
                cmd = cmd + '   eap=PEAP\n'
                cmd = cmd + '   identity=\"%s\"\n' % sta.params['radius_identity']
                cmd = cmd + '   password=\"%s\"\n' % sta.params['radius_passwd']
                cmd = cmd + '   phase2=\"autheap=MSCHAPV2\"\n'
        cmd = cmd + '}'

        fileName = '%s_%s.staconf' % (sta.name, wlan)
        os.system('echo \'%s\' > %s' % (cmd, fileName))

    @classmethod
    def associate_wpa(cls, sta, ap, wlan, ap_wlan):
        """Association when WPA

        :param sta: station
        :param ap: access point
        :param wlan: wlan ID"""
        pidfile = "mn%d_%s_%s_wpa.pid" % (os.getpid(), sta.name, wlan)
        cls.wpaFile(sta, ap, wlan, ap_wlan)
        debug("wpa_supplicant -B -Dnl80211 -P %s -i %s -c %s_%s.staconf\n"
              % (pidfile, sta.params['wlan'][wlan], sta.name, wlan))
        sta.pexec("wpa_supplicant -B -Dnl80211 -P %s -i %s -c %s_%s.staconf"
                  % (pidfile, sta.params['wlan'][wlan], sta.name, wlan))

    @classmethod
    def handover_ieee80211r(cls, sta, ap, wlan, ap_wlan):
        debug('wpa_cli -i %s roam %s\n' % (sta.params['wlan'][wlan],
                                           ap.params['mac'][ap_wlan]))
        sta.pexec('wpa_cli -i %s roam %s' % (sta.params['wlan'][wlan],
                                             ap.params['mac'][ap_wlan]))

    @classmethod
    def associate_wep(cls, sta, ap, wlan, ap_wlan):
        """Association when WEP

        :param sta: station
        :param ap: access point
        :param wlan: wlan ID"""
        if 'passwd' not in sta.params:
            passwd = ap.params['passwd'][ap_wlan]
        else:
            passwd = sta.params['passwd'][wlan]
        sta.pexec('iw dev %s connect %s key d:0:%s' \
                % (sta.params['wlan'][wlan], ap.params['ssid'][ap_wlan], passwd))

    @classmethod
    def update_association(cls, sta, ap, wlan):
        """Updates attributes regarding association

        :param sta: station
        :param ap: access point
        :param wlan: wlan ID"""
        if sta.params['associatedTo'][wlan] != '' \
                and sta in sta.params['associatedTo'][wlan].params['associatedStations']:
            sta.params['associatedTo'][wlan].params['associatedStations'].remove(sta)
        cls.updateParams(sta, ap, wlan)
        sta.params['associatedTo'][wlan] = ap
