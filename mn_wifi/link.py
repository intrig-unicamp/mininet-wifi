# author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)


import os
import re
import glob
import subprocess
from time import sleep
from sys import version_info as py_version_info

from mininet.util import BaseString
from mininet.log import error, debug, info
from mn_wifi.manetRoutingProtocols import manetProtocols
from mn_wifi.wmediumdConnector import DynamicIntfRef, \
    w_starter, SNRLink, w_pos, w_cst, w_server, ERRPROBLink, \
    wmediumd_mode, w_txpower, w_gain, w_height


class IntfWireless(object):

    "Basic interface object that can configure itself."

    def __init__(self, name, node=None, port=None, link=None,
                 mac=None, **params):
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

        node.addWIntf(self, port=port)

        # Save params for future reference
        self.params = params
        self.config(**params)

    def cmd(self, *args, **kwargs):
        "Run a command in our owning node"
        return self.node.cmd(*args, **kwargs)

    def pexec(self, *args, **kwargs):
        "Run a command in our owning node"
        return self.node.pexec(*args, **kwargs)

    def set_dev_type(self, type):
        self.iwdev_cmd('%s set type %s' % (self.name, type))

    def add_dev_type(self, new_name, type):
        self.iwdev_cmd('%s interface add %s type %s' % (self.name, new_name, type))

    def iwdev_cmd(self, *args):
        return self.cmd('iw dev', *args)

    def iwdev_pexec(self, *args):
        return self.pexec('iw dev', *args)

    def join_ibss(self):
        return self.iwdev_cmd('{} ibss join {} {} {} 02:CA:FF:EE:BA:01'.
                              format(self.name, self.ssid,
                                     self.format_freq(), self.ht_cap))

    def join_mesh(self):
        return self.iwdev_cmd('{} mesh join {} freq {} {}'.
                              format(self.name, self.ssid,
                                     self.format_freq(), self.ht_cap))

    def get_pid_filename(self):
        pidfile = 'mn{}_{}_{}_wpa.pid'.format(os.getpid(), self.node.name, self.id)
        return pidfile

    def get_wpa_cmd(self):
        pidfile = self.get_pid_filename()
        wpasup_flags = ''
        if 'wpasup_flags' in self.node.params:
            wpasup_flags = self.node.params['wpasup_flags']
        cmd = ('wpa_supplicant -B -Dnl80211 -P {} -i {} -c {}_{}.staconf {}'.
               format(pidfile, self.name, self.name, self.id, wpasup_flags))
        return cmd

    def wpa_cmd(self):
        return self.cmd(self.get_wpa_cmd())

    def wpa_pexec(self):
        return self.node.pexec(self.get_wpa_cmd())

    def setGainWmediumd(self):
        "Set Antenna Gain for wmediumd"
        if wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
            gain = self.antennaGain
            w_server.update_gain(w_gain(self.wmIface, int(gain)))

    def setHeightWmediumd(self):
        "Set Antenna Height for wmediumd"
        if wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
            height = self.antennaHeight
            w_server.update_height(w_height(self.wmIface, int(height)))

    def setTXPowerWmediumd(self):
        "Set TxPower for wmediumd"
        if wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
            txpower = self.txpower
            w_server.update_txpower(w_txpower(self.wmIface, int(txpower)))

    def getCustomRate(self):
        modes = ['a', 'b', 'g', 'n', 'ac']
        rates = [11, 3, 11, 600, 1000]
        rate = rates[modes.index(self.mode)]
        return rate

    def getRate(self):
        modes = ['a', 'b', 'g', 'n', 'ac']
        rates = [54, 11, 54, 300, 600]
        rate = rates[modes.index(self.mode)]
        return rate

    def get_freq(self):
        "Gets frequency based on channel number"
        channel = int(self.channel)
        chan_list_2ghz = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]
        chan_list_5ghz = [36, 40, 44, 48, 52, 56, 60, 64, 100,
                          104, 108, 112, 116, 120, 124, 128, 132,
                          136, 140, 149, 153, 157, 161, 165,
                          169, 171, 172, 173, 174, 175, 176,
                          177, 178, 179, 180, 181, 182, 183, 184,
                          185]
        freq_list_2ghz = [2.412, 2.417, 2.422, 2.427, 2.432, 2.437,
                          2.442, 2.447, 2.452, 2.457, 2.462]
        freq_list_5ghz = [5.18, 5.2, 5.22, 5.24, 5.26, 5.28, 5.30, 5.32,
                          5.50, 5.52, 5.54, 5.56, 5.58, 5.6, 5.62,
                          5.64, 5.66, 5.68, 5.7, 5.745, 5.765, 5.785,
                          5.805, 5.825, 5.845, 5.855, 5.86, 5.865, 5.87,
                          5.875, 5.88, 5.885, 5.89, 5.895, 5.9, 5.905,
                          5.91, 5.915, 5.92, 5.925]
        all_chan = chan_list_2ghz + chan_list_5ghz
        all_freq = freq_list_2ghz + freq_list_5ghz
        if channel in all_chan:
            idx = all_chan.index(channel)
            return all_freq[idx]
        else:
            return '2.412'

    def setFreq(self, freq, intf=None):
        return self.iwdev_cmd('{} set freq {}'.format(intf, freq))

    def format_freq(self):
        return int(format(self.freq, '.3f').replace('.', ''))

    def setReg(self):
        if self.mode == 'a' or self.mode == 'ac':
            self.pexec('iw reg set US')

    def setAPChannel(self):
        self.freq = self.get_freq()
        self.pexec('hostapd_cli -i %s chan_switch %s %s' % (
            self.name, self.channel, str(self.format_freq())))

    def setChannel(self):
        self.freq = self.get_freq()
        self.iwdev_cmd('%s set channel %s' % (self.name, self.channel))

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

    def setMode(self, mode):
        self.mode = mode

    def setTxPower(self):
        txpower = self.txpower
        self.node.pexec('iw dev %s set txpower fixed %s' % (self.name, txpower * 100))
        debug('\n')

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

    def setIP6(self, ipstr, prefixLen=None, **args):
        """Set our IP6 address"""
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

    def configureMacAddr(self):
        """Configure Mac Address
        :param node: node"""
        if not self.mac:
            self.mac = self.getMAC()
        else:
            self.setMAC(self.mac)

    def getMAC(self):
        "get Mac Address of any Interface"
        try:
            _macMatchRegex = re.compile(r'..:..:..:..:..:..')
            debug('getting mac address from %s\n' % self.name)
            macaddr = str(self.pexec('ip addr show %s' % self.name))
            mac = _macMatchRegex.findall(macaddr)
            debug('\n%s' % mac[0])
            return mac[0]
        except:
            info('Error: Please run sudo mn -c.\n')

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

    def config(self, mac=None, ip=None, ip6=None,
               ipAddr=None, up=True, **_params):
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
        self.setParam(r, 'setIP6', ip=ip6)
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

        if not hasattr(node1, 'position'):
            self.set_pos(node1)
        if not hasattr(node2, 'position'):
            self.set_pos(node2)

        if cl_intfName not in cl.params['wlan']:

            wlan = cl.params['wlan'].index(port1) if port1 else 0
            apwlan = ap.params['wlan'].index(port2) if port2 else 0

            intf = cl.wintfs[wlan]
            ap_intf = ap.wintfs[apwlan]

            self.node = cl
            self.add4addrIface(wlan, cl_intfName)

            self.setMAC(intf)
            self.setMAC(ap_intf)
            self.bring4addrIfaceUP()

            intf.mode = ap_intf.mode
            intf.channel = ap_intf.channel
            intf.freq = ap_intf.freq
            intf.txpower = ap_intf.txpower
            intf.antennaGain = ap_intf.antennaGain
            cl.params['wlan'].append(cl_intfName)
            sleep(1)
            self.iwdev_cmd('%s connect %s %s' % (cl.params['wlan'][1],
                                                 ap_intf.ssid, ap_intf.mac))

            params1, params2 = {}, {}
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

            _4addrAP(ap, (len(ap.params['wlan'])-1))
            _4addrClient(cl, (len(cl.params['wlan'])-1))

            cl.wintfs[1].mac = (intf.mac[:3] + '09' + intf.mac[5:])

        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2

    def set_pos(self, node):
        nums = re.findall(r'\d+', node.name)
        if nums:
            id = int(hex(int(nums[0]))[2:])
            node.position = (10, round(id, 2), 0)

    def bring4addrIfaceUP(self):
        self.cmd('ip link set dev %s.wds up' % self.node)

    def setMAC(self, intf):
        self.cmd('ip link set dev %s.wds addr %s'
                 % (intf.node, intf.mac))

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
    def __init__(self, node, port=None, intfName=None, addr=None,
                 cls=None, params=None):
        """Create veth link to another node, making two new interfaces.
           node: first node
           port: node port number (optional)
           intf: default interface class/constructor
           cls: optional interface-specific constructors
           intfName: node interface name (optional)
           params: parameters for interface 1"""
        # This is a bit awkward; it seems that having everything in
        # params is more orthogonal, but being able to specify
        # in-line arguments is more convenient! So we support both.
        params = dict( params ) if params else {}

        if port is not None:
            params[ 'port' ] = port

        params['port'] = node.newPort()
        if not intfName:
            ifacename = 'wlan'
            intfName = self.wlanName(node, ifacename, params['port'])
        intf1 = cls(name=intfName, node=node,
                    link=self, mac = addr, ** params)

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
    def __init__(self, node, port=None, intfName=None, addr=None,
                 intf=IntfWireless, cls=None, params=None):
        """Create veth link to another node, making two new interfaces.
           node: first node
           port: node port number (optional)
           intf: default interface class/constructor
           cls: optional interface-specific constructors
           intfName: node interface name (optional)
           params: parameters for interface 1"""
        # This is a bit awkward; it seems that having everything in
        # params is more orthogonal, but being able to specify
        # in-line arguments is more convenient! So we support both.
        if params is None:
            params = {}

        if port is not None:
            params[ 'port' ] = port

        if 'port' not in params:
            params[ 'port' ] = node.newPort()

        if not intfName:
            ifacename = 'wlan'
            intfName = self.wlanName(node, ifacename, node.newPort())

        if not cls:
            cls = intf

        intf1 = cls(name=intfName, node=node,
                    link=self, mac=addr, **params)
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
    def __init__(self, node, port=None, intfName=None,
                 addr=None, cls=TCWirelessLink, **params):
        WirelessLinkStation.__init__(self, node=node, port=port,
                                     intfName=intfName,
                                     cls=cls, addr=addr,
                                     params=params)


