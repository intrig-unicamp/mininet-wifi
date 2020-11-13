"""
   Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
   @author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
   @Contributor: Joaquin Alvarez (j.alvarez@uah.es)
"""

import os
import re
import glob
import subprocess
from time import sleep
from subprocess import check_output as co, CalledProcessError

from mn_wifi.devices import DeviceRate
from mininet.link import Intf, TCIntf, Link
from mininet.log import error, debug, info
from mn_wifi.manetRoutingProtocols import manetProtocols
from mn_wifi.propagationModels import SetSignalRange, GetPowerGivenRange
from mn_wifi.wmediumdConnector import DynamicIntfRef, \
    WStarter, SNRLink, w_pos, w_cst, w_server, ERRPROBLink, \
    wmediumd_mode, w_txpower, w_gain, w_height


class IntfWireless(Intf):
    "Basic interface object that can configure itself."

    def __init__(self, name, node=None, port=None, link=None,
                 mac=None, **params):
        """name: interface name (e.g. sta1-wlan0)
           node: owning node (where this intf most likely lives)
           link: parent link if we're part of a link
           other arguments are passed to config()"""
        self.node = node
        self.name = name
        self.link = link
        self.port = port
        self.mac = mac
        self.ip, self.ip6, self.prefixLen, self.prefixLen6 = None, None, None, None
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

    def iwdev_cmd(self, *args):
        return self.cmd('iw dev', *args)

    def iwdev_pexec(self, *args):
        return self.pexec('iw dev', *args)

    def wpa_cli_pexec(self, *args):
        return self.pexec('wpa_cli -i ', self.name, *args)

    def wpa_cli_cmd(self, *args):
        return self.cmd('wpa_cli -i ', self.name, *args)

    def set_dev_type(self, *args):
        return self.iwdev_cmd('{} set type {}'.format(self.name, *args))

    def add_dev_type(self, new_name, type):
        return self.iwdev_cmd('{} interface add {} type {}'.format(
            self.name, new_name, type))

    def set_bitrates(self, *args):
        return self.iwdev_cmd('{} set bitrates'.format(self.name), *args)

    def join_ibss(self, *args):
        return self.iwdev_cmd('{} ibss join'.format(self.name), *args)

    def ibss_leave(self):
        return self.iwdev_cmd('{} ibss leave'.format(self.name))

    def mesh_join(self, ssid, *args):
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

    def kill_hostapd_process(self):
        info('\n')
        pattern = "mn%d_%s.apconf" % (os.getpid(), self.name)
        while True:
            try:
                pids = co(['pgrep', '-f', pattern])
            except CalledProcessError:
                pids = ''
            if pids:
                self.cmd('rm %s' % pattern)
                self.cmd('pkill -9 -f \'%s\'' % pattern)
            else:
                break

    def setGainWmediumd(self, gain):
        "Sends Antenna Gain to wmediumd"
        if wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
            w_server.update_gain(w_gain(self.wmIface, int(gain)))

    def setHeightWmediumd(self, height):
        "Sends Antenna Height to wmediumd"
        if wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
            w_server.update_height(w_height(self.wmIface, int(height)))

    def setTXPowerWmediumd(self, txpower):
        "Sends TxPower to wmediumd"
        if wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
            w_server.update_txpower(w_txpower(self.wmIface, int(txpower)))

    def setSNRWmediumd(self, ap_intf, snr):
        "Send SNR to wmediumd"
        w_server.send_snr_update(SNRLink(self.wmIface, ap_intf.wmIface, snr))
        w_server.send_snr_update(SNRLink(ap_intf.wmIface, self.wmIface, snr))

    def sendIntfTowmediumd(self):
        "Dynamically sending nodes to wmediumd"
        self.wmIface = DynamicIntfRef(self.node, intf=self.name)
        self.node.wmIfaces.append(self.wmIface)
        # register interface
        w_server.register_interface(self.mac)

    def getCustomRate(self):
        mode_rate = {'a': 11, 'b': 3, 'g': 11, 'n': 600, 'ac': 1000}
        return mode_rate.get(self.mode)

    def getRate(self):
        mode_rate = {'a': 54, 'b': 11, 'g': 54, 'n': 300, 'ac': 600}
        return mode_rate.get(self.mode)

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
        return '2.412'

    def setFreq(self, freq, intf=None):
        return self.iwdev_cmd('{} set freq {}'.format(intf, freq))

    def format_freq(self):
        return int(format(self.freq, '.3f').replace('.', ''))

    def setReg(self):
        if self.mode == 'a' or self.mode == 'ac':
            self.pexec('iw reg set US')

    def setIntfName(self, *args):
        self.cmd('ip link set %s down' % self.name)
        self.cmd('ip link set %s name %s' % (self.name, args[0]))
        self.cmd('ip link set %s up' % args[0])
        self.setIntfAttrs(*args)

    def setIntfAttrs(self, *args):
        self.node.params['wlan'][int(args[1])] = args[0]
        for intf in self.node.intfs:
            if self.node.intfs[intf].name == self.name:
                self.node.intfs[intf].name = args[0]
        self.name = args[0]

    def setAPChannel(self, channel):
        self.freq = self.get_freq()
        self.pexec('hostapd_cli -i %s chan_switch %s %s' % (
            self.name, channel, str(self.format_freq())))

    def setMeshChannel(self, channel):
        self.freq = self.get_freq()
        self.iwdev_cmd('{} set channel {}'.format(self.name, channel))

    def setChannel(self, channel):
        "Set Channel"
        from mn_wifi.node import AP
        self.channel = channel
        if isinstance(self, AP):
            self.setAPChannel(channel)
        elif isinstance(self, mesh):
            self.setMeshChannel(channel)
        elif isinstance(self, adhoc):
            self.ibss_leave()
            adhoc(node=self.node, intf=self, chann=channel)

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
                        cmd += ' && ip -6 addr add %s dev %s' % \
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

    def getTxPowerGivenRange(self):
        txpower = GetPowerGivenRange(self).txpower
        self.setTxPower(txpower)
        if self.txpower == 1:
            min_range = int(SetSignalRange(self).range)
            if self.range < min_range:
                info("*** %s: the signal range should be "
                     "changed to (at least) %sm\n" % (self.name, min_range))
                info("*** >>> See https://mininet-wifi.github.io/faq/#q7 for more information\n")
        else:
            info("*** %s: signal range of %sm requires tx power equal to %sdBm.\n"
                 % (self.name, self.range, txpower))

    def setDefaultRange(self):
        if not self.static_range:
            self.range = SetSignalRange(self).range

    def setAntennaGain(self, gain):
        self.antennaGain = int(gain)
        self.setDefaultRange()
        self.setGainWmediumd(gain)
        self.node.configLinks()

    def setAntennaHeight(self, height):
        self.antennaHeight = int(height)
        self.setDefaultRange()
        self.setHeightWmediumd(height)
        self.node.configLinks()

    def setRange(self, range):
        self.range = float(range)
        self.getTxPowerGivenRange()
        self.node.configLinks()

    def setTxPower(self, txpower):
        self.iwdev_cmd('{} set txpower fixed {}'.format(self.name, txpower * 100))
        debug('\n')
        self.txpower = txpower
        self.setDefaultRange()
        self.setTXPowerWmediumd(txpower)
        self.node.configLinks()

    def setManagedMode(self):
        wlan = self.node.params['wlan'].index(self.name)
        if isinstance(self, mesh):
            self.iwdev_cmd('%s del' % self.name)
            self.name = '%s-wlan%s' % (self, self.id)
            self.node.params['wlan'][wlan] = self.name
        elif isinstance(self, master):
            self.kill_hostapd_process()
        self.set_dev_type('managed')
        managed(self.node, wlan, intf=self)

    def setMasterMode(self, ssid='new-ssid', **kwargs):
        "set Interface to AP mode"
        wlan = self.node.params['wlan'].index(self.name)
        master(self.node, wlan, port=wlan, intf=self)

        intf = self.node.getNameToWintf(self)
        if int(intf.range) == 0:
            intf.setDefaultRange()

        intf.ssid = ssid
        for key, value in kwargs.items():
            setattr(intf.node, key, value)

        if not intf.mac:
            intf.mac = intf.getMAC()

        HostapdConfig(intf)

    def setMeshMode(self, **kwargs):
        mesh(self.node, intf=self, **kwargs)

    def setAdhocMode(self, **kwargs):
        if isinstance(self, adhoc):
            self.ibss_leave()
        adhoc(self.node, intf=self, **kwargs)

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

    def setIP6(self, ipstr, prefixLen6=None, **args):
        """Set our IP6 address"""
        # This is a sign that we should perhaps rethink our prefix
        # mechanism and/or the way we specify IP addresses
        if '/' in ipstr:
            self.ip6, self.prefixLen6 = ipstr.split('/')
            return self.ipAddr(ipstr)
        else:
            if prefixLen6 is None:
                raise Exception('No prefix length set for IP address %s'
                                % (ipstr,))
            self.ip6, self.prefixLen6 = ipstr, prefixLen6
            return self.ipAddr('%s/%s' % (ipstr, prefixLen6))

    def check_if_wpafile_exist(self):
        file = '%s_%s.staconf' % (self.name, self.id)
        return glob.glob(file)

    def wpaFile(self, ap_intf):
        cmd = ''
        if not ap_intf.config or not self.config:
            if not ap_intf.authmode:
                passwd = ap_intf.passwd if not self.passwd else self.passwd

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
                if self.bgscan_module:
                    cmd += '   bgscan=\"%s:%d:%d:%d\"\n' % \
                             (self.bgscan_module, self.s_inverval,
                              self.bgscan_threshold, self.l_interval)
                if ap_intf.authmode == '8021x':
                    cmd += '   eap=PEAP\n'
                    cmd += '   identity=\"%s\"\n' % self.radius_identity
                    cmd += '   password=\"%s\"\n' % self.radius_passwd
                    cmd += '   phase2=\"autheap=MSCHAPV2\"\n'
            cmd += '}'

        pattern = '%s_%s.staconf' % (self.name, self.id)
        os.system('echo \'%s\' > %s' % (cmd, pattern))

    def wpa(self, ap_intf):
        self.wpaFile(ap_intf)
        self.wpa_pexec()

    def update_client_params(self, ap_intf):
        self.freq = ap_intf.freq
        self.channel = ap_intf.channel
        self.mode = ap_intf.mode
        self.ssid = ap_intf.ssid

    def wep(self, ap_intf):
        passwd = self.passwd if self.passwd else ap_intf.passwd
        self.wep_connect(passwd, ap_intf)

    def setConnected(self, ap_intf):
        self.associatedTo = ap_intf
        ap_intf.associatedStations.append(self)

    def setDisconnected(self, ap_intf):
        self.rssi = 0
        self.channel = 0
        self.associatedTo = None
        if self in ap_intf.associatedStations:
            ap_intf.associatedStations.remove(self)

    def roam(self, bssid):
        self.wpa_cli_cmd('roam %s' % bssid)

    def wep_connect(self, passwd, ap_intf):
        self.iwdev_cmd('{} connect {} key d:0:{}'.format(
            self.name, ap_intf.ssid, passwd))

    def disconnect_pexec(self, ap_intf):
        self.iwdev_pexec('{} disconnect'.format(self.name))
        self.setDisconnected(ap_intf)

    def disconnect(self, ap_intf):
        self.iwdev_cmd('{} disconnect'.format(self.name))
        self.setDisconnected(ap_intf)

    def iw_connect(self, ap_intf):
        self.pexec('iw dev %s connect %s %s' %
                   (self.name, ap_intf.ssid, ap_intf.mac))
        self.setConnected(ap_intf)

    def iwconfig_connect(self, ap_intf):
        self.pexec('iwconfig %s essid %s ap %s' %
                   (self.name, ap_intf.ssid, ap_intf.mac))
        self.setConnected(ap_intf)

    def associate_infra(self, ap_intf):
        associated = 0
        if ap_intf.ieee80211r and (not self.encrypt or 'wpa' in self.encrypt):
            if not self.associatedTo:
                wpa_file_exists = self.check_if_wpafile_exist()
                if wpa_file_exists:
                    self.roam(ap_intf.mac)
                else:
                    self.wpa(ap_intf)
            else:
                self.roam(ap_intf.mac)
            associated = 1
        elif not ap_intf.encrypt:
            associated = 1
            self.iwconfig_connect(ap_intf)
        else:
            if not self.associatedTo:
                if 'wpa' in ap_intf.encrypt and (not self.encrypt or 'wpa' in self.encrypt):
                    self.wpa(ap_intf)
                    associated = 1
                elif ap_intf.encrypt == 'wep':
                    self.wep(ap_intf)
                    associated = 1
        if associated:
            self.update_client_params(ap_intf)

    def configureWirelessLink(self, ap_intf):
        dist = self.node.get_distance_to(ap_intf.node)
        if dist <= ap_intf.range:
            if not wmediumd_mode.mode == w_cst.INTERFERENCE_MODE:
                if self.rssi == 0:
                    self.update_client_params(ap_intf)
            if ap_intf != self.associatedTo or not self.associatedTo:
                self.associate_infra(ap_intf)
                if wmediumd_mode.mode == w_cst.WRONG_MODE:
                    if dist >= 0.01: ConfigWLink(self, dist)
                if self not in ap_intf.associatedStations:
                    ap_intf.associatedStations.append(self)
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
        ipAddr, _err, _exitCode = self.node.pexec('ip addr show %s' % self.name)
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
            # error( "Error setting %s up: %s " % ( self.name, cmdOutput ) )
            return False if cmdOutput else True
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
        # self.cmd('iw dev ' + self.name + ' del')
        # We used to do this, but it slows us down:
        # if self.node.inNamespace:
        # Link may have been dumped into root NS
        # quietRun( 'ip link del ' + self.name )
        # self.node.delIntf(self)
        self.link = None


