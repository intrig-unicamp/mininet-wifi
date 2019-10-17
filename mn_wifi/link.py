# author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)


import os
import re
import subprocess
from time import sleep
from sys import version_info as py_version_info
from six import string_types

from mininet.log import info, error, debug
from mn_wifi.devices import GetRate
from mn_wifi.manetRoutingProtocols import manetProtocols
from mn_wifi.wmediumdConnector import DynamicIntfRef, \
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
        self.ip, self.ip6, self.prefixLen = None, None, None
        # if interface is lo, we know the ip is 127.0.0.1.
        # This saves an ip link/addr command per node
        if self.name == 'lo':
            self.ip = '127.0.0.1'
        # Add to node (and move ourselves if necessary )
        if not tc:
            moveIntfFn = params.pop('moveIntfFn', None)
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

    @classmethod
    def get_intf(cls, node, wlan):
        return node.params['wlan'][wlan]

    @classmethod
    def get_mac(cls, node, wlan):
        return node.params['mac'][wlan]

    @classmethod
    def get_ssid(cls, node, wlan):
        return node.params['ssid'][wlan]

    def set_dev_type(self, type):
        self.iwdev_cmd('%s set type %s' % (self.name, type))

    def add_dev_type(self, new_name, type):
        self.iwdev_cmd('%s interface add %s type %s' % (self.name, new_name, type))

    def iwdev_cmd(self, *args):
        return self.cmd('iw dev', *args)

    def iwdev_pexec(self, *args):
        return self.pexec('iw dev', *args)

    def join_ibss(self, ssid, freq, ht_cap):
        return self.iwdev_cmd('{} ibss join {} {} {} 02:CA:FF:EE:BA:01'.
                              format(self.name, ssid, freq, ht_cap))

    def join_mesh(self, ssid, freq, ht_cap, intf):
        return self.iwdev_cmd('{} mesh join {} freq {} {}'.
                              format(intf, ssid, freq, ht_cap))

    def setFreq(self, freq, intf=None):
        return self.iwdev_cmd('{} set freq {}' % (intf, freq))

    def get_freq(self, freq):
        return format(freq, '.3f').replace('.', '')

    def setChanParam(self, channel, wlan):
        self.node.params['channel'][wlan] = str(channel)
        self.node.params['freq'][wlan] = self.node.get_freq(wlan)

    def setModeParam(self, mode, wlan):
        if mode == 'a' or mode == 'ac':
            self.pexec('iw reg set US')
        self.node.params['mode'][wlan] = mode

    def setChannel(self, channel, wlan, AP=None):
        self.setChanParam(channel, wlan)
        if AP and self.node.func[wlan] != 'mesh':
            self.pexec(
                'hostapd_cli -i %s chan_switch %s %s' % (
                    self.name, str(channel),
                    str(self.node.params['freq'][wlan]).replace(".", "")))
        else:
            self.iwdev_cmd('%s set channel %s'
                           % (self.node.params['wlan'][wlan],
                              str(channel)))

    def ipAddr(self, *args):
        "Configure ourselves using ip link/addr"
        if self.name not in self.node.params['wlan']:
            self.cmd('ip addr flush ', self.name)
            return self.cmd('ip addr add', args[0], 'dev', self.name)
        else:
            if len(args) == 0:
                return self.cmd('ip addr show', self.name)
            else:
                if ':' not in args[0]:
                    self.cmd('ip addr flush ', self.name)
                    cmd = 'ip addr add %s dev %s' % (args[0], self.name)
                    if self.ip6:
                        cmd = cmd + ' && ip -6 addr add %s dev %s' % \
                                    (self.ip6, self.name)
                    return self.cmd(cmd)
                else:
                    self.cmd('ip -6 addr flush ', self.name)
                    return self.cmd('ip -6 addr add', args[0], 'dev', self.name)

    def ipLink(self, *args):
        "Configure ourselves using ip link"
        return self.cmd('ip link set', self.name, *args)

    def setIP(self, ipstr, prefixLen=None, **args):
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

    def setIPv6(self, ipstr, prefixLen=None, **args):
        """Set our IP address"""
        # This is a sign that we should perhaps rethink our prefix
        # mechanism and/or the way we specify IP addresses
        if '/' in ipstr:
            self.ip6, self.prefixLen = ipstr.split('/')
            return self.ipAddr(ipstr)
        else:
            if prefixLen is None:
                raise Exception('No prefix length set for IP address %s'
                                % (ipstr,))
            self.ip6, self.prefixLen = ipstr, prefixLen
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
        if self.node and self.name in self.node.nameToIntf:
            # rename intf in node's nameToIntf
            self.node.nameToIntf[newname] = self.node.nameToIntf.pop(self.name)
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