class TCLinkWirelessAP(WirelessLinkAP):
    "Link with symmetric TC interfaces configured via opts"
    def __init__(self, node, port=None, intfName=None,
                 addr=None, cls=TCWirelessLink, **params):
        WirelessLinkAP.__init__(self, node, port=port,
                                intfName=intfName,
                                cls=cls, addr=addr,
                                params=params)


class master(TCWirelessLink):
    "master class"
    def __init__(self, node, wlan, port=None, intf=None):
        self.name = node.params['wlan'][wlan]
        node.addWAttr(self, port=port)
        self.node = node
        self.params = {}
        self.stationsInRange = {}
        self.associatedStations = []
        self.antennaGain = 5.0
        self.antennaHeight = 1.0
        self.channel = 1
        self.freq = 2.412
        self.range = 0
        self.txpower = 14
        self.ieee80211r = None
        self.band = None
        self.authmode = None
        self.beacon_int = None
        self.config = None
        self.driver = 'nl80211'
        self.encrypt = None
        self.ht_capab = None
        self.id = wlan
        self.ip = None
        self.ip6 = None
        self.isolate_clients = None
        self.mac = None
        self.mode = 'g'
        self.passwd = None
        self.shared_secret = None
        self.ssid = None
        self.wpa_key_mgmt = None
        self.rsn_pairwise = None
        self.radius_server = None
        self.wps_state = None
        self.device_type = None
        self.wpa_psk_file = None
        self.config_methods = None
        self.mobility_domain = None
        self.link = None

        if intf:
            self.wmIface = intf.wmIface

        for key in self.__dict__.keys():
            if key in node.params:
                if isinstance(node.params[key], BaseString):
                    setattr(self, key, node.params[key])
                elif isinstance(node.params[key], list):
                    arg_ = node.params[key][0].split(',')
                    setattr(self, key, arg_[wlan])
                elif isinstance(node.params[key], int):
                    setattr(self, key, node.params[key])


