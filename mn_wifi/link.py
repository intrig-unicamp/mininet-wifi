"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

import os
import re
import subprocess
from time import sleep
from sys import version_info as py_version_info
from six import string_types

from mininet.log import info, error, debug
from mn_wifi.devices import GetRate
from mn_wifi.wmediumdConnector import DynamicWmediumdIntfRef, \
    w_starter, SNRLink, w_txpower, w_pos, \
    w_cst, w_server, ERRPROBLink, wmediumd_mode


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

    def pexec(self, *args, **kwargs):
        "Run a command in our owning node"
        return self.node.pexec(*args, **kwargs)

    def wpa_cmd(self, pidfile, intf, wlan):
        return self.pexec("wpa_supplicant -B -Dnl80211 -P %s "
                          "-i %s -c %s_%s.staconf"
                          % (pidfile, intf, self.node.name, wlan))

    def join(self, type=None, ssid=None, freq=None, ht_cap=None, intf=None):
        if type == 'ibss':
            return self.pexec('iw dev %s %s join %s %s %s 02:CA:FF:EE:BA:01'
                              % (self.name, type, ssid, freq, ht_cap))
        elif type == 'mesh':
            return self.pexec('iw dev %s %s join %s freq %s %s'
                            % (intf, type, ssid, freq, ht_cap))

    def setType(self, type=None, intf=None):
        if intf:
            return self.cmd('iw dev %s interface add %s type %s'
                            % (intf, self.name, type))
        else:
            return self.cmd('iw dev %s set type %s'
                            % (self.name, type))

    def setFreq(self, freq, intf=None):
        return self.cmd('iw dev %s set freq %s' % (intf, freq))

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
        if py_version_info < (3, 0):
            ips = self._ipMatchRegex.findall(ipAddr)
        else:
            ips = self._ipMatchRegex.findall(ipAddr.decode('utf-8'))
        self.ip = ips[0] if ips else None
        return self.ip

    def updateMAC(self):
        "Return updated MAC address based on ip addr"
        ipAddr = self.ipAddr()
        if py_version_info < (3, 0):
            macs = self._macMatchRegex.findall(ipAddr)
        else:
            macs = self._macMatchRegex.findall(ipAddr.decode('utf-8'))
        self.mac = macs[0] if macs else None
        return self.mac

    # Instead of updating ip and mac separately,
    # use one ipAddr call to do it simultaneously.
    # This saves an ipAddr command, which improves performance.

    def updateAddr(self):
        "Return IP address and MAC address based on ipAddr."
        ipAddr = self.ipAddr()
        if py_version_info < (3, 0):
            ips = self._ipMatchRegex.findall(ipAddr)
            macs = self._macMatchRegex.findall(ipAddr)
        else:
            ips = self._ipMatchRegex.findall(ipAddr.decode('utf-8'))
            macs = self._macMatchRegex.findall(ipAddr.decode('utf-8'))
        self.ip = ips[0] if ips else None
        self.mac = macs[0] if macs else None
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
        self.cmd('iw dev ' + self.name + ' del')
        # We used to do this, but it slows us down:
        # if self.node.inNamespace:
        # Link may have been dumped into root NS
        # quietRun( 'ip link del ' + self.name )
        #self.node.delIntf(self)
        self.link = None

    def status(self):
        "Return intf status as a string"
        links, _err, _result = self.node.pexec('ip link show')
        if self.name in str(links):
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
            delay_ = float(delay.replace("ms", ""))
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
        if "priomap" not in tcoutput and "noqueue" not in tcoutput \
                and "fq_codel" not in tcoutput and "qdisc fq" not in tcoutput:
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


