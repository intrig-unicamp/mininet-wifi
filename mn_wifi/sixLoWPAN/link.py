"author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)"

import os
import re

from mininet.link import Intf, TCIntf, Link
from mininet.log import error, debug
from mn_wifi.sixLoWPAN.wmediumdConnector import interference, wmediumd_mode as wmediumd_802154_mode, \
    ERRPROBLink, WStarter, SNRLink, DynamicIntfRef, w_cst, w_txpower, w_pos


class IntfSixLoWPAN(Intf):

    "Basic interface object that can configure itself."

    def __init__(self, name, node=None, port=None, link=None,
                 mac=None, ip6=None, **params):
        """name: interface name (e.g. h1-eth0)
           node: owning node (where this intf most likely lives)
           link: parent link if we're part of a link
           other arguments are passed to config()"""
        self.node = node
        self.name = name
        self.link = link
        self.mac = mac
        self.ip, self.ip6, self.prefixLen = None, ip6, None

        # if interface is lo, we know the ip is 127.0.0.1.
        # This saves an ipaddr command per node
        if self.name == 'lo':
            self.ip = '127.0.0.1'
            self.prefixLen = 8

        node.addWIntf(self, port=port)

        # Save params for future reference
        self.params = params
        self.config(**params)

    def ipAddr6(self, *args):
        self.cmd('ip -6 addr flush ', self.name)
        return self.cmd('ip -6 addr add ', args[0], 'dev', self.name)

    def ipLink(self, *args):
        "Configure ourselves using ip link"
        return self.cmd('ip link set', self.name, *args)

    def setIP6(self, ipstr, prefixLen=None, **args):
        """Set our IP6 address"""
        # This is a sign that we should perhaps rethink our prefix
        # mechanism and/or the way we specify IP addresses
        if '/' in ipstr:
            self.ip6, self.prefixLen = ipstr.split('/')
            return self.ipAddr6(ipstr)
        else:
            if prefixLen is None:
                raise Exception('No prefix length set for IP address %s'
                                % (ipstr,))
            self.ip6, self.prefixLen = ipstr, prefixLen
            return self.ipAddr6('%s/%s' % (ipstr, prefixLen))

    _ip6MatchRegex = re.compile(r'\d+\::\d+')

    def updateIP(self):
        "Return updated IP address based on ip addr"
        # use pexec instead of node.cmd so that we dont read
        # backgrounded output from the cli.
        ipAddr, _err, _exitCode = self.node.pexec(
            'ip -6 addr show %s' % self.name)
        ips = self._ip6MatchRegex.findall(ipAddr)
        self.ip = ips[0] if ips else None
        return self.ip

    def updateMAC(self):
        "Return updated MAC address based on ip addr"
        ipAddr = self.ipAddr()
        macs = self._macMatchRegex.findall(ipAddr)
        self.mac = macs[0] if macs else None
        return self.mac

    # Instead of updating ip and mac separately,
    # use one ipAddr call to do it simultaneously.
    # This saves an ipAddr command, which improves performance.

    def updateAddr(self):
        "Return IP address and MAC address based on ipAddr."
        ipAddr = self.ipAddr()
        ips = self._ipMatchRegex.findall(ipAddr)
        macs = self._macMatchRegex.findall(ipAddr)
        self.ip = ips[0] if ips else None
        self.mac = macs[0] if macs else None
        return self.ip, self.mac

    def IP6( self ):
        "Return IPv6 address"
        return self.ip6

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
            return "UP" in self.ipAddr6()

    def config( self, mac=None, ip6=None, ipAddr=None,
                up=True, **_params ):
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
        self.setParam(r, 'setIP6', ip=ip6)
        self.setParam(r, 'isUp', up=up)
        self.setParam(r, 'ipAddr', ipAddr=ipAddr)
        return r

    def delete( self ):
        "Delete interface"
        self.cmd( 'iwpan dev ' + self.node.params['wpan'][0] + ' del' )
        # We used to do this, but it slows us down:
        # if self.node.inNamespace:
        # Link may have been dumped into root NS
        # quietRun( 'ip link del ' + self.name )
        self.node.delIntf( self )
        self.link = None