class managed(TCWirelessLink):
    "managed class"
    def __init__(self, node, wlan, intf=None):
        self.name = node.params['wlan'][wlan]
        node.addWIntf(self, port=wlan)
        node.addWAttr(self, port=wlan)
        self.node = node
        self.apsInRange = {}
        self.range = 0
        self.ifb = None
        self.active_scan = None
        self.associatedTo = None
        self.associatedStations = None
        self.authmode = None
        self.config = None
        self.encrypt = None
        self.freq_list = None
        self.ip = None
        self.ip6 = None
        self.link = None
        self.mac = None
        self.passwd = None
        self.radius_identity = None
        self.radius_passwd = None
        self.scan_freq = None
        self.ssid = None
        self.stationsInRange = None
        self.bgscan_module = 'simple'
        self.s_inverval = 0
        self.bgscan_threshold = 0
        self.l_interval = 0
        self.txpower = 14
        self.id = wlan
        self.rssi = -60
        self.mode = 'g'
        self.freq = 2.412
        self.channel = 1
        self.antennaGain = 5.0
        self.antennaHeight = 1.0

        if intf:
            self.wmIface = intf.wmIface

        for key in self.__dict__.keys():
            if key in node.params:
                if isinstance(node.params[key], BaseString):
                    setattr(self, key, node.params[key])
                elif isinstance(node.params[key], list):
                    arg_ = node.params[key][0].split(',')
                    setattr(self, key, arg_[wlan])
                elif isinstance(node.params[key], int):
                    setattr(self, key, node.params[key])


class _4addrClient(TCWirelessLink):
    "managed class"
    def __init__(self, node, wlan):
        self.node = node
        self.id = wlan
        self.ip = None
        self.mac = node.wintfs[wlan-1].mac
        self.range = node.wintfs[0].range
        self.txpower = 0
        self.antennaGain = 5.0
        self.name = node.params['wlan'][wlan]
        self.stationsInRange = {}
        self.associatedStations = []
        self.apsInRange = {}
        self.params = {}
        node.addWIntf(self)
        node.addWAttr(self)


class _4addrAP(TCWirelessLink):
    "managed class"
    def __init__(self, node, wlan):
        self.node = node
        self.ip = None
        self.id = wlan
        self.mac = node.wintfs[0].mac
        self.range = node.wintfs[0].range
        self.txpower = 0
        self.antennaGain = 5.0
        self.name = node.params['wlan'][wlan]
        self.stationsInRange = {}
        self.associatedStations = []
        self.params = {}
        node.addWIntf(self)
        node.addWAttr(self)


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
            for intf in node.wintfs.values():
                intf.wmIface = DynamicIntfRef(node, intf=intf.name)
                intfrefs.append(intf.wmIface)

                if (isinstance(intf, master)
                    or (node in aps and (not isinstance(intf, managed)
                                         and not isinstance(intf, adhoc)))):
                    isnodeaps.append(1)
                else:
                    isnodeaps.append(0)
            '''for mac in maclist:
                for key in mac:
                    if key == node:
                        key.wmIface.append(DynamicIntfRef(key, intf=len(key.wmIface)))
                        key.params['wlan'].append(mac[key][1])
                        key.params['mac'].append(mac[key][0])
                        key.params['range'].append(0)
                        key.params['freq'].append(key.params['freq'][0])
                        key.params['antennaGain'].append(0)
                        key.params['txpower'].append(14)
                        intfrefs.append(key.wmIface[len(key.wmIface) - 1])
                        isnodeaps.append(0)'''

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
            if not hasattr(node, 'position'):
                posX, posY, posZ = 0, 0, 0
            else:
                posX = float(node.position[0])
                posY = float(node.position[1])
                posZ = float(node.position[2])
            node.lastpos = [posX, posY, posZ]

            for wlan, intf in enumerate(node.wintfs.values()):
                if wlan >= 1:
                    posX += 0.1
                wmediumd.positions.append(w_pos(intf.wmIface,
                                                [posX, posY, posZ]))
                wmediumd.txpowers.append(w_txpower(
                    intf.wmIface, float(intf.txpower)))


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
            wmediumd.links.append(ERRPROBLink(node[0].wintfs[0].wmIface,
                                              node[1].wintfs[0].wmIface, node[2]))
            wmediumd.links.append(ERRPROBLink(node[1].wintfs[0].wmIface,
                                              node[0].wintfs[0].wmIface, node[2]))


class set_snr(object):
    "wmediumd: set snr"
    def __init__(self):
        self.snr()

    @classmethod
    def snr(cls):
        "wmediumd: snr"
        for node in wmediumd.wlinks:
            wmediumd.links.append(SNRLink(node[0].wintfs[0].wmIface, node[1].wintfs[0].wmIface,
                                          node[0].wintfs[0].rssi - (-91)))
            wmediumd.links.append(SNRLink(node[1].wintfs[0].wmIface, node[0].wintfs[0].wmIface,
                                          node[0].wintfs[0].rssi - (-91)))