class _4address(object):

    def __init__(self, node1, node2, port1=None, port2=None):
        """Create 4addr link to another node.
           node1: first node
           node2: second node
           intf: default interface class/constructor"""
        intf1 = None
        intf2 = None
        cls = IntfWireless

        ap = node1 # ap
        cl = node2 # client
        cl_intfName = '%s.wds' % cl.name

        if 'position' not in node1.params:
            nums = re.findall(r'\d+', node1.name)
            if nums:
                id = hex(int(nums[0]))[2:]
                node1.params['position'] = (10, round(id,2), 0)
        if 'position' not in node2.params:
            nums = re.findall(r'\d+', node2.name)
            if nums:
                id = hex(int(nums[0]))[2:]
                node2.params['position'] = (10, round(id,2), 0)

        if cl_intfName not in cl.params['wlan']:

            if port1:
                wlan = cl.params['wlan'].index(port1)
            else:
                wlan = 0

            if port2:
                apwlan = ap.params['wlan'].index(port2)
            else:
                apwlan = 0

            self.add4addrIface(cl, wlan, cl_intfName)
            cl.params['mac'].append(cl.params['mac'][wlan][:3] +
                                    '09' + cl.params['mac'][wlan][5:])
            self.setMAC(cl, wlan)
            self.bring4addrIfaceUP(cl)
            cl.func.append('client')
            ap.func.append('ap')

            cl.params['mode'].append(cl.params['mode'][apwlan])
            cl.params['channel'].append(cl.params['channel'][apwlan])
            cl.params['freq'].append(cl.params['freq'][apwlan])
            cl.params['txpower'].append(14)
            cl.params['antennaGain'].append(cl.params['antennaGain'][apwlan])
            cl.params['wlan'].append(cl_intfName)
            sleep(1)
            cl.cmd('iw dev %s connect %s %s' % (cl.params['wlan'][1],
                   ap.params['ssid'][apwlan], ap.params['mac'][apwlan]))

            params1 = {}
            params2 = {}
            params1[ 'port' ] = cl.newPort()
            params2[ 'port' ] = ap.newPort()
            intf1 = cls(name=cl_intfName, node=cl, link=self, **params1)
            if hasattr(ap, 'wds'):
                ap.wds += 1
            else:
                ap.wds = 1
            intfName2 = ap.params['wlan'][apwlan] + '.sta%s' % ap.wds
            intf2 = cls(name=intfName2, node=ap, link=self, **params2)
            ap.params['wlan'].append(intfName2)

        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2

    @classmethod
    def bring4addrIfaceUP(cls, node):
        node.cmd('ip link set dev %s.wds up' % node.name)

    @classmethod
    def setMAC(cls, node, wlan):
        nif = len(node.params['mac'])-1
        node.cmd('ip link set dev %s.wds addr %s'
                 % (node.name, node.params['mac'][wlan+nif]))

    @classmethod
    def add4addrIface(cls, node, wlan, intfName):
        node.cmd('iw dev %s interface add %s type managed 4addr on'
                 % (node.params['wlan'][wlan], intfName))

    def status(self):
        "Return link status as a string"
        return "(%s %s)" % (self.intf1.status(), self.intf2)

    def __str__(self):
        return '%s<->%s' % (self.intf1, self.intf2)

    def delete(self):
        "Delete this link"
        self.intf1.delete()
        self.intf1 = None
        self.intf2.delete()
        self.intf2 = None

    def stop(self):
        "Override to stop and clean up link as needed"
        self.delete()


class WirelessLinkAP(object):

    """A basic link is just a veth pair.
       Other types of links could be tunnels, link emulators, etc.."""

    # pylint: disable=too-many-branches
    def __init__(self, node1, port1=None,
                 intfName1=None, addr1=None,
                 intf=IntfWireless, cls1=None, params1=None):
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

        intf2 = 'wifi'
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
        self.intf2 = None

    def stop(self):
        "Override to stop and clean up link as needed"
        self.delete()

    def status(self):
        "Return link status as a string"
        return "(%s %s)" % (self.intf1.status(), self.intf2)

    def __str__(self):
        return '%s<->%s' % (self.intf1, self.intf2)