class TC6LoWPANLink(TCIntf, IntfSixLoWPAN):
    """Interface customized by tc (traffic control) utility
       Allows specification of bandwidth limits (various methods)
       as well as delay, loss and max queue length"""

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

        result = IntfSixLoWPAN.config(self, **params)

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


class LoWPAN(Link, IntfSixLoWPAN):

    def __init__(self, node1, node2, **params):
        """Create 6LoWPAN pair link
           node: node
           intf: default interface class/constructor"""
        os.system('wpan-hwsim edge add {} {} >/dev/null 2>&1'.format(node1.id, node2.id))
        os.system('wpan-hwsim edge add {} {} >/dev/null 2>&1'.format(node2.id, node1.id))


class LowPANLink(Link, IntfSixLoWPAN):

    def __init__(self, node, wpan, port=None, addr=None,
                 cls=IntfSixLoWPAN, **params):
        """Create 6LoWPAN link to another node.
           node: node
           intf: default interface class/constructor"""
        self.pan = '{}-pan{}'.format(node.name, wpan)
        if 'phy' in node.params:
            self.pan = 'lo{}'.format(node.params['phy'])
        self.name = self.pan
        self.node = node
        node.addWAttr(self, port=wpan)
        self.range = 50
        self.voltage = 10.0
        self.current = 3.0
        self.consumption = 0.0
        self.battery_capacity = 100000
        self.remaining_capacity = self.battery_capacity - self.consumption
        self.ip6 = None
        self.set_attr(node, wpan)
        self.txpower = 20

        wpan = '{}-wpan{}'.format(node.name, wpan)
        if 'phy' in self.node.params:
            os.system('ip link del lo{}'.format(self.node.params['phy']))
            wpan = self.node.params['phy']
        self.name = wpan
        self.ipLink('down')
        self.set_pan_id(wpan, '0xbeef')
        self.add_lowpan_iface(wpan)
        self.ipLink('up')
        self.name = self.pan
        self.ipLink('up')

        if params is None:
            params = {}
        if port is not None:
            params['port'] = port
        if 'port' not in params:
            params['port'] = node.newPort()
        if self.name:
            if 'mac' in self.node.params:
                self.mac = self.node.params['mac']
            if wmediumd_802154_mode.mode:
                self.wmIface = DynamicIntfRef(node, intf=self.name)
                node.wmIfaces[wpan] = self.wmIface
        else:
            ifacename = 'pan%s' % wpan
            self.name = self.wpanName(node, ifacename, node.newPort())
        if not cls:
            cls = IntfSixLoWPAN

        if 'ip6' in node.params:
            params['ip6'] = node.params['ip6']

        intf1 = cls(name=self.name, node=node, mac=addr,
                    link=self, **params)
        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, '6lowpan'

    def add_lowpan_iface(self, wpan):
        return self.cmd('ip link add link {} name {} '
                        'type lowpan'.format(wpan, self.pan))

    def set_pan_id(self, wpan, pan_id):
        return self.cmd('iwpan dev %s set pan_id "%s"' % (wpan, pan_id))

    def set_attr(self, node, wpan):
        for key in self.__dict__.keys():
            if key in node.params:
                if isinstance(node.params[key], list):
                    value = node.params[key][wpan]
                    setattr(self, key, value)
                else:
                    setattr(self, key, node.params[key])

    def wpanName(self, node, ifacename, n):
        "Construct a canonical interface name node-ethN for interface n."
        # Leave this as an instance method for now
        assert self
        return node.name + '-' + ifacename  # + repr(n)

    def pexec(self, *args, **kwargs):
        "Run a command in our owning node"
        return self.node.pexec(*args, **kwargs)

    def delete(self):
        "Delete this link"
        self.intf1.delete()
        self.intf1 = None

    def status(self):
        "Return link status as a string"
        return "(%s %s)" % (self.intf1.status(), self.intf2)


