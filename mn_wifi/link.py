"""
   Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
   @author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
   @Colaborator: Joaquin Alvarez (j.alvarez@uah.es)
"""

import os
import re
import glob
import subprocess
from time import sleep

from mininet.util import BaseString
from mininet.link import Intf, TCIntf, Link
from mininet.log import error, debug, info
from mn_wifi.manetRoutingProtocols import manetProtocols
from mn_wifi.wmediumdConnector import DynamicIntfRef, \
    WStarter, SNRLink, w_pos, w_cst, w_server, ERRPROBLink, \
    wmediumd_mode, w_txpower, w_gain, w_height


class IntfWireless(Intf):

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

    def join_ibss(self, *args):
        return self.iwdev_cmd('{} ibss join'.format(self.name), *args)
    
    def remove_ibss(self): #uah
        return self.iwdev_cmd('{} ibss leave'. format(self.name))

    def join_mesh(self, ssid, *args):
        return self.iwdev_cmd('{} mesh join'.format(self.name), ssid, 'freq', *args)

    def get_pid_filename(self):
        pidfile = 'mn{}_{}_{}_wpa.pid'.format(os.getpid(), self.node.name, self.id)
        return pidfile

    def get_wpa_cmd(self):
        pidfile = self.get_pid_filename()
        wpasup_flags = self.node.params.get('wpasup_flags', '')
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

    def rec_rssi(self):
        # it sends rssi value to hwsim
        cmd = 'hwsim_mgmt -k %s %s >/dev/null 2>&1' % (self.node.phyid[self.id],
                                                       abs(int(self.rssi)))
        self.cmd(cmd)

    def get_rssi(self, ap_intf, dist):
        from mn_wifi.propagationModels import PropagationModel as ppm
        return float(ppm(self, ap_intf, dist).rssi)

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

    def setSNRWmediumd(self, ap_intf, snr):
        "Send SNR to wmediumd"
        w_server.send_snr_update(SNRLink(self.wmIface, ap_intf.wmIface, snr))
        w_server.send_snr_update(SNRLink(ap_intf.wmIface, self.wmIface, snr))

    def disconnect(self):
        self.node.pexec('iw dev %s disconnect' % self.name)
        self.rssi = 0
        self.associatedTo = ''
        self.channel = 0

    def check_if_wpafile_exist(self):
        file = '%s_%s.staconf' % (self.name, self.id)
        return glob.glob(file)

    def wpaFile(self, ap_intf):
        cmd = ''
        if not ap_intf.config or not self.config:
            if not ap_intf.authmode:
                if not self.passwd:
                    passwd = ap_intf.passwd
                else:
                    passwd = self.passwd

        if 'wpasup_globals' not in self.node.params \
                or ('wpasup_globals' in self.node.params
                    and 'ctrl_interface=' not in self.node.params['wpasup_globals']):
            cmd = 'ctrl_interface=/var/run/wpa_supplicant\n'
        if ap_intf.wps_state and not self.passwd:
            cmd += 'ctrl_interface_group=0\n'
            cmd += 'update_config=1\n'
        else:
            if 'wpasup_globals' in self.node.params:
                cmd += self.node.params['wpasup_globals'] + '\n'
            cmd += 'network={\n'

            if self.config:
                config = self.config
                if config is not []:
                    config = self.config.split(',')
                    self.node.params.pop("config", None)
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
                    if self.active_scan:
                        cmd += '   scan_ssid=1\n'
                    if self.scan_freq:
                        cmd += '   scan_freq=%s\n' % self.scan_freq
                    if self.freq_list:
                        cmd += '   freq_list=%s\n' % self.freq_list
                wpa_key_mgmt = ap_intf.wpa_key_mgmt
                if ap_intf.encrypt == 'wpa3':
                    wpa_key_mgmt = 'SAE'
                cmd += '   key_mgmt=%s\n' % wpa_key_mgmt
                if 'bgscan_threshold' in self.node.params:
                    if 'bgscan_module' not in self.node.params:
                        self.node.params['bgscan_module'] = 'simple'
                    bgscan = 'bgscan=\"%s:%d:%d:%d\"' % \
                             (self.bgscan_module, self.s_inverval,
                              self.bgscan_threshold, self.l_interval)
                    cmd += '   %s\n' % bgscan
                if ap_intf.authmode == '8021x':
                    cmd += '   eap=PEAP\n'
                    cmd += '   identity=\"%s\"\n' % self.radius_identity
                    cmd += '   password=\"%s\"\n' % self.radius_passwd
                    cmd += '   phase2=\"autheap=MSCHAPV2\"\n'
            cmd += '}'

        fileName = '%s_%s.staconf' % (self.name, self.id)
        os.system('echo \'%s\' > %s' % (cmd, fileName))

    def wpa(self, ap_intf):
        self.wpaFile(ap_intf)
        self.wpa_pexec()

    def handover_ieee80211r(self, ap_intf):
        self.node.pexec('wpa_cli -i %s roam %s' % (self.name, ap_intf.mac))

    def wep(self, ap_intf):
        if self.passwd:
            passwd = self.passwd
        else:
            passwd = ap_intf.passwd
        self.wep_connect(passwd, ap_intf)

    def wep_connect(self, passwd, ap_intf):
        self.node.pexec('iw dev %s connect %s key d:0:%s' % (self.name, ap_intf.ssid, passwd))

    def update_client_params(self, ap_intf):
        self.freq = ap_intf.freq
        self.channel = ap_intf.channel
        self.mode = ap_intf.mode
        self.ssid = ap_intf.ssid
        self.range = self.node.getRange(self)

    def iwconfig_con(self, ap_intf):
        cmd = 'iwconfig %s essid %s ap %s' % (self.name, ap_intf.ssid, ap_intf.mac)
        return cmd

    def associate_noEncrypt(self, ap_intf):
        # iwconfig is still necessary, since iw doesn't include essid like iwconfig does.
        self.node.pexec(self.iwconfig_con(ap_intf))
        debug('\n')

    def update(self, ap_intf):
        if self.associatedTo and self.node in ap_intf.associatedStations:
            ap_intf.associatedStations.remove(self.node)
        self.update_client_params(ap_intf)
        ap_intf.associatedStations.append(self.node)
        self.associatedTo = ap_intf.node

    def associate_infra(self, ap_intf):
        associated = 0
        if ap_intf.ieee80211r and (not self.encrypt or 'wpa' in self.encrypt):
            if not self.associatedTo:
                wpa_file_exists = self.check_if_wpafile_exist()
                if wpa_file_exists:
                    self.handover_ieee80211r(ap_intf)
                else:
                    self.wpa(ap_intf)
            else:
                self.handover_ieee80211r(ap_intf)
            associated = 1
        elif not ap_intf.encrypt:
            associated = 1
            self.associate_noEncrypt(ap_intf)
        else:
            if not self.associatedTo:
                if 'wpa' in ap_intf.encrypt and (not self.encrypt or 'wpa' in self.encrypt):
                    self.wpa(ap_intf)
                    associated = 1
                elif ap_intf.encrypt == 'wep':
                    self.wep(ap_intf)
                    associated = 1
        if associated:
            self.update(ap_intf)

    def configureWirelessLink(self, ap_intf):
        dist = self.node.get_distance_to(ap_intf.node)
        if dist <= ap_intf.range:
            if not wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
                if self.rssi == 0:
                    self.update_client_params(ap_intf)
            if ap_intf != self.associatedTo or \
                    not self.associatedTo:
                self.associate_infra(ap_intf)
                if wmediumd_mode.mode == w_cst.WRONG_MODE:
                    if dist >= 0.01:
                        wirelessLink(self, dist)
                if self.node != ap_intf.associatedStations:
                    ap_intf.associatedStations.append(self.node)
            if not wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
                self.rssi = self.get_rssi(ap_intf, dist)

    def associate(self, ap_intf):
        "Associate to Access Point"
        if hasattr(self.node, 'position'):
            self.configureWirelessLink(ap_intf)
        else:
            self.associate_infra(ap_intf)

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

    def updateIP(self):
        "Return updated IP address based on ip addr"
        # use pexec instead of node.cmd so that we dont read
        # backgrounded output from the cli.
        ipAddr, _err, _exitCode = self.node.pexec(
            'ip addr show %s' % self.name)
        ips = self._ipMatchRegex.findall(ipAddr)
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
        #self.cmd('iw dev ' + self.name + ' del')
        # We used to do this, but it slows us down:
        # if self.node.inNamespace:
        # Link may have been dumped into root NS
        # quietRun( 'ip link del ' + self.name )
        #self.node.delIntf(self)
        self.link = None