class WirelessLinkStation(object):

    """A basic link is just a veth pair.
       Other types of links could be tunnels, link emulators, etc.."""

    # pylint: disable=too-many-branches
    def __init__(self, node1, port1=None,
                 intfName1=None, addr1=None,
                 intf=IntfWireless, cls1=None, params1=None):
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
        intf2 = 'wifi'
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
        self.intf2 = None

    def stop(self):
        "Override to stop and clean up link as needed"
        self.delete()

    def status(self):
        "Return link status as a string"
        return "(%s %s)" % (self.intf1.status(), self.intf2)

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

    def __init__(self, fading_coefficient, noise_threshold, stations,
                 aps, cars, propagation_model, maclist=None):

        self.configureWmediumd(fading_coefficient, noise_threshold, stations,
                               aps, cars, propagation_model, maclist)

    @classmethod
    def configureWmediumd(cls, fading_coefficient, noise_threshold, stations,
                          aps, cars, propagation_model, maclist):
        "Configure wmediumd"
        intfrefs = []
        isnodeaps = []
        fading_coefficient = fading_coefficient
        noise_threshold = noise_threshold

        cls.nodes = stations + aps + cars
        for node in cls.nodes:
            node.wmIface = []
            for wlan in range(0, len(node.params['wlan'])):
                if wlan < len(node.params['mac']):
                    node.wmIface.append(wlan)
                    node.wmIface[wlan] = DynamicWmediumdIntfRef(node, intf=wlan)
                    intfrefs.append(node.wmIface[wlan])
                    if (node.func[wlan] == 'ap' or (node in aps
                                                    and node.func[wlan] is not 'client')):
                        isnodeaps.append(1)
                    else:
                        isnodeaps.append(0)
            for n in maclist:
                for key in n:
                    if key == node:
                        key.wmIface.append(DynamicWmediumdIntfRef(key, intf=len(key.wmIface)))
                        key.func.append('none')
                        key.params['wlan'].append(n[key][1])
                        key.params['mac'].append(n[key][0])
                        key.params['range'].append(0)
                        key.params['freq'].append(key.params['freq'][0])
                        key.params['antennaGain'].append(0)
                        key.params['txpower'].append(14)
                        intfrefs.append(key.wmIface[len(key.wmIface) - 1])
                        isnodeaps.append(0)

        if wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
            set_interference()
        elif wmediumd_mode.mode == w_cst.SPECPROB_MODE:
            spec_prob_link()
        elif wmediumd_mode.mode == w_cst.ERRPROB_MODE:
            set_error_prob()
        else:
            set_snr()
        start_wmediumd(intfrefs, wmediumd.links, wmediumd.positions,
                       fading_coefficient, noise_threshold,
                       wmediumd.txpowers, isnodeaps, propagation_model,
                       maclist)


class start_wmediumd(object):
    def __init__(cls, intfrefs, links, positions,
                 fading_coefficient, noise_threshold, txpowers, isnodeaps,
                 propagation_model, maclist):

        w_starter.start(intfrefs, links, pos=positions,
                              fading_coefficient=fading_coefficient,
                              noise_threshold=noise_threshold,
                              txpowers=txpowers, isnodeaps=isnodeaps,
                              ppm=propagation_model, maclist=maclist)


class set_interference(object):

    def __init__(self):
        self.interference()

    @classmethod
    def interference(cls):
        'configure interference model'
        for node in wmediumd.nodes:
            if 'position' not in node.params:
                posX = 0
                posY = 0
                posZ = 0
            else:
                posX = float(node.params['position'][0])
                posY = float(node.params['position'][1])
                posZ = float(node.params['position'][2])
            node.lastpos = [posX, posY, posZ]

            for wlan in range(0, len(node.params['wlan'])):
                if wlan >= 1:
                    posX+=0.1
                if wlan < len(node.params['mac']):
                    wmediumd.positions.append(w_pos(node.wmIface[wlan],
                                                    [posX, posY, posZ]))
                    wmediumd.txpowers.append(w_txpower(
                        node.wmIface[wlan], float(node.params['txpower'][wlan])))


class spec_prob_link(object):
    "wmediumd: spec prob link"
    def __init__(self):
        'do nothing'


class set_error_prob(object):
    "wmediumd: set error prob"
    def __init__(self):
        self.error_prob()

    @classmethod
    def error_prob(cls):
        "wmediumd: error prob"
        for node in wmediumd.wlinks:
            wmediumd.links.append(ERRPROBLink(node[0].wmIface[0],
                                              node[1].wmIface[0], node[2]))
            wmediumd.links.append(ERRPROBLink(node[1].wmIface[0],
                                              node[0].wmIface[0], node[2]))


class set_snr(object):
    "wmediumd: set snr"
    def __init__(self):
        self.snr()

    @classmethod
    def snr(cls):
        "wmediumd: snr"
        for node in wmediumd.wlinks:
            wmediumd.links.append(SNRLink(node[0].wmIface[0], node[1].wmIface[0],
                                          node[0].params['rssi'][0] - (-91)))
            wmediumd.links.append(SNRLink(node[1].wmIface[0], node[0].wmIface[0],
                                          node[0].params['rssi'][0] - (-91)))


class wirelessLink (object):

    dist = 0
    noise = 0
    equationLoss = '(dist * 2) / 1000'
    equationDelay = '(dist / 10) + 1'
    equationLatency = '(dist / 10)/2'
    equationBw = ' * (1.01 ** -dist)'
    ifb = False

    def __init__(self, sta=None, ap=None, dist=0, **params):
        """":param sta: station
        :param ap: access point
        :param wlan: wlan ID
        :param dist: distance between source and destination"""
        wlan = params['wlan']
        latency_ = self.getLatency(dist)
        loss_ = self.getLoss(dist)
        bw_ = self.getBW(sta=sta, ap=ap, dist=dist, **params)
        self.config_tc(sta, wlan, bw_, loss_, latency_)

    @classmethod
    def getDelay(cls, dist):
        "Based on RandomPropagationDelayModel"
        return eval(cls.equationDelay)

    @classmethod
    def getLatency(cls, dist):
        return eval(cls.equationLatency)

    @classmethod
    def getLoss(cls, dist):
        return eval(cls.equationLoss)

    @classmethod
    def getBW(cls, sta=None, ap=None, dist=0, **params):
        value = GetRate(sta=sta, ap=ap, **params)
        custombw = value.rate
        rate = eval(str(custombw) + cls.equationBw)

        if rate <= 0.0:
            rate = 0.1
        return rate

    @classmethod
    def delete(cls, node):
        "Delete interfaces"
        for wlan in node.params['wlan']:
            if isinstance(wlan, string_types):
                node.cmd('iw dev ' + wlan + ' del')
                node.delIntf(wlan)
                node.intf = None

    @classmethod
    def config_tc(cls, node, wlan, bw, loss, latency):
        """config_tc
        :param node: node
        :param wlan: wlan ID
        :param bw: bandwidth (mbps)
        :param loss: loss (%)
        :param latency: latency (ms)"""
        if cls.ifb:
            iface = 'ifb%s' % node.ifb[wlan]
            cls.tc(node, iface, bw, loss, latency)
        iface = node.params['wlan'][wlan]
        cls.tc(node, iface, bw, loss, latency)

    @classmethod
    def tc(cls, node, iface, bw, loss, latency):
        cmd = "tc qdisc replace dev %s root handle 2: netem " % iface
        rate = "rate %.4fmbit " % bw
        cmd += rate
        if latency > 0.1:
            latency = "latency %.2fms " % latency
            cmd += latency
        if loss > 0.1:
            loss = "loss %.1f%% " % loss
            cmd += loss
        node.pexec(cmd)


class ITSLink(IntfWireless):

    def __init__(self, node, port=None, physical=False, **params):
        "configure ieee80211p"
        if port:
            for port_ in node.params['wlan']:
                if params['port'] == port_:
                    wlan = node.params['wlan'].index(port_)
        else:
            wlan = node.ifaceToAssociate

        node.func[wlan] = 'its'
        node.params['freq'][wlan] = node.get_freq(wlan)
        node.setOCBIface(wlan)

        if not port:
            node.ifaceToAssociate += 1


class wifiDirectLink(IntfWireless):

    def __init__(self, node, port=None, physical=False, **params):
        "configure wifi-direct"
        if port:
            for port_ in node.params['wlan']:
                if params['port'] == port_:
                    wlan = node.params['wlan'].index(port_)
        else:
            wlan = node.ifaceToAssociate

        if 'position' not in node.params:
            nums = re.findall(r'\d+', node.name)
            if nums:
                id = hex(int(nums[0]))[2:]
                node.params['position'] = (10, round(id,2), 0)

        node.func[wlan] = 'wifiDirect'

        iface = None
        if physical:
            iface = 'phy' + node.name
            wlan = 0
        filename = self.get_filename(node, wlan, iface)
        self.config_(node, wlan, filename)

        if physical:
            iface = params['intf']
            cmd = self.get_wpa_cmd(filename, iface)
            os.system(cmd)
        else:
            iface = node.params['wlan'][wlan]
            cmd = self.get_wpa_cmd(filename, iface)
            node.cmd(self.get_wpa_cmd(filename, cmd))

        if not port:
            node.ifaceToAssociate += 1

    @classmethod
    def get_filename(cls, node, wlan, iface=None):
        suffix = '_wifiDirect.conf'
        filename = "mn%d_%s-%s_%s" % (os.getpid(), node, wlan, suffix)
        if iface:
            filename = "%s-%s_%s" % (iface, wlan, suffix)
        return filename

    @classmethod
    def get_wpa_cmd(cls, filename, intf):
        cmd = ('wpa_supplicant -B -Dnl80211 -c%s -i%s' %
               (filename, intf))
        return cmd

    @classmethod
    def config_(cls, node, wlan, filename):
        cmd = ("echo \'")
        cmd += 'ctrl_interface=/var/run/wpa_supplicant\
              \nap_scan=1\
              \np2p_go_ht40=1\
              \ndevice_name=%s-%s\
              \ndevice_type=1-0050F204-1\
              \np2p_no_group_iface=1' % (node, wlan)
        cmd += ("\' > %s" % filename)
        cls.set_config(cmd)

    @classmethod
    def set_config(cls, cmd):
        subprocess.check_output(cmd, shell=True)


