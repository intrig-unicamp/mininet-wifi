"""
    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

import os
import pty
import re
import signal
import select
import fileinput
import subprocess
from subprocess import Popen, PIPE
from time import sleep
from distutils.version import StrictVersion
from re import findall
from sys import version_info as py_version_info
import numpy as np
from scipy.spatial.distance import pdist
from six import string_types

from mininet.log import info, error, warn, debug
from mininet.util import (quietRun, errRun, isShellBuiltin)
from mininet.node import Node
from mininet.moduledeps import moduleDeps, pathCheck, TUN
from mininet.link import Link, Intf, OVSIntf
from mn_wifi.link import TCWirelessLink, TCLinkWirelessAP,\
    Association, wirelessLink, adhoc, mesh
from mn_wifi.wmediumdConnector import WmediumdServer, \
    WmediumdPos, WmediumdTXPower, WmediumdGain, WmediumdHeight, \
    wmediumd_mode, WmediumdCst
from mn_wifi.propagationModels import GetSignalRange, \
    GetPowerGivenRange, propagationModel
from mn_wifi.util import moveIntf
from mn_wifi.module import module
from mn_wifi.utils.private_folder_manager import PrivateFolderManager


class Node_wifi(Node):
    """A virtual network node is simply a shell in a network namespace.
       We communicate with it using pipes."""

    portBase = 0  # Nodes always start with eth0/port0, even in OF 1.0

    def __init__(self, name, inNamespace=True, **params):
        """name: name of node
           inNamespace: in network namespace?
           privateDirs: list of private directory strings or tuples
           params: Node parameters (see config() for details)"""

        # Make sure class actually works
        self.checkSetup()

        self.name = params.get('name', name)
        self.inNamespace = params.get('inNamespace', inNamespace)

        # Stash configuration parameters for future reference
        self.params = params

        self.intfs = {}  # dict of port numbers to interfaces
        self.ports = {}  # dict of interfaces to port numbers
        self.wlanports = -1  # dict of wlan interfaces to port numbers
        self.nameToIntf = {}  # dict of interface names to Intfs

        self.func = []
        self.isStationary = True

        # Make pylint happy
        (self.shell, self.execed, self.pid, self.stdin, self.stdout,
         self.lastPid, self.lastCmd, self.pollOut) = (
             None, None, None, None, None, None, None, None)
        self.waiting = False
        self.readbuf = ''

        # Start command interpreter shell
        self.startShell()
        self.private_folder_manager = \
            PrivateFolderManager(self, params.get('privateDirs', []))

    # File descriptor to node mapping support
    # Class variables and methods
    inToNode = {}  # mapping of input fds to nodes
    outToNode = {}  # mapping of output fds to nodes

    @classmethod
    def fdToNode(cls, fd):
        """Return node corresponding to given file descriptor.
           fd: file descriptor
           returns: node"""
        node = cls.outToNode.get(fd)
        return node or cls.inToNode.get(fd)

    # Command support via shell process in namespace
    def startShell(self, mnopts=None):
        # pdb.set_trace()
        "Start a shell process for running commands"
        if self.shell:
            error("%s: shell is already running\n" % self.name)
            return
        # mnexec: (c)lose descriptors, (d)etach from tty,
        # (p)rint pid, and run in (n)amespace
        opts = '-cd' if mnopts is None else mnopts
        if self.inNamespace:
            opts += 'n'
        # bash -i: force interactive
        # -s: pass $* to shell, and make process easy to find in ps
        # prompt is set to sentinel chr( 127 )
        # pdb.set_trace()
        cmd = [ 'mnexec', opts, 'env', 'PS1=' + chr(127),
                'bash', '--norc', '-is', 'mininet:' + self.name ]
        # Spawn a shell subprocess in a pseudo-tty, to disable buffering
        # in the subprocess and insulate it from signals (e.g. SIGINT)
        # received by the parent
        master, slave = pty.openpty()
        self.shell = self._popen(cmd, stdin=slave, stdout=slave, stderr=slave,
                                 close_fds=False)
        if py_version_info < (3, 0):
            self.stdin = os.fdopen(master, 'rw')
        else:
            self.stdin = os.fdopen(master, 'bw')
        self.stdout = self.stdin
        self.pid = self.shell.pid
        self.pollOut = select.poll()
        self.pollOut.register(self.stdout)
        # Maintain mapping between file descriptors and nodes
        # This is useful for monitoring multiple nodes
        # using select.poll()
        self.outToNode[ self.stdout.fileno() ] = self
        self.inToNode[ self.stdin.fileno() ] = self
        self.execed = False
        self.lastCmd = None
        self.lastPid = None
        self.readbuf = ''
        # Wait for prompt
        while True:
            data = self.read(1024)
            if data[ -1 ] == chr(127):
                break
            self.pollOut.poll()
        self.waiting = False
        # +m: disable job control notification
        self.cmd('unset HISTFILE; stty -echo; set +m')

    def plot(self, position):
        self.params['position'] = position.split(',')
        self.params['range'] = [0]
        self.plotted = True

    def setMeshIface(self, iface, ssid='', **params):
        wlan = self.params['wlan'].index(iface)
        if self.func[wlan] == 'adhoc':
            self.cmd('iw dev %s set type managed' %
                     self.params['wlan'][wlan])
        iface = '%s-mp%s' % (self, wlan)
        if self.func[wlan] == 'mesh' and isinstance(self, AP):
            iface = '%s-mp%s' % (self, wlan+1)

        self.cmd('iw dev %s interface add %s type mp' %
                 (self.params['wlan'][wlan], iface))
        self.cmd('ip link set %s down' % iface)
        self.cmd('ip link set %s address %s' %
                 (iface, self.params['mac'][wlan]))
        self.cmd('ip link set %s down' % self.params['wlan'][wlan])
        self.params['wlan'][wlan] = iface

        if 'channel' in params:
            self.setChannel(params['channel'], intf=self.params['wlan'][wlan])

        if 'mode' in params and (params['mode'] == 'a'
                                 or params['mode'] == 'ac'):
            self.pexec('iw reg set US')

        if 'freq' in params:
            self.setFreq(params['freq'], intf=self.params['wlan'][wlan])

        if 'ip' in self.params:
            self.cmd('ip addr add %s dev %s' % (self.params['ip'][wlan],
                                                self.params['wlan'][wlan]))
            self.cmd('ip link set %s up' % iface)
        else:
            self.cmd('ip link set %s up' % self.params['wlan'][wlan])

        if ssid != '':
            if 'ssid' not in self.params:
                self.params['ssid'] = []
                self.params['ssid'].append(0)
            self.params['ssid'][wlan] = ssid
            mesh.configureMesh(self, wlan)

    def setPhysicalMeshIface(self, **params):
        wlan = 0
        iface = 'phy%s-mp%s' % (self, wlan)
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
                        command = ('iw dev %s set channel %s'
                                 % (iface, str(params['channel'])))
                        subprocess.check_output(command, shell=True)
                    os.system('ip link set %s up' % iface)
                    debug("associating %s to %s...\n" % (iface,
                                                         self.params['ssid'][wlan]))
                    command = ('iw dev %s mesh join %s' % (iface,
                                                           self.params['ssid'][wlan]))
                    subprocess.check_output(command, shell=True)
                    break
                except:
                    break

    def setMasterMode(self, intf, **params):
        wlan = self.params['wlan'].index(intf)
        if 'ssid' in params:
            self.params['ssid'][wlan] = params['ssid']
        else:
            self.params['ssid'] = []
            for _ in self.params['wlan']:
                self.params['ssid'].append('')
            self.params['ssid'][wlan] = self.name

        self.params['driver'] = 'nl80211'
        AccessPoint.setConfig(self, wlan=wlan)

    def setAdhocIface(self, iface, ssid=''):
        "Set Adhoc Interface"
        wlan = self.params['wlan'].index(iface)
        if self.func[wlan] == 'mesh':
            self.cmd('iw dev %s del' % self.params['wlan'][wlan])
            iface = '%s-wlan%s' % (self, wlan)
            self.params['wlan'][wlan] = iface
        else:
            iface = self.params['wlan'][wlan]
        if ssid != '':
            if 'ssid' not in self.params:
                self.params['ssid'] = []
                self.params['ssid'].append(0)
            self.params['ssid'][wlan] = ssid
            adhoc.configureAdhoc(self, wlan, enable_wmediumd=True)

    def setOCBIface(self, iface):
        "Set OCB Interface"
        wlan = self.params['wlan'].index(iface)
        iface = self.params['wlan'][wlan]
        self.cmd('ip link set %s down' % iface)
        self.cmd('iw dev %s set type ocb' % iface)
        self.cmd('ip link set %s up' % iface)
        self.configureOCB(wlan)

    def configureOCB(self, wlan):
        "Configure Wireless OCB"
        iface = self.params['wlan'][wlan]
        freq = self.params['frequency'][wlan]
        self.func[wlan] = 'ocb'
        self.cmd('iw dev %s ocb join %s 10MHz' % (iface, freq))

    def configLinks(self):
        "Applies channel params and handover"
        from mn_wifi.mobility import mobility
        mobility.configLinks(self)

    def getMAC(self, iface):
        "get Mac Address of any Interface"
        try:
            _macMatchRegex = re.compile(r'..:..:..:..:..:..')
            debug('getting mac address from %s\n' % iface)
            macaddr = str(self.pexec('ip addr show %s' % iface))
            mac = _macMatchRegex.findall(macaddr)
            debug('%s\n' % mac[0])
            return mac[0]
        except:
            info('Please run sudo mn -c.\n')

    def ifbSupport(self, wlan, ifbID):
        "Support to Intermediate Functional Block (IFB) Devices"
        os.system('ip link set dev ifb%s netns %s' % (ifbID, self.pid))
        self.cmd('ip link set ifb%s up' % ifbID)
        self.cmd('tc qdisc add dev %s handle ffff: ingress' %
                 self.params['wlan'][wlan])
        self.cmd('tc filter add dev %s parent ffff: protocol ip u32 '
                 'match u32 0 0 action mirred egress redirect dev ifb%s'
                 % (self.params['wlan'][wlan], ifbID))
        self.ifb.append(ifbID)

    def getRange(self, intf=None, noiseLevel=0):
        "Get the Signal Range"
        interference_enabled = False
        if wmediumd_mode.mode == WmediumdCst.INTERFERENCE_MODE:
            interference_enabled = True
        wlan = self.params['wlan'].index(intf)
        if noiseLevel != 0:
            GetSignalRange.NOISE_LEVEL = noiseLevel
        if not isinstance(self, Station) and not isinstance(self, Car) \
                and not isinstance(self, AP):
            self = self.params['associatedTo'][0]
        value = GetSignalRange(self, wlan, interference_enabled)

        return int(value.dist)

    def setRange(self, value, intf=None):
        "Set Signal Range"
        from mn_wifi.plot import plot2d
        wlan = 0
        if intf:
            wlan = self.params['wlan'].index(intf)
        self.params['range'][wlan] = value
        if self.autoTxPower:
            self.params['txpower'][wlan] = self.get_txpower_prop_model(0)
            self.setTxPower(value, intf=self.params['wlan'][wlan])
        if self.isStationary:
            self.updateGraph()
            self.configLinks()
        else:
            if plot2d.fig_exists():
                plot2d.updateCircleRadius(self)

    def updateGraph(self):
        "Update the Graph"
        from mn_wifi.plot import plot2d, plot3d
        cls = plot2d
        if plot3d.is3d:
            cls = plot3d
        if cls.fig_exists():
            cls.updateCircleRadius(self)
            cls.graphUpdate(self)
            cls.graphPause()

    def setPosition(self, pos):
        "Set Position"
        pos = pos.split(',')
        self.params['position'] = float(pos[0]), float(pos[1]), float(pos[2])
        if isinstance(self, Car):
            car = self.params['carsta']
            x = self.params['position'][0] + 0.1
            y = self.params['position'][1]
            z = self.params['position'][2]
            car.params['position'] = (x,y,z)
        self.updateGraph()

        if wmediumd_mode.mode == WmediumdCst.INTERFERENCE_MODE:
            self.set_pos_wmediumd()
            if isinstance(self, Car):
                car = self.params['carsta']
                car.set_pos_wmediumd()
        self.configLinks()

    def setAntennaGain(self, value, intf=None, setParam=True):
        "Set Antenna Gain"
        wlan = self.params['wlan'].index(intf)
        self.params['antennaGain'][wlan] = int(value)
        self.setGainWmediumd(wlan)
        if setParam:
            self.configLinks()

    def setAntennaHeight(self, value, intf=None):
        "Set Antenna Height"
        wlan = self.params['wlan'].index(intf)
        self.params['antennaHeight'][wlan] = int(value)
        self.setHeightWmediumd(wlan)
        self.configLinks()

    def setChannel(self, value, intf=None):
        "Set Channel"
        wlan = self.params['wlan'].index(intf)
        self.params['channel'][wlan] = str(value)
        self.params['frequency'][wlan] = self.get_freq(wlan)
        if isinstance(self, AP) and self.func[wlan] != 'mesh':
            self.pexec(
                'hostapd_cli -i %s chan_switch %s %s' % (
                    intf, str(value),
                    str(self.params['frequency'][wlan]).replace(".", "")))
        else:
            self.cmd('iw dev %s set channel %s'
                     % (self.params['wlan'][wlan], str(value)))

    def setFreq(self, value, intf=None):
        "Set Frequency"
        wlan = self.params['wlan'].index(intf)
        self.cmd('iw dev %s set freq %s' % (self.params['wlan'][wlan], value))
        self.params['frequency'][wlan] = value

    def setTxPower(self, value, intf=None, setParam=True):
        "Set Tx Power"
        wlan = self.params['wlan'].index(intf)
        self.pexec('iw dev %s set txpower fixed %s'
                   % (intf, (int(value) * 100)))
        self.params['txpower'][wlan] = value
        self.setTXPowerWmediumd(wlan)
        if setParam:
            self.configLinks()

    def get_freq(self, wlan):
        """Gets frequency based on channel number
        :param wlan: wlan ID
        """
        freq = 0
        channel = int(self.params['channel'][wlan])
        if channel == 1:
            freq = 2.412
        elif channel == 2:
            freq = 2.417
        elif channel == 3:
            freq = 2.422
        elif channel == 4:
            freq = 2.427
        elif channel == 5:
            freq = 2.432
        elif channel == 6:
            freq = 2.437
        elif channel == 7:
            freq = 2.442
        elif channel == 8:
            freq = 2.447
        elif channel == 9:
            freq = 2.452
        elif channel == 10:
            freq = 2.457
        elif channel == 11:
            freq = 2.462
        elif channel == 36:
            freq = 5.18
        elif channel == 40:
            freq = 5.2
        elif channel == 44:
            freq = 5.22
        elif channel == 48:
            freq = 5.24
        elif channel == 52:
            freq = 5.26
        elif channel == 56:
            freq = 5.28
        elif channel == 60:
            freq = 5.30
        elif channel == 64:
            freq = 5.32
        elif channel == 100:
            freq = 5.50
        elif channel == 104:
            freq = 5.52
        elif channel == 108:
            freq = 5.54
        elif channel == 112:
            freq = 5.56
        elif channel == 116:
            freq = 5.58
        elif channel == 120:
            freq = 5.60
        elif channel == 124:
            freq = 5.62
        elif channel == 128:
            freq = 5.64
        elif channel == 132:
            freq = 5.66
        elif channel == 136:
            freq = 5.68
        elif channel == 140:
            freq = 5.70
        elif channel == 149:
            freq = 5.745
        elif channel == 153:
            freq = 5.765
        elif channel == 157:
            freq = 5.785
        elif channel == 161:
            freq = 5.805
        elif channel == 165:
            freq = 5.825
        else:
            freq = 2.412
        return freq

    def set_rssi(self, node2=None, wlan=0, dist=0):
        """set RSSI
        :param node1: self
        :param node2: access point
        :param wlan: wlan ID
        :param dist: distance
        """
        value = propagationModel(self, node2, dist, wlan)
        return float(value.rssi)  # random.uniform(value.rssi-1, value.rssi+1)

    def set_pos_wmediumd(self):
        "Set Position for wmediumd"
        posX = self.params['position'][0]
        posY = self.params['position'][1]
        posZ = self.params['position'][2]

        wlans = len(self.params['wlan'])
        if isinstance(self, Car):
            wlans = 1

        for wlan in range(0, wlans):
            self.lastpos = self.params['position']
            WmediumdServer.update_pos(WmediumdPos(
                self.wmIface[wlan], [(float(posX)+wlan), float(posY), float(posZ)]))

    def setGainWmediumd(self, wlan):
        "Set Antenna Gain for wmediumd"
        if wmediumd_mode.mode == WmediumdCst.INTERFERENCE_MODE:
            gain_ = self.params['antennaGain'][wlan]
            WmediumdServer.update_gain(WmediumdGain(
                self.wmIface[wlan], int(gain_)))

    def setHeightWmediumd(self, wlan):
        "Set Antenna Height for wmediumd"
        if wmediumd_mode.mode == WmediumdCst.INTERFERENCE_MODE:
            height_ = self.params['antennaHeight'][wlan]
            WmediumdServer.update_height(WmediumdHeight(
                self.wmIface[wlan], int(height_)))

    def setTXPowerWmediumd(self, wlan):
        "Set TxPower for wmediumd"
        if wmediumd_mode.mode == WmediumdCst.INTERFERENCE_MODE:
            txpower_ = self.params['txpower'][wlan]
            WmediumdServer.update_txpower(WmediumdTXPower(
                self.wmIface[wlan], int(txpower_)))

    def get_txpower_prop_model(self, wlan):
        "Get Tx Power Given the propagation Model"
        interference_enabled = False
        if wmediumd_mode.mode == WmediumdCst.INTERFERENCE_MODE:
            interference_enabled = True
        value = GetPowerGivenRange(self, wlan, self.params['range'][wlan],
                                   interference_enabled)
        return int(value.txpower)

    def get_txpower(self, iface):
        connected = self.cmd('iw dev %s link | awk \'{print $1}\'' % iface)
        if connected != 'Not':
            try:
                txpower = int(self.cmd('iw dev %s info | grep txpower | '
                                       'awk \'{print $2}\'' % iface))
            except:
                txpower = 20
            return txpower
        elif isinstance(self, AP):
            try:
                txpower = int(self.cmd('iw dev %s info | grep txpower | '
                                       'awk \'{print $2}\'' % iface))
            except:
                txpower = 14
            return txpower

    def get_distance_to(self, dst):
        """Get the distance between two nodes
        :param self: source node
        :param dst: destination node"""

        pos_src = self.params['position']
        pos_dst = dst.params['position']
        points = np.array([(pos_src[0], pos_src[1], pos_src[2]),
                           (pos_dst[0], pos_dst[1], pos_dst[2])])
        dist = pdist(points)
        return float("%.2f" % dist)

    def associateTo(self, ap, intf=None):
        "Force association to given AP"
        self.moveAssociationTo(ap, intf)

    def moveAssociationTo(self, ap, intf=None):
        "Force association to specific AP"
        sta = self
        wlan = 0
        for idx, wlan in enumerate(sta.params['wlan']):
            if wlan == intf:
                wlan = idx
                break
        dist = 100000
        if 'position' in sta.params and 'position' in ap.params:
            dist = sta.get_distance_to(ap)

        if dist < ap.params['range'][wlan] or 'position' not in sta.params \
                and 'position' not in ap.params:
            if sta.params['associatedTo'][wlan] != ap:
                if sta.params['associatedTo'][wlan] != '':
                    sta.cmd('iw dev %s disconnect' % intf)
                if 'encrypt' not in ap.params:
                    sta.cmd('iw dev %s connect %s %s' %
                            (sta.params['wlan'][wlan], ap.params['ssid'][0],
                             ap.params['mac'][0]))
                    debug ('%s is now associated with %s\n' % (sta, ap))
                else:
                    if ap.params['encrypt'][0] == 'wpa' or \
                                    ap.params['encrypt'][0] == 'wpa2':
                        self.associate_wpa(ap, wlan, ap_wlan=0)
                    elif ap.params['encrypt'][0] == 'wep':
                        self.associate_wep(ap, wlan, ap_wlan=0)
                Association.update_association(sta, ap, wlan)
                wirelessLink(sta, ap, wlan, 0, dist)
            else:
                info ('%s is already connected!\n' % ap)
            self.configLinks()
        else:
            info("%s is out of range!" % (ap))

    def associate_wpa(self, ap, wlan, ap_wlan):
        "Association with WPA"
        passwd = self.params['passwd'][wlan]
        if 'passwd' not in self.params:
            passwd = ap.params['passwd'][ap_wlan]

        pidfile = "mn%d_%s_%s_wpa.pid" % (os.getpid(), self.name, wlan)
        self.cmd("wpa_supplicant -B -Dnl80211 -P %s -i %s -c "
                 "<(wpa_passphrase \"%s\" \"%s\")"
                 % (pidfile, self.params['wlan'][wlan], wlan,
                    ap.params['ssid'][ap_wlan], passwd))

    def associate_wep(self, ap, wlan, ap_wlan):
        "Association with WEP"
        passwd = self.params['passwd'][wlan]
        if 'passwd' not in self.params:
            passwd = ap.params['passwd'][ap_wlan]

        self.pexec('iw dev %s connect %s key d:0:%s' \
                % (self.params['wlan'][wlan], ap.params['ssid'][ap_wlan], passwd))

    def get_private_folder_manager(self):
        # type: () -> PrivateFolderManager
        return self.private_folder_manager

    def _popen(self, cmd, **params):
        """Internal method: spawn and return a process
            cmd: command to run (list)
            params: parameters to Popen()"""
        # Leave this is as an instance method for now
        assert self
        return Popen(cmd, **params)

    def cleanup(self):
        "Help python collect its garbage."
        # We used to do this, but it slows us down:
        # Intfs may end up in root NS
        # for intfName in self.intfNames():
        # if self.name in intfName:
        # quietRun( 'ip link del ' + intfName )
        self.shell = None

    # Subshell I/O, commands and control

    def read(self, maxbytes=1024):
        """Buffered read from node, non-blocking.
           maxbytes: maximum number of bytes to return"""
        count = len(self.readbuf)
        if count < maxbytes:
            data = os.read(self.stdout.fileno(), maxbytes - count)
            if py_version_info < (3, 0):
                self.readbuf += data
            else:
                self.readbuf += data.decode('utf-8')
        if maxbytes >= len(self.readbuf):
            result = self.readbuf
            self.readbuf = ''
        else:
            result = self.readbuf[ :maxbytes ]
            self.readbuf = self.readbuf[ maxbytes: ]
        return result

    def readline(self):
        """Buffered readline from node, non-blocking.
           returns: line (minus newline) or None"""
        self.readbuf += self.read(1024)
        if '\n' not in self.readbuf:
            return None
        pos = self.readbuf.find('\n')
        line = self.readbuf[ 0: pos ]
        self.readbuf = self.readbuf[ pos + 1: ]
        return line

    def write(self, data):
        """Write data to node.
           data: string"""
        if py_version_info < (3, 0):
            os.write( self.stdin.fileno(), data )
        else:
            os.write(self.stdin.fileno(), data.encode())

    def terminate(self):
        "Send kill signal to Node and clean up after it."
        self.private_folder_manager.finish()
        if self.shell:
            if self.shell.poll() is None:
                os.killpg(self.shell.pid, signal.SIGHUP)
        self.cleanup()

    def stop(self, deleteIntfs=False):
        """Stop node.
           deleteIntfs: delete interfaces? (False)"""
        if deleteIntfs:
            self.deleteIntfs()
        self.terminate()

    def waitReadable(self, timeoutms=None):
        """Wait until node's output is readable.
           timeoutms: timeout in ms or None to wait indefinitely."""
        if len(self.readbuf) == 0:
            self.pollOut.poll(timeoutms)

    def sendCmd(self, *args, **kwargs):
        """Send a command, followed by a command to echo a sentinel,
           and return without waiting for the command to complete.
           args: command and arguments, or string
           printPid: print command's PID? (False)"""
        assert self.shell and not self.waiting
        printPid = kwargs.get('printPid', False)
        # Allow sendCmd( [ list ] )
        if len(args) == 1 and isinstance(args[ 0 ], list):
            cmd = args[ 0 ]
        # Allow sendCmd( cmd, arg1, arg2... )
        elif len(args) > 0:
            cmd = args
        # Convert to string
        if not isinstance(cmd, str):
            cmd = ' '.join([ str(c) for c in cmd ])
        if not re.search(r'\w', cmd):
            # Replace empty commands with something harmless
            cmd = 'echo -n'
        self.lastCmd = cmd
        # if a builtin command is backgrounded, it still yields a PID
        if len(cmd) > 0 and cmd[ -1 ] == '&':
            # print ^A{pid}\n so monitor() can set lastPid
            cmd += ' printf "\\001%d\\012" $! '
        elif printPid and not isShellBuiltin(cmd):
            cmd = 'mnexec -p ' + cmd
        self.write(cmd + '\n')
        self.lastPid = None
        self.waiting = True

    def sendInt(self, intr=chr(3)):
        "Interrupt running command."
        debug('sendInt: writing chr(%d)\n' % ord(intr))
        self.write(intr)

    def monitor(self, timeoutms=None, findPid=True):
        """Monitor and return the output of a command.
           Set self.waiting to False if command has completed.
           timeoutms: timeout in ms or None to wait indefinitely
           findPid: look for PID from mnexec -p"""
        self.waitReadable(timeoutms)
        data = self.read(1024)
        pidre = r'\[\d+\] \d+\r\n'
        # Look for PID
        marker = chr(1) + r'\d+\r\n'
        if findPid and chr(1) in data:
            # suppress the job and PID of a backgrounded command
            if re.findall(pidre, data):
                data = re.sub(pidre, '', data)
            # Marker can be read in chunks; continue until all of it is read
            while not re.findall(marker, data):
                data += self.read(1024)
            markers = re.findall(marker, data)
            if markers:
                self.lastPid = int(markers[ 0 ][ 1: ])
                data = re.sub(marker, '', data)
        # Look for sentinel/EOF
        if len(data) > 0 and data[ -1 ] == chr(127):
            self.waiting = False
            data = data[ :-1 ]
        elif chr(127) in data:
            self.waiting = False
            data = data.replace(chr(127), '')
        return data

    def waitOutput(self, verbose=False, findPid=True):
        """Wait for a command to complete.
           Completion is signaled by a sentinel character, ASCII(127)
           appearing in the output stream.  Wait for the sentinel and return
           the output, including trailing newline.
           verbose: print output interactively"""
        log = info if verbose else debug
        output = ''
        while self.waiting:
            data = self.monitor(findPid=findPid)
            output += data
            log(data)
        return output

    def cmd(self, *args, **kwargs):
        """Send a command, wait for output, and return it.
           cmd: string"""
        verbose = kwargs.get('verbose', False)
        log = info if verbose else debug
        log('*** %s : %s\n' % (self.name, args))
        if self.shell:
            self.sendCmd(*args, **kwargs)
            return self.waitOutput(verbose)
        else:
            pass
            # warn( '(%s exited - ignoring cmd%s)\n' % ( self, args ) )

    def cmdPrint(self, *args):
        """Call cmd and printing its output
           cmd: string"""
        return self.cmd(*args, **{ 'verbose': True })

    def popen(self, *args, **kwargs):
        """Return a Popen() object in our namespace
           args: Popen() args, single list, or string
           kwargs: Popen() keyword args"""
        defaults = { 'stdout': PIPE, 'stderr': PIPE,
                     'mncmd':
                     [ 'mnexec', '-da', str(self.pid) ] }
        defaults.update(kwargs)
        if len(args) == 1:
            if isinstance(args[ 0 ], list):
                # popen([cmd, arg1, arg2...])
                cmd = args[ 0 ]
            elif isinstance(args[ 0 ], string_types):
                # popen("cmd arg1 arg2...")
                cmd = args[ 0 ].split()
            else:
                raise Exception('popen() requires a string or list')
        elif len(args) > 0:
            # popen( cmd, arg1, arg2... )
            cmd = list(args)
        # Attach to our namespace  using mnexec -a
        cmd = defaults.pop('mncmd') + cmd
        # Shell requires a string, not a list!
        if defaults.get('shell', False):
            cmd = ' '.join(cmd)
        popen = self._popen(cmd, **defaults)
        return popen

    def pexec(self, *args, **kwargs):
        """Execute a command using popen
           returns: out, err, exitcode"""
        popen = self.popen(*args, stdin=PIPE, stdout=PIPE,
                           stderr=PIPE,**kwargs)
        # Warning: this can fail with large numbers of fds!
        out, err = popen.communicate()
        exitcode = popen.wait()
        return out, err, exitcode

    # Interface management, configuration, and routing

    # BL notes: This might be a bit redundant or over-complicated.
    # However, it does allow a bit of specialization, including
    # changing the canonical interface names. It's also tricky since
    # the real interfaces are created as veth pairs, so we can't
    # make a single interface at a time.

    def newWlanPort(self):
        "Return the next port number to allocate."
        self.wlanports += 1
        return self.wlanports

    def newPort(self):
        "Return the next port number to allocate."
        if len(self.ports) > 0:
            return max(self.ports.values()) + 1
        return self.portBase

    def addIntf(self, intf, port=None, moveIntfFn=moveIntf):
        """Add an interface.
           intf: interface
           port: port number (optional, typically OpenFlow port number)
           moveIntfFn: function to move interface (optional)"""
        if port is None:
            port = self.newPort()
        self.intfs[ port ] = intf
        self.ports[ intf ] = port
        self.nameToIntf[ intf.name ] = intf
        debug('\n')
        debug('added intf %s (%s) to node %s\n' %
              (intf, port, self.name))
        if self.inNamespace:
            if hasattr(self, 'type'):
                debug('moving', intf, 'into namespace for', self.name, '\n')
                moveIntfFn(intf.name, self)

    def delIntf(self, intf):
        """Remove interface from Node's known interfaces
           Note: to fully delete interface, call intf.delete() instead"""
        port = self.ports.get(intf)
        if port is not None:
            del self.intfs[ port ]
            del self.ports[ intf ]
            del self.nameToIntf[ intf.name ]

    def defaultIntf(self):
        "Return interface for lowest port"
        ports = self.intfs.keys()
        if ports:
            return self.intfs[ min(ports) ]
        else:
            warn('*** defaultIntf: warning:', self.name,
                 'has no interfaces\n')

    def intf(self, intf=None):
        """Return our interface object with given string name,
           default intf if name is falsy (None, empty string, etc).
           or the input intf arg.
        Having this fcn return its arg for Intf objects makes it
        easier to construct functions with flexible input args for
        interfaces (those that accept both string names and Intf objects).
        """
        if not intf:
            return self.defaultIntf()
        elif isinstance(intf, string_types):
            return self.nameToIntf[ intf ]
        else:
            return intf

    def connectionsTo(self, node):
        "Return [ intf1, intf2... ] for all intfs that connect self to node."
        # We could optimize this if it is important
        connections = []
        for intf in self.intfList():
            link = intf.link
            if link and link.intf2 != None and link.intf2 != 'wifi':
                node1, node2 = link.intf1.node, link.intf2.node
                if node1 == self and node2 == node:
                    connections += [ (intf, link.intf2) ]
                elif node1 == node and node2 == self:
                    connections += [ (intf, link.intf1) ]
        return connections

    def deleteIntfs(self, checkName=True):
        """Delete all of our interfaces.
           checkName: only delete interfaces that contain our name"""
        # In theory the interfaces should go away after we shut down.
        # However, this takes time, so we're better off removing them
        # explicitly so that we won't get errors if we run before they
        # have been removed by the kernel. Unfortunately this is very slow,
        # at least with Linux kernels before 2.6.33
        for intf in list(self.intfs.values()):
            # Protect against deleting hardware interfaces
            if (self.name in intf.name) or (not checkName):
                intf.delete()
                info('.')

    # Routing support

    def setARP(self, ip, mac):
        """Add an ARP entry.
           ip: IP address as string
           mac: MAC address as string"""
        result = self.cmd('arp', '-s', ip, mac)
        return result

    def setHostRoute(self, ip, intf):
        """Add route to host.
           ip: IP address as dotted decimal
           intf: string, interface name"""
        return self.cmd('route add -host', ip, 'dev', intf)

    def setDefaultRoute(self, intf=None):
        """Set the default route to go through intf.
           intf: Intf or {dev <intfname> via <gw-ip> ...}"""
        # Note setParam won't call us if intf is none
        if isinstance(intf, string_types) and ' ' in intf:
            params = intf
        else:
            params = 'dev %s' % intf
        # Do this in one line in case we're messing with the root namespace
        self.cmd('ip route del default; ip route add default', params)

    # Convenience and configuration methods
    def setMAC(self, mac, intf=None):
        """Set the MAC address for an interface.
           intf: intf or intf name
           mac: MAC address as string"""
        return self.intf(intf).setMAC(mac)

    def setIP(self, ip, prefixLen=8, intf=None, **kwargs):
        """Set the IP address for an interface.
           intf: intf or intf name
           ip: IP address as a string
           prefixLen: prefix length, e.g. 8 for /8 or 16M addrs
           kwargs: any additional arguments for intf.setIP"""
        if intf is not None and (isinstance(self, Station)
                                 or isinstance(self, Car)):
            if intf in self.params['wlan']:
                wlan = int(intf[-1:])
                self.params['ip'][wlan] = ip

        return self.intf(intf).setIP(ip, prefixLen, **kwargs)

    def IP(self, intf=None):
        "Return IP address of a node or specific interface."
        return self.intf(intf).IP()

    def MAC(self, intf=None):
        "Return MAC address of a node or specific interface."
        return self.intf(intf).MAC()

    def intfIsUp(self, intf=None):
        "Check if an interface is up."
        return self.intf(intf).isUp()

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
        if value is None:
            return
        f = getattr(self, method, None)
        if not f:
            return
        if isinstance(value, list):
            result = f(*value)
        elif isinstance(value, dict):
            result = f(**value)
        else:
            result = f(value)
        results[ name ] = result
        return result

    def config(self, mac=None, ip=None, defaultRoute=None, lo='up', **_params):
        """Configure Node according to (optional) parameters:
           mac: MAC address for default interface
           ip: IP address for default interface
           ip addr: arbitrary interface configuration
           Subclasses should override this method and call
           the parent class's config(**params)"""
        # If we were overriding this method, we would call
        # the superclass config method here as follows:
        # r = Parent.config( **_params )
        r = {}
        if isinstance(self, Station) or isinstance(self, Car):
            if len(ip) > 1:
                ip = ip[0]
        if not isinstance(self, Station) and not isinstance(self, Car):
            self.setParam(r, 'setMAC', mac=mac)
        self.setParam(r, 'setIP', ip=ip)
        self.setParam(r, 'setDefaultRoute', defaultRoute=defaultRoute)

        # This should be examined
        self.cmd('ip link set lo ' + lo)
        return r

    def configDefault(self, **moreParams):
        "Configure with default parameters"
        self.params.update(moreParams)
        self.config(**self.params)

    # This is here for backward compatibility
    def linkTo(self, node, link=Link):
        """(Deprecated) Link to another node
           replace with Link( node1, node2)"""
        return link(self, node)

    # Other methods
    def intfList(self):
        "List of our interfaces sorted by port number"
        return [ self.intfs[ p ] for p in sorted(self.intfs.keys()) ]

    def intfNames(self):
        "The names of our interfaces sorted by port number"
        return [ str(i) for i in self.intfList() ]

    def __repr__(self):
        "More informative string representation"
        intfs = (','.join([ '%s:%s' % (i.name, i.IP())
                            for i in self.intfList() ]))
        return '<%s %s: %s pid=%s> ' % (
            self.__class__.__name__, self.name, intfs, self.pid)

    def __str__(self):
        "Abbreviated string representation"
        return self.name

    # Automatic class setup support
    isSetup = False

    @classmethod
    def checkSetup(cls):
        "Make sure our class and superclasses are set up"
        while cls and not getattr(cls, 'isSetup', True):
            cls.setup()
            cls.isSetup = True
            # Make pylint happy
            cls = getattr(type(cls), '__base__', None)

    @classmethod
    def setup(cls):
        "Make sure our class dependencies are available"
        pathCheck('mnexec', 'ip addr', moduleName='Mininet')

    def stop_(self):
        "Stops hostapd"
        from mn_wifi.plot import plot2d
        process = 'mn%d_%s' % (os.getpid(), self.name)
        os.system('pkill -f \'hostapd -B %s\'' % process)
        if plot2d.fig_exists():
            plot2d.updateCircleColor(self, 'w')

    def start_(self):
        "Starts hostapd"
        from mn_wifi.plot import plot2d
        process = 'mn%d_%s' % (os.getpid(), self.name)
        os.system('hostapd -B %s-wlan1.apconf' % process)
        if plot2d.fig_exists():
            plot2d.updateCircleColor(self, 'b')