class TCWirelessLink(TCIntf, IntfWireless):
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


class _4address(Link, IntfWireless):

    node = None

    def __init__(self, node1, node2, port1=None, port2=None):
        """Create 4addr link to another node.
           node1: first node
           node2: second node
           intf: default interface class/constructor"""
        ap = node1  # ap
        cl = node2  # client
        cl_intfname = '%s.wds' % cl.name

        if not hasattr(node1, 'position'):
            self.set_pos(node1)
        if not hasattr(node2, 'position'):
            self.set_pos(node2)

        if cl_intfname not in cl.params['wlan']:
            wlan = cl.params['wlan'].index(port1) if port1 else 0
            apwlan = ap.params['wlan'].index(port2) if port2 else 0

            intf = cl.wintfs[wlan]
            ap_intf = ap.wintfs[apwlan]

            self.node = cl
            self.add4addrIface(intf, cl_intfname)
            sleep(1)

            intf.mode = ap_intf.mode
            intf.channel = ap_intf.channel
            intf.freq = ap_intf.freq
            intf.txpower = ap_intf.txpower
            intf.antennaGain = ap_intf.antennaGain
            cl.params['wlan'].append(cl_intfname)
            sleep(1)

            params1, params2 = {}, {}
            params1['port'] = cl.newPort()
            params2['port'] = ap.newPort()
            intf1 = IntfWireless(name=cl_intfname, node=cl, link=self, **params1)
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
            self.bring4addrIfaceDown()
            self.setMAC(cl.wintfs[1])
            self.bring4addrIfaceUP()
            self.iwdev_cmd('%s connect %s %s' % (cl.params['wlan'][1],
                                                 ap_intf.ssid, ap_intf.mac))

        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2

    def set_pos(self, node):
        nums = re.findall(r'\d+', node.name)
        if nums:
            id = int(hex(int(nums[0]))[2:])
            node.position = (10, round(id, 2), 0)

    def bring4addrIfaceDown(self):
        self.cmd('ip link set dev %s.wds down' % self.node)

    def bring4addrIfaceUP(self):
        self.cmd('ip link set dev %s.wds up' % self.node)

    def setMAC(self, intf):
        self.cmd('ip link set dev %s.wds addr %s'
                 % (intf.node, intf.mac))

    def add4addrIface(self, intf, intfName):
        self.iwdev_cmd('%s interface add %s type managed 4addr on' %
                       (intf.name, intfName))

    def status(self):
        "Return link status as a string"
        return "(%s %s)" % (self.intf1.status(), self.intf2)


class WirelessLinkAP(Link):

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
            params['port'] = port

        params['port'] = node.newPort()
        if not intfName:
            ifacename = 'wlan'
            intfName = self.wlanName(node, ifacename, params['port'])
        intf1 = cls(name=intfName, node=node,
                    link=self, mac=addr, **params)

        intf2 = 'wifi'
        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2
    # pylint: enable=too-many-branches

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

    def status(self):
        "Return link status as a string"
        return "(%s %s)" % (self.intf1.status(), self.intf2)


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
        self.vssid = []
        self.vifaces = []
        self.associatedStations = []
        self.antennaGain = 5.0
        self.antennaHeight = 1.0
        self.channel = 1
        self.freq = 2.412
        self.range = 0
        self.txpower = 14
        self.auth_algs = None
        self.authmode = None
        self.band = None
        self.beacon_int = None
        self.config = None
        self.config_methods = None
        self.encrypt = None
        self.ht_capab = None
        self.ieee80211r = None
        self.id = wlan
        self.ip = None
        self.ip6 = None
        self.isolate_clients = None
        self.mac = None
        self.mode = 'g'
        self.mobility_domain = None
        self.passwd = None
        self.shared_secret = None
        self.ssid = None
        self.wpa_key_mgmt = None
        self.rsn_pairwise = None
        self.radius_server = None
        self.wps_state = None
        self.device_type = None
        self.wpa_psk_file = None
        self.wep_key0 = None
        self.link = None

        if intf and hasattr(intf, 'wmIface'):
            self.wmIface = intf.wmIface

        for key in self.__dict__.keys():
            if key in node.params:
                if isinstance(node.params[key], BaseString):
                    setattr(self, key, node.params[key])
                elif isinstance(node.params[key], list):
                    value = node.params[key][wlan].split(',')
                    setattr(self, key, value[0])
                elif isinstance(node.params[key], int):
                    setattr(self, key, node.params[key])


class VirtualMaster(master, TCWirelessLink):
    "master class"
    def __init__(self, node, wlan, id, port=None, intf=None):
        self.name = intf
        node.addWAttr(self, port=port)
        self.node = node
        self.params = {}
        self.stationsInRange = {}
        self.vssid = []
        self.vifaces = []
        self.ssid = 'none'
        self.associatedStations = []
        self.antennaGain = 5.0
        self.antennaHeight = 1.0
        self.channel = 1
        self.freq = 2.412
        self.range = 0
        self.txpower = 14
        self.auth_algs = None
        self.authmode = None
        self.band = None
        self.beacon_int = None
        self.config = None
        self.config_methods = None
        self.encrypt = None
        self.ht_capab = None
        self.ieee80211r = None
        self.id = wlan
        self.ip = None
        self.ip6 = None
        self.isolate_clients = None
        self.mac = None
        self.mode = 'g'
        self.mobility_domain = None
        self.passwd = None
        self.shared_secret = None
        self.wpa_key_mgmt = None
        self.rsn_pairwise = None
        self.radius_server = None
        self.wps_state = None
        self.device_type = None
        self.wpa_psk_file = None
        self.wep_key0 = None
        self.link = None

        if intf and hasattr(intf, 'wmIface'):
            self.wmIface = intf.wmIface

        for key in self.__dict__.keys():
            if key in node.params:
                if key != 'ssid':
                    if isinstance(node.params[key], BaseString):
                        setattr(self, key, node.params[key])
                    elif isinstance(node.params[key], list):
                        value = node.params[key][wlan].split(',')
                        setattr(self, key, value[id])
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
                    arg = node.params[key][0].split(',')
                    setattr(self, key, arg[wlan])
                elif isinstance(node.params[key], int):
                    setattr(self, key, node.params[key])


class _4addrClient(TCWirelessLink):
    "managed class"
    def __init__(self, node, wlan):
        self.node = node
        self.id = wlan
        self.ip = None
        self.freq = node.wintfs[0].freq
        self.mac = node.wintfs[wlan-1].mac
        self.range = node.wintfs[0].range
        self.txpower = node.wintfs[wlan-1].txpower
        self.antennaGain = 5.0
        self.name = node.params['wlan'][wlan]
        self.stationsInRange = {}
        self.associatedStations = []
        self.apsInRange = {}
        self.params = {}
        #node.addWIntf(self, port=wlan)
        node.addWAttr(self, port=wlan)


class _4addrAP(TCWirelessLink):
    "managed class"
    def __init__(self, node, wlan):
        self.node = node
        self.ip = None
        self.id = wlan
        self.freq = node.wintfs[0].freq
        self.mac = node.wintfs[0].mac
        self.range = node.wintfs[0].range
        self.txpower =  node.wintfs[0].txpower
        self.antennaGain = 5.0
        self.name = node.params['wlan'][wlan]
        self.stationsInRange = {}
        self.associatedStations = []
        self.params = {}
        #node.addWIntf(self, port=wlan)
        node.addWAttr(self, port=wlan)


class wmediumd(object):

    positions = []
    txpowers = []
    links = []

    def __init__(self, **kwargs):
        self.configWmediumd(**kwargs)

    def configWmediumd(self, wlinks, fading_cof, noise_th, stations,
                       aps, cars, ppm, maclist):
        "Configure wmediumd"
        intfrefs = []
        isnodeaps = []
        mac_list = []

        nodes = stations + aps + cars
        for node in nodes:
            node.wmIfaces = []
            for intf in node.wintfs.values():
                if intf.mac not in mac_list:
                    intf.wmIface = DynamicIntfRef(node, intf=intf.name)
                    intfrefs.append(intf.wmIface)
                    node.wmIfaces.append(intf.wmIface)

                    if (isinstance(intf, master)
                        or (node in aps and (not isinstance(intf, managed)
                                             and not isinstance(intf, adhoc)))):
                        isnodeaps.append(1)
                    else:
                        isnodeaps.append(0)
                    mac_list.append(intf.mac)

        if wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
            self.interference(nodes)
        elif wmediumd_mode.mode == w_cst.SPECPROB_MODE:
            pass
        elif wmediumd_mode.mode == w_cst.ERRPROB_MODE:
            self.error_prob(wlinks)
        else:
            self.snr(wlinks)
        WStarter(intfrefs=intfrefs, links=self.links, pos=self.positions,
                 fading_cof=fading_cof, noise_th=noise_th, txpowers=self.txpowers,
                 isnodeaps=isnodeaps, ppm=ppm, maclist=maclist)

    def interference(self, nodes):
        for node in nodes:
            if not hasattr(node, 'position'):
                posX, posY, posZ = 0, 0, 0
            else:
                posX = float(node.position[0])
                posY = float(node.position[1])
                posZ = float(node.position[2])
            node.lastpos = [posX, posY, posZ]

            mac_list = []
            for wlan, intf in enumerate(node.wintfs.values()):
                if intf.mac not in mac_list:
                    if wlan >= 1:
                        posX += 0.1
                    self.positions.append(w_pos(intf.wmIface, [posX, posY, posZ]))
                    self.txpowers.append(w_txpower(intf.wmIface, float(intf.txpower)))
                    mac_list.append(intf.mac)

    def error_prob(self, wlinks):
        for link in wlinks:
            self.links.append(ERRPROBLink(link[0].wmIface, link[1].wmIface,
                              link[2]))
            self.links.append(ERRPROBLink(link[1].wmIface, link[0].wmIface,
                              link[2]))

    def snr(self, wlinks):
        for link in wlinks:
            self.links.append(SNRLink(link[0].wmIface, link[1].wmIface,
                              link[0].rssi - (-91)))
            self.links.append(SNRLink(link[1].wmIface, link[0].wmIface,
                              link[0].rssi - (-91)))


class wirelessLink(object):

    dist = 0
    noise = 0
    eqLoss = '(dist * 2) / 1000'
    eqDelay = '(dist / 10) + 1'
    eqLatency = '(dist / 10)/2'
    eqBw = ' * (1.01 ** -dist)'

    def __init__(self, intf, dist=0):
        params = dict()
        params['latency'] = self.getLatency(dist)
        params['loss'] = self.getLoss(dist)
        params['bw'] = self.getBW(intf, dist)
        self.config_tc(intf, **params)

    def getDelay(self, dist):
        "Based on RandomPropagationDelayModel"
        return eval(self.eqDelay)

    def getLatency(self, dist):
        return eval(self.eqLatency)

    def getLoss(self, dist):
        return eval(self.eqLoss)

    def getBW(self, intf, dist):
        # dist is used by eval
        custombw = intf.getCustomRate()
        rate = eval(str(custombw) + self.eqBw)

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
    def config_tc(cls, intf, **params):
        if intf.ifb:
            cls.tc(intf.node, intf.ifb, **params)
        cls.tc(intf.node, intf.name, **params)

    @classmethod
    def tc(cls, node, iface, bw=0, loss=0, latency=0):
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