class wmediumd_802154(object):

    def __init__(self, **kwargs):
        self.positions = []
        self.txpowers = []
        self.links = []
        self.mac_list = []
        self.configWmediumd(**kwargs)

    def configWmediumd(self, wlinks, fading_cof, noise_th, sensors, ppm, mediums):
        "Configure wmediumd"
        intfrefs = []
        isnodeaps = []
        intfs = {}
        intf_ids = {}
        nodes = sensors
        intf_id = 0

        for node in nodes:
            node.wmIfaces = []

            for id, intf in enumerate(node.wintfs.values()):
                if not intf.mac:
                    intf.mac = node.wintfs[0].mac[:-1] + str(id)

                if intf.mac not in self.mac_list:
                    intf.wmIface = DynamicIntfRef(node, intf=intf.name)
                    intfrefs.append(intf.wmIface)
                    node.wmIfaces.append(intf.wmIface)
                    intfs[intf.name] = intf
                    intf_ids[intf.name] = intf_id
                    intf_id += 1
                    isnodeaps.append(0)
                    self.mac_list.append(intf.mac)

        if wmediumd_802154_mode.mode == w_cst.INTERFERENCE_MODE:
            self.interference(nodes)
        elif wmediumd_802154_mode.mode == w_cst.SPECPROB_MODE:
            pass
        elif wmediumd_802154_mode.mode == w_cst.ERRPROB_MODE:
            self.error_prob(wlinks)
        else:
            self.snr(wlinks, noise_th)
        mediums_id_list = []
        for medium_index, medium_elements in enumerate(mediums):
            medium_id = medium_index + 1  # Default one is 0
            medium_intf_ids = []
            for node in medium_elements:
                if isinstance(node, str):
                    # Node is a interface
                    medium_intf_ids.append(intf_ids[node])
                    intfs[node].medium_id = medium_id
                else:
                    # Node is station or AP
                    for interface in node.wintfs.values():
                        medium_intf_ids.append(intf_ids[interface.name])
                        interface.medium_id = medium_id
            mediums_id_list.append(medium_intf_ids)
        WStarter(intfrefs=intfrefs, links=self.links, pos=self.positions,
                 fading_cof=fading_cof, noise_th=noise_th, txpowers=self.txpowers,
                 isnodeaps=isnodeaps, ppm=ppm, mediums=mediums_id_list)

    @staticmethod
    def get_position(pos=None):
        if pos: return float(pos[0]), float(pos[1]), float(pos[2])
        return 0, 0, 0

    def interference(self, nodes):
        for node in nodes:
            posX, posY, posZ = self.get_position(node.position) \
                if hasattr(node, 'position') else self.get_position()
            node.lastpos = [posX, posY, posZ]

            for wlan, intf in enumerate(node.wintfs.values()):
                if intf.mac in self.mac_list:
                    if wlan >= 1: posX += 0.1
                    self.positions.append(w_pos(intf.wmIface, [posX, posY, posZ]))
                    self.txpowers.append(w_txpower(intf.wmIface, int(intf.txpower)))

    def error_prob(self, wlinks):
        for link in wlinks:
            link0, link1 = link[0].wmIface, link[1].wmIface
            self.links.append(ERRPROBLink(link0, link1, link[2]))
            self.links.append(ERRPROBLink(link1, link0, link[2]))

    def snr(self, wlinks, noise_th):
        for link in wlinks:
            link0, link1 = link[0].wmIface, link[1].wmIface
            self.links.append(SNRLink(link0, link1, link[0].rssi - noise_th))
            self.links.append(SNRLink(link1, link0, link[0].rssi - noise_th))