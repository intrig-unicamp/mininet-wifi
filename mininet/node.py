"""
Node objects for Mininet.

Nodes provide a simple abstraction for interacting with hosts, switches
and controllers. Local nodes are simply one or more processes on the local
machine.

Node: superclass for all (primarily local) network nodes.

Host: a virtual host. By default, a host is simply a shell; commands
    may be sent using Cmd (which waits for output), or using sendCmd(),
    which returns immediately, allowing subsequent monitoring using
    monitor(). Examples of how to run experiments using this
    functionality are provided in the examples/ directory. By default,
    hosts share the root file system, but they may also specify private
    directories.

CPULimitedHost: a virtual host whose CPU bandwidth is limited by
    RT or CFS bandwidth limiting.

Switch: superclass for switch nodes.

UserSwitch: a switch using the user-space switch from the OpenFlow
    reference implementation.

OVSSwitch: a switch using the Open vSwitch OpenFlow-compatible switch
    implementation (openvswitch.org).

OVSBridge: an Ethernet bridge implemented using Open vSwitch.
    Supports STP.

IVSSwitch: OpenFlow switch using the Indigo Virtual Switch.

Controller: superclass for OpenFlow controllers. The default controller
    is controller(8) from the reference implementation.

OVSController: The test controller from Open vSwitch.

NOXController: a controller node using NOX (noxrepo.org).

Ryu: The Ryu controller (https://osrg.github.io/ryu/)

RemoteController: a remote controller node, which may use any
    arbitrary OpenFlow-compatible controller, and which is not
    created or managed by Mininet.

Future enhancements:

- Possibly make Node, Switch and Controller more abstract so that
  they can be used for both local and remote nodes

- Create proxy objects for remote nodes (Mininet: Cluster Edition)
"""

import os
import pty
import re
import signal
import select
import fileinput

from subprocess import Popen, PIPE
from time import sleep

from mininet.log import info, error, warn, debug
from mininet.util import (quietRun, errRun, errFail, moveIntf, isShellBuiltin,
                           numCores, retry, mountCgroups)
from mininet.moduledeps import moduleDeps, pathCheck, TUN
from mininet.link import Link, Intf, TCIntf, OVSIntf
from re import findall
from distutils.version import StrictVersion
from mininet.wifiMobility import mobility
from mininet.wifiChannel import channelParams, setAdhocChannelParams, setInfraChannelParams
from mininet.wifiDevices import deviceRange
from mininet.wifiPlot import plot2d, plot3d