class physicalWifiDirectLink(IntfWireless):

    def __init__(self, node, port=None, **params):
        "configure physical wifi-direct"
        wifiDirectLink(node, port, physical=True, **params)


class adhoc(IntfWireless):

    def __init__(self, node, link=None, **params):
        """Configure AdHoc
        node: name of the node
        self: custom association class/constructor
        params: parameters for station"""
        self.node = node
        if 'intf' in params:
            for intf_ in node.params['wlan']:
                if params['intf'] == intf_:
                    wlan = node.params['wlan'].index(intf_)
        else:
            wlan = node.ifaceToAssociate

        node.params['ssid'] = []
        self.name = node.params['wlan'][wlan]
        for _ in range(0, len(node.params['wlan'])):
            node.params['ssid'].append('')

        node.params['ssid'][wlan] = 'adhocNetwork'
        ssid = ("%s" % params.pop('ssid', {}))
        if ssid != "{}":
            node.params['ssid'][wlan] = ssid

        if 'channel' in params:
            node.setChannel(params['channel'], intf=node.params['wlan'][wlan])

        if link and link == wmediumd:
            params['enable_wmediumd'] = True
        self.configureAdhoc(node, wlan, **params)
        if 'intf' not in params:
            node.ifaceToAssociate += 1

    def configureAdhoc(self, node, wlan, **params):
        "Configure Wireless Ad Hoc"
        iface = node.params['wlan'][wlan]
        ip = node.params['ip'][wlan]
        node.func[wlan] = 'adhoc'
        node.setIP(ip, intf='%s' % iface)
        self.setType('ibss')
        if 'position' not in node.params or 'enable_wmediumd' in params:
            if 'passwd' in params:
                self.setSecuredAdhoc(node, wlan, **params)
            else:
                ssid = node.params['ssid'][wlan]
                freq = str(node.params['freq'][wlan]).replace('.', '')
                ht_cap = ''
                if 'ht_cap' in params:
                    ht_cap = params['ht_cap']
                self.join('ibss', ssid, freq, ht_cap)

    def setSecuredAdhoc(self, node, wlan, **params):
        "Set secured adhoc"
        cmd = 'ctrl_interface=/var/run/wpa_supplicant GROUP=wheel\n'
        cmd += 'ap_scan=2\n'
        cmd += 'network={\n'
        cmd += '         ssid="%s"\n' % node.params['ssid'][wlan]
        cmd += '         mode=1\n'
        cmd += '         frequency=%s\n' % str(node.params['freq'][wlan]).replace('.', '')
        cmd += '         proto=RSN\n'
        cmd += '         key_mgmt=WPA-PSK\n'
        cmd += '         pairwise=CCMP\n'
        cmd += '         group=CCMP\n'
        cmd += '         psk="%s"\n' % params['passwd']
        cmd += '}'

        fileName = '%s_%s.staconf' % (node.name, wlan)
        os.system('echo \'%s\' > %s' % (cmd, fileName))
        pidfile = "mn%d_%s_%s_wpa.pid" % (os.getpid(), node.name, wlan)
        intf = node.params['wlan'][wlan]
        self.wpa_cmd(pidfile, intf, wlan)