class Station(Node_wifi):
    "A station is simply a Node"
    pass


class Car(Node_wifi):
    "A car is simply a Node"
    pass


class AP(Node_wifi):
    """A Switch is a Node that is running (or has execed?)
       an OpenFlow switch."""
    portBase = 1  # Switches start with port 1 in OpenFlow
    dpidLen = 16  # digits in dpid passed to switch

    def __init__(self, name, dpid=None, opts='', listenPort=None, **params):
        """dpid: dpid hex string (or None to derive from name, e.g. s1 -> 1)
           opts: additional switch options
           listenPort: port to listen on for dpctl connections"""
        Node_wifi.__init__(self, name, **params)
        self.dpid = self.defaultDpid(dpid)
        self.opts = opts
        self.listenPort = listenPort
        if not self.inNamespace:
            self.controlIntf = Intf('lo', self, port=0)

    def defaultDpid(self, dpid=None):
        "Return correctly formatted dpid from dpid or switch name (s1 -> 1)"
        if dpid:
            # Remove any colons and make sure it's a good hex number
            if py_version_info < (3, 0):
                dpid = dpid.translate(None, ':')
            else:
                dpid = dpid.translate(str.maketrans('', '', ':'))
            assert len(dpid) <= self.dpidLen and int(dpid, 16) >= 0
        else:
            # Use hex of the first number in the switch name
            nums = re.findall(r'\d+', self.name)
            if nums:
                dpid = hex(int(nums[ 0 ]))[ 2: ]
            else:
                raise Exception('Unable to derive default datapath ID - '
                                'please either specify a dpid or use a '
                                'canonical ap name such as ap23.')
        return '1' + '0' * (self.dpidLen -1 - len(dpid)) + dpid

    def defaultIntf(self):
        "Return control interface"
        if self.controlIntf:
            return self.controlIntf
        else:
            return Node_wifi.defaultIntf(self)


    def sendCmd(self, *cmd, **kwargs):
        """Send command to Node.
           cmd: string"""
        kwargs.setdefault('printPid', False)
        if not self.execed:
            return Node_wifi.sendCmd(self, *cmd, **kwargs)
        else:
            error('*** Error: %s has execed and cannot accept commands' %
                  self.name)

    def connected(self):
        "Is the switch connected to a controller? (override this method)"
        # Assume that we are connected by default to whatever we need to
        # be connected to. This should be overridden by any OpenFlow
        # switch, but not by a standalone bridge.
        debug('Assuming', repr(self), 'is connected to a controller\n')
        return True

    def stop(self, deleteIntfs=True):
        """Stop switch
           deleteIntfs: delete interfaces? (True)"""
        if deleteIntfs:
            self.deleteIntfs()

    def __repr__(self):
        "More informative string representation"
        intfs = (','.join([ '%s:%s' % (i.name, i.IP())
                            for i in self.intfList() ]))
        return '<%s %s: %s pid=%s> ' % (
            self.__class__.__name__, self.name, intfs, self.pid)