class Node(object):
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
        self.privateDirs = params.get('privateDirs', [])
        self.inNamespace = params.get('inNamespace', inNamespace)

        # Stash configuration parameters for future reference
        self.params = params

        self.intfs = {}  # dict of port numbers to interfaces
        self.ports = {}  # dict of interfaces to port numbers
        self.wlanports = -1  # dict of wlan interfaces to port numbers
        self.nameToIntf = {}  # dict of interface names to Intfs

        self.func = []
        self.type = 'host'        

        # Make pylint happy
        (self.shell, self.execed, self.pid, self.stdin, self.stdout,
            self.lastPid, self.lastCmd, self.pollOut) = (
                None, None, None, None, None, None, None, None)
        self.waiting = False
        self.readbuf = ''

        # Start command interpreter shell
        self.startShell()
        self.mountPrivateDirs()

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
        self.stdin = os.fdopen(master, 'rw')
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
        
    @classmethod
    def addParameters(self, node, wifiRadios, autoSetMacs, params, defaults):
        """adding parameters in wifinodes"""
        
        node.params['rssi'] = []
        node.params['snr'] = []
        node.params['frequency'] = []
        node.params['wlan'] = []
        node.params['mac'] = []
        
        passwd = ("%s" % params.pop('passwd', {}))
        if(passwd != "{}"):
            passwd = passwd.split(',')
            node.params['passwd'] = []
                        
        encrypt = ("%s" % params.pop('encrypt', {}))
        if(encrypt != "{}"):
            encrypt = encrypt.split(',')
            node.params['encrypt'] = []
            
               
        if node.type == 'station' or node.type == 'vehicle':
            node.params['apsInRange'] = []
            node.params['associatedTo'] = []
            node.ifaceToAssociate = 0 
            node.max_x = 0
            node.max_y = 0
            node.min_x = 0
            node.min_y = 0
            node.max_v = 0
            node.min_v = 0
            node.constantVelocity = 1.0
            node.constantDistance = 1.0
           
        # config
        if 'config' in params:
            node.speed = params['config']   
        
        # speed
        if 'speed' in params:
            node.speed = int(params['speed'])

        # max_x
        if 'max_x' in params:
            node.max_x = int(params['max_x'])

        # max_y
        if 'max_y' in params:
            node.max_y = int(params['max_y'])

        # min_x
        if 'min_x' in params:
            node.min_x = int(params['min_x'])

        # min_y
        if 'min_y' in params:
            node.min_y = int(params['min_y'])

        # min_v
        if 'min_v' in params:
            node.min_v = int(params['min_v'])

        # max_v
        if 'max_v' in params:
            node.max_v = int(params['max_v'])

        # constantVelocity
        if 'constantVelocity' in params:
            node.constantVelocity = int(params['constantVelocity'])

        # constantDistance
        if 'constantDistance' in params:
            node.constantDistance = int(params['constantDistance'])
            
        # position
        position = ("%s" % params.pop('position', {}))
        if(position != "{}"):
            position = position.split(',')
            node.params['position'] = position

        # Wifi Interfaces
        wlans = ("%s" % params.pop('wlans', {}))
        if(wlans != "{}"):
            wlans = int(wlans)
            wifiRadios += int(wlans)
        else:
            wlans = 1
            wifiRadios += 1
        for n in range(wlans):
            node.params['frequency'].append(0)
            if node.type != 'accessPoint':
                node.func.append('none')
                node.params['associatedTo'].append('')
                if(passwd != "{}"):
                    if len(passwd) == 1:
                        node.params['passwd'].append(passwd[0])
                    else:
                        node.params['passwd'].append(passwd[n])
                if(encrypt != "{}"):
                    if len(encrypt) == 1:
                        node.params['encrypt'].append(encrypt[0])
                    else:
                        node.params['encrypt'].append(encrypt[n])
            if node.type == 'accessPoint':
                node.params['wlan'].append(node.name + '-wlan' + str(n + 1))
            else:
                node.params['wlan'].append(node.name + '-wlan' + str(n))
                node.params['rssi'].append(0)
                node.params['snr'].append(0)
            node.params.pop("wlans", None)
            
                        
        if node.type == 'station' or node.type == 'vehicle':
            mac = ("%s" % params.pop('mac', {}))
            if(mac != "{}"):
                mac = mac.split(',')
                node.params['mac'] = []
                for n in range(len(node.params['wlan'])):
                    node.params['mac'].append('')
                    if len(mac) > n:
                        node.params['mac'][n] = mac[n]
            elif autoSetMacs:
                for n in range(wlans):
                    node.params['mac'].append('')
                    node.params['mac'][n] = defaults[ 'mac' ]
            else:
                node.params['mac'] = []
                for n in range(wlans):
                    node.params['mac'].append('')
    
            ip = ("%s" % params.pop('ip', {}))
            if(ip != "{}"):
                ip = ip.split(',')
                node.params['ip'] = []
                for n in range(len(node.params['wlan'])):
                    node.params['ip'].append('0/0')
                    if len(ip) > n:
                        node.params['ip'][n] = ip[n]
            elif autoSetMacs:
                for n in range(wlans):
                    node.params['ip'].append('0/0')
                    node.params['ip'][n] = defaults[ 'ip' ]
            else:
                try:
                    for n in range(wlans):
                        node.params['ip'].append('0/0')
                except:
                    node.params['ip'] = []
                    node.params['ip'].append(defaults[ 'ip' ])
                    for n in range(1, wlans):
                        node.params['ip'].append('0/0')
            
            # max_speed
            if 'max_speed' in params:
                node.max_speed = int(params['max_speed'])
            else:
                node.max_speed = 10
                
            # min_speed
            if 'min_speed' in params:
                node.min_speed = int(params['min_speed'])
            else:
                node.min_speed = 1            

        # mode
        if 'mode' in params:
            node.params['mode'] = []
            for n in range(wlans):
                node.params['mode'].append(params['mode'])
        else:
            node.params['mode'] = []
            for n in range(wlans):
                node.params['mode'].append(defaults['mode'])
                
        # antennaHeight
        if 'antennaHeight' in params:
            node.params['antennaHeight'] = []
            for n in range(wlans):
                node.params['antennaHeight'].append(int(params['antennaHeight']))
        else:
            node.params['antennaHeight'] = []
            for n in range(wlans):
                node.params['antennaHeight'].append(1)
        
        # antennaGain
        if 'antennaGain' in params:
            node.params['antennaGain'] = []
            for n in range(wlans):
                node.params['antennaGain'].append(int(params['antennaGain']))
        else:
            node.params['antennaGain'] = []
            for n in range(wlans):
                node.params['antennaGain'].append(5)
  
        # txpower
        if 'txpower' in params:
            node.params['txpower'] = []
            for n in range(wlans):
                node.params['txpower'].append(int(params['txpower']))
        else:
            node.params['txpower'] = []
            for n in range(wlans):
                node.params['txpower'].append(14)
                
        # Channel
        if 'channel' in params:
            node.params['channel'] = []
            for n in range(wlans):
                node.params['channel'].append(int(params['channel']))
        else:
            node.params['channel'] = []
            for n in range(wlans):
                node.params['channel'].append(1)
        
        equipmentModel = ("%s" % params.pop('equipmentModel', {}))
        if(equipmentModel != "{}"):
            node.equipmentModel = equipmentModel
        
        # Range
        if 'range' in params:
            node.params['range'] = int(params['range'])
        else:
            if node.type == 'accessPoint':
                value = deviceRange(node)
                node.params['range'] = value.range
            else:
                value = deviceRange(node)
                node.params['range'] = value.range - 15

        if node.type == 'accessPoint':
            node.params['associatedStations'] = []
            
            ssid = ("%s" % params.pop('ssid', {}))
            ssid = ssid.split(',')
            node.params['ssid'] = []
            if(ssid[0] != "{}"):
                if len(ssid) == 1:
                    node.params['ssid'].append(ssid[0])
                    if(encrypt != "{}"):
                        node.params['encrypt'].append(encrypt[0])
                    if(passwd != "{}"):
                        node.params['passwd'].append(passwd[0])
                else:
                    for n in range(len(ssid)):
                        node.params['ssid'].append(ssid[n])
                        if(passwd != "{}"):
                            if len(passwd) == 1:
                                node.params['passwd'].append(passwd[0])
                            else:
                                node.params['passwd'].append(passwd[n])
                        if(encrypt != "{}"):
                            if len(encrypt) == 1:
                                node.params['encrypt'].append(encrypt[0])
                            else:
                                node.params['encrypt'].append(encrypt[n])
            else:
                node.params['ssid'].append(defaults[ 'ssid' ])
                
        return wifiRadios

    @classmethod
    def calculateWiFiParameters(self, sta):
        for wlan in range(len(sta.params['wlan'])):
            if sta.func[wlan] == 'mesh' or sta.func[wlan] == 'adhoc':
                associate = False
                for station in mobility.staList:
                    dist = channelParams.getDistance(sta, station)
                    if dist < int(sta.params['range']) + int(station.params['range']):
                        associate = True
                setAdhocChannelParams(sta, wlan, 0, mobility.staList)
                if associate == False:
                    sta.cmd('iw dev %s-mp%s mesh leave' % (sta, wlan))
            else:
                for ap in mobility.apList:
                    dist = channelParams.getDistance(sta, ap)
                    mobility.handover(sta, ap, wlan, dist)
        mobility.getAPsInRange(sta)

    @classmethod
    def verifyingNodes(self, node):
        if node in mobility.staList:
            self.calculateWiFiParameters(node)
        elif node in mobility.apList:
            for sta in mobility.staList:
                self.calculateWiFiParameters(sta)

    def meshLeave(self, ssid):
        for key, val in self.params.items():
            if val == ssid:
                self.cmd('iw dev %s-%s mesh leave' % (self, key))

    @classmethod
    def setMac(self, sta, iface, index):
        sta.pexec('ip link set %s down' % (iface))
        sta.pexec('ip link set %s address %s' % (iface, sta.params['mac'][index]))
        sta.pexec('ip link set %s up' % (iface))

    def setRange(self, _range):
        self.params['range'] = _range
        try:
            if mobility.DRAW:
                plot2d.updateCircleRadius(self)
                plot2d.graphUpdate(self)
        except:
            pass
        self.verifyingNodes(self)

    def moveNodeTo(self, pos):
        pos = pos.split(',')
        self.params['position'] = float(pos[0]), float(pos[1]), float(pos[2])
        if mobility.DRAW and mobility.is3d == False:
            plot2d.graphUpdate(self)
            plot2d.graphPause()
        elif mobility.DRAW and mobility.is3d == True:
            plot3d.graphUpdate(self)            
        self.verifyingNodes(self)

    def setAntennaGain(self, iface, value):
        wlan = int(iface[-1:])
        self.params['antennaGain'][wlan] = int(value)
        if mobility.DRAW:
            try:
                plot2d.graphUpdate(self)
            except:
                pass
        self.verifyingNodes(self)

    def setTxPower(self, iface, txpower):
        wlan = int(iface[-1:])
        self.pexec('iwconfig %s txpower %s' % (iface, txpower))
        self.params['txpower'][wlan] = txpower
        self.verifyingNodes(self)

    def associateTo(self, iface, ap):
        self.moveAssociationTo(iface, ap)

    def moveAssociationTo(self, iface, ap):
        sta = self
        for idx, wlan in enumerate(sta.params['wlan']):
            if wlan == iface:
                wlan = idx
                break
        for ap_ref in mobility.apList:
            if ap == ap_ref:
                if ('position' in sta.params and 'position' in ap.params):
                    d = channelParams.getDistance(sta, ap)
                else:
                    d = 100000
                if (d < ap.params['range']) or ('position' not in sta.params and 'position' not in ap.params):
                    if sta.params['associatedTo'][wlan] != ap:
                        if sta.params['associatedTo'][wlan] != '':
                            sta.cmd('iw dev %s disconnect' % iface)
                        if 'encrypt' not in ap.params:
                            sta.cmd('iwconfig %s essid %s ap %s' % (sta.params['wlan'][wlan], ap.params['ssid'][0], ap.params['mac'][0]))
                        else:
                            if ap.params['encrypt'][0] == 'wpa' or ap.params['encrypt'][0] == 'wpa2':
                                sta.associate_wpa(sta, ap, wlan)
                            elif ap.params['encrypt'][0] == 'wep':
                                sta.associate_wep(sta, ap, wlan)
                        setInfraChannelParams(sta, ap, wlan, d, mobility.staList)
                        mobility.updateAssociation(sta, ap, wlan)
                    else:
                        info ('%s is already connected!\n' % ap)
                    mobility.getAPsInRange(sta)
                else:
                    print "%s is out of range!" % (ap)

    def mountPrivateDirs(self):
        "mount private directories"
        for directory in self.privateDirs:
            if isinstance(directory, tuple):
                # mount given private directory
                privateDir = directory[ 1 ] % self.__dict__
                mountPoint = directory[ 0 ]
                self.cmd('mkdir -p %s' % privateDir)
                self.cmd('mkdir -p %s' % mountPoint)
                self.cmd('mount --bind %s %s' % 
                               (privateDir, mountPoint))
            else:
                # mount temporary filesystem on directory
                self.cmd('mkdir -p %s' % directory)
                self.cmd('mount -n -t tmpfs tmpfs %s' % directory)

    def unmountPrivateDirs(self):
        "mount private directories"
        for directory in self.privateDirs:
            if isinstance(directory, tuple):
                self.cmd('umount ', directory[ 0 ])
            else:
                self.cmd('umount ', directory)

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
            self.readbuf += data
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
        os.write(self.stdin.fileno(), data)

    def terminate(self):
        "Send kill signal to Node and clean up after it."
        self.unmountPrivateDirs()
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
            elif isinstance(args[ 0 ], basestring):
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
        popen = self.popen(*args, stdin=PIPE, stdout=PIPE, stderr=PIPE,
                            **kwargs)
        # Warning: this can fail with large numbers of fds!
        out, err = popen.communicate()
        exitcode = popen.wait()
        return out, err, exitcode

        # Warning: this can fail with large numbers of fds!
        # out, err = popen.communicate()
        # return out, err
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
        debug('added intf %s (%s) to node %s\n' % (
                intf, port, self.name))
        if self.inNamespace:
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
            if 'station' != self.type and 'vehicle' != self.type:
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
        elif isinstance(intf, basestring):
            return self.nameToIntf[ intf ]
        else:
            return intf

    def connectionsTo(self, node):
        "Return [ intf1, intf2... ] for all intfs that connect self to node."
        # We could optimize this if it is important
        connections = []
        for intf in self.intfList():
            link = intf.link
            if link and link.intf2 != None:
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
        for intf in self.intfs.values():
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
        if isinstance(intf, basestring) and ' ' in intf:
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
        if intf != None:
            wlan = int(intf[-1:])
            self.params['mac'][wlan] = mac
        return self.intf(intf).setMAC(mac)

    def setIP(self, ip, prefixLen=8, intf=None, **kwargs):
        """Set the IP address for an interface.
           intf: intf or intf name
           ip: IP address as a string
           prefixLen: prefix length, e.g. 8 for /8 or 16M addrs
           kwargs: any additional arguments for intf.setIP"""
        if intf != None and (self.type == 'station' or self.type == 'vehicle'):
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
        name, value = param.items()[ 0 ]
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

    def config(self, mac=None, ip=None,
                defaultRoute=None, lo='up', **_params):
        """Configure Node according to (optional) parameters:
           mac: MAC address for default interface
           ip: IP address for default interface
           ifconfig: arbitrary interface configuration
           Subclasses should override this method and call
           the parent class's config(**params)"""
        # If we were overriding this method, we would call
        # the superclass config method here as follows:
        # r = Parent.config( **_params )
        r = {}
        if 'station' == self.type or 'vehicle' == self.type:
            if len(ip) > 1:
                ip = ip[0]
        if 'station' != self.type and 'vehicle' != self.type:  # or 'isMesh' in self.params:
            self.setParam(r, 'setMAC', mac=mac)
        self.setParam(r, 'setIP', ip=ip)
        self.setParam(r, 'setDefaultRoute', defaultRoute=defaultRoute)

        # This should be examined
        self.cmd('ifconfig lo ' + lo)
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
        return [ self.intfs[ p ] for p in sorted(self.intfs.iterkeys()) ]

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
        pathCheck('mnexec', 'ifconfig', moduleName='Mininet')