class wirelessLink(object):

    dist = 0
    noise = 0
    equationLoss = '(dist * 2) / 1000'
    equationDelay = '(dist / 10) + 1'
    equationLatency = '(dist / 10)/2'
    equationBw = ' * (1.01 ** -dist)'

    def __init__(self, intf, dist=0):
        latency_ = self.getLatency(dist)
        loss_ = self.getLoss(dist)
        bw_ = self.getBW(intf, dist)
        self.config_tc(intf, bw_, loss_, latency_)

    def getDelay(self, dist):
        "Based on RandomPropagationDelayModel"
        return eval(self.equationDelay)

    def getLatency(self, dist):
        return eval(self.equationLatency)

    def getLoss(self, dist):
        return eval(self.equationLoss)

    def getBW(self, intf, dist):
        # dist is used by eval
        custombw = intf.getCustomRate()
        rate = eval(str(custombw) + self.equationBw)

        if rate <= 0.0:
            rate = 0.1
        return rate

    @classmethod
    def delete(cls, node):
        "Delete interfaces"
        for intf in node.wintfs.values():
            node.cmd('iw dev ' + intf.name + ' del')
            node.delIntf(intf.name)
            node.intf = None

    @classmethod
    def config_tc(cls, intf, bw, loss, latency):
        if intf.ifb:
            cls.tc(intf.node, intf.ifb, bw, loss, latency)
        cls.tc(intf.node, intf.name, bw, loss, latency)

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
        if isinstance(intf, BaseString):
            wlan = node.params['wlan'].index(intf)
            intf = node.wintfs[wlan]
        else:
            wlan = intf.id

        if isinstance(self, master):
            self.kill_hostapd()

        self.channel = channel
        self.freq = self.get_freq()
        self.range = intf.range
        self.name = intf.name
        self.mac = intf.mac

        if isinstance(intf, master):
            self.name = '%s-ocb' % node.name
            self.add_ocb_mode()
        else:
            self.set_ocb_mode()
        #node.addWIntf(self, port=wlan)
        node.addWAttr(self, port=wlan)
        self.configure_ocb()

    def kill_hostapd(self):
        self.node.setManagedMode(self)

    def add_ocb_mode(self):
        "Set OCB Interface"
        self.ipLink('down')
        self.node.delIntf(self.name)
        self.add_dev_type(self.name, 'ocb')
        # we set the port to remove the existing wlan from node.intfs
        IntfWireless(name=self.name, node=self.node, port=1)
        self.setMAC(self.name)
        self.ipLink('up')

    def set_ocb_mode(self):
        "Set OCB Interface"
        self.ipLink('down')
        self.set_dev_type('ocb')
        self.ipLink('up')

    def configure_ocb(self):
        "Configure Wireless OCB"
        self.iwdev_cmd('%s ocb join %s 20MHz' % (self.name, self.freq))


class wifiDirectLink(IntfWireless):

    def __init__(self, node, intf=None):
        "configure wifi-direct"
        self.node = node
        if isinstance(intf, BaseString):
            wlan = node.params['wlan'].index(intf)
            intf = node.wintfs[wlan]
        else:
            wlan = intf.id

        self.mac = intf.mac
        self.name = intf.name
        self.range = intf.range
        self.txpower = intf.txpower
        self.ip6 = intf.ip6
        self.ip = intf.ip

        node.addWIntf(self, port=wlan)
        node.addWAttr(self, port=wlan)

        self.config_()
        cmd = self.get_wpa_cmd()
        node.cmd(cmd)

    def get_filename(self):
        suffix = 'wifiDirect.conf'
        filename = "mn%d_%s_%s" % (os.getpid(), self.name, suffix)
        return filename

    def get_wpa_cmd(self):
        filename = self.get_filename()
        cmd = ('wpa_supplicant -B -Dnl80211 -c%s -i%s' %
               (filename, self.name))
        return cmd

    def config_(self):
        filename = self.get_filename()
        cmd = ("echo \'")
        cmd += 'ctrl_interface=/var/run/wpa_supplicant\
              \nap_scan=1\
              \np2p_go_ht40=1\
              \ndevice_name=%s\
              \ndevice_type=1-0050F204-1\
              \np2p_no_group_iface=1' % self.name
        cmd += ("\' > %s" % filename)
        self.set_config(cmd)

    def set_config(self, cmd):
        subprocess.check_output(cmd, shell=True)