class WifiDirectLink(IntfWireless):

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
        self.antennaGain = intf.antennaGain
        self.txpower = intf.txpower
        self.freq = intf.freq
        self.ip6 = intf.ip6
        self.ip = intf.ip

        node.addWIntf(self, port=wlan)
        node.addWAttr(self, port=wlan)

        self.config_()
        cmd = self.get_wpa_cmd()
        node.cmd(cmd)

    def get_filename(self):
        suffix = 'wifiDirect.conf'
        filename = "mn%d_{}_{}".format(os.getpid(), self.name, suffix)
        return filename

    def get_wpa_cmd(self):
        filename = self.get_filename()
        cmd = 'wpa_supplicant -B -Dnl80211 -c{} -i{}'.format(filename, self.name)
        return cmd

    def config_(self):
        filename = self.get_filename()
        cmd = "echo \'"
        cmd += 'ctrl_interface=/var/run/wpa_supplicant\
              \nap_scan=1\
              \np2p_go_ht40=1\
              \ndevice_name=%s\
              \ndevice_type=1-0050F204-1\
              \np2p_no_group_iface=1' % self.name
        cmd += ("\' > %s" % filename)
        self.set_config(cmd)

    @staticmethod
    def set_config(cmd):
        subprocess.check_output(cmd, shell=True)


class PhysicalWifiDirectLink(WifiDirectLink):

    def __init__(self, node, freq=2.412, txpower=14, intf=None):
        "configure wifi-direct"
        self.name = intf
        self.node = node
        self.txpower = txpower
        self.freq = freq
        self.antennaGain = 5
        self.range = 100
        node.addWIntf(self)
        node.addWAttr(self)

        for wlan, intf in enumerate(node.wintfs.values()):
            if intf.name == self.name:
                break

        self.txpower = node.wintfs[0].txpower
        node.params['wlan'].append(self.name)
        self.mac = None

        self.config_()
        cmd = self.get_wpa_cmd()
        os.system(cmd)


class adhoc(IntfWireless):

    node = None

    def __init__(self, node, intf=None, ssid='adhocNet',
                 channel=1, mode='g', passwd=None, ht_cap='',
                 proto=None, ibss="02:CA:FF:EE:BA:01", **params):
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
        self.link = intf.link
        self.encrypt = intf.encrypt
        self.antennaGain = intf.antennaGain
        self.passwd = passwd
        self.mode = mode
        self.channel = channel
        self.ht_cap = ht_cap
        self.associatedTo = 'adhoc'
        self.ibss = ibss

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
        self.configureAdhoc() #modificacion JAHUAH

        self.txpower = intf.txpower
        self.range = intf.range

        if proto:
            manetProtocols(intf, proto, **params)

    def configureAdhoc(RUSAGE_SELF):
        "Configure Wireless Ad Hoc"
        self.set_dev_type('ibss')
        self.ipLink('up')

        if self.passwd:
            self.setSecuredAdhoc()
        else:
            self.join_ibss(self.ssid, self.format_freq(),
                self.ht_cap, self.ibss)

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
            node.wmIfaces[wlan] = self.wmIface

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
            self.join_mesh(self.ssid, self.format_freq(), self.ht_cap)

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
                 ht_cap='', freq=2.412):
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
        self.freq = freq

        node.wintfs[wlan].ssid = ssid
        if int(node.wintfs[wlan].range) == 0:
            intf = node.params['wlan'][wlan]
            node.wintfs[wlan].range = node.getRange(intf, 95)

        self.name = intf
        self.setPhysicalMeshIface(node, wlan, intf)
        self.freq = self.format_freq()

        self.join_mesh(self.ssid, self.format_freq(), self.ht_cap)

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