class Host(Node):
    "A host is simply a Node"
    pass

class Station(Node):
    "A station is simply a Node"
    pass

class Car(Node):
    "A car is simply a Node"
    pass

class CPULimitedHost(Host):

    "CPU limited host"

    def __init__(self, name, sched='cfs', **kwargs):
        Host.__init__(self, name, **kwargs)
        # Initialize class if necessary
        if not CPULimitedHost.inited:
            CPULimitedHost.init()
        # Create a cgroup and move shell into it
        self.cgroup = 'cpu,cpuacct,cpuset:/' + self.name
        errFail('cgcreate -g ' + self.cgroup)
        # We don't add ourselves to a cpuset because you must
        # specify the cpu and memory placement first
        errFail('cgclassify -g cpu,cpuacct:/%s %s' % (self.name, self.pid))
        # BL: Setting the correct period/quota is tricky, particularly
        # for RT. RT allows very small quotas, but the overhead
        # seems to be high. CFS has a mininimum quota of 1 ms, but
        # still does better with larger period values.
        self.period_us = kwargs.get('period_us', 100000)
        self.sched = sched
        if sched == 'rt':
            self.checkRtGroupSched()
            self.rtprio = 20

    def cgroupSet(self, param, value, resource='cpu'):
        "Set a cgroup parameter and return its value"
        cmd = 'cgset -r %s.%s=%s /%s' % (
            resource, param, value, self.name)
        quietRun(cmd)
        nvalue = int(self.cgroupGet(param, resource))
        if nvalue != value:
            error('*** error: cgroupSet: %s set to %s instead of %s\n'
                   % (param, nvalue, value))
        return nvalue

    def cgroupGet(self, param, resource='cpu'):
        "Return value of cgroup parameter"
        cmd = 'cgget -r %s.%s /%s' % (
            resource, param, self.name)
        return int(quietRun(cmd).split()[ -1 ])

    def cgroupDel(self):
        "Clean up our cgroup"
        # info( '*** deleting cgroup', self.cgroup, '\n' )
        _out, _err, exitcode = errRun('cgdelete -r ' + self.cgroup)
        return exitcode == 0  # success condition

    def popen(self, *args, **kwargs):
        """Return a Popen() object in node's namespace
           args: Popen() args, single list, or string
           kwargs: Popen() keyword args"""
        # Tell mnexec to execute command in our cgroup
        mncmd = [ 'mnexec', '-g', self.name,
                  '-da', str(self.pid) ]
        # if our cgroup is not given any cpu time,
        # we cannot assign the RR Scheduler.
        if self.sched == 'rt':
            if int(self.cgroupGet('rt_runtime_us', 'cpu')) <= 0:
                mncmd += [ '-r', str(self.rtprio) ]
            else:
                debug('*** error: not enough cpu time available for %s.' % 
                       self.name, 'Using cfs scheduler for subprocess\n')
        return Host.popen(self, *args, mncmd=mncmd, **kwargs)

    def cleanup(self):
        "Clean up Node, then clean up our cgroup"
        super(CPULimitedHost, self).cleanup()
        retry(retries=3, delaySecs=1, fn=self.cgroupDel)

    _rtGroupSched = False  # internal class var: Is CONFIG_RT_GROUP_SCHED set?

    @classmethod
    def checkRtGroupSched(cls):
        "Check (Ubuntu,Debian) kernel config for CONFIG_RT_GROUP_SCHED for RT"
        if not cls._rtGroupSched:
            release = quietRun('uname -r').strip('\r\n')
            output = quietRun('grep CONFIG_RT_GROUP_SCHED /boot/config-%s' % 
                               release)
            if output == '# CONFIG_RT_GROUP_SCHED is not set\n':
                error('\n*** error: please enable RT_GROUP_SCHED '
                       'in your kernel\n')
                exit(1)
            cls._rtGroupSched = True

    def chrt(self):
        "Set RT scheduling priority"
        quietRun('chrt -p %s %s' % (self.rtprio, self.pid))
        result = quietRun('chrt -p %s' % self.pid)
        firstline = result.split('\n')[ 0 ]
        lastword = firstline.split(' ')[ -1 ]
        if lastword != 'SCHED_RR':
            error('*** error: could not assign SCHED_RR to %s\n' % self.name)
        return lastword

    def rtInfo(self, f):
        "Internal method: return parameters for RT bandwidth"
        pstr, qstr = 'rt_period_us', 'rt_runtime_us'
        # RT uses wall clock time for period and quota
        quota = int(self.period_us * f)
        return pstr, qstr, self.period_us, quota

    def cfsInfo(self, f):
        "Internal method: return parameters for CFS bandwidth"
        pstr, qstr = 'cfs_period_us', 'cfs_quota_us'
        # CFS uses wall clock time for period and CPU time for quota.
        quota = int(self.period_us * f * numCores())
        period = self.period_us
        if f > 0 and quota < 1000:
            debug('(cfsInfo: increasing default period) ')
            quota = 1000
            period = int(quota / f / numCores())
        # Reset to unlimited on negative quota
        if quota < 0:
            quota = -1
        return pstr, qstr, period, quota

    # BL comment:
    # This may not be the right API,
    # since it doesn't specify CPU bandwidth in "absolute"
    # units the way link bandwidth is specified.
    # We should use MIPS or SPECINT or something instead.
    # Alternatively, we should change from system fraction
    # to CPU seconds per second, essentially assuming that
    # all CPUs are the same.

    def setCPUFrac(self, f, sched=None):
        """Set overall CPU fraction for this host
           f: CPU bandwidth limit (positive fraction, or -1 for cfs unlimited)
           sched: 'rt' or 'cfs'
           Note 'cfs' requires CONFIG_CFS_BANDWIDTH,
           and 'rt' requires CONFIG_RT_GROUP_SCHED"""
        if not sched:
            sched = self.sched
        if sched == 'rt':
            if not f or f < 0:
                raise Exception('Please set a positive CPU fraction'
                                 ' for sched=rt\n')
            pstr, qstr, period, quota = self.rtInfo(f)
        elif sched == 'cfs':
            pstr, qstr, period, quota = self.cfsInfo(f)
        else:
            return
        # Set cgroup's period and quota
        setPeriod = self.cgroupSet(pstr, period)
        setQuota = self.cgroupSet(qstr, quota)
        if sched == 'rt':
            # Set RT priority if necessary
            sched = self.chrt()
        info('(%s %d/%dus) ' % (sched, setQuota, setPeriod))

    def setCPUs(self, cores, mems=0):
        "Specify (real) cores that our cgroup can run on"
        if not cores:
            return
        if isinstance(cores, list):
            cores = ','.join([ str(c) for c in cores ])
        self.cgroupSet(resource='cpuset', param='cpus',
                        value=cores)
        # Memory placement is probably not relevant, but we
        # must specify it anyway
        self.cgroupSet(resource='cpuset', param='mems',
                        value=mems)
        # We have to do this here after we've specified
        # cpus and mems
        errFail('cgclassify -g cpuset:/%s %s' % (
                 self.name, self.pid))

    def config(self, cpu=-1, cores=None, **params):
        """cpu: desired overall system CPU fraction
           cores: (real) core(s) this host can run on
           params: parameters for Node.config()"""
        r = Node.config(self, **params)
        # Was considering cpu={'cpu': cpu , 'sched': sched}, but
        # that seems redundant
        self.setParam(r, 'setCPUFrac', cpu=cpu)
        self.setParam(r, 'setCPUs', cores=cores)
        return r

    inited = False

    @classmethod
    def init(cls):
        "Initialization for CPULimitedHost class"
        mountCgroups()
        cls.inited = True

# Some important things to note:
#
# The "IP" address which setIP() assigns to the switch is not
# an "IP address for the switch" in the sense of IP routing.
# Rather, it is the IP address for the control interface,
# on the control network, and it is only relevant to the
# controller. If you are running in the root namespace
# (which is the only way to run OVS at the moment), the
# control interface is the loopback interface, and you
# normally never want to change its IP address!
#
# In general, you NEVER want to attempt to use Linux's
# network stack (i.e. ifconfig) to "assign" an IP address or
# MAC address to a switch data port. Instead, you "assign"
# the IP and MAC addresses in the controller by specifying
# packets that you want to receive or send. The "MAC" address
# reported by ifconfig for a switch data port is essentially
# meaningless. It is important to understand this if you
# want to create a functional router using OpenFlow.

class Switch(Node):
    """A Switch is a Node that is running (or has execed?)
       an OpenFlow switch."""
    portBase = 1  # Switches start with port 1 in OpenFlow
    dpidLen = 16  # digits in dpid passed to switch

    def __init__(self, name, dpid=None, opts='', listenPort=None, **params):
        """dpid: dpid hex string (or None to derive from name, e.g. s1 -> 1)
           opts: additional switch options
           listenPort: port to listen on for dpctl connections"""
        Node.__init__(self, name, **params)
        self.dpid = self.defaultDpid(dpid)
        self.opts = opts
        self.listenPort = listenPort
        if not self.inNamespace:
            self.controlIntf = Intf('lo', self, port=0)


    def defaultDpid(self, dpid=None):
        "Return correctly formatted dpid from dpid or switch name (s1 -> 1)"
        if dpid:
            # Remove any colons and make sure it's a good hex number
            dpid = dpid.translate(None, ':')
            assert len(dpid) <= self.dpidLen and int(dpid, 16) >= 0
        else:
            # Use hex of the first number in the switch name
            nums = re.findall(r'\d+', self.name)
            if nums:
                dpid = hex(int(nums[ 0 ]))[ 2: ]
            else:
                raise Exception('Unable to derive default datapath ID - '
                                 'please either specify a dpid or use a '
                                 'canonical switch name such as s23.')
        return '0' * (self.dpidLen - len(dpid)) + dpid

    def defaultIntf(self):
        "Return control interface"
        if self.controlIntf:
            return self.controlIntf
        else:
            return Node.defaultIntf(self)


    def sendCmd(self, *cmd, **kwargs):
        """Send command to Node.
           cmd: string"""
        kwargs.setdefault('printPid', False)
        if not self.execed:
            return Node.sendCmd(self, *cmd, **kwargs)
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
        