class physicalWifiDirectLink(wifiDirectLink):

    def __init__(self, node, intf=None):
        "configure wifi-direct"
        self.name = intf
        node.addWIntf(self)
        node.addWAttr(self)

        for wlan, intf in enumerate(node.wintfs.values()):
            if intf.name == self.name:
                break

        self.txpower = node.wintfs[0].txpower
        self.mac = None

        self.config_()
        cmd = self.get_wpa_cmd()
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
        if isinstance(intf, BaseString):
            wlan = node.params['wlan'].index(intf)
            intf = node.wintfs[wlan]
        else:
            wlan = intf.id

        self.id = wlan
        self.ssid = ssid
        self.ip6 = intf.ip6
        self.ip = intf.ip
        self.mac = intf.mac
        self.ip6 = intf.ip6
        self.link = intf.link
        self.encrypt = intf.encrypt
        self.antennaGain = intf.antennaGain
        self.passwd = passwd
        self.mode = mode
        self.channel = channel
        self.ht_cap = ht_cap
        self.associatedTo = 'adhoc'
        if wmediumd_mode.mode:
            self.wmIface = intf.wmIface

        if 'mp' in intf.name:
            self.iwdev_cmd('%s del' % intf.name)
            node.params['wlan'][wlan] = intf.name.replace('mp', 'wlan')

        self.name = intf.name

        node.addWIntf(self, port=wlan)
        node.addWAttr(self, port=wlan)

        self.freq = self.get_freq()
        self.setReg()
        self.configureAdhoc()

        self.txpower = intf.txpower
        self.range = intf.range

        if proto:
            manetProtocols(intf, proto, **params)

    def configureAdhoc(self):
        "Configure Wireless Ad Hoc"
        self.set_dev_type('ibss')
        self.ipLink('up')

        if self.passwd:
            self.setSecuredAdhoc()
        else:
            self.join_ibss()

    def get_sta_confname(self):
        fileName = '%s.staconf' % self.name
        return fileName

    def setSecuredAdhoc(self):
        "Set secured adhoc"
        cmd = 'ctrl_interface=/var/run/wpa_supplicant GROUP=wheel\n'
        cmd += 'ap_scan=2\n'
        cmd += 'network={\n'
        cmd += '         ssid="%s"\n' % self.ssid
        cmd += '         mode=1\n'
        cmd += '         frequency=%s\n' % self.format_freq()
        cmd += '         proto=RSN\n'
        cmd += '         key_mgmt=WPA-PSK\n'
        cmd += '         pairwise=CCMP\n'
        cmd += '         group=CCMP\n'
        cmd += '         psk="%s"\n' % self.passwd
        cmd += '}'

        fileName = self.get_sta_confname()
        os.system('echo \'%s\' > %s' % (cmd, fileName))


class mesh(IntfWireless):

    node = None

    def __init__(self, node, intf=None, mode='g', channel=1,
                 ssid='meshNet', passwd=None, ht_cap=''):
        from mn_wifi.node import AP
        """Configure wireless mesh
        node: name of the node
        self: custom association class/constructor
        params: parameters for node"""
        self.node = node
        if isinstance(intf, BaseString):
            wlan = node.params['wlan'].index(intf)
            intf = node.wintfs[wlan]
        else:
            wlan = intf.id
        iface = intf

        self.name = self.name = '%s-mp%s' % (node, intf.name[-1:])
        self.id = wlan
        self.mac = intf.mac
        self.ip6 = intf.ip6
        self.ip = intf.ip
        self.link = intf.link
        self.txpower = intf.txpower
        self.encrypt = intf.encrypt
        self.antennaGain = intf.antennaGain
        self.stationsInRange = intf.stationsInRange
        self.associatedStations = intf.associatedStations
        self.range = intf.range
        self.ssid = ssid
        self.mode = mode
        self.channel = channel
        self.ht_cap = ht_cap
        self.passwd = passwd
        self.associatedTo = 'mesh'
        if wmediumd_mode.mode:
            self.wmIface = DynamicIntfRef(node, intf=self.name)

        node.addWAttr(self, port=wlan)
        if isinstance(node, AP):
            node.addWIntf(self, port=wlan+1)
        else:
            node.addWIntf(self, port=wlan)

        self.setMeshIface(wlan, iface)
        self.configureMesh()

    def set_mesh_type(self, intf):
        return '%s interface add %s type mp' % (intf.name, self.name)

    def setMeshIface(self, wlan, intf):
        if isinstance(intf, adhoc):
            self.set_dev_type('managed')
        self.iwdev_cmd(self.set_mesh_type(intf))
        self.node.cmd('ip link set %s down' % intf)

        self.setMAC(intf.mac)
        self.node.params['wlan'][wlan] = self.name

        self.setChannel()
        self.setReg()

        self.ipLink('up')

    def configureMesh(self):
        "Configure Wireless Mesh Interface"
        if self.passwd:
            self.setSecuredMesh()
        else:
            self.join_mesh()

    def get_sta_confname(self):
        fileName = '%s.staconf' % self.name
        return fileName

    def setSecuredMesh(self):
        "Set secured mesh"
        cmd = 'ctrl_interface=/var/run/wpa_supplicant\n'
        cmd += 'ctrl_interface_group=adm\n'
        cmd += 'user_mpm=1\n'
        cmd += 'network={\n'
        cmd += '         ssid="%s"\n' % self.ssid
        cmd += '         mode=5\n'
        cmd += '         frequency=%s\n' % self.format_freq()
        cmd += '         key_mgmt=SAE\n'
        cmd += '         psk="%s"\n' % self.passwd
        cmd += '}'

        fileName = self.get_sta_confname()
        os.system('echo \'%s\' > %s' % (cmd, fileName))