class mesh(IntfWireless):

    def __init__(self, node, isAP=False, **params):
        """Configure wireless mesh
        node: name of the node
        self: custom association class/constructor
        params: parameters for node"""
        self.name = ''
        self.node = node

        if 'intf' in params:
            for intf_ in node.params['wlan']:
                if params['intf'] == intf_:
                    wlan = node.params['wlan'].index(intf_)
        else:
            wlan = node.ifaceToAssociate

        if not isAP:
            node.params['ssid'] = []
            for _ in range(len(node.params['wlan'])):
                node.params['ssid'].append('')

        node.params['ssid'][wlan] = 'meshNetwork'
        ssid = ("%s" % params['ssid'])
        if ssid != "{}":
            node.params['ssid'][wlan] = ssid

        self.name = node.params['wlan'][wlan]
        self.setMeshIface(node, **params)

        if 'intf' not in params:
            node.ifaceToAssociate += 1

    def setMeshIface(self, node, ssid='', **params):
        wlan = node.params['wlan'].index(self.name)
        intf = node.params['wlan'][wlan]
        if node.func[wlan] == 'adhoc':
            self.setType('managed', intf)
        self.name = '%s-mp%s' % (node, wlan)
        if node.func[wlan] == 'mesh' and 'phyap' in params:
            self.name = '%s-mp%s' % (node, wlan+1)

        self.setType('mp', node.params['wlan'][wlan])
        self.setMAC(node.params['mac'][wlan])
        node.cmd('ip link set %s down' % intf)
        node.params['wlan'][wlan] = self.name

        if 'channel' in params:
            node.setChannel(params['channel'], intf=self.name)

        if 'mode' in params and (params['mode'] == 'a'
                                 or params['mode'] == 'ac'):
            node.pexec('iw reg set US')

        intf = node.params['wlan'][wlan]
        if 'freq' in params:
            freq = params['freq']
            self.setFreq(freq, intf)
            self.params['freq'][wlan] = freq

        if 'ip' in node.params:
            node.cmd('ip addr add %s dev %s' % (node.params['ip'][wlan],
                                                self.name))
        else:
            self.name = intf
        self.ipLink('up')

        if ssid != '':
            if 'ssid' not in node.params:
                node.params['ssid'] = []
                node.params['ssid'].append(0)
            node.params['ssid'][wlan] = ssid
            self.configureMesh(node, wlan, **params)

    def configureMesh(self, node, wlan, **params):
        "Configure Wireless Mesh Interface"
        node.func[wlan] = 'mesh'
        if 'passwd' in params:
            self.setSecuredMesh(node, wlan, **params)
        else:
            self.associate(node, wlan, **params)
        if node.params['wlan'][wlan] not in str(node.intf):
            TCLinkWirelessStation(node, intfName1=node.params['wlan'][wlan])

    def associate(self, node, wlan, **params):
        "Performs Mesh Association"
        name = node.params['wlan'][wlan]
        ssid = node.params['ssid'][wlan]
        freq = str(node.params['freq'][wlan]).replace('.','')
        ht_cap = ''
        if 'ht_cap' in params:
            ht_cap = params['ht_cap']
        self.join('mesh', ssid, freq, ht_cap, name)

    def setSecuredMesh(self, node, wlan, **params):
        "Set secured mesh"
        cmd = 'ctrl_interface=/var/run/wpa_supplicant\n'
        cmd += 'ctrl_interface_group=adm\n'
        cmd += 'user_mpm=1\n'
        cmd += 'network={\n'
        cmd += '         ssid="%s"\n' % node.params['ssid'][wlan]
        cmd += '         mode=5\n'
        cmd += '         frequency=%s\n' \
               % str(node.params['freq'][wlan]).replace('.', '')
        cmd += '         key_mgmt=SAE\n'
        cmd += '         psk="%s"\n' % params['passwd']
        cmd += '}'

        fileName = '%s_%s.staconf' % (node.name, wlan)
        os.system('echo \'%s\' > %s' % (cmd, fileName))
        pidfile = "mn%d_%s_%s_wpa.pid" % (os.getpid(), node.name, wlan)
        intf = node.params['wlan'][wlan]
        self.wpa_cmd(pidfile, intf, wlan)


class physicalMesh(IntfWireless):

    def __init__(self, node, isAP=False, **params):
        """Configure wireless mesh
        node: name of the node
        self: custom association class/constructor
        params: parameters for node"""
        wlan = node.ifaceToAssociate

        if not isAP:
            node.params['ssid'] = []
            for _ in range(len(node.params['wlan'])):
                node.params['ssid'].append('')

        node.params['ssid'][wlan] = 'meshNetwork'
        ssid = ("%s" % params['ssid'])
        if ssid != "{}":
            node.params['ssid'][wlan] = ssid

        if int(node.params['range'][wlan]) == 0:
            intf = node.params['wlan'][wlan]
            node.params['range'][wlan] = node.getRange(intf=intf, noiseLevel=95)

        params['phyap'] = True
        mesh.setMeshIface(node, node.params['wlan'][wlan], **params)
        self.setPhysicalMeshIface(node, **params)
        ssid = node.params['ssid'][wlan]
        freq = node.params['freq'][wlan]
        iface = node.params['wlan'][wlan]
        ht_cap = ''
        self.join('mesh', ssid, freq, ht_cap, iface)

        node.ifaceToAssociate += 1

    def setPhysicalMeshIface(self, node, **params):
        wlan = 0
        iface = 'phy%s-mp%s' % (node, wlan)
        os.system('ip link set %s down' % params['intf'])
        while True:
            id = ''
            command = 'ip link show | grep %s' % iface
            try:
                id = subprocess.check_output(command, shell=True).split("\n")
            except:
                pass
            if len(id) == 0:
                command = ('iw dev %s interface add %s type mp' %
                           (params['intf'], iface))
                subprocess.check_output(command, shell=True)
            else:
                try:
                    if 'channel' in params:
                        command = ('iw dev %s set channel %s' %
                                   (iface, str(params['channel'])))
                        subprocess.check_output(command, shell=True)
                    os.system('ip link set %s up' % iface)
                    debug("associating %s to %s...\n" %
                          (iface, node.params['ssid'][wlan]))
                    command = ('iw dev %s mesh join %s' %
                               (iface, node.params['ssid'][wlan]))
                    subprocess.check_output(command, shell=True)
                    break
                except:
                    break