class AccessPoint(Switch):
    """An AccessPoint is a Switch equipped with wireless interface that is running (or has execed?)
       an OpenFlow switch."""

    @classmethod
    def init_ (self, ap, country_code=None, auth_algs=None, wpa=None, intf=None, wlan=None,
              wpa_key_mgmt=None, rsn_pairwise=None, wpa_passphrase=None,
              wep_key0=None, **params):
        
        ap.params['mac'] = []
        
        for i in range(len(ap.params['ssid'])):
            ap.params['mac'].append('')

        if 'phywlan' not in ap.params:
            self.renameIface(ap, intf, ap.params['wlan'][wlan])
            ap.params['mac'][wlan] = self.getMacAddr(ap, wlan)
            self.checkNetworkManager(ap.params['mac'][wlan])
            if 'inNamespace' in ap.params and 'ip' in ap.params:
                self.setIPAddr(ap, wlan)

        self.start_(ap, country_code, auth_algs, wpa, wlan,
              wpa_key_mgmt, rsn_pairwise, wpa_passphrase,
              wep_key0, **params)
        
    @classmethod
    def renameIface(self, ap, intf, newname):
        "Rename interface"
        ap.pexec('ifconfig %s down' % intf)
        ap.pexec('ip link set %s name %s' % (intf, newname))
        ap.pexec('ifconfig %s up' % newname)

    @classmethod
    def start_(self, ap, country_code=None, auth_algs=None, wpa=None, wlan=None,
              wpa_key_mgmt=None, rsn_pairwise=None, wpa_passphrase=None,
              wep_key0=None, **params):
        """ Starting Access Point """

        cmd = ("echo \'")
        if 'phywlan' not in ap.params:
            cmd = cmd + ("interface=%s" % ap.params['wlan'][wlan])  # the interface used by the AP
        else:
            cmd = cmd + ("interface=%s" % ap.params.get('phywlan'))  # the interface used by the AP
        cmd = cmd + ("\ndriver=nl80211")
        cmd = cmd + ("\nssid=%s" % ap.params['ssid'][0])  # the name of the AP

        if ap.params['mode'][0] == 'n' or ap.params['mode'][0] == 'ac'or ap.params['mode'][0] == 'a':
            cmd = cmd + ("\nhw_mode=g")
        else:
            cmd = cmd + ("\nhw_mode=%s" % ap.params['mode'][0])
        cmd = cmd + ("\nchannel=%s" % ap.params['channel'][0])  # the channel to use
        if 'phywlan' not in ap.params:
            cmd = cmd + ("\nwme_enabled=1")
            cmd = cmd + ("\nwmm_enabled=1")
        if 'encrypt' in ap.params:
            if ap.params['encrypt'][0] == 'wpa':
                cmd = cmd + ("\nauth_algs=%s" % auth_algs)
                cmd = cmd + ("\nwpa=%s" % wpa)
                cmd = cmd + ("\nwpa_key_mgmt=%s" % wpa_key_mgmt)
                cmd = cmd + ("\nwpa_passphrase=%s" % wpa_passphrase)
            elif ap.params['encrypt'][0] == 'wpa2':
                cmd = cmd + ("\nauth_algs=%s" % auth_algs)
                cmd = cmd + ("\nwpa=%s" % wpa)
                cmd = cmd + ("\nwpa_key_mgmt=%s" % wpa_key_mgmt)
                # cmd = cmd + ("\nrsn_pairwise=%s" % rsn_pairwise)
                cmd = cmd + ("\nwpa_passphrase=%s" % wpa_passphrase)
            elif ap.params['encrypt'][0] == 'wep':
                cmd = cmd + ("\nauth_algs=%s" % auth_algs)
                cmd = cmd + ("\nwep_default_key=%s" % 0)
                cmd = cmd + self.verifyWepKey(wep_key0)
            
        cmd = cmd + '\nctrl_interface=/var/run/hostapd'
        cmd = cmd + '\nctrl_interface_group=0'

        if 'config' in ap.params.keys():
            config = ap.params['config']
            if(config != []):
                config = ap.params['config'].split(',')
                ap.params.pop("config", None)
                for conf in config:
                    cmd = cmd + "\n" + conf

        if(len(ap.params['ssid'])) > 1:
            for i in range(1, len(ap.params['ssid'])):
                ap.params['wlan'].append('%s-%s' % (ap.params['wlan'][0], i))
                ssid = ap.params['ssid'][i]
                cmd = cmd + ('\n')
                cmd = cmd + ("\nbss=%s" % ap.params['wlan'][i])
                cmd = cmd + ("\nssid=%s" % ssid)
                if 'encrypt' in ap.params:
                    if (ap.params['encrypt'][i] == 'wep'):
                        cmd = cmd + ("\nauth_algs=%s" % auth_algs)
                        cmd = cmd + ("\nwep_default_key=0")
                        cmd = cmd + self.verifyWepKey(wep_key0)
                ap.params['mac'][i] = ap.params['mac'][wlan][:-1] + str(i)
                # self.checkNetworkManager(ap.params['mac'][i])
        self.APConfigFile(cmd, ap, wlan)
    
    @classmethod    
    def verifyWepKey(self, wep_key0):
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
    def setIPAddr(self, ap, wlan):
        ap.cmd('ifconfig %s %s' % (ap.params['wlan'][wlan], ap.params['ip']))
    
    @classmethod
    def getMacAddr(self, ap, wlan):
        """ get Mac Address of any Interface """
        ifconfig = str(ap.pexec('ifconfig %s' % ap.params['wlan'][wlan]))
        mac = self._macMatchRegex.findall(ifconfig)
        mac = mac[0]
        return mac

    @classmethod
    def checkNetworkManager(self, mac):
        """ add mac address inside of /etc/NetworkManager/NetworkManager.conf """
        self.printMac = False
        unmatch = ""
        if(os.path.exists('/etc/NetworkManager/NetworkManager.conf')):
            if(os.path.isfile('/etc/NetworkManager/NetworkManager.conf')):
                self.resultIface = open('/etc/NetworkManager/NetworkManager.conf')
                lines = self.resultIface

            isNew = True
            for n in lines:
                if("unmanaged-devices" in n):
                    unmatch = n
                    echo = n
                    echo.replace(" ", "")
                    echo = echo[:-1] + ";"
                    isNew = False
            if(isNew):
                os.system("echo '#' >> /etc/NetworkManager/NetworkManager.conf")
                unmatch = "#"
                echo = "[keyfile]\nunmanaged-devices="

            if mac not in unmatch:
                echo = echo + "mac:" + mac + ';'
                self.printMac = True

            if(self.printMac):
                for line in fileinput.input('/etc/NetworkManager/NetworkManager.conf', inplace=1):
                    if(isNew):
                        if line.__contains__('#'):
                            print line.replace(unmatch, echo)
                        else:
                            print line.rstrip()
                    else:
                        if line.__contains__('unmanaged-devices'):
                            print line.replace(unmatch, echo)
                        else:
                            print line.rstrip()
                # os.system('service network-manager restart')
            
    @classmethod
    def customDataRate(self, node, wlan):
        """Custom Maximum Data Rate - Useful when there is mobility"""
        mode = node.params['mode'][wlan]
        
        if (mode == 'a'):
            self.rate = 54
        elif(mode == 'b'):
            self.rate = 11
        elif(mode == 'g'):
            self.rate = 54
        elif(mode == 'n'):
            self.rate = 600
        elif(mode == 'ac'):
            self.rate = 6777
        return self.rate

    @classmethod
    def setBw(self, ap, wlan, iface):
        """ Set bw to AP """
        value = self.customDataRate(ap, wlan)
        bw = value

        ap.cmd("tc qdisc replace dev %s \
            root handle 2: tbf rate %sMbit burst 15000 latency 1ms" % (iface, bw))
        # Reordering packets
        ap.cmd('tc qdisc add dev %s parent 2:1 handle 10: pfifo limit 1000' % (iface))

    @classmethod
    def APConfigFile(self, cmd, ap, wlan):
        """ run an Access Point and create the config file """
        if 'phywlan' not in ap.params:
            iface = ap.params['wlan'][wlan]
        else:
            iface = ap.params.get('phywlan')
            ap.cmd('ifconfig %s down' % iface)
            ap.cmd('ifconfig %s up' % iface)
        content = cmd + ("\' > %s.apconf" % iface)
        ap.cmd(content)
        cmd = ("hostapd -B %s.apconf" % iface)
        try:
            ap.cmd(cmd)
        except:
            print ('error with hostapd. Please, run sudo mn -c in order to fix it or check if hostapd is\
                                             working properly in your machine.')
            exit(1)
        self.setBw(ap, wlan, iface)
        
class UserSwitch(Switch):
    "User-space switch."

    dpidLen = 12

    def __init__(self, name, dpopts='--no-slicing', **kwargs):
        """Init.
           name: name for the switch
           dpopts: additional arguments to ofdatapath (--no-slicing)"""
        Switch.__init__(self, name, **kwargs)
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
        return self.cmd('dpctl ' + ' '.join(args) + 
                         ' ' + listenAddr)

    def connected(self):
        "Is the switch connected to a controller?"
        status = self.dpctl('status')
        return ('remote.is-connected=true' in status and
                 'local.is-connected=true' in status)

    @staticmethod
    def TCReapply(intf):
        """Unfortunately user switch and Mininet are fighting
           over tc queuing disciplines. To resolve the conflict,
           we re-create the user switch's configuration, but as a
           leaf of the TCIntf-created configuration."""
        if isinstance(intf, TCIntf):
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

        if 'ssid' in self.params:
            if len(self.params['ssid']) > 1:
                iface = intfs[0]
                for n in range(1, len(self.params['ssid'])):
                    intfs.append(iface + str('-%s' % n))

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
        if self.type != 'accessPoint':
            self.cmd('kill %ofdatapath')
            self.cmd('kill %ofprotocol')
            super(UserSwitch, self).stop(deleteIntfs)
        # if self.type == 'accessPoint':
        #    self.cmd('ovs-vsctl del-br', self.name)