class physicalMesh(IntfWireless):

    def __init__(self, node, intf=None, channel=1, ssid='meshNet',
                 ht_cap=''):
        """Configure wireless mesh
        node: name of the node
        self: custom association class/constructor
        params: parameters for node"""
        wlan = 0
        self.name = ''
        self.node = node

        self.ssid = ssid
        self.ht_cap = ht_cap
        self.channel = channel

        node.wintfs[wlan].ssid = ssid
        if int(node.wintfs[wlan].range) == 0:
            intf = node.params['wlan'][wlan]
            node.wintfs[wlan].range = node.getRange(intf, 95)

        self.name = intf
        self.setPhysicalMeshIface(node, wlan, intf)
        self.freq = self.format_freq()

        self.join_mesh()

    def ipLink(self, state=None):
        "Configure ourselves using ip link"
        os.system('ip link set %s %s' % (self.name, state))

    def setPhysicalMeshIface(self, node, wlan, intf):
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
                    if self.channel:
                        cmd = ('iw dev %s set channel %s' %
                               (iface, self.channel))
                        subprocess.check_output(cmd, shell=True)
                    self.ipLink('up')
                    command = ('iw dev %s mesh join %s' % (iface, self.ssid))
                    subprocess.check_output(command, shell=True)
                    break
                except:
                    break