class AccessPoint(AP):
    """An AccessPoint is a Switch equipped with wireless interface that is
    running (or has execed?) an OpenFlow switch."""

    writeMacAddress = False

    def __init__(cls, aps, driver, link):
        'configure ap'
        cls.configure(aps, driver, link)

    @classmethod
    def configure(cls, aps, driver, link):
        """Configure APs

        :param aps: list of access points"""
        for ap in aps:
            if 'vssids' in ap.params:
                for i in range(1, ap.params['vssids'] + 1):
                    ap.params['range'].append(ap.params['range'][0])
                    ap.params['wlan'].append('%s-%s'
                                             % (ap.params['wlan'][0], i))
                    ap.params['mode'].append(ap.params['mode'][0])
                    ap.params['frequency'].append(
                        ap.params['frequency'][0])
                    ap.params['mac'].append('')
            else:
                for i in range(1, len(ap.params['wlan'])):
                    ap.params['mac'].append('')
            ap.params['driver'] = driver
            cls.verifyNetworkManager(ap)
        cls.restartNetworkManager()

        for ap in aps:
            wlans = len(ap.params['wlan'])
            if 'link' not in ap.params:
                if 'phywlan' in ap.params:
                    for wlan in range(wlans):
                        cls.setConfig(ap, aps, wlan, link)
                        if 'vssids' in ap.params:
                            break
                for wlan in range(wlans):
                    cls.setConfig(ap, aps, wlan, link)
                    if 'vssids' in ap.params:
                        break
                ap.phyID = module.phyID
                module.phyID += 1

    @classmethod
    def setConfig(cls, ap, aplist=None, wlan=None, link=None):
        """Configure AP

        :param ap: ap node
        :param aplist: list of aps
        :param wlan: wlan id
        :param link: if wmediumd"""
        if ap.params['ssid'][wlan] != '':
            if 'encrypt' in ap.params and 'config' not in ap.params:
                if ap.params['encrypt'][wlan] == 'wpa':
                    ap.auth_algs = 1
                    ap.wpa = 1
                    if 'ieee80211r' in ap.params \
                            and ap.params['ieee80211r'] == 'yes':
                        ap.wpa_key_mgmt = 'FT-EAP'
                    else:
                        ap.wpa_key_mgmt = 'WPA-EAP'
                    ap.rsn_pairwise = 'TKIP CCMP'
                    ap.wpa_passphrase = ap.params['passwd'][0]
                elif ap.params['encrypt'][wlan] == 'wpa2':
                    ap.auth_algs = 1
                    ap.wpa = 2
                    if 'ieee80211r' in ap.params \
                            and ap.params['ieee80211r'] == 'yes' \
                            and 'authmode' not in ap.params:
                        ap.wpa_key_mgmt = 'FT-PSK'
                    elif 'authmode' in ap.params \
                            and ap.params['authmode'] == '8021x':
                        ap.wpa_key_mgmt = 'WPA-EAP'
                    else:
                        ap.wpa_key_mgmt = 'WPA-PSK'
                    ap.rsn_pairwise = 'CCMP'
                    if 'authmode' not in ap.params:
                        ap.wpa_passphrase = ap.params['passwd'][0]
                elif ap.params['encrypt'][wlan] == 'wep':
                    ap.auth_algs = 2
                    ap.wep_key0 = ap.params['passwd'][0]

            cls.setHostapdConfig(ap, wlan, aplist, link)

    @classmethod
    def setHostapdConfig(cls, ap, wlan, aplist, link):
        "Set hostapd config"
        cmd = ("echo \'")

        if 'phywlan' in ap.params:
            cmd = cmd + ("interface=%s" % ap.params.get('phywlan'))
        else:
            cmd = cmd + ("interface=%s" % ap.params['wlan'][wlan])

        cmd = cmd + ("\ndriver=%s" % ap.params['driver'])
        cmd = cmd + ("\nssid=%s" % ap.params['ssid'][wlan])
        cmd = cmd + ('\nwds_sta=1')
        if ap.params['mode'][wlan] == 'n':
            if 'band' in ap.params:
                if ap.params['band'] == 2.4:
                    cmd = cmd + ("\nhw_mode=g")
                elif ap.params['band'] == 5:
                    cmd = cmd + ("\nhw_mode=a")
                ap.params.pop("band", None)
            else:
                cmd = cmd + ("\nhw_mode=g")
        elif ap.params['mode'][wlan] == 'a':
            cmd = cmd + ('\ncountry_code=US')
            cmd = cmd + ("\nhw_mode=%s" % ap.params['mode'][wlan])
        elif ap.params['mode'][wlan] == 'ac':
            cmd = cmd + ('\ncountry_code=US')
            cmd = cmd + ("\nhw_mode=a")
        else:
            cmd = cmd + ("\nhw_mode=%s" % ap.params['mode'][wlan])
        cmd = cmd + ("\nchannel=%s" % ap.params['channel'][wlan])
        if 'ht_capab' in ap.params:
            cmd = cmd + ('\nht_capab=%s' % ap.params['ht_capab'])
        if 'beacon_int' in ap.params:
            cmd = cmd + ('\nbeacon_int=%s' % ap.params['beacon_int'])
        if 'client_isolation' in ap.params and ap.params['cliente_isolation'][wlan]:
            cmd = cmd + ('\nap_isolate=1')
        if 'config' in ap.params:
            config = ap.params['config']
            if config is not []:
                config = ap.params['config'].split(',')
                # ap.params.pop("config", None)
                for conf in config:
                    cmd = cmd + "\n" + conf
        else:
            if 'authmode' in ap.params and ap.params['authmode'] == '8021x':
                cmd = cmd + ("\nieee8021x=1")
                cmd = cmd + ("\nwpa_key_mgmt=WPA-EAP")
                if 'encrypt' in ap.params:
                    cmd = cmd + ("\nauth_algs=%s" % ap.auth_algs)
                    cmd = cmd + ("\nwpa=%s" % ap.wpa)
                else:
                    cmd = cmd + ('\nwpa=3')
                cmd = cmd + ('\neap_server=0')
                cmd = cmd + ('\neapol_version=2')

                if 'radius_server' not in ap.params:
                    ap.params['radius_server'] = '127.0.0.1'
                cmd = cmd + ("\nwpa_pairwise=TKIP CCMP")
                cmd = cmd + ("\neapol_key_index_workaround=0")
                cmd = cmd + ("\nown_ip_addr=%s" % ap.params['radius_server'])
                cmd = cmd + ("\nnas_identifier=%s.example.com" % ap.name)
                cmd = cmd + ("\nauth_server_addr=%s"
                             % ap.params['radius_server'])
                cmd = cmd + ("\nauth_server_port=1812")
                if 'shared_secret' not in ap.params:
                    ap.params['shared_secret'] = 'secret'
                cmd = cmd + ("\nauth_server_shared_secret=%s"
                             % ap.params['shared_secret'])
            else:
                #cmd = cmd + ("\nwme_enabled=1")
                #cmd = cmd + ("\nwmm_enabled=1")

                if 'encrypt' in ap.params:
                    if 'wpa' in ap.params['encrypt'][wlan]:
                        cmd = cmd + ("\nauth_algs=%s" % ap.auth_algs)
                        cmd = cmd + ("\nwpa=%s" % ap.wpa)
                        cmd = cmd + ("\nwpa_key_mgmt=%s" % ap.wpa_key_mgmt)
                        cmd = cmd + ("\nwpa_pairwise=%s" % ap.rsn_pairwise)
                        cmd = cmd + ("\nwpa_passphrase=%s" % ap.wpa_passphrase)
                    elif ap.params['encrypt'][wlan] == 'wep':
                        cmd = cmd + ("\nauth_algs=%s" % ap.auth_algs)
                        cmd = cmd + ("\nwep_default_key=%s" % 0)
                        cmd = cmd + cls.verifyWepKey(ap.wep_key0)

                if ap.params['mode'][wlan] == 'ac':
                    cmd = cmd + ("\nieee80211ac=1")
                elif ap.params['mode'][wlan] == 'n':
                    cmd = cmd + ("\nieee80211n=1")

                if 'ieee80211r' in ap.params and \
                                ap.params['ieee80211r'] is 'yes':
                    if 'mobility_domain' in ap.params:
                        cmd = cmd + ("\nmobility_domain=%s" %
                                     ap.params['mobility_domain'])
                        # cmd = cmd + ("\nown_ip_addr=127.0.0.1")
                        cmd = cmd + ("\nnas_identifier=%s.example.com"
                                     % ap.name)
                        for apref in aplist:
                            cmd = cmd + ('\nr0kh=%s r0kh-%s.example.com '
                                         '000102030405060708090a0b0c0d0e0f'
                                         % (apref.params['mac'][wlan],
                                            aplist.index(apref)))
                            cmd = cmd + ('\nr1kh=%s %s '
                                         '000102030405060708090a0b0c0d0e0f'
                                         % (apref.params['mac'][wlan],
                                            apref.params['mac'][wlan]))
                        cmd = cmd + ('\npmk_r1_push=1')
                        cmd = cmd + ('\nft_over_ds=1')
                        cmd = cmd + ('\nft_psk_generate_local=1')
        if 'vssids' in ap.params:
            for i in range(1, ap.params['vssids']+1):
                ap.params['txpower'].append(ap.params['txpower'][wlan])
                ap.params['antennaGain'].append(ap.params['antennaGain'][wlan])
                ap.params['antennaHeight'].append(ap.params['antennaHeight'][wlan])
                ssid = ap.params['ssid'][i]
                cmd = cmd + ('\n')
                cmd = cmd + ("\nbss=%s" % ap.params['wlan'][i])
                cmd = cmd + ("\nssid=%s" % ssid)
                if 'encrypt' in ap.params:
                    if ap.params['encrypt'][i] == 'wep':
                        cmd = cmd + ("\nauth_algs=%s" % ap.auth_algs)
                        cmd = cmd + ("\nwep_default_key=0")
                        cmd = cmd + cls.verifyWepKey(ap.wep_key0)
                ap.params['mac'][i] = ap.params['mac'][wlan][:-1] + str(i)
        cmd = cmd + ("\nctrl_interface=/var/run/hostapd")
        cmd = cmd + ("\nctrl_interface_group=0")
        cls.APConfigFile(cmd, ap, wlan)

        if 'vssids' in ap.params:
            for i in range(1, ap.params['vssids']+1):
                wlan = i
                ap.params['mac'][wlan] = ''
                cls.setIPMAC(ap, wlan)
                intf = ap.params['wlan'][wlan]
                TCLinkWirelessAP(ap, intfName1=intf)

        iface = ap.params['wlan'][wlan]
        if 'phywlan' in ap.params:
            iface = ap.params['phywlan']
            ap.params.pop('phywlan', None)

        if not link or (link and str(link.__name__) != 'wmediumd'):
            AccessPoint.setBw(ap, wlan, iface)

        ap.params['frequency'][wlan] = ap.get_freq(0)

    @classmethod
    def setBw(cls, node, wlan, iface):
        "Set bw"
        bw = cls.getRate(node, wlan)
        node.cmd("tc qdisc replace dev %s \
                root handle 2: tbf rate %sMbit burst 15000 "
                 "latency 1ms" % (iface, bw))
        # Reordering packets
        node.cmd('tc qdisc add dev %s parent 2:1 handle 10: '
                 'pfifo limit 1000' % (iface))

    @classmethod
    def getRate(cls, node, wlan):
        "Get rate"
        mode = node.params['mode'][wlan]

        if mode == 'a':
            rate = 54
        elif mode == 'b':
            rate = 11
        elif mode == 'g':
            rate = 54
        elif mode == 'n':
            rate = 300
        elif mode == 'ac':
            rate = 600
        else:
            rate = 54
        return rate

    @classmethod
    def verifyWepKey(cls, wep_key0):
        "Check WEP key"
        if len(wep_key0) == 10 or len(wep_key0) == 26 or len(wep_key0) == 32:
            cmd = ("\nwep_key0=%s" % wep_key0)
        elif len(wep_key0) == 5 or len(wep_key0) == 13 or len(wep_key0) == 16:
            cmd = ("\nwep_key0=\"%s\"" % wep_key0)
        else:
            info("Warning! Wep Key is wrong!\n")
            exit(1)
        return cmd

    _macMatchRegex = re.compile(r'..:..:..:..:..:..')

    @classmethod
    def setIPMAC(cls, ap, wlan):
        if ap.params['mac'][wlan] != '':
            ap.setMAC(ap.params['mac'][wlan], ap.params['wlan'][wlan])
        else:
            ap.params['mac'][wlan] = \
                ap.getMAC(ap.params['wlan'][wlan])
        if ap.params['mac'][wlan] != None:
            cls.checkNetworkManager(ap.params['mac'][wlan])
        if 'inNamespace' in ap.params and 'ip' in ap.params:
            ap.setIP(ap.params['ip'], intf=ap.params['wlan'][wlan])

    @classmethod
    def restartNetworkManager(cls):
        """Restart network manager if the mac address of the AP
        is not included at /etc/NetworkManager/NetworkManager.conf"""
        nm_is_running = os.system('service network-manager status 2>&1 | grep '
                                  '-ic running >/dev/null 2>&1')
        if AccessPoint.writeMacAddress and nm_is_running != 256:
            info('Mac Address(es) of AP(s) is(are) being added into '
                 '/etc/NetworkManager/NetworkManager.conf\n')
            info('Restarting network-manager...\n')
            os.system('service network-manager restart')
        AccessPoint.writeMacAddress = False

    @classmethod
    def configureIface(cls, node, wlan):
        intf = module.wlan_list[0]
        module.wlan_list.pop(0)
        node.renameIface(intf, node.params['wlan'][wlan])

    @classmethod
    def verifyNetworkManager(cls, node):
        """First verify if the mac address of the ap is included at
        NetworkManager.conf

        :param node: node"""
        for wlan in range(len(node.params['wlan'])):
            if 'inNamespace' not in node.params:
                if not isinstance(node, Station):
                    options = dict()
                    cls.configureIface(node, wlan)
                    link = TCLinkWirelessAP(node, **options)
                    #cls.links.append(link)
            elif 'inNamespace' in node.params:
                link = TCLinkWirelessAP(node)
                #cls.links.append(link)
            AccessPoint.setIPMAC(node, wlan)
            if 'vssids' in node.params:
                break

    @classmethod
    def checkNetworkManager(cls, mac):
        "add mac address into /etc/NetworkManager/NetworkManager.conf"
        writeMacAddress = False
        unmatch = ""
        if os.path.exists('/etc/NetworkManager/NetworkManager.conf'):
            if os.path.isfile('/etc/NetworkManager/NetworkManager.conf'):
                cls.resultIface = open('/etc/NetworkManager/'
                                       'NetworkManager.conf')
                lines = cls.resultIface

            isNew = True
            for n in lines:
                if "unmanaged-devices" in n:
                    unmatch = n
                    echo = n
                    echo.replace(" ", "")
                    echo = echo[:-1] + ";"
                    isNew = False
            if isNew:
                os.system("echo '#' >> /etc/NetworkManager/"
                          "NetworkManager.conf")
                unmatch = "#"
                echo = "[keyfile]\nunmanaged-devices="

            if mac not in unmatch:
                echo = echo + "mac:" + mac + ';'
                writeMacAddress = True

            if writeMacAddress:
                for line in fileinput.input('/etc/NetworkManager/'
                                            'NetworkManager.conf', inplace=1):
                    if isNew:
                        if line.__contains__('#'):
                            print(line.replace(unmatch, echo))
                        else:
                            print(line.rstrip())
                    else:
                        if line.__contains__('unmanaged-devices'):
                            print(line.replace(unmatch, echo))
                        else:
                            print(line.rstrip())
        if cls.writeMacAddress is False:
            cls.writeMacAddress = writeMacAddress

    @classmethod
    def APConfigFile(cls, cmd, ap, wlan):
        "run an Access Point and create the config file"
        iface = ap.params['wlan'][wlan]
        if 'phywlan' in ap.params:
            iface = ap.params['phywlan']
            ap.cmd('ip link set %s down' % iface)
            ap.cmd('ip link set %s up' % iface)
        apconfname = "mn%d_%s.apconf" % (os.getpid(), iface)
        content = cmd + ("\' > %s" % apconfname)
        ap.cmd(content)
        cmd = ("hostapd -B %s" % apconfname)
        try:
            ap.cmd(cmd)
            if int(ap.params['channel'][wlan]) == 0 \
                    or ap.params['channel'][wlan] == 'acs_survey':
                info("*** Waiting for ACS... It takes 10 seconds.\n")
                sleep(10)
        except:
            print('error with hostapd. Please, run sudo mn -c in order ' \
            'to fix it or check if hostapd is working properly in ' \
            'your system.')
            exit(1)


class UserAP(AP):
    "User-space AP."

    dpidLen = 12

    def __init__(self, name, dpopts='--no-slicing', **kwargs):
        """Init.
           name: name for the switch
           dpopts: additional arguments to ofdatapath (--no-slicing)"""
        AP.__init__(self, name, **kwargs)
        pathCheck('ofdatapath', 'ofprotocol',
                  moduleName='the OpenFlow reference user switch' +
                  '(openflow.org)')
        if self.listenPort:
            self.opts += ' --listen=ptcp:%i ' % self.listenPort
        else:
            self.opts += ' --listen=punix:/tmp/%s.listen' % self.name
        self.dpopts = dpopts

    @classmethod
    def setup(cls):
        "Ensure any dependencies are loaded; if not, try to load them."
        if not os.path.exists('/dev/net/tun'):
            moduleDeps(add=TUN)

    def dpctl(self, *args):
        "Run dpctl command"
        listenAddr = None
        if not self.listenPort:
            listenAddr = 'unix:/tmp/%s.listen' % self.name
        else:
            listenAddr = 'tcp:127.0.0.1:%i' % self.listenPort
        return self.cmd('dpctl ' + ' '.join(args) + ' ' + listenAddr)

    def connected(self):
        "Is the ap connected to a controller?"
        status = self.dpctl('status')
        return ('remote.is-connected=true' in status and
                'local.is-connected=true' in status)

    @staticmethod
    def TCReapply(intf):
        """Unfortunately user switch and Mininet are fighting
           over tc queuing disciplines. To resolve the conflict,
           we re-create the user switch's configuration, but as a
           leaf of the TCIntf-created configuration."""
        if isinstance(intf, TCWirelessLink):
            ifspeed = 10000000000  # 10 Gbps
            minspeed = ifspeed * 0.001

            res = intf.config(**intf.params)

            if res is None:  # link may not have TC parameters
                return

            # Re-add qdisc, root, and default classes user switch created, but
            # with new parent, as setup by Mininet's TCIntf
            parent = res['parent']
            intf.tc("%s qdisc add dev %s " + parent +
                    " handle 1: htb default 0xfffe")
            intf.tc("%s class add dev %s classid 1:0xffff parent 1: htb rate "
                    + str(ifspeed))
            intf.tc("%s class add dev %s classid 1:0xfffe parent 1:0xffff " +
                    "htb rate " + str(minspeed) + " ceil " + str(ifspeed))

    def start(self, controllers):
        """Start OpenFlow reference user datapath.
           Log to /tmp/sN-{ofd,ofp}.log.
           controllers: list of controller objects"""
        # Add controllers
        clist = ','.join([ 'tcp:%s:%d' % (c.IP(), c.port)
                           for c in controllers ])
        ofdlog = '/tmp/' + self.name + '-ofd.log'
        ofplog = '/tmp/' + self.name + '-ofp.log'
        intfs = [ str(i) for i in self.intfList() if not i.IP() ]

        self.cmd('ofdatapath -i ' + ','.join(intfs) +
                 ' punix:/tmp/' + self.name + ' -d %s ' % self.dpid +
                 self.dpopts +
                 ' 1> ' + ofdlog + ' 2> ' + ofdlog + ' &')
        self.cmd('ofprotocol unix:/tmp/' + self.name +
                 ' ' + clist +
                 ' --fail=closed ' + self.opts +
                 ' 1> ' + ofplog + ' 2>' + ofplog + ' &')
        if "no-slicing" not in self.dpopts:
            # Only TCReapply if slicing is enable
            sleep(1)  # Allow ofdatapath to start before re-arranging qdisc's
            for intf in self.intfList():
                if not intf.IP():
                    self.TCReapply(intf)

    def stop(self, deleteIntfs=True):
        """Stop OpenFlow reference user datapath.
           deleteIntfs: delete interfaces? (True)"""
        # self.cmd('kill %ofdatapath')
        # self.cmd('kill %ofprotocol')
        # super(UserAP, self).stop(deleteIntfs)

    def setManagedIface(self, iface):
        wlan = self.params['wlan'].index(iface)
        if self.func[wlan] == 'mesh':
            self.cmd('iw dev %s del' % self.params['wlan'][wlan])
            iface = '%s-wlan%s' % (self, wlan)
            self.params['wlan'][wlan] = iface
        self.cmd('iw dev %s set type managed' % (self.params['wlan'][wlan]))

    def renameIface(self, intf, newname):
        "Rename interface"
        self.pexec('ip link set %s down' % intf)
        self.pexec('ip link set %s name %s' % (intf, newname))
        self.pexec('ip link set %s up' % newname)