class _4address(IntfWireless):

    node = None

    def __init__(self, node1, node2, port1=None, port2=None):
        """Create 4addr link to another node.
           node1: first node
           node2: second node
           intf: default interface class/constructor"""
        intf1 = None
        intf2 = None

        ap = node1 # ap
        cl = node2 # client
        cl_intfName = '%s.wds' % cl.name

        if 'position' not in node1.params:
            nums = re.findall(r'\d+', node1.name)
            if nums:
                id = hex(int(nums[0]))[2:]
                node1.params['position'] = (10, round(id, 2), 0)
        if 'position' not in node2.params:
            nums = re.findall(r'\d+', node2.name)
            if nums:
                id = hex(int(nums[0]))[2:]
                node2.params['position'] = (10, round(id, 2), 0)

        if cl_intfName not in cl.params['wlan']:
            if port1:
                wlan = cl.params['wlan'].index(port1)
            else:
                wlan = 0

            if port2:
                apwlan = ap.params['wlan'].index(port2)
            else:
                apwlan = 0

            self.node = cl
            self.add4addrIface(wlan, cl_intfName)
            cl.params['mac'].append(cl.params['mac'][wlan][:3] +
                                    '09' + cl.params['mac'][wlan][5:])
            self.setMAC(wlan)
            self.bring4addrIfaceUP()
            cl.func.append('client')
            ap.func.append('ap')

            cl.params['mode'].append(cl.params['mode'][apwlan])
            cl.params['channel'].append(cl.params['channel'][apwlan])
            cl.params['freq'].append(cl.params['freq'][apwlan])
            cl.params['txpower'].append(14)
            cl.params['antennaGain'].append(cl.params['antennaGain'][apwlan])
            cl.params['wlan'].append(cl_intfName)
            sleep(1)
            self.iwdev_cmd('%s connect %s %s' % (cl.params['wlan'][1],
                                                 ap.params['ssid'][apwlan],
                                                 ap.params['mac'][apwlan]))

            params1 = {}
            params2 = {}
            params1['port'] = cl.newPort()
            params2['port'] = ap.newPort()
            intf1 = IntfWireless(name=cl_intfName, node=cl, link=self, **params1)
            if hasattr(ap, 'wds'):
                ap.wds += 1
            else:
                ap.wds = 1
            intfName2 = ap.params['wlan'][apwlan] + '.sta%s' % ap.wds
            intf2 = IntfWireless(name=intfName2, node=ap, link=self, **params2)
            ap.params['wlan'].append(intfName2)

        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2

    def bring4addrIfaceUP(self):
        self.cmd('ip link set dev %s.wds up' % self.node)

    def setMAC(self, wlan):
        nif = len(self.node.params['mac'])-1
        self.cmd('ip link set dev %s.wds addr %s'
                 % (self.node, self.node.params['mac'][wlan+nif]))

    def add4addrIface(self, wlan, intfName):
        self.iwdev_cmd('%s interface add %s type managed 4addr on' %
                       (self.node.params['wlan'][wlan], intfName))

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
                    node.wmIface[wlan] = DynamicIntfRef(node, intf=wlan)
                    intfrefs.append(node.wmIface[wlan])
                    if (node.func[wlan] == 'ap'
                        or (node in aps and (node.func[wlan] is not 'client'
                                             and node.func[wlan] is not 'adhoc'))):
                        isnodeaps.append(1)
                    else:
                        isnodeaps.append(0)
            for n in maclist:
                for key in n:
                    if key == node:
                        key.wmIface.append(DynamicIntfRef(key, intf=len(key.wmIface)))
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
                    posX += 0.1
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