class OVSSwitch(Switch):
    "Open vSwitch switch. Depends on ovs-vsctl."

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
        Switch.__init__(self, name, **params)
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
        return (StrictVersion(cls.OVSVersion) < 
                 StrictVersion('1.10'))

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
        if isinstance(intf, TCIntf):
            intf.config(**intf.params)

    def attach(self, intf):
        "Connect a data port"
        self.vsctl('add-port', self, intf)
        self.cmd('ifconfig', intf, 'up')
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
                'OVS kernel switch does not work in a namespace')
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
            for intf in switch.intfs.itervalues():
                if isinstance(intf, TCIntf):
                    intf.config(**intf.params)
        return switches

    def stop(self, deleteIntfs=True):
        """Terminate OVS switch.
           deleteIntfs: delete interfaces? (True)"""
        self.cmd('ovs-vsctl del-br', self)
        if self.datapath == 'user':
            self.cmd('ip link del', self)
        super(OVSSwitch, self).stop(deleteIntfs)

    @classmethod
    def batchShutdown(cls, switches, run=errRun):
        "Shut down a list of OVS switches"
        delcmd = 'del-br %s'
        if switches and not switches[ 0 ].isOldOVS():
            delcmd = '--if-exists ' + delcmd
        # First, delete them all from ovsdb
        run('ovs-vsctl ' + 
             ' -- '.join(delcmd % s for s in switches))
        # Next, shut down all of the processes
        pids = ' '.join(str(switch.pid) for switch in switches)
        run('kill -HUP ' + pids)
        for switch in switches:
            switch.shell = None
        return switches

OVSKernelSwitch = OVSSwitch

class OVSBridge(OVSSwitch):
    "OVSBridge is an OVSSwitch in standalone/bridge mode"

    def __init__(self, *args, **kwargs):
        """stp: enable Spanning Tree Protocol (False)
           see OVSSwitch for other options"""
        kwargs.update(failMode='standalone')
        OVSSwitch.__init__(self, *args, **kwargs)

    def start(self, controllers):
        "Start bridge, ignoring controllers argument"
        OVSSwitch.start(self, controllers=[])

    def connected(self):
        "Are we forwarding yet?"
        if self.stp:
            status = self.dpctl('show')
            return 'STP_FORWARD' in status and not 'STP_LEARN' in status
        else:
            return True

class IVSSwitch(Switch):
    "Indigo Virtual Switch"

    def __init__(self, name, verbose=False, **kwargs):
        Switch.__init__(self, name, **kwargs)
        self.verbose = verbose

    @classmethod
    def setup(cls):
        "Make sure IVS is installed"
        pathCheck('ivs-ctl', 'ivs',
                   moduleName="Indigo Virtual Switch (projectfloodlight.org)")
        out, err, exitcode = errRun('ivs-ctl show')
        if exitcode:
            error(out + err + 
                   'ivs-ctl exited with code %d\n' % exitcode + 
                   '*** The openvswitch kernel module might '
                   'not be loaded. Try modprobe openvswitch.\n')
            exit(1)

    @classmethod
    def batchShutdown(cls, switches):
        "Kill each IVS switch, to be waited on later in stop()"
        for switch in switches:
            switch.cmd('kill %ivs')
        return switches

    def start(self, controllers):
        "Start up a new IVS switch"
        args = ['ivs']
        args.extend(['--name', self.name])
        args.extend(['--dpid', self.dpid])
        if self.verbose:
            args.extend(['--verbose'])
        for intf in self.intfs.values():
            if not intf.IP():
                args.extend(['-i', intf.name])
        for c in controllers:
            args.extend(['-c', '%s:%d' % (c.IP(), c.port)])
        if self.listenPort:
            args.extend(['--listen', '127.0.0.1:%i' % self.listenPort])
        args.append(self.opts)

        logfile = '/tmp/ivs.%s.log' % self.name

        self.cmd(' '.join(args) + ' >' + logfile + ' 2>&1 </dev/null &')

    def stop(self, deleteIntfs=True):
        """Terminate IVS switch.
           deleteIntfs: delete interfaces? (True)"""
        self.cmd('kill %ivs')
        self.cmd('wait')
        super(IVSSwitch, self).stop(deleteIntfs)

    def attach(self, intf):
        "Connect a data port"
        self.cmd('ivs-ctl', 'add-port', '--datapath', self.name, intf)

    def detach(self, intf):
        "Disconnect a data port"
        self.cmd('ivs-ctl', 'del-port', '--datapath', self.name, intf)

    def dpctl(self, *args):
        "Run dpctl command"
        if not self.listenPort:
            return "can't run dpctl without passive listening port"
        return self.cmd('ovs-ofctl ' + ' '.join(args) + 
                         ' tcp:127.0.0.1:%i' % self.listenPort)