class OVSAP(AP):
    "Open vSwitch AP. Depends on ovs-vsctl."

    def __init__(self, name, failMode='secure', datapath='kernel',
                 inband=False, protocols=None,
                 reconnectms=1000, stp=False, batch=False, **params):
        """name: name for switch
           failMode: controller loss behavior (secure|open)
           datapath: userspace or kernel mode (kernel|user)
           inband: use in-band control (False)
           protocols: use specific OpenFlow version(s) (e.g. OpenFlow13)
                      Unspecified (or old OVS version) uses OVS default
           reconnectms: max reconnect timeout in ms (0/None for default)
           stp: enable STP (False, requires failMode=standalone)
           batch: enable batch startup (False)"""
        AP.__init__(self, name, **params)
        self.failMode = failMode
        self.datapath = datapath
        self.inband = inband
        self.protocols = protocols
        self.reconnectms = reconnectms
        self.stp = stp
        self._uuids = []  # controller UUIDs
        self.batch = batch
        self.commands = []  # saved commands for batch startup

    @classmethod
    def setup(cls):
        "Make sure Open vSwitch is installed and working"
        pathCheck('ovs-vsctl',
                  moduleName='Open vSwitch (openvswitch.org)')
        # This should no longer be needed, and it breaks
        # with OVS 1.7 which has renamed the kernel module:
        #  moduleDeps( subtract=OF_KMOD, add=OVS_KMOD )
        out, err, exitcode = errRun('ovs-vsctl -t 1 show')
        if exitcode:
            error(out + err +
                  'ovs-vsctl exited with code %d\n' % exitcode +
                  '*** Error connecting to ovs-db with ovs-vsctl\n'
                  'Make sure that Open vSwitch is installed, '
                  'that ovsdb-server is running, and that\n'
                  '"ovs-vsctl show" works correctly.\n'
                  'You may wish to try '
                  '"service openvswitch-switch start".\n')
            exit(1)
        version = quietRun('ovs-vsctl --version')
        cls.OVSVersion = findall(r'\d+\.\d+', version)[ 0 ]

    @classmethod
    def isOldOVS(cls):
        "Is OVS ersion < 1.10?"
        return StrictVersion(cls.OVSVersion) < StrictVersion('1.10')

    def dpctl(self, *args):
        "Run ovs-ofctl command"
        return self.cmd('ovs-ofctl', args[ 0 ], self, *args[ 1: ])

    def vsctl(self, *args, **kwargs):
        "Run ovs-vsctl command (or queue for later execution)"
        if self.batch:
            cmd = ' '.join(str(arg).strip() for arg in args)
            self.commands.append(cmd)
        else:
            return self.cmd('ovs-vsctl', *args, **kwargs)

    @staticmethod
    def TCReapply(intf):
        """Unfortunately OVS and Mininet are fighting
           over tc queuing disciplines. As a quick hack/
           workaround, we clear OVS's and reapply our own."""
        if isinstance(intf, TCWirelessLink):
            intf.config(**intf.params)

    def attach(self, intf):
        "Connect a data port"
        self.vsctl('add-port', self, intf)
        self.cmd('ip link set', intf, 'up')
        self.TCReapply(intf)

    def detach(self, intf):
        "Disconnect a data port"
        self.vsctl('del-port', self, intf)

    def controllerUUIDs(self, update=False):
        """Return ovsdb UUIDs for our controllers
           update: update cached value"""
        if not self._uuids or update:
            controllers = self.cmd('ovs-vsctl -- get Bridge', self,
                                   'Controller').strip()
            if controllers.startswith('[') and controllers.endswith(']'):
                controllers = controllers[ 1 :-1 ]
                if controllers:
                    self._uuids = [ c.strip()
                                    for c in controllers.split(',') ]
        return self._uuids

    def connected(self):
        "Are we connected to at least one of our controllers?"
        for uuid in self.controllerUUIDs():
            if 'true' in self.vsctl('-- get Controller',
                                    uuid, 'is_connected'):
                return True
        return self.failMode == 'standalone'

    def deleteIface(self, intf_):
        for intf in self.intfs.values():
            if intf.name == intf_:
                self.delIntf(intf)

    def intfOpts(self, intf):
        "Return OVS interface options for intf"
        opts = ''
        if not self.isOldOVS():
            # ofport_request is not supported on old OVS
            opts += ' ofport_request=%s' % self.ports[ intf ]
            # Patch ports don't work well with old OVS
            if isinstance(intf, OVSIntf):
                intf1, intf2 = intf.link.intf1, intf.link.intf2
                peer = intf1 if intf1 != intf else intf2

                opts += ' type=patch options:peer=%s' % peer
        return '' if not opts else ' -- set Interface %s' % intf + opts

    def bridgeOpts(self):
        "Return OVS bridge options"
        opts = (' other_config:datapath-id=%s' % self.dpid +
                ' fail_mode=%s' % self.failMode)
        if not self.inband:
            opts += ' other-config:disable-in-band=true'
        if self.datapath == 'user':
            opts += ' datapath_type=netdev'
        if self.protocols and not self.isOldOVS():
            opts += ' protocols=%s' % self.protocols
        if self.stp and self.failMode == 'standalone':
            opts += ' stp_enable=true' % self
        return opts

    def start(self, controllers):
        "Start up a new OVS OpenFlow switch using ovs-vsctl"
        if self.inNamespace:
            raise Exception(
                'OVS kernel AP does not work in a namespace')

        int(self.dpid, 16)  # DPID must be a hex string
        # Command to add interfaces
        intfs = ''.join(' -- add-port %s %s' % (self, intf) +
                        self.intfOpts(intf)
                        for intf in self.intfList()
                        if self.ports[ intf ] and not intf.IP())

        # Command to create controller entries
        clist = [ (self.name + c.name, '%s:%s:%d' %
                   (c.protocol, c.IP(), c.port))
                  for c in controllers ]
        if self.listenPort:
            clist.append((self.name + '-listen',
                          'ptcp:%s' % self.listenPort))
        ccmd = '-- --id=@%s create Controller target=\\"%s\\"'
        if self.reconnectms:
            ccmd += ' max_backoff=%d' % self.reconnectms
        cargs = ' '.join(ccmd % (name, target)
                         for name, target in clist)
        # Controller ID list
        cids = ','.join('@%s' % name for name, _target in clist)
        # Try to delete any existing bridges with the same name
        if not self.isOldOVS():
            cargs += ' -- --if-exists del-br %s' % self
        # One ovs-vsctl command to rule them all!
        self.vsctl(cargs +
                   ' -- add-br %s' % self +
                   ' -- set bridge %s controller=[%s]' % (self, cids) +
                   self.bridgeOpts() +
                   intfs)
        # If necessary, restore TC config overwritten by OVS
        if not self.batch:
            for intf in self.intfList():
                self.TCReapply(intf)

    # This should be ~ int( quietRun( 'getconf ARG_MAX' ) ),
    # but the real limit seems to be much lower
    argmax = 128000

    @classmethod
    def batchStartup(cls, switches, run=errRun):
        """Batch startup for OVS
           switches: switches to start up
           run: function to run commands (errRun)"""
        info('...')
        cmds = 'ovs-vsctl'
        for switch in switches:
            if switch.isOldOVS():
                # Ideally we'd optimize this also
                run('ovs-vsctl del-br %s' % switch)
            for cmd in switch.commands:
                cmd = cmd.strip()
                # Don't exceed ARG_MAX
                if len(cmds) + len(cmd) >= cls.argmax:
                    run(cmds, shell=True)
                    cmds = 'ovs-vsctl'
                cmds += ' ' + cmd
                switch.cmds = []
                switch.batch = False
        if cmds:
            run(cmds, shell=True)
        # Reapply link config if necessary...
        for switch in switches:
            for intf in switch.intfs.values():
                if isinstance(intf, TCWirelessLink):
                    intf.config(**intf.params)
        return switches

    def stop(self, deleteIntfs=True):
        """Terminate OVS switch.
           deleteIntfs: delete interfaces? (True)"""
        self.cmd('ovs-vsctl del-br', self)
        if self.datapath == 'user':
            self.cmd('ip link del', self)
        super(OVSAP, self).stop(deleteIntfs)

    @classmethod
    def batchShutdown(cls, switches, run=errRun):
        "Shut down a list of OVS switches"
        delcmd = 'del-br %s'
        if switches and not switches[ 0 ].isOldOVS():
            delcmd = '--if-exists ' + delcmd
        # First, delete them all from ovsdb
        run('ovs-vsctl ' + ' -- '.join(delcmd % s for s in switches))
        # Next, shut down all of the processes
        pids = ' '.join(str(switch.pid) for switch in switches)
        run('kill -HUP ' + pids)
        for switch in switches:
            switch.shell = None
        return switches

    def renameIface(self, intf, newname):
        "Rename interface"
        self.pexec('ip link set %s down' % intf)
        self.pexec('ip link set %s name %s' % (intf, newname))
        self.pexec('ip link set %s up' % newname)


OVSKernelAP = OVSAP
physicalAP = OVSAP