class wirelessLink(object):

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

    def getDelay(self, dist):
        "Based on RandomPropagationDelayModel"
        return eval(self.equationDelay)

    def getLatency(self, dist):
        return eval(self.equationLatency)

    def getLoss(self, dist):
        return eval(self.equationLoss)

    def getBW(self, sta=None, ap=None, dist=0, **params):
        value = GetRate(sta=sta, ap=ap, **params)
        custombw = value.rate
        rate = eval(str(custombw) + self.equationBw)

        if rate <= 0.0:
            rate = 0.1
        return rate

    @classmethod
    def delete(cls, node):
        "Delete interfaces"
        for intf in node.params['wlan']:
            if isinstance(intf, string_types):
                node.cmd('iw dev ' + intf + ' del')
                node.delIntf(intf)
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

    def __init__(self, node, intf=None, channel=161):
        "configure ieee80211p"
        self.node = node

        if intf:
            wlan = node.params['wlan'].index(intf)
        else:
            wlan = 0
            intf = node.params['wlan'][wlan]

        if node.func[wlan] == 'ap':
            self.kill_hostapd(node, intf)

        node.params['channel'][wlan] = channel
        node.params['freq'][wlan] = node.get_freq(wlan)
        self.name = intf
        if node.func[wlan] == 'ap':
            intf = '%s-ocb' % node.name
            self.add_ocb_mode(intf)
        else:
            self.set_ocb_mode()
        self.configure_ocb(wlan)
        node.func[wlan] = 'its'

    def kill_hostapd(self, node, intf):
        node.cmd('iw dev %s set type managed' % intf)
        apconfname = "mn%d_%s.apconf" % (os.getpid(), intf)
        node.cmd('rm %s' % apconfname)
        node.cmd('pkill -f \'%s\'' % apconfname)

    def add_ocb_mode(self, new_name):
        "Set OCB Interface"
        wlan = self.node.params['wlan'].index(self.name)
        self.ipLink('down')
        self.node.delIntf(self.name)
        self.add_dev_type(new_name, 'ocb')
        # we set the port to remove the existing wlan from node.intfs
        IntfWireless(name=new_name, node=self.node, port=1)
        self.name = new_name
        self.setMAC(self.node.params['mac'][wlan])
        self.ipLink('up')

    def set_ocb_mode(self):
        "Set OCB Interface"
        self.ipLink('down')
        self.set_dev_type('ocb')
        self.ipLink('up')

    def configure_ocb(self, wlan):
        "Configure Wireless OCB"
        freq = str(self.node.params['freq'][wlan]).replace(".", "")
        self.iwdev_cmd('%s ocb join %s 20MHz' % (self.name, freq))


class wifiDirectLink(IntfWireless):

    def __init__(self, node, intf=None):
        "configure wifi-direct"
        if intf:
            wlan = node.params['wlan'].index(intf)
        else:
            wlan = 0
            intf = node.params['wlan'][wlan]

        node.func[wlan] = 'wifiDirect'

        iface = None
        filename = self.get_filename(node, wlan, iface)
        self.config_(node, wlan, filename)

        cmd = self.get_wpa_cmd(filename, intf)
        node.cmd(cmd)

    @classmethod
    def get_filename(cls, node, wlan, iface=None):
        suffix = 'wifiDirect.conf'
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


class physicalWifiDirectLink(wifiDirectLink):

    def __init__(self, node, intf=None):
        "configure wifi-direct"
        wlan = 0
        node.func[wlan] = 'wifiDirect'

        filename = self.get_filename(node, wlan, intf)
        self.config_(node, wlan, filename)

        cmd = self.get_wpa_cmd(filename, intf)
        os.system(cmd)


class adhoc(IntfWireless):

    node = None

    def __init__(self, node, intf=None, ssid='adhocNet',
                 channel=1, mode='g', passwd=None, ht_cap='',
                 proto=None, **params):
        """Configure AdHoc
        node: name of the node
        self: custom association class/constructor
        params: parameters for station"""
        self.node = node
        if intf:
            wlan = node.params['wlan'].index(intf)
            if 'mp' in intf:
                self.iwdev_cmd('%s del' % node.params['wlan'][wlan])
                node.params['wlan'][wlan] = intf.replace('mp', 'wlan')
        else:
            wlan = 0
            intf = node.params['wlan'][wlan]

        self.name = node.params['wlan'][wlan]
        if 'ssid' not in node.params:
            node.params['ssid'] = []
            for _ in range(0, len(node.params['wlan'])):
                node.params['ssid'].append('')

        node.params['ssid'][wlan] = ssid
        self.setChanParam(channel, wlan)
        self.setModeParam(mode, wlan)
        self.configureAdhoc(node, wlan, passwd, ht_cap)

        if proto:
            manetProtocols(proto, node, wlan, **params)

    def configureAdhoc(self, node, wlan, passwd, ht_cap):
        "Configure Wireless Ad Hoc"
        node.func[wlan] = 'adhoc'
        self.set_dev_type('ibss')
        self.ipLink('up')

        if passwd:
            self.setSecuredAdhoc(node, wlan, passwd)
        else:
            ssid = node.params['ssid'][wlan]
            freq = self.get_freq(node.params['freq'][wlan])
            self.join_ibss(ssid, freq, ht_cap)

    def setSecuredAdhoc(self, node, wlan, passwd):
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
        cmd += '         psk="%s"\n' % passwd
        cmd += '}'

        fileName = '%s_%s.staconf' % (node.name, wlan)
        os.system('echo \'%s\' > %s' % (cmd, fileName))
        pidfile = "mn%d_%s_%s_wpa.pid" % (os.getpid(), node.name, wlan)
        intf = node.params['wlan'][wlan]
        node.wpa_cmd(pidfile, intf, wlan)


class mesh(IntfWireless):

    node = None

    def __init__(self, node, intf=None, mode='g', channel=1,
                 ssid='meshNet', passwd=None, ht_cap=''):
        """Configure wireless mesh
        node: name of the node
        self: custom association class/constructor
        params: parameters for node"""
        self.name = ''
        self.node = node

        if intf:
            wlan = node.params['wlan'].index(intf)
        else:
            wlan = 0
            intf = node.params['wlan'][wlan]

        node.params['ssid'][wlan] = ssid
        self.setMeshIface(node, mode, channel, wlan, intf)
        self.configureMesh(node, wlan, ssid, ht_cap, passwd)

    def set_mesh_type(self, intf):
        return '%s interface add %s type mp' % (intf, self.name)

    def setMeshIface(self, node, mode, channel, wlan, intf):
        if node.func[wlan] == 'adhoc':
            self.set_dev_type('managed')
        self.name = '%s-mp%s' % (node, node.params['wlan'][wlan][-1:])
        self.iwdev_cmd(self.set_mesh_type(intf))
        node.cmd('ip link set %s down' % intf)
        self.setMAC(node.params['mac'][wlan])
        node.params['wlan'][wlan] = self.name

        self.setChanParam(channel, wlan)
        self.setChannel(channel, wlan)
        self.setModeParam(mode, wlan)

        if 'ip' in node.params:
            node.cmd('ip addr add %s dev %s' % (node.params['ip'][wlan],
                                                self.name))
        else:
            self.name = intf
        self.ipLink('up')

    def configureMesh(self, node, wlan, ssid, ht_cap, passwd):
        "Configure Wireless Mesh Interface"
        node.func[wlan] = 'mesh'
        if passwd:
            self.setSecuredMesh(node, wlan, passwd)
        else:
            self.associate(node, wlan, ssid, ht_cap)
        if node.params['wlan'][wlan] not in str(node.intf):
            TCLinkWirelessStation(node, intfName1=node.params['wlan'][wlan])

    def associate(self, node, wlan, ssid, ht_cap):
        "Performs Mesh Association"
        name = node.params['wlan'][wlan]
        freq = self.get_freq(node.params['freq'][wlan])
        self.join_mesh(ssid, freq, ht_cap, name)

    def setSecuredMesh(self, node, wlan, passwd):
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
        cmd += '         psk="%s"\n' % passwd
        cmd += '}'

        fileName = '%s_%s.staconf' % (node.name, wlan)
        os.system('echo \'%s\' > %s' % (cmd, fileName))
        pidfile = "mn%d_%s_%s_wpa.pid" % (os.getpid(), node.name, wlan)
        intf = node.params['wlan'][wlan]
        node.wpa_cmd(pidfile, intf, wlan)


class physicalMesh(IntfWireless):

    def __init__(self, node, intf=None, channel=1, ssid='meshNet'):
        """Configure wireless mesh
        node: name of the node
        self: custom association class/constructor
        params: parameters for node"""
        wlan = 0
        self.name = ''
        self.node = node

        node.params['ssid'][wlan] = ssid
        if int(node.params['range'][wlan]) == 0:
            intf = node.params['wlan'][wlan]
            node.params['range'][wlan] = node.getRange(intf, 95)

        self.name = intf
        self.setPhysicalMeshIface(node, wlan, intf, channel, ssid)
        freq = self.get_freq(node.params['freq'][wlan])
        iface = node.params['wlan'][wlan]
        ht_cap = ''
        self.join_mesh(ssid, freq, ht_cap, iface)

    def ipLink(self, state=None):
        "Configure ourselves using ip link"
        os.system('ip link set %s %s' % (self.name, state))

    def setPhysicalMeshIface(self, node, wlan, intf, channel, ssid):
        iface = 'phy%s-mp%s' % (node, wlan)
        self.ipLink('down')
        while True:
            id = ''
            cmd = 'ip link show | grep %s' % iface
            try:
                id = subprocess.check_output(cmd, shell=True).split("\n")
            except:
                pass
            if len(id) == 0:
                cmd = ('iw dev %s interface add %s type mp' %
                       (intf, iface))
                self.name = iface
                subprocess.check_output(cmd, shell=True)
            else:
                try:
                    if channel:
                        cmd = ('iw dev %s set channel %s' %
                               (iface, channel))
                        subprocess.check_output(cmd, shell=True)
                    self.ipLink('up')
                    command = ('iw dev %s mesh join %s' % (iface, ssid))
                    subprocess.check_output(command, shell=True)
                    break
                except:
                    break