class Association(IntfWireless):

    @classmethod
    def setSNRWmediumd(cls, sta, ap, snr):
        "Send SNR to wmediumd"
        w_server.send_snr_update(SNRLink(sta.wintfs[0].wmIface,
                                         ap.wintfs[0].wmIface, snr))
        w_server.send_snr_update(SNRLink(ap.wintfs[0].wmIface,
                                         sta.wintfs[0].wmIface, snr))

    @classmethod
    def configureWirelessLink(cls, intf, ap_intf):
        dist = intf.node.get_distance_to(ap_intf.node)
        if dist <= ap_intf.range:
            if not wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
                if intf.rssi == 0:
                    cls.updateClientParams(intf, ap_intf)
            if ap_intf != intf.associatedTo or \
                    not intf.associatedTo:
                cls.associate_infra(intf, ap_intf)
                if wmediumd_mode.mode == w_cst.WRONG_MODE:
                    if dist >= 0.01:
                        wirelessLink(intf, dist)
                if intf.node != ap_intf.associatedStations:
                    ap_intf.associatedStations.append(intf.node)
            if not wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
                cls.get_rssi(intf, ap_intf, dist)

    @classmethod
    def get_rssi(cls, intf, ap_intf, dist):
        from mn_wifi.propagationModels import propagationModel
        rssi = float(propagationModel(intf, ap_intf, dist).rssi)
        intf.rssi = rssi
        if ap_intf.node not in intf.apsInRange:
            intf.apsInRange[ap_intf.node] = rssi
            ap_intf.stationsInRange[intf.node] = rssi
        return rssi

    @classmethod
    def updateClientParams(cls, intf, ap_intf):
        intf.freq = ap_intf.freq
        intf.channel = ap_intf.channel
        intf.mode = ap_intf.mode
        intf.ssid = ap_intf.ssid
        intf.range = intf.node.getRange(intf)

    @classmethod
    def associate(cls, intf, ap_intf):
        "Associate to Access Point"
        if hasattr(intf.node, 'position'):
            cls.configureWirelessLink(intf, ap_intf)
        else:
            cls.associate_infra(intf, ap_intf)

    @classmethod
    def associate_noEncrypt(cls, intf, ap_intf):
        #iwconfig is still necessary, since iw doesn't include essid like iwconfig does.
        intf.node.pexec(cls.iwconfig_con(intf, ap_intf))
        debug('\n')

    @classmethod
    def iwconfig_con(cls, intf, ap_intf):
        cmd = 'iwconfig %s essid %s ap %s' % (intf, ap_intf.ssid, ap_intf.mac)
        return cmd

    @classmethod
    def disconnect(cls, intf):
        intf.node.pexec('iw dev %s disconnect' % intf.name)
        intf.rssi = 0
        intf.associatedTo = ''
        intf.channel = 0

    @classmethod
    def check_if_wpafile_exist(cls, intf):
        file = '%s_%s.staconf' % (intf.name, intf.id)
        return glob.glob(file)

    @classmethod
    def associate_infra(cls, intf, ap_intf):
        associated = 0
        if ap_intf.ieee80211r and (not intf.encrypt or 'wpa' in intf.encrypt):
            if not intf.associatedTo:
                wpa_file_exists = cls.check_if_wpafile_exist(intf)
                if wpa_file_exists:
                    cls.handover_ieee80211r(intf, ap_intf)
                else:
                    cls.wpa(intf, ap_intf)
            else:
                cls.handover_ieee80211r(intf, ap_intf)
            associated = 1
        elif not ap_intf.encrypt:
            associated = 1
            cls.associate_noEncrypt(intf, ap_intf)
        else:
            if not intf.associatedTo:
                if 'wpa' in ap_intf.encrypt and (not intf.encrypt or 'wpa' in intf.encrypt):
                    cls.wpa(intf, ap_intf)
                    associated = 1
                elif ap_intf.encrypt == 'wep':
                    cls.wep(intf, ap_intf)
                    associated = 1
        if associated:
            cls.update(intf, ap_intf)

    @classmethod
    def wpaFile(cls, intf, ap_intf):
        cmd = ''
        if not ap_intf.config or not intf.config:
            if not ap_intf.authmode:
                if not intf.passwd:
                    passwd = ap_intf.passwd
                else:
                    passwd = intf.passwd

        if 'wpasup_globals' not in intf.node.params \
                or ('wpasup_globals' in intf.node.params
                    and 'ctrl_interface=' not in intf.node.params['wpasup_globals']):
            cmd = 'ctrl_interface=/var/run/wpa_supplicant\n'
        if ap_intf.wps_state and not intf.passwd:
            cmd += 'ctrl_interface_group=0\n'
            cmd += 'update_config=1\n'
        else:
            if 'wpasup_globals' in intf.node.params:
                cmd += intf.node.params['wpasup_globals'] + '\n'
            cmd += 'network={\n'

            if intf.config:
                config = intf.config
                if config is not []:
                    config = intf.config.split(',')
                    intf.node.params.pop("config", None)
                    for conf in config:
                        cmd += "   " + conf + "\n"
            else:
                cmd += '   ssid=\"%s\"\n' % ap_intf.ssid

                if not ap_intf.authmode:
                    cmd += '   psk=\"%s\"\n' % passwd
                    encrypt = ap_intf.encrypt
                    if ap_intf.encrypt == 'wpa3':
                        encrypt = 'wpa2'
                    cmd += '   proto=%s\n' % encrypt.upper()
                    cmd += '   pairwise=%s\n' % ap_intf.rsn_pairwise
                    if intf.active_scan:
                        cmd += '   scan_ssid=1\n'
                    if intf.scan_freq:
                        cmd += '   scan_freq=%s\n' % intf.scan_freq
                    if intf.freq_list:
                        cmd += '   freq_list=%s\n' % intf.freq_list
                wpa_key_mgmt = ap_intf.wpa_key_mgmt
                if ap_intf.encrypt == 'wpa3':
                    wpa_key_mgmt = 'SAE'
                cmd += '   key_mgmt=%s\n' % wpa_key_mgmt
                if 'bgscan_threshold' in intf.node.params:
                    if 'bgscan_module' not in intf.node.params:
                        intf.node.params['bgscan_module'] = 'simple'
                    bgscan = 'bgscan=\"%s:%d:%d:%d\"' % \
                             (intf.bgscan_module, intf.s_inverval,
                              intf.bgscan_threshold, intf.l_interval)
                    cmd += '   %s\n' % bgscan
                if ap_intf.authmode == '8021x':
                    cmd += '   eap=PEAP\n'
                    cmd += '   identity=\"%s\"\n' % intf.radius_identity
                    cmd += '   password=\"%s\"\n' % intf.radius_passwd
                    cmd += '   phase2=\"autheap=MSCHAPV2\"\n'
            cmd += '}'

        fileName = '%s_%s.staconf' % (intf.name, intf.id)
        os.system('echo \'%s\' > %s' % (cmd, fileName))

    @classmethod
    def wpa(cls, intf, ap_intf):
        cls.wpaFile(intf, ap_intf)
        intf.wpa_pexec()

    @classmethod
    def handover_ieee80211r(cls, intf, ap_intf):
        intf.node.pexec('wpa_cli -i %s roam %s' % (intf.name, ap_intf.mac))

    @classmethod
    def wep(cls, intf, ap_intf):
        if not intf.passwd:
            passwd = ap_intf.passwd
        else:
            passwd = intf.passwd
        cls.wep_connect(passwd, intf, ap_intf)

    @classmethod
    def wep_connect(cls, passwd, intf, ap_intf):
        intf.node.pexec('iw dev %s connect %s key d:0:%s' % (intf.name, ap_intf.ssid, passwd))

    @classmethod
    def update(cls, intf, ap_intf):
        if intf.associatedTo \
                and intf.node in ap_intf.associatedStations:
            ap_intf.associatedStations.remove(intf.node)
        cls.updateClientParams(intf, ap_intf)
        ap_intf.associatedStations.append(intf.node)
        intf.associatedTo = ap_intf.node