class HostapdConfig(IntfWireless):

    write_mac = False
    nm_conf_file = '/etc/NetworkManager/conf.d/unmanaged.conf'

    def __init__(self, intf):
        'configure hostapd'
        self.check_nm(intf)
        self.configure(intf)
        self.set_mac_viface(intf)

    @staticmethod
    def set_mac_viface(intf):
        for vintf in intf.node.wintfs.values():
            if isinstance(vintf, VirtualMaster):
                vintf.node.params['wlan'].append(vintf.name)
                vintf.mac = vintf.getMAC()

    def check_nm(self, intf):
        self.checkNetworkManager(intf)

        if 'vssids' in intf.node.params:
            if isinstance(intf.node.params['vssids'], list):
                vssids = intf.node.params['vssids'][intf.id].split(',')
            else:
                vssids = intf.node.params['vssids'].split(',')
            for id, vssid in enumerate(vssids):
                iface = '%s-%s' % (intf.name, id)
                intf.vifaces.append(iface)
                intf.vssid.append(vssid)
                TCLinkWireless(intf.node, intfName=iface)
                VirtualMaster(intf.node, intf.id, intf=iface)

    def configure(self, intf):
        if 'link' not in intf.node.params:
            cmd = ''
            if 'phywlan' in intf.node.params:
                for id, intf in enumerate(intf.node.wintfs.values()):
                    cmd = self.setConfig(intf)
            else:
                cmd = self.setConfig(intf)
            if 'vssids' in intf.node.params:
                for vwlan, id in enumerate(intf.vifaces):
                    cmd += self.virtual_intf(intf, vwlan)

            cmd += "\nctrl_interface=/var/run/hostapd"
            cmd += "\nctrl_interface_group=0"
            self.ap_config_file(cmd, intf)

            if '-' in intf.name: intf.setMAC(intf.mac)

            if 'phywlan' in intf.node.params: intf.node.params.pop('phywlan', None)

            if not wmediumd_mode.mode: self.setBw(intf, intf.id)
            intf.freq = intf.get_freq()

    def setConfig(self, intf):
        """Configure AP
        :param ap: ap node
        :param wlan: wlan id"""
        if intf.ssid:
            if intf.encrypt and 'config' not in intf.node.params:
                if 'wpa' in intf.encrypt:
                    intf.auth_algs = 1
                    if intf.ieee80211r:
                        intf.wpa_key_mgmt = 'FT-EAP' if intf.authmode else 'FT-PSK'
                    else:
                        intf.wpa_key_mgmt = 'WPA-EAP' if intf.authmode else 'WPA-PSK'
                    intf.rsn_pairwise = 'TKIP CCMP' if intf.encrypt == 'wpa' else 'CCMP'
                    if not intf.authmode: intf.wpa_passphrase = intf.passwd
                elif intf.encrypt == 'wep':
                    intf.auth_algs = 2
                    intf.wep_key0 = intf.passwd

            if isinstance(intf, adhoc):
                adhoc(intf.node, intf.id)
            else:
                return self.setHostapdConfig(intf)

    @staticmethod
    def get_mode_config(intf):
        cmd = ''
        if intf.mode == 'n':
            cmd += "\nhw_mode=a" if intf.freq and str(intf.freq) == '5.0' else "\nhw_mode=g"
        elif intf.mode == 'a':
            cmd += "\ncountry_code=%s" % intf.country_code
            cmd += "\nhw_mode=%s" % intf.mode
        elif intf.mode == 'ac' or intf.mode == 'ax':
            cmd += "\ncountry_code=%s" % intf.country_code
            cmd += "\nhw_mode=a"
            if intf.mode == 'ax': cmd += "\nieee80211ax=1"
        else:
            cmd += "\nhw_mode=%s" % intf.mode
        return cmd

    def virtual_intf(self, intf, vwlan, cmd=''):
        intf.txpower = intf.node.wintfs[0].txpower
        intf.antennaGain = intf.node.wintfs[0].antennaGain
        intf.antennaHeight = intf.node.wintfs[0].antennaHeight
        cmd += '\n'
        cmd += "\nbss=%s" % intf.vifaces[vwlan]
        cmd += "\nssid=%s" % intf.vssid[vwlan]
        if intf.encrypt:
            if intf.encrypt == 'wep':
                cmd += "\nauth_algs=%s" % intf.auth_algs
                cmd += "\nwep_default_key=0"
                cmd += self.verifyWepKey(intf.wep_key0)
        return cmd

    def setHostapdConfig(self, intf):
        "Set hostapd config"
        cmd = "echo \'"
        args = ['max_num_sta', 'beacon_int', 'rsn_preauth']

        cmd += 'interface=%s' % intf.node.params.get('phywlan', intf.name)
        cmd += '\ndriver=nl80211'
        cmd += '\nssid=%s' % intf.ssid
        cmd += '\nwds_sta=1'
        cmd += self.get_mode_config(intf)  # get mode
        cmd += "\nchannel=%s" % intf.channel

        for arg in args:
            if arg in intf.node.params:
                cmd += '\n%s=%s' % (arg, intf.node.params.get(arg))

        if intf.ht_capab: cmd += '\nht_capab=%s' % intf.ht_capab
        if intf.vht_capab: cmd += '\nvht_capab=%s' % intf.vht_capab
        if intf.beacon_int: cmd += '\nbeacon_int=%s' % intf.beacon_int
        if intf.client_isolation: cmd += '\nap_isolate=1'
        if 'config' in intf.node.params:
            config = intf.node.params['config']
            if config is not []:
                config = config.split(',')
                # ap.params.pop("config", None)
                for conf in config: cmd += "\n" + conf
        else:
            if intf.authmode == '8021x':
                cmd += "\nieee8021x=1"
                cmd += "\nwpa_key_mgmt=WPA-EAP"
                if intf.encrypt:
                    cmd += "\nauth_algs=%s" % intf.auth_algs
                    cmd += "\nwpa=2"
                cmd += '\neap_server=0'
                cmd += '\neapol_version=2'

                if not intf.radius_server: intf.radius_server = '127.0.0.1'
                cmd += "\nwpa_pairwise=TKIP CCMP"
                cmd += "\neapol_key_index_workaround=0"
                cmd += "\nown_ip_addr=%s" % intf.radius_server
                cmd += "\nnas_identifier=%s.example.com" % intf.node.name
                cmd += "\nauth_server_addr=%s" % intf.radius_server
                cmd += "\nauth_server_port=1812"
                if not intf.shared_secret: intf.shared_secret = 'secret'
                cmd += "\nauth_server_shared_secret=%s" % intf.shared_secret
            else:
                if intf.encrypt:
                    if 'wpa' in intf.encrypt:
                        cmd += "\nauth_algs=%s" % intf.auth_algs
                        cmd += "\nwpa=1" if intf.encrypt == 'wpa' else "\nwpa=2"
                        if intf.encrypt == 'wpa3':
                            cmd += "\nwpa_key_mgmt=WPA-PSK SAE"
                        else:
                            cmd += "\nwpa_key_mgmt=%s" % intf.wpa_key_mgmt
                        cmd += "\nwpa_pairwise=%s" % intf.rsn_pairwise
                        cmd += "\nwpa_passphrase=%s" % intf.wpa_passphrase
                    elif intf.encrypt == 'wep':
                        cmd += "\nauth_algs=%s" % intf.auth_algs
                        cmd += "\nwep_default_key=%s" % 0
                        cmd += self.verifyWepKey(intf.wep_key0)

                if intf.wps_state:
                    cmd += '\nwps_state=%s' % intf.wps_state
                    cmd += '\neap_server=1'
                    if intf.config_methods:
                        cmd += '\nconfig_methods=%s' % intf.config_methods
                    else:
                        cmd += '\nconfig_methods=label display push_button keypad'
                    cmd += '\nwps_pin_requests=/var/run/hostapd.pin-req'
                    cmd += '\nap_setup_locked=0'

                if intf.mode == 'ac' or intf.mode == 'n':
                    cmd += "\nwmm_enabled=1"
                    cmd += "\nieee80211n=1"
                    if intf.mode == 'ac': cmd += "\nieee80211ac=1"

                if intf.ieee80211r:
                    if intf.mobility_domain:
                        cmd += "\nmobility_domain=%s" % intf.mobility_domain
                        # cmd += ("\nown_ip_addr=127.0.0.1")
                        cmd += "\nnas_identifier=%s.example.com" \
                               % intf.node.name
                        for apref in intf.bssid_list:
                            cmd += '\nr0kh=%s r0kh-%s.example.com 000102030405060708090a0b0c0d0e0f' % (intf.mac, apref)
                            cmd += '\nr1kh=%s %s 000102030405060708090a0b0c0d0e0f' % (intf.mac, intf.mac)
                        cmd += '\nrsn_preauth=1'
                        cmd += '\npmk_r1_push=1'
                        cmd += '\nft_over_ds=1'
                        cmd += '\nft_psk_generate_local=1'
        return cmd

    def setBw(self, intf, wlan):
        bw = intf.node.params['bw'][wlan] if 'bw' in intf.node.params else self.getBW(intf)

        intf.node.cmd("tc qdisc replace dev %s root handle 2: tbf rate %sMbit "
                      "burst 15000 latency 1ms" % (intf, bw))
        # Reordering packets
        intf.node.cmd('tc qdisc add dev %s parent 2:1 handle 10: '
                      'pfifo limit 1000' % intf)

        if intf.ifb:
            intf.node.cmd("tc qdisc replace dev %s root handle 2: tbf "
                          "rate %sMbit burst 15000 latency 1ms" % (intf.ifb, bw))

    def getBW(self, intf):
        return DeviceRate(intf).rate if 'model' in intf.node.params else intf.getRate()

    def verifyWepKey(self, wep_key0):
        "Check WEP key"
        len_list = [5, 10, 13, 16, 26, 32]
        if len(wep_key0) in len_list:
            cmd = "\nwep_key0=%s" % wep_key0
        else:
            info("Warning! Wep Key length is wrong!\n")
            exit(1)
        return cmd

    _macMatchRegex = re.compile(r'..:..:..:..:..:..')

    @classmethod
    def restartNetworkManager(cls):
        """Restart network manager if the mac address of the AP
        is not included at /etc/NetworkManager/conf.d/unmanaged.conf"""
        nms = 'network-manager'
        nm_is_running = os.system('service %s status 2>&1 | grep '
                                  '-ic running >/dev/null 2>&1' % nms)
        if nm_is_running != 256:
            info('Mac address(es) added into %s\n' % cls.nm_conf_file)
            info('Restarting %s...\n' % nms)
            os.system('sudo service network-manager restart')
            #os.system('nmcli general reload')
            sleep(2)

    def checkNetworkManager(self, intf):
        "add mac address into /etc/NetworkManager/conf.d/unmanaged.conf"
        unmanaged = 'unmanaged-devices'
        nmdir = '/etc/NetworkManager'
        old_content = ''

        if os.path.isdir(nmdir) and not os.path.isfile(self.nm_conf_file):
            open(self.nm_conf_file, 'w').close()

        if os.path.isdir(nmdir):
            file = open(self.nm_conf_file, 'rt')
            data = file.read()
            isNew = True
            for content in data.split('\n'):
                if unmanaged in content:
                    old_content = content
                    new_content = old_content
                    isNew = False
            if isNew:
                os.system('echo \'#\' >> {}'.format(self.nm_conf_file))
                new_content = "[keyfile]\n%s=" % unmanaged

            name = intf.node.name + '*'
            if name not in old_content:
                new_content += "interface-name:{};".format(name)
                data = data.replace(old_content, new_content)
                file.close()
                file = open(self.nm_conf_file, "wt")
                file.write(data)
                file.close()
                HostapdConfig.write_mac = True

    def ap_config_file(self, cmd, intf):
        "run an Access Point and create the config file"
        if 'phywlan' in intf.node.params:
            intf_ = intf.node.params['phywlan']
            intf.cmd('ip link set %s down' % intf_)
            intf.cmd('ip link set %s up' % intf_)
        apconfname = "mn%d_%s-wlan%s.apconf" % (os.getpid(), intf.node.name, intf.id+1)
        content = cmd + ("\' > %s" % apconfname)
        intf.cmd(content)
        cmd = self.get_hostapd_cmd(intf)
        try:
            intf.cmd(cmd)
            if int(intf.channel) == 0 or intf.channel == 'acs_survey':
                info("*** Waiting for ACS... It takes 10 seconds.\n")
                sleep(10)
        except:
            info("*** error with hostapd. Please, run sudo mn -c in order "
                 "to fix it or check if hostapd is working properly in "
                 "your system.")
            exit(1)

    @staticmethod
    def get_hostapd_cmd(intf):
        apconfname = "mn%d_%s-wlan%s.apconf" % (os.getpid(), intf.node.name, intf.id+1)
        hostapd_flags = intf.node.params.get('hostapd_flags', '')
        cmd = "hostapd -B %s %s" % (apconfname, hostapd_flags)
        return cmd


class WirelessLink(TCIntf, IntfWireless):
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
        self.cmd('ethtool -K', self, 'gro', on(gro))

        # Optimization: return if nothing else to configure
        # Question: what happens if we want to reset things?
        if bw is None and not delay and not loss and max_queue_size is None:
            return

        # Clear existing configuration
        tcoutput = self.tc('%s qdisc show dev %s')
        if "priomap" not in tcoutput and "noqueue" not in tcoutput \
                and "fq_codel" not in tcoutput and "qdisc fq" not in tcoutput:
            cmds = ['%s qdisc del dev %s root']
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
        tcoutputs = [self.tc(cmd) for cmd in cmds]
        for output in tcoutputs:
            if output != '':
                error("*** Error: %s" % output)
        debug("cmds:", cmds, '\n')
        debug("outputs:", tcoutputs, '\n')
        result['tcoutputs'] = tcoutputs
        result['parent'] = parent

        return result

    def set_attrs(self, node, wlan):
        self.ip = None
        self.ip6 = None
        self.mac = None
        self.prefixLen = None
        self.prefixLen6 = None
        self.ssid = None
        self.antennaGain = 5.0
        self.antennaHeight = 1.0
        self.channel = 1
        self.freq = 2.412
        self.txpower = 14
        self.range = 0
        self.static_range = 0  # when the range is manually defined
        self.mode = 'g'
        self.node = node
        self.id = wlan

    def assign_params_to_intf(self, intf, wlan):
        if intf and hasattr(intf, 'wmIface'): self.wmIface = intf.wmIface

        for key in self.__dict__.keys():
            if key in self.node.params:
                if isinstance(self.node.params[key], list):
                    value = self.node.params[key][wlan]
                    setattr(self, key, value)
                else:
                    if wlan > 0 and key == 'mac':
                        self.mac = self.getMAC()
                    else:
                        if key == 'range':
                            setattr(self, key, int(self.node.params[key]))
                            self.static_range = True
                        else:
                            setattr(self, key, self.node.params[key])


class _4address(Link, IntfWireless):
    node = None

    def __init__(self, node1, node2, port1=None, port2=None, **params):
        """Create 4addr link to another node.
           node1: first node
           node2: second node
           intf: default interface class/constructor"""
        ap = node1  # ap
        cl = node2  # client
        cl_intfname = '%s.wds' % cl.name

        if not hasattr(node1, 'position'): self.set_pos(node1)
        if not hasattr(node2, 'position'): self.set_pos(node2)

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

            params1, params2 = params, params
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

            _4addrAP(ap, (len(ap.params['wlan']) - 1))
            _4addrClient(cl, (len(cl.params['wlan']) - 1))

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
        self.cmd('ip link set dev %s.wds addr %s' % (intf.node, intf.mac))

    def add4addrIface(self, intf, intfName):
        self.iwdev_cmd('%s interface add %s type managed 4addr on' %
                       (intf.name, intfName))

    def status(self):
        "Return link status as a string"
        return "(%s %s)" % (self.intf1.status(), self.intf2)


class WirelessIntf(object):
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
        params = dict(params) if params else {}
        params['port'] = port if port is not None else node.newPort()

        if not intfName: intfName = self.wlanName(node, params['port'])

        intf1 = cls(name=intfName, node=node, link=self, mac=addr, **params)
        intf2 = 'wifi'

        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2

    # pylint: enable=too-many-branches

    @staticmethod
    def _ignore(*args, **kwargs):
        "Ignore any arguments"
        pass

    def wlanName(self, node, n):
        "Construct a canonical interface name node-ethN for interface n."
        # Leave this as an instance method for now
        assert self
        return node.name + '-' + 'wlan' + repr(n)

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


class TCLinkWireless(WirelessIntf):
    "Link with symmetric TC interfaces configured via opts"

    def __init__(self, node, port=None, intfName=None,
                 addr=None, cls=WirelessLink, **params):
        WirelessIntf.__init__(self, node=node, port=port,
                              intfName=intfName, cls=cls,
                              addr=addr, params=params)


class master(WirelessLink):
    "master class"

    def __init__(self, node, wlan, port=None, intf=None):
        WirelessLink.set_attrs(self, node, wlan)
        self.name = node.params['wlan'][wlan]
        node.addWAttr(self, port=port)
        self.params = {}
        self.stationsInRange = {}
        self.vssid = []
        self.vifaces = []
        self.associatedStations = []
        self.bssid_list = []
        self.ifb = None
        self.auth_algs = None
        self.authmode = None
        self.freq = None
        self.beacon_int = None
        self.config = None
        self.config_methods = None
        self.country_code = 'US'
        self.encrypt = None
        self.ht_capab = None
        self.vht_capab = None
        self.ieee80211r = None
        self.client_isolation = None
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
        self.band = 20  # bandwidth channel
        self.consumption = 0.0
        self.voltage = 10.0
        self.assign_params_to_intf(intf, wlan)


class VirtualMaster(master):
    "master class"

    def __init__(self, node, wlan, port=None, intf=None):
        WirelessLink.set_attrs(self, node, wlan)
        self.name = intf
        node.addWAttr(self, port=port)
        self.params = {}
        self.stationsInRange = {}
        self.vssid = []
        self.vifaces = []
        self.associatedStations = []
        self.ifb = None
        self.auth_algs = None
        self.authmode = None
        self.freq = None
        self.beacon_int = None
        self.config = None
        self.config_methods = None
        self.encrypt = None
        self.ht_capab = None
        self.vht_capab = None
        self.ieee80211r = None
        self.client_isolation = None
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
        self.assign_params_to_intf(intf, wlan)


class phyAP(WirelessLink):
    "phyap class"

    def __init__(self, node, wlan, port=None, intf=None):
        WirelessLink.set_attrs(self, node, wlan)
        self.name = node.params['wlan'][wlan]
        node.addWAttr(self, port=port)
        self.params = {}
        self.stationsInRange = {}
        self.vssid = []
        self.vifaces = []
        self.associatedStations = []
        self.bssid_list = []
        self.ifb = None
        self.auth_algs = None
        self.authmode = None
        self.freq = None
        self.beacon_int = None
        self.config = None
        self.config_methods = None
        self.country_code = 'US'
        self.encrypt = None
        self.ht_capab = None
        self.vht_capab = None
        self.ieee80211r = None
        self.client_isolation = None
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
        self.band = 20  # bandwidth channel
        self.assign_params_to_intf(intf, wlan)


class managed(WirelessLink):
    "managed class"

    def __init__(self, node, wlan, intf=None):
        WirelessLink.set_attrs(self, node, wlan)
        self.name = node.params['wlan'][wlan]
        node.addWIntf(self, port=wlan)
        node.addWAttr(self, port=wlan)
        self.apsInRange = {}
        self.ifb = None
        self.active_scan = None
        self.associatedTo = None
        self.authmode = None
        self.config = None
        self.encrypt = None
        self.freq_list = None
        self.link = None
        self.passwd = None
        self.radius_identity = None
        self.radius_passwd = None
        self.scan_freq = None
        self.bgscan_module = None
        self.s_inverval = 0  # short interval
        self.l_interval = 0  # long interval
        self.bgscan_threshold = 0
        self.consumption = 0.0
        self.voltage = 10.0
        self.band = 20  # bandwidth channel
        self.rssi = -60
        self.assign_params_to_intf(intf, wlan)


class _4addrClient(WirelessLink):
    "managed class"

    def __init__(self, node, wlan):
        self.node = node
        self.id = wlan
        self.ip = None
        self.freq = node.wintfs[0].freq
        self.mac = node.wintfs[wlan - 1].mac
        self.range = node.wintfs[0].range
        self.static_range = node.wintfs[0].static_range
        self.txpower = node.wintfs[wlan - 1].txpower
        self.antennaGain = 5.0
        self.name = node.params['wlan'][wlan]
        self.stationsInRange = {}
        self.associatedStations = []
        self.apsInRange = {}
        self.params = {}
        # node.addWIntf(self, port=wlan)
        node.addWAttr(self, port=wlan)


class _4addrAP(WirelessLink):
    "managed class"

    def __init__(self, node, wlan):
        self.node = node
        self.ip = None
        self.id = wlan
        self.freq = node.wintfs[0].freq
        self.mac = node.wintfs[0].mac
        self.range = node.wintfs[0].range
        self.static_range = node.wintfs[0].static_range
        self.txpower = node.wintfs[0].txpower
        self.antennaGain = 5.0
        self.name = node.params['wlan'][wlan]
        self.stationsInRange = {}
        self.associatedStations = []
        self.params = {}
        # node.addWIntf(self, port=wlan)
        node.addWAttr(self, port=wlan)


class wmediumd(object):
    positions = []
    txpowers = []
    links = []

    def __init__(self, **kwargs):
        self.configWmediumd(**kwargs)

    def configWmediumd(self, wlinks, fading_cof, noise_th, stations,
                       aps, cars, ppm):
        "Configure wmediumd"
        intfrefs = []
        isnodeaps = []
        mac_list = []

        nodes = stations + aps + cars
        for node in nodes:
            node.wmIfaces = []
            for intf in node.wintfs.values():
                if intf.mac not in mac_list and not isinstance(intf, phyAP):
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
            self.snr(wlinks, noise_th)
        WStarter(intfrefs=intfrefs, links=self.links, pos=self.positions,
                 fading_cof=fading_cof, noise_th=noise_th, txpowers=self.txpowers,
                 isnodeaps=isnodeaps, ppm=ppm)

    @staticmethod
    def get_position(pos=None):
        if pos: return float(pos[0]), float(pos[1]), float(pos[2])
        return 0, 0, 0

    def interference(self, nodes):
        for node in nodes:
            posX, posY, posZ = self.get_position(node.position) \
                if hasattr(node, 'position') else self.get_position()
            node.lastpos = [posX, posY, posZ]

            mac_list = []
            for wlan, intf in enumerate(node.wintfs.values()):
                if intf.mac not in mac_list and not isinstance(intf, phyAP):
                    if wlan >= 1: posX += 0.1
                    self.positions.append(w_pos(intf.wmIface, [posX, posY, posZ]))
                    self.txpowers.append(w_txpower(intf.wmIface, int(intf.txpower)))
                    mac_list.append(intf.mac)

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


class ConfigWLink(object):
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
        if rate <= 0.0: rate = 0.1
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
        if intf.ifb: cls.tc(intf.node, intf.ifb, **params)
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


class LinkAttrs(WirelessLink):

    def __init__(self, node, intf, wlan):
        self.node = node
        self.antennaGain = intf.antennaGain
        self.antennaHeight = intf.antennaHeight
        self.band = intf.band
        self.encrypt = intf.encrypt
        self.freq = intf.freq
        self.id = wlan
        self.ip6 = intf.ip6
        self.ip = intf.ip
        self.prefixLen = intf.prefixLen
        self.prefixLen6 = intf.prefixLen6
        self.link = intf.link
        self.mac = intf.mac
        self.txpower = intf.txpower
        self.range = intf.range
        self.static_range = intf.static_range

    def check_channel_band(self, ht_cap):
        if '40' in ht_cap:
            self.band = 40
        elif '80' in ht_cap:
            self.band = 80
        elif '160' in ht_cap:
            self.band = 160

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


class ITSLink(LinkAttrs):

    def __init__(self, node, intf=None, proto_args='', **params):
        "configure ieee80211p"
        intf = node.getNameToWintf(intf)
        wlan = node.params['wlan'].index(intf.name)
        LinkAttrs.__init__(self, node, intf, wlan)

        if isinstance(self, master):
            self.kill_hostapd()

        self.node = node
        self.name = intf.name
        self.mac = intf.mac

        # It takes default values if keys are not set
        kwargs = {'channel': '181', 'band': 10, 'proto': None, 'txpower': 17}

        for k, v in kwargs.items():
            setattr(self, k, params.get(k, v))

        self.freq = self.get_freq()

        if isinstance(intf, master):
            self.name = '%s-ocb' % node.name
            self.add_ocb_mode()
        else:
            self.set_ocb_mode()

        intf1 = WirelessLink(name=node.params['wlan'][wlan], node=node,
                             link=self, port=wlan)
        intf2 = 'ITS'

        node.addWAttr(self, port=wlan)
        intf.setTxPower(self.txpower)
        self.configure_ocb()

        if self.proto: manetProtocols(self, proto_args)

        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2

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
        freq = int('{:<04s}'.format(str(self.freq).replace('.', '')))
        self.iwdev_cmd('{} ocb join {} {}MHz'.format(self.name, freq, self.band))


class WifiDirectLink(LinkAttrs):

    def __init__(self, node, intf=None):
        "configure wifi-direct"
        intf = node.getNameToWintf(intf)
        wlan = node.params['wlan'].index(intf.name)
        LinkAttrs.__init__(self, node, intf, wlan)
        self.name = intf.name

        intf1 = WirelessLink(name=node.params['wlan'][wlan], node=node,
                             link=self, port=wlan)
        intf2 = 'wifiDirect'

        # node.addWIntf(self, port=wlan)
        node.addWAttr(self, port=wlan)

        self.config_()
        cmd = self.get_wpa_cmd()
        node.cmd(cmd)

        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2

    def get_filename(self):
        suffix = 'wifiDirect.conf'
        pattern = "mn{}_{}_{}".format(os.getpid(), self.name, suffix)
        return pattern

    def get_wpa_cmd(self):
        pattern = self.get_filename()
        cmd = 'wpa_supplicant -B -Dnl80211 -c{} -i{}'.format(pattern, self.name)
        return cmd

    def config_(self):
        pattern = self.get_filename()
        cmd = "echo \'"
        cmd += 'ctrl_interface=/var/run/wpa_supplicant\
              \nap_scan=1\
              \np2p_go_ht40=1\
              \ndevice_name=%s\
              \ndevice_type=1-0050F204-1\
              \np2p_no_group_iface=1' % self.name
        cmd += ("\' > %s" % pattern)
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

        intf1 = WirelessLink(name=self.name, node=node, link=self)
        intf2 = 'wifiDirect'

        node.addWAttr(self)

        for wlan, intf in enumerate(node.wintfs.values()):
            if intf.name == self.name: break

        self.txpower = node.wintfs[0].txpower
        node.params['wlan'].append(self.name)
        self.mac = None

        self.config_()
        cmd = self.get_wpa_cmd()
        os.system(cmd)

        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2


class adhoc(LinkAttrs):
    node = None

    def __init__(self, node, intf, proto_args='', **params):
        """Configure AdHoc
        node: name of the node
        self: custom association class/constructor
        params: parameters for station"""
        intf = node.getNameToWintf(intf)
        wlan = node.params['wlan'].index(intf.name)

        if isinstance(intf, master): intf.kill_hostapd_process()

        LinkAttrs.__init__(self, node, intf, wlan)
        self.associatedTo = 'adhoc'

        # It takes default values if keys are not set
        kwargs = {'ibss': '02:CA:FF:EE:BA:01', 'ht_cap': '',
                  'passwd': None, 'ssid': 'adhocNet', 'proto': None,
                  'mode': 'g', 'channel': 1, 'txpower': 15,
                  'ap_scan': 2, 'bitrates': ''}

        for k, v in kwargs.items():
            setattr(self, k, params.get(k, v))

        self.check_channel_band(self.ht_cap)

        if intf and hasattr(intf, 'wmIface'):
            self.wmIface = intf.wmIface

        if 'mp' in intf.name:
            self.iwdev_cmd('%s del' % intf.name)
            node.params['wlan'][wlan] = intf.name.replace('mp', 'wlan')

        self.name = intf.name

        intf1 = WirelessLink(name=node.params['wlan'][wlan], node=node,
                             link=self, port=wlan)
        intf2 = 'wifiAdhoc'

        node.addWAttr(self, port=wlan)

        self.freq = self.get_freq()
        self.setReg()
        self.configureAdhoc()

        self.setTxPower(self.txpower)

        if self.proto: manetProtocols(self, proto_args)

        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2

    def configureAdhoc(self):
        "Configure Wireless Ad Hoc"
        self.set_dev_type('ibss')
        self.ipLink('up')

        if self.passwd:
            self.setSecuredAdhoc()
        else:
            if self.bitrates:
                self.set_bitrates(self.bitrates)
            self.join_ibss(self.ssid, self.format_freq(),
                           self.ht_cap, self.ibss)

    def get_sta_confname(self):
        return '{}_0.staconf'.format(self.name)

    def setSecuredAdhoc(self):
        "Set secured adhoc"
        cmd = 'ctrl_interface=/var/run/wpa_supplicant\n'
        cmd += 'ap_scan=%s\n' % self.ap_scan
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

        pattern = self.get_sta_confname()
        os.system('echo \'%s\' > %s' % (cmd, pattern))
        self.cmd(self.get_wpa_cmd())


class mesh(LinkAttrs):
    node = None

    def __init__(self, node, intf, proto_args='', **params):
        from mn_wifi.node import AP
        """Configure wireless mesh
        node: name of the node
        self: custom association class/constructor
        params: parameters for node"""
        intf = node.getNameToWintf(intf)
        wlan = node.params['wlan'].index(intf.name)

        if isinstance(intf, master): intf.kill_hostapd_process()

        LinkAttrs.__init__(self, node, intf, wlan)
        iface = intf
        self.name = '%s-mp%s' % (node, intf.name[-1:])
        self.associatedTo = 'mesh'

        # It takes default values if keys are not set
        kwargs = {'ibss': '02:CA:FF:EE:BA:01', 'ht_cap': '',
                  'passwd': None, 'ssid': 'meshNet', 'proto': None,
                  'mode': 'g', 'channel': 1, 'txpower': 15}

        for k, v in kwargs.items():
            setattr(self, k, params.get(k, v))

        self.check_channel_band(self.ht_cap)

        if wmediumd_mode.mode:
            self.wmIface = DynamicIntfRef(node, intf=self.name)
            node.wmIfaces[wlan] = self.wmIface

        port = wlan + 1 if isinstance(node, AP) else wlan

        # mp interface must be created before ethtool
        self.iwdev_cmd(self.set_mesh_type(intf))
        intf1 = WirelessLink(name=self.name, node=node,
                             link=self, port=port)
        intf2 = 'wifiMesh'

        node.addWAttr(self, port=wlan)
        self.setTxPower(self.txpower)
        self.setMeshIface(wlan, iface)
        self.configureMesh()

        if self.proto: manetProtocols(self, proto_args)

        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2

    def set_mesh_type(self, intf):
        return '%s interface add %s type mp' % (intf.name, self.name)

    def setMeshIface(self, wlan, intf):
        if isinstance(intf, adhoc): self.set_dev_type('managed')
        self.node.cmd('ip link set %s down' % intf)

        self.setMAC(intf.mac)
        self.node.params['wlan'][wlan] = self.name

        self.setMeshChannel(self.channel)
        self.setReg()
        self.ipLink('up')

        if self.ip:
            self.setIP(self.ip, self.prefixLen)
            self.cmd('ip link set lo up')

    def configureMesh(self):
        "Configure Wireless Mesh Interface"
        if self.passwd:
            self.setSecuredMesh()
        else:
            self.mesh_join(self.ssid, self.format_freq(), self.ht_cap)

    def get_sta_confname(self):
        return '{}.staconf'.format(self.name)

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

        pattern = self.get_sta_confname()
        os.system('echo \'%s\' > %s' % (cmd, pattern))


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
            node.setDefaultRange(intf)
            node.wintfs[wlan].range = intf.range

        self.name = intf
        self.setPhysicalMeshIface(node, wlan, intf)
        self.setMeshChannel(channel)
        self.ipLink('up')
        self.freq = self.format_freq()
        self.mesh_join(self.ssid, self.freq, self.ht_cap)

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
                id = subprocess.check_output(cmd, shell=True).split('\n')
            except:
                pass
            if len(id) == 0:
                cmd = ('iw dev %s interface add %s type mp' %
                       (intf, iface))
                self.name = iface
                subprocess.check_output(cmd, shell=True)
                break