class Association(IntfWireless):

    @classmethod
    def setSNRWmediumd(cls, sta, ap, snr):
        "Send SNR to wmediumd"
        w_server.send_snr_update(SNRLink(sta.wmIface[0],
                                         ap.wmIface[0], snr))
        w_server.send_snr_update(SNRLink(ap.wmIface[0],
                                         sta.wmIface[0], snr))

    @classmethod
    def configureWirelessLink(cls, sta, ap, **params):
        dist = sta.get_distance_to(ap)
        wlan = params['wlan']
        if dist <= ap.params['range'][0]:
            if not wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
                if sta.params['rssi'][wlan] == 0:
                    cls.updateParams(sta, ap, wlan)
            if ('doAssociation' in params and ap != sta.params['associatedTo'][wlan]) or \
                    (not sta.params['associatedTo'][wlan]
                     and ap not in sta.params['associatedTo']):
                cls.associate_infra(sta, ap, **params)
                if wmediumd_mode.mode == w_cst.WRONG_MODE:
                    if dist >= 0.01:
                        wirelessLink(sta, ap, dist, **params)
                if sta not in ap.params['associatedStations']:
                    ap.params['associatedStations'].append(sta)
            if not wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
                cls.setRSSI(sta, ap, wlan, dist)

    @classmethod
    def setRSSI(cls, sta, ap, wlan, dist):
        rssi = sta.get_rssi(ap, wlan, dist)
        sta.params['rssi'][wlan] = rssi
        if ap not in sta.params['apsInRange']:
            sta.params['apsInRange'][ap] = rssi
            ap.params['stationsInRange'][sta] = rssi

    @classmethod
    def updateParams(cls, sta, ap, wlan):
        sta.params['freq'][wlan] = ap.get_freq(0)
        sta.params['channel'][wlan] = ap.params['channel'][0]
        sta.params['mode'][wlan] = ap.params['mode'][0]
        sta.params['ssid'][wlan] = ap.params['ssid'][0]

    @classmethod
    def associate(cls, sta, ap, **params):
        "Associate to Access Point"
        if 'position' in sta.params:
            cls.configureWirelessLink(sta, ap, **params)
        else:
            cls.associate_infra(sta, ap, **params)

    @classmethod
    def associate_noEncrypt(cls, sta, ap, wlan, ap_wlan):
        #iwconfig is still necessary, since iw doesn't include essid like iwconfig does.
        intf = cls.get_intf(sta, wlan)
        ssid = cls.get_ssid(ap, ap_wlan)
        mac = cls.get_mac(ap, ap_wlan)
        debug(cls.iwconfig_con(intf, ssid, mac))
        sta.pexec(cls.iwconfig_con(intf, ssid, mac))

    @classmethod
    def iwconfig_con(cls, intf, ssid, mac):
        cmd = 'iwconfig %s essid %s ap %s' % (intf, ssid, mac)
        return cmd

    @classmethod
    def disconnect(cls, node, wlan):
        intf = node.params['wlan'][wlan]
        node.pexec('iw dev %s disconnect' % intf)
        node.params['rssi'][wlan] = 0
        node.params['associatedTo'][wlan] = ''
        node.params['channel'][wlan] = 0

    @classmethod
    def associate_infra(cls, sta, ap, **params):
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
        cmd = ''
        if 'config' not in ap.params or 'config' not in sta.params:
            if 'authmode' not in ap.params:
                if 'passwd' not in sta.params:
                    passwd = ap.params['passwd'][ap_wlan]
                else:
                    passwd = sta.params['passwd'][wlan]

        if 'wpasup_globals' not in sta.params \
                or ('wpasup_globals' in sta.params
                    and 'ctrl_interface=' not in sta.params['wpasup_globals']):
            cmd = 'ctrl_interface=/var/run/wpa_supplicant\n'
        if 'wpasup_globals' in sta.params:
            cmd += sta.params['wpasup_globals'] + '\n'
        cmd = cmd + 'network={\n'

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
            if 'bgscan_threshold' in sta.params:
                if 'bgscan_module' not in sta.params:
                    sta.params['bgscan_module'] = 'simple'
                bgscan = 'bgscan=\"%s:%d:%d:%d\"' % \
                         (sta.params['bgscan_module'],
                          sta.params['s_inverval'],
                          sta.params['bgscan_threshold'],
                          sta.params['l_interval'])
                cmd += '   %s\n' % bgscan
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
        pidfile = "mn%d_%s_%s_wpa.pid" % (os.getpid(), sta.name, wlan)
        intf = sta.params['wlan'][wlan]
        cls.wpaFile(sta, ap, wlan, ap_wlan)
        sta.wpa_pexec(pidfile, intf, wlan)

    @classmethod
    def handover_ieee80211r(cls, sta, ap, wlan, ap_wlan):
        intf = cls.get_intf(sta, wlan)
        mac = cls.get_mac(ap, ap_wlan)
        sta.pexec('wpa_cli -i %s roam %s' % (intf, mac))

    @classmethod
    def wep(cls, sta, ap, wlan, ap_wlan):
        if 'passwd' not in sta.params:
            passwd = ap.params['passwd'][ap_wlan]
        else:
            passwd = sta.params['passwd'][wlan]
        cls.wep_connect(sta, wlan, ap, ap_wlan, passwd)

    @classmethod
    def wep_connect(cls, sta, wlan, ap, ap_wlan, passwd):
        intf = cls.get_intf(sta, wlan)
        ssid = cls.get_ssid(ap, ap_wlan)
        sta.pexec('iw dev %s connect %s key d:0:%s' % (intf, ssid, passwd))

    @classmethod
    def update(cls, sta, ap, wlan):
        no_upt = ['active_scan', 'bgscan']
        if sta.params['associatedTo'][wlan] not in no_upt:
            if sta.params['associatedTo'][wlan] \
                    and sta in sta.params['associatedTo'][wlan].params['associatedStations']:
                sta.params['associatedTo'][wlan].params['associatedStations'].remove(sta)
            cls.updateParams(sta, ap, wlan)
            ap.params['associatedStations'].append(sta)
            sta.params['associatedTo'][wlan] = ap