class Controller(Node):
    """A Controller is a Node that is running (or has execed?) an
       OpenFlow controller."""

    def __init__(self, name, inNamespace=False, command='controller',
                  cargs='-v ptcp:%d', cdir=None, ip="127.0.0.1",
                  port=6633, protocol='tcp', **params):
        self.command = command
        self.cargs = cargs
        self.cdir = cdir
        # Accept 'ip:port' syntax as shorthand
        if ':' in ip:
            ip, port = ip.split(':')
            port = int(port)
        self.ip = ip
        self.port = port
        self.protocol = protocol
        Node.__init__(self, name, inNamespace=inNamespace,
                       ip=ip, **params)
        self.checkListening()

    def checkListening(self):
        "Make sure no controllers are running on our port"
        # Verify that Telnet is installed first:
        out, _err, returnCode = errRun("which telnet")
        if 'telnet' not in out or returnCode != 0:
            raise Exception("Error running telnet to check for listening "
                             "controllers; please check that it is "
                             "installed.")
        listening = self.cmd("echo A | telnet -e A %s %d" % 
                              (self.ip, self.port))
        if 'Connected' in listening:
            servers = self.cmd('netstat -natp').split('\n')
            pstr = ':%d ' % self.port
            clist = servers[ 0:1 ] + [ s for s in servers if pstr in s ]
            raise Exception("Please shut down the controller which is"
                             " running on port %d:\n" % self.port + 
                             '\n'.join(clist))

    def start(self):
        """Start <controller> <args> on controller.
           Log to /tmp/cN.log"""
        pathCheck(self.command)
        cout = '/tmp/' + self.name + '.log'
        if self.cdir is not None:
            self.cmd('cd ' + self.cdir)
        self.cmd(self.command + ' ' + self.cargs % self.port + 
                  ' 1>' + cout + ' 2>' + cout + ' &')
        self.execed = False

    def stop(self, *args, **kwargs):
        "Stop controller."
        self.cmd('kill %' + self.command)
        self.cmd('wait %' + self.command)
        super(Controller, self).stop(*args, **kwargs)

    def IP(self, intf=None):
        "Return IP address of the Controller"
        if self.intfs:
            ip = Node.IP(self, intf)
        else:
            ip = self.ip
        return ip

    def __repr__(self):
        "More informative string representation"
        return '<%s %s: %s:%s pid=%s> ' % (
            self.__class__.__name__, self.name,
            self.IP(), self.port, self.pid)

    @classmethod
    def isAvailable(cls):
        "Is controller available?"
        return quietRun('which controller')


class OVSController(Controller):
    "Open vSwitch controller"
    def __init__(self, name, **kwargs):
        kwargs.setdefault('command', self.isAvailable() or
            'ovs-controller')
        Controller.__init__(self, name, **kwargs)

    @classmethod
    def isAvailable(cls):
        return (quietRun('which ovs-controller') or
                 quietRun('which test-controller') or
                 quietRun('which ovs-testcontroller')).strip()

class NOX(Controller):
    "Controller to run a NOX application."

    def __init__(self, name, *noxArgs, **kwargs):
        """Init.
           name: name to give controller
           noxArgs: arguments (strings) to pass to NOX"""
        if not noxArgs:
            warn('warning: no NOX modules specified; '
                  'running packetdump only\n')
            noxArgs = [ 'packetdump' ]
        elif type(noxArgs) not in (list, tuple):
            noxArgs = [ noxArgs ]

        if 'NOX_CORE_DIR' not in os.environ:
            exit('exiting; please set missing NOX_CORE_DIR env var')
        noxCoreDir = os.environ[ 'NOX_CORE_DIR' ]

        Controller.__init__(self, name,
                             command=noxCoreDir + '/nox_core',
                             cargs='--libdir=/usr/local/lib -v -i ptcp:%s ' + 
                             ' '.join(noxArgs),
                             cdir=noxCoreDir,
                             **kwargs)

class Ryu(Controller):
    "Controller to run Ryu application"
    def __init__(self, name, *ryuArgs, **kwargs):
        """Init.
        name: name to give controller.
        ryuArgs: arguments and modules to pass to Ryu"""
        homeDir = quietRun('printenv HOME').strip('\r\n')
        ryuCoreDir = '%s/ryu/ryu/app/' % homeDir
        if not ryuArgs:
            warn('warning: no Ryu modules specified; '
                  'running simple_switch only\n')
            ryuArgs = [ ryuCoreDir + 'simple_switch.py' ]
        elif type(ryuArgs) not in (list, tuple):
            ryuArgs = [ ryuArgs ]

        Controller.__init__(self, name,
                             command='ryu-manager',
                             cargs='--ofp-tcp-listen-port %s ' + 
                             ' '.join(ryuArgs),
                             cdir=ryuCoreDir,
                             **kwargs)

class RemoteController(Controller):
    "Controller running outside of Mininet's control."

    def __init__(self, name, ip='127.0.0.1',
                  port=None, **kwargs):
        """Init.
           name: name to give controller
           ip: the IP address where the remote controller is
           listening
           port: the port where the remote controller is listening"""
        Controller.__init__(self, name, ip=ip, port=port, **kwargs)

    def start(self):
        "Overridden to do nothing."
        return

    def stop(self):
        "Overridden to do nothing."
        return

    def checkListening(self):
        "Warn if remote controller is not accessible"
        if self.port is not None:
            self.isListening(self.ip, self.port)
        else:
            for port in 6653, 6633:
                if self.isListening(self.ip, port):
                    self.port = port
                    info("Connecting to remote controller"
                          " at %s:%d\n" % (self.ip, self.port))
                    break

        if self.port is None:
            self.port = 6653
            warn("Setting remote controller"
                  " to %s:%d\n" % (self.ip, self.port))

    def isListening(self, ip, port):
        "Check if a remote controller is listening at a specific ip and port"
        listening = self.cmd("echo A | telnet -e A %s %d" % (ip, port))
        if 'Connected' not in listening:
            warn("Unable to contact the remote controller"
                  " at %s:%d\n" % (ip, port))
            return False
        else:
            return True

DefaultControllers = (Controller, OVSController)

def findController(controllers=DefaultControllers):
    "Return first available controller from list, if any"
    for controller in controllers:
        if controller.isAvailable():
            return controller

def DefaultController(name, controllers=DefaultControllers, **kwargs):
    "Find a controller that is available and instantiate it"
    controller = findController(controllers)
    if not controller:
        raise Exception('Could not find a default OpenFlow controller')
    return controller(name, **kwargs)

def NullController(*_args, **_kwargs):
    "Nonexistent controller - simply returns None"
    return None