class Association(object):

    bgscan = None

    @classmethod
    def setSNRWmediumd(cls, sta, ap, snr):
        "Send SNR to wmediumd"
        w_server.send_snr_update(SNRLink(sta.wmIface[0],
                                         ap.wmIface[0], snr))
        w_server.send_snr_update(SNRLink(ap.wmIface[0],
                                         sta.wmIface[0], snr))

    @classmethod
    def configureWirelessLink(cls, sta, ap, enable_wmediumd=False,
                              enable_interference=False, **params):
        """Updates RSSI and Others...

        :param sta: station
        :param ap: access point
        :param ap_wlan: AP wlan ID"""
        dist = sta.get_distance_to(ap)
        wlan = params['wlan']
        if dist <= ap.params['range'][0]:
            if not enable_interference:
                if sta.params['rssi'][wlan] == 0:
                    cls.updateParams(sta, ap, wlan)
            if ('doAssociation' in params and ap != sta.params['associatedTo'][wlan]) or \
                    (not sta.params['associatedTo'][wlan]
                     and ap not in sta.params['associatedTo']):
                cls.associate_infra(sta, ap, **params)
                if not enable_wmediumd:
                    if dist >= 0.01:
                        wirelessLink(sta, ap, dist, **params)
                if sta not in ap.params['associatedStations']:
                    ap.params['associatedStations'].append(sta)
            if not enable_interference:
                rssi = sta.set_rssi(ap, wlan, dist)
                sta.params['rssi'][wlan] = rssi
            if ap not in sta.params['apsInRange']:
                sta.params['apsInRange'].append(ap)
                if not enable_interference:
                    ap.params['stationsInRange'][sta] = rssi

    @classmethod
    def updateParams(cls, sta, ap, wlan):
        """
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID"""

        sta.params['freq'][wlan] = ap.get_freq(0)
        sta.params['channel'][wlan] = ap.params['channel'][0]
        sta.params['mode'][wlan] = ap.params['mode'][0]

    @classmethod
    def associate(cls, sta, ap, enable_wmediumd, enable_interference,
                  **params):
        "Associate to Access Point"
        if 'position' in sta.params:
            cls.configureWirelessLink(sta, ap, enable_wmediumd,
                                      enable_interference, **params)
        else:
            cls.associate_infra(sta, ap, **params)
        sta.ifaceToAssociate += 1

    @classmethod
    def associate_noEncrypt(cls, sta, ap, wlan, ap_wlan):
        """Association when there is no encrypt

        :param sta: station
        :param ap: access point
        :param wlan: wlan ID"""
        #iwconfig is still necessary, since iw doesn't include essid like iwconfig does.
        intf = sta.params['wlan'][wlan]
        ssid = ap.params['ssid'][ap_wlan]
        mac = ap.params['mac'][ap_wlan]
        debug('iwconfig %s essid %s ap %s\n' % (intf, ssid, mac))
        sta.pexec('iwconfig %s essid %s ap %s' % (intf, ssid, mac))

    @classmethod
    def associate_infra(cls, sta, ap, **params):
        """Association when infra

        :param sta: station
        :param ap: access point
        :param wlan: wlan ID"""
        wlan = params['wlan']
        ap_wlan = params['ap_wlan']
        associated = 0
        if 'ieee80211r' in ap.params and ap.params['ieee80211r'] == 'yes' \
        and ('encrypt' not in sta.params or 'encrypt' in sta.params and
             'wpa' in sta.params['encrypt'][wlan]):
            if not sta.params['associatedTo'][wlan]:
                command = ('ps -aux | grep %s | wc -l' % sta.params['wlan'][wlan])
                np = int(subprocess.check_output(command, shell=True))
                if np == 2:
                    cls.wpa(sta, ap, wlan, ap_wlan)
                else:
                    cls.handover_ieee80211r(sta, ap, wlan, ap_wlan)
            else:
                cls.handover_ieee80211r(sta, ap, wlan, ap_wlan)
            associated = 1
        elif 'encrypt' not in ap.params:
            associated = 1
            cls.associate_noEncrypt(sta, ap, wlan, ap_wlan)
        else:
            if not sta.params['associatedTo'][wlan]:
                if 'wpa' in ap.params['encrypt'][ap_wlan] \
                and ('encrypt' not in sta.params or 'encrypt' in sta.params and
                     'wpa' in sta.params['encrypt'][wlan]):
                    cls.wpa(sta, ap, wlan, ap_wlan)
                    associated = 1
                elif ap.params['encrypt'][ap_wlan] == 'wep':
                    cls.wep(sta, ap, wlan, ap_wlan)
                    associated = 1
        if 'printCon' in params:
            iface = sta.params['wlan'][wlan]
            info("Associating %s to %s\n" % (iface, ap))
        if associated:
            cls.update(sta, ap, wlan)

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
                    cmd += "   " + conf + "\n"
        else:
            cmd += '   ssid=\"%s\"\n' % ap.params['ssid'][ap_wlan]
            if 'authmode' not in ap.params:
                cmd += '   psk=\"%s\"\n' % passwd
                encrypt = ap.params['encrypt'][ap_wlan]
                if ap.params['encrypt'][ap_wlan] == 'wpa3':
                    encrypt = 'wpa2'
                cmd += '   proto=%s\n' % encrypt.upper()
                cmd += '   pairwise=%s\n' % ap.rsn_pairwise
                if 'active_scan' in sta.params and sta.params['active_scan'] == 1:
                    cmd += '   scan_ssid=1\n'
                if 'scan_freq' in sta.params and sta.params['scan_freq'][wlan]:
                    cmd += '   scan_freq=%s\n' % sta.params['scan_freq'][wlan]
                if 'freq_list' in sta.params and sta.params['freq_list'][wlan]:
                    cmd += '   freq_list=%s\n' % sta.params['freq_list'][wlan]
            wpa_key_mgmt = ap.wpa_key_mgmt
            if ap.params['encrypt'][ap_wlan] == 'wpa3':
                wpa_key_mgmt = 'SAE'
            cmd += '   key_mgmt=%s\n' % wpa_key_mgmt
            if cls.bgscan:
                cmd += '   %s\n' % cls.bgscan
            if 'authmode' in ap.params and ap.params['authmode'][0] == '8021x':
                cmd += '   eap=PEAP\n'
                cmd += '   identity=\"%s\"\n' % sta.params['radius_identity']
                cmd += '   password=\"%s\"\n' % sta.params['radius_passwd']
                cmd += '   phase2=\"autheap=MSCHAPV2\"\n'
        cmd += '}'

        fileName = '%s_%s.staconf' % (sta.name, wlan)
        os.system('echo \'%s\' > %s' % (cmd, fileName))

    @classmethod
    def wpa(cls, sta, ap, wlan, ap_wlan):
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
    def wep(cls, sta, ap, wlan, ap_wlan):
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
    def update(cls, sta, ap, wlan):
        """Updates attributes regarding association

        :param sta: station
        :param ap: access point
        :param wlan: wlan ID"""
        if sta.params['associatedTo'][wlan] != 'active_scan' \
            and sta.params['associatedTo'][wlan] != 'bgscan':
            if sta.params['associatedTo'][wlan] \
                    and sta in sta.params['associatedTo'][wlan].params['associatedStations']:
                sta.params['associatedTo'][wlan].params['associatedStations'].remove(sta)
            cls.updateParams(sta, ap, wlan)
            ap.params['associatedStations'].append(sta)
            sta.params['associatedTo'][wlan] = ap
