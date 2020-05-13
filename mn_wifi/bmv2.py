"""
Copyright 2019-present Open Networking Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

"""
Inspired from https://github.com/p4lang/tutorials and https://github.com/opennetworkinglab/onos
"""

import json
import multiprocessing
import os
import random
import re
import socket
import threading
import urllib2
import tempfile
import psutil
from contextlib import closing

import time
from mininet.log import info, warn, debug, error
from mininet.node import Switch, Host
from mininet.moduledeps import pathCheck
from mn_wifi.node import AP, Station


SIMPLE_SWITCH_GRPC = 'simple_switch_grpc'
SIMPLE_SWITCH_CLI = 'simple_switch_CLI'
PKT_BYTES_TO_DUMP = 80
VALGRIND_PREFIX = 'valgrind --leak-check=yes'
SWITCH_START_TIMEOUT = 5  # seconds
BMV2_LOG_LINES = 5
BMV2_DEFAULT_DEVICE_ID = 1
# TODO: move these values to env variables
ONOS_WEB_USER = 'onos'
ONOS_WEB_PASS = 'rocks'


def parseBoolean(value):
    if value in ['1', 1, 'true', 'True']:
        return True
    else:
        return False


def pickUnusedPort():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(('localhost', 0))
    addr, port = s.getsockname()
    s.close()
    return port


def writeToFile(path, value):
    with open(path, "w") as f:
        f.write(str(value))


def watchDog(sw):
    while True:
        if ONOSBmv2Switch.mininet_exception == 1:
            sw.killBmv2(log=False)
            return
        if sw.stopped:
            return
        with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as s:
            if s.connect_ex(('127.0.0.1', sw.grpcPort)) == 0:
                time.sleep(1)
            else:
                warn("\n*** WARN: BMv2 instance %s died!\n" % sw.name)
                sw.printBmv2Log()
                print ("-" * 80) + "\n"
                return


class P4Host(Host):
    def __init__(self, name, inNamespace=True, **params):
        Host.__init__(self, name, inNamespace=inNamespace, **params)

    def config(self, **params):
        r = super(Host, self).config(**params)
        for off in ["rx", "tx", "sg"]:
            cmd = "/sbin/ethtool --offload %s %s off" \
                  % (self.defaultIntf(), off)
            self.cmd(cmd)
        # disable IPv6
        self.cmd("sysctl -w net.ipv6.conf.all.disable_ipv6=1")
        self.cmd("sysctl -w net.ipv6.conf.default.disable_ipv6=1")
        self.cmd("sysctl -w net.ipv6.conf.lo.disable_ipv6=1")
        return r


class P4Station(Station):
    def __init__(self, name, inNamespace=True, **params):
        Station.__init__(self, name, inNamespace=inNamespace, **params)

    def config(self, **params):
        r = super(Station, self).config(**params)
        for off in ["rx", "tx", "sg"]:
            cmd = "/sbin/ethtool --offload %s %s off" \
                  % (self.defaultIntf(), off)
            self.cmd(cmd)
        # disable IPv6
        self.cmd("sysctl -w net.ipv6.conf.all.disable_ipv6=1")
        self.cmd("sysctl -w net.ipv6.conf.default.disable_ipv6=1")
        self.cmd("sysctl -w net.ipv6.conf.lo.disable_ipv6=1")
        return r


class ONOSBmv2Switch(Switch):
    """BMv2 software switch with gRPC server"""
    # Shared value used to notify to all instances of this class that a Mininet
    # exception occurred. Mininet exception handling doesn't call the stop()
    # method, so the mn process would hang after clean-up since Bmv2 would still
    # be running.
    mininet_exception = multiprocessing.Value('i', 0)

    def __init__(self, name, json=None, debugger=False, loglevel="info",
                 elogger=False, grpcport=None, cpuport=255, notifications=False,
                 thriftport=None, netcfg=True, dryrun=False, pipeconf="",
                 pktdump=False, valgrind=False, gnmi=False,
                 portcfg=True, onosdevid=None, switch_config=None, **kwargs):
        Switch.__init__(self, name, **kwargs)
        self.wintfs = {}  # dict of wireless port numbers
        self.wports = {}  # dict of interfaces to port numbers
        self.grpcPort = grpcport
        self.thriftPort = thriftport
        self.cpuPort = cpuport
        self.json = json
        self.debugger = parseBoolean(debugger)
        self.notifications = parseBoolean(notifications)
        self.loglevel = loglevel
        # Important: Mininet removes all /tmp/*.log files in case of exceptions.
        # We want to be able to see the bmv2 log if anything goes wrong, hence
        # avoid the .log extension.
        self.logfile = '/tmp/bmv2-%s-log' % self.name
        self.elogger = parseBoolean(elogger)
        self.pktdump = parseBoolean(pktdump)
        self.netcfg = parseBoolean(netcfg)
        self.dryrun = parseBoolean(dryrun)
        self.valgrind = parseBoolean(valgrind)
        self.netcfgfile = '/tmp/bmv2-%s-netcfg.json' % self.name
        self.pipeconfId = pipeconf
        self.injectPorts = parseBoolean(portcfg)
        self.withGnmi = parseBoolean(gnmi)
        self.longitude = kwargs['longitude'] if 'longitude' in kwargs else None
        self.latitude = kwargs['latitude'] if 'latitude' in kwargs else None
        if onosdevid is not None and len(onosdevid) > 0:
            self.onosDeviceId = onosdevid
        else:
            self.onosDeviceId = "device:bmv2:%s" % self.name
        nums = re.findall(r'\d+', name)
        self.p4DeviceId = nums[0]
        self.logfd = None
        self.bmv2popen = None
        self.stopped = False
        self.switch_config = switch_config
        # Remove files from previous executions
        self.cleanupTmpFiles()

    def getSourceIp(self, dstIP):
        """
        Queries the Linux routing table to get the source IP that can talk with
        dstIP, and vice versa.
        """
        ipRouteOut = self.cmd('ip route get %s' % dstIP)
        r = re.search(r"src (\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})", ipRouteOut)
        return r.group(1) if r else None

    def getDeviceConfig(self, srcIP):
        basicCfg = {
            "managementAddress": "grpc://%s:%d?device_id=%d" % (
                srcIP, self.grpcPort, self.p4DeviceId),
            "driver": "bmv2",
            "pipeconf": self.pipeconfId
        }

        if self.longitude and self.latitude:
            basicCfg["longitude"] = self.longitude
            basicCfg["latitude"] = self.latitude

        cfgData = {
            "basic": basicCfg
        }

        if self.withGnmi:
            cfgData["generalprovider"]["gnmi"] = {
                "ip": srcIP,
                "port": self.grpcPort
            }

        if self.injectPorts:
            portData = {}
            portId = 1
            for intfName in self.intfNames():
                if intfName == 'lo':
                    continue
                portData[str(portId)] = {
                    "number": portId,
                    "name": intfName,
                    "enabled": True,
                    "removed": False,
                    "type": "copper",
                    "speed": 10000
                }
                portId += 1

            cfgData['ports'] = portData

        return cfgData

    def doOnosNetcfg(self, controllerIP):
        """
        Notifies ONOS about the new device via Netcfg.
        """
        srcIP = self.getSourceIp(controllerIP)
        if not srcIP:
            warn("\n*** WARN: unable to get controller IP address, won't do netcfg\n")
            return

        cfgData = {
            "devices": {
                self.onosDeviceId: self.getDeviceConfig(srcIP)
            }
        }
        with open(self.netcfgfile, 'w') as fp:
            json.dump(cfgData, fp, indent=4)

        if not self.netcfg:
            # Do not push config to ONOS.
            return

        # Build netcfg URL
        url = 'http://%s:8181/onos/v1/network/configuration/' % controllerIP
        # Instantiate password manager for HTTP auth
        pm = urllib2.HTTPPasswordMgrWithDefaultRealm()
        pm.add_password(None, url,
                        # os.environ['ONOS_WEB_USER'],
                        # os.environ['ONOS_WEB_PASS']
                        ONOS_WEB_USER,
                        ONOS_WEB_PASS)

        urllib2.install_opener(urllib2.build_opener(
            urllib2.HTTPBasicAuthHandler(pm)))
        # Push config data to controller
        req = urllib2.Request(url, json.dumps(cfgData),
                              {'Content-Type': 'application/json'})
        try:
            f = urllib2.urlopen(req)
            print(f.read())
            f.close()
        except urllib2.URLError as e:
            warn("*** WARN: unable to push config to ONOS (%s)\n" % e.reason)

    def start(self, controllers):
        bmv2Args = [SIMPLE_SWITCH_GRPC] + self.grpcTargetArgs()
        if self.valgrind:
            bmv2Args = VALGRIND_PREFIX.split() + bmv2Args

        cmdString = " ".join(bmv2Args)

        if self.dryrun:
            info("\n*** DRY RUN (not executing bmv2)")

        debug("\nStarting BMv2 target: %s\n" % cmdString)

        writeToFile("/tmp/bmv2-%s-grpc-port" % self.name, self.grpcPort)
        writeToFile("/tmp/bmv2-%s-thrift-port" % self.name, self.thriftPort)

        try:
            if not self.dryrun:
                # Start the switch
                self.logfd = open(self.logfile, "w")
                self.bmv2popen = self.popen(cmdString,
                                            stdout=self.logfd,
                                            stderr=self.logfd)
                self.waitBmv2Start()
                # We want to be notified if BMv2 dies...
                threading.Thread(target=watchDog, args=[self]).start()
                if self.json is not None:
                    if self.switch_config is not None:
                        # Switch initial configuration using Thrift CLI
                        try:
                            with open(self.switch_config, mode='r') as f:
                                # map(self.bmv2Thrift(), f.readlines())
                                for cmd_row in f:
                                    self.bmv2Thrift(cmd_row)
                            debug("\nSwitch has been configured with %s configuration file" % self.switch_config)
                        except IOError:
                            info("\nSwitch configuration file %s not found" % self.switch_config)
                if controllers is not None and len(controllers) > 0:
                    self.doOnosNetcfg(self.controllerIp(controllers))
                else:
                    self.doOnosNetcfg(None)
                    info("NO ONOS NETCFG\n")
        except Exception:
            ONOSBmv2Switch.mininet_exception = 1
            self.killBmv2()
            self.printBmv2Log()
            raise

    def bmv2Thrift(self, *args, **kwargs):
        "Run ovs-vsctl command (or queue for later execution)"
        cli_command = SIMPLE_SWITCH_CLI + " --thrift-port " + str(self.thriftPort) + " <<< "
        switch_cmd = ' '.join(map(str, filter(lambda ar: ar is not None, args)))
        command = cli_command + '"' + switch_cmd + '"'
        self.cmd(command)

    def attach(self, intf):
        """ Connect a new data port """
        # TODO: find a better way to add a port at runtime
        if self.pktdump:
            pcapFiles = ["./" + str(intf) + "_out.pcap", "./" + str(intf) + "_in.pcap"]
            self.bmv2Thrift('port_add', intf, next(key for key, value in self.intfs.items() if value == intf), *pcapFiles)
        else:
            self.bmv2Thrift('port_add', intf, next(key for key, value in self.intfs.items() if value == intf))
        self.cmd('ifconfig', intf, 'up')
        # TODO: check if need to send a new netcfg
        # if self.controllers is not None and len(controllers) > 0 :
        #   self.doOnosNetcfg(self.controllerIp(controllers))
        # else:
        #    self.doOnosNetcfg(None)

    def grpcTargetArgs(self):
        if self.grpcPort is None:
            self.grpcPort = pickUnusedPort()
        if self.thriftPort is None:
            self.thriftPort = pickUnusedPort()
        args = ['--device-id %s' % self.p4DeviceId]
        for port, intf in self.intfs.items():
            if not intf.IP():
                args.append('-i %d@%s' % (port, intf.name))
        args.append('--thrift-port %s' % self.thriftPort)
        if self.notifications:
            ntfaddr = 'ipc:///tmp/bmv2-%s-notifications.ipc' % self.name
            args.append('--notifications-addr %s' % ntfaddr)
        if self.elogger:
            nanologaddr = 'ipc:///tmp/bmv2-%s-nanolog.ipc' % self.name
            args.append('--nanolog %s' % nanologaddr)
        if self.debugger:
            dbgaddr = 'ipc:///tmp/bmv2-%s-debug.ipc' % self.name
            args.append('--debugger-addr %s' % dbgaddr)
        args.append('--log-console')
        if self.pktdump:
            args.append('--pcap --dump-packet-data %s' % PKT_BYTES_TO_DUMP)
        args.append('-L%s' % self.loglevel)
        if not self.json:
            args.append('--no-p4')
        else:
            args.append(self.json)
        # gRPC target-specific options
        args.append('--')
        args.append('--cpu-port %s' % self.cpuPort)
        args.append('--grpc-server-addr 0.0.0.0:%s' % self.grpcPort)
        return args

    def waitBmv2Start(self):
        # Wait for switch to open gRPC port, before sending ONOS the netcfg.
        # Include time-out just in case something hangs.
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        endtime = time.time() + SWITCH_START_TIMEOUT
        while True:
            result = sock.connect_ex(('127.0.0.1', self.grpcPort))
            if result == 0:
                # The port is open. Let's go! (Close socket first)
                sock.close()
                break
            # Port is not open yet. If there is time, we wait a bit.
            if endtime > time.time():
                time.sleep(0.1)
            else:
                # Time's up.
                raise Exception("Switch did not start before timeout")

    def printBmv2Log(self):
        if os.path.isfile(self.logfile):
            print("-" * 80)
            print("%s log (from %s):" % (self.name, self.logfile))
            with open(self.logfile, 'r') as f:
                lines = f.readlines()
                if len(lines) > BMV2_LOG_LINES:
                    print("...")
                for line in lines[-BMV2_LOG_LINES:]:
                    print(line.rstrip())

    @staticmethod
    def controllerIp(controllers):
        try:
            # onos.py
            clist = controllers[0].nodes()
        except AttributeError:
            clist = controllers
        assert len(clist) > 0
        return random.choice(clist).IP()

    def killBmv2(self, log=False):
        if self.bmv2popen is not None:
            self.bmv2popen.kill()
        if self.logfd is not None:
            if log:
                self.logfd.write("*** PROCESS TERMINATED BY MININET ***\n")
            self.logfd.close()

    def cleanupTmpFiles(self):
        self.cmd("rm -f /tmp/bmv2-%s-*" % self.name)

    def stop(self, deleteIntfs=True):
        """Terminate switch."""
        self.stopped = True
        self.killBmv2(log=True)
        Switch.stop(self, deleteIntfs)


class ONOSBmv2AP(ONOSBmv2Switch, AP):
    mininet_exception = multiprocessing.Value('i', 0)

    def __init__(self, name, **kwargs):
        ONOSBmv2Switch.__init__(self, name, **kwargs)
        self.wintfs = {}  # dict of wireless port numbers
        self.wports = {}  # dict of interfaces to port numbers


class Bmv2AP(ONOSBmv2AP):
    def __init__(self, name, **kwargs):
        ONOSBmv2AP.__init__(self, name, **kwargs)
        self.netcfg = False


class Bmv2Switch(ONOSBmv2Switch):
    def __init__(self, name, **kwargs):
        ONOSBmv2Switch.__init__(self, name, **kwargs)
        self.netcfg = False




class P4AP(AP):
    """P4 virtual ap"""
    device_id = 0
    next_thrift_port = 9090

    def __init__(self, name, sw_path = 'simple_switch_grpc', json_path = None,
                 thrift_port = None,
                 pcap_dump = False,
                 log_console = False,
                 log_file = None,
		 log_dir = None,
                 verbose = False,
                 device_id = None,
                 enable_debugger = False,
                 **kwargs):
        """
	   Builder of P4AP class, which supports all the common elements of the BMv2.
	"""
	
	AP.__init__(self, name, **kwargs)
        global next_thrift_port
	
        # Make sure that the provided sw_path is valid
	assert(sw_path)
        pathCheck(sw_path)
        self.sw_path = sw_path
 

        # Make sure that the provided JSON file exists
        if json_path is not None:
            if not os.path.isfile(json_path):
                error("Invalid JSON file.\n")
                exit(1)
            self.json_path = json_path
        else:
            self.json_path = None
        
	
	self.thrift_port = P4AP.next_thrift_port
	P4AP.next_thrift_port += 1

        if P4AP.check_listening_on_port(self.thrift_port):
            error('%s cannot bind port %d because it is bound by another process\n' % (self.name, self.grpc_port))
            exit(1)


        self.verbose = verbose
        logfile = "/tmp/p4s.{}.log".format(self.name)
        self.output = open(logfile, 'w')
        self.pcap_dump = pcap_dump
        self.enable_debugger = enable_debugger
        self.log_console = log_console

        if log_file is not None:
            self.log_file = log_file
        else:
            self.log_file = "/tmp/p4s.{}.log".format(self.name)

        if log_dir is not None:
                self.log_dir = log_dir
        else:
                self.log_dir = '/tmp' 
        
	if device_id is not None:
            self.device_id = device_id
            P4AP.device_id = max(P4AP.device_id, device_id)
        else:
            self.device_id = P4AP.device_id
            P4AP.device_id += 1
        self.nanomsg = "ipc:///tmp/bm-{}-log.ipc".format(self.device_id)



    @classmethod
    def setup(cls):
        pass



    def check_switch_started(self, pid):
        """While the process is running (pid exists), we check if the Thrift
        server has been started. If the Thrift server is ready, we assume that
        the switch was started successfully. This is only reliable if the Thrift
        server is started at the end of the init process"""
        
	while True:
            if not os.path.exists(os.path.join("/proc", str(pid))):
                return False
            if P4AP.check_listening_on_port(self.thrift_port):
                return True
            time.sleep(0.5)



    def start(self, controllers):
        """Start up a new P4 ap"""
    
	info("Starting P4 ap {}.\n".format(self.name))
        args = [self.sw_path]
        pid = None
        
	for port, intf in self.intfs.items():
            if not intf.IP():
                args.extend(['-i', str(port) + "@" + intf.name])
        
	if self.pcap_dump:
            args.append("--pcap %s" % self.pcap_dump)
        
	if self.thrift_port:
            args.extend(['--thrift-port', str(self.thrift_port)])
        
	if self.nanomsg:
            args.extend(['--nanolog', self.nanomsg])
        
	args.extend(['--device-id', str(self.device_id)])
        P4AP.device_id += 1
        args.append(self.json_path)
        
	if self.enable_debugger:
            args.append("--debugger")
        
	if self.log_console:
            args.append("--log-console")
        
	info(' '.join(args) + "\n")

        with tempfile.NamedTemporaryFile() as f:
            # self.cmd(' '.join(args) + ' > /dev/null 2>&1 &')
            self.cmd(' '.join(args) + ' >' + self.log_file + ' 2>&1 & echo $! >> ' + f.name)
            pid = int(f.read())
        
	debug("P4 ap {} PID is {}.\n".format(self.name, pid))
        if not self.check_switch_started(pid):
            error("P4 ap {} did not start correctly.\n".format(self.name))
            exit(1)
        
	info("P4 ap {} has been started.\n".format(self.name))



    def stop(self):
        """Terminate P4 switch."""
        self.output.flush()
        self.cmd('kill %' + self.sw_path)
        self.cmd('wait')
        self.deleteIntfs()

    def attach(self, intf):
        """Connect a data port"""
        assert(0)

    def detach(self, intf):
        """Disconnect a data port"""
        assert(0)

    @classmethod
    def check_listening_on_port(self, port):
    	"""Check if a port is listening"""
	for c in psutil.net_connections(kind='inet'):
        	if c.status == 'LISTEN' and c.laddr[1] == port:
            		return True
    	return False



class P4RuntimeAP(P4AP):
    """BMv2 AP with gRPC support"""
    next_grpc_port = 50051	

    def __init__(self, name, sw_path = 'simple_switch_grpc', runtime_json_path = None,
                 grpc_port = None,		
                 device_id = None,	
                 **kwargs):	
	"""
	   Builder of the P4Runtime class, this in turn inherits from the P4AP class,
	   which supports all the common elements of the BMv2.	    
	"""

        P4AP.__init__(self, name, **kwargs)	

       	# Make sure that the BMV2-exe exists 	
        pathCheck(sw_path)	
        self.sw_path = sw_path	

	
        # Make sure that the provided Runtime JSON file exists
	if runtime_json_path is not None:
            if not os.path.isfile(runtime_json_path):
                error("Invalid runtime JSON file.\n")
                exit(1)
            self.runtime_json_path = runtime_json_path
        else:
            self.runtime_json_path = None
	
	# We assign the indicated gRPC port univocally,
	# making sure that it is in use by another process 
        if grpc_port is not None:
            self.grpc_port = grpc_port	
        else:	
            self.grpc_port = P4RuntimeAP.next_grpc_port	
            P4RuntimeAP.next_grpc_port += 1	

        if P4AP.check_listening_on_port( self.grpc_port ):
            error('%s cannot bind port %d because it is bound by another process\n' % (self.name, self.grpc_port))	
            exit(1)	

	
	if device_id is not None:	
            self.device_id = device_id	
            P4AP.device_id = max(P4AP.device_id, device_id)	
        else:	
            self.device_id = P4AP.device_id	
            P4AP.device_id += 1	
        self.nanomsg = "ipc:///tmp/bm-{}-log.ipc".format(self.device_id)	
	


    def check_ap_started(self, pid):
	"""
            This method will check if the process is up and if the
	    BMv2 is listening on the indicated gRPC port. 
	"""

	for _ in range(SWITCH_START_TIMEOUT * 2):	
            if not os.path.exists(os.path.join("/proc", str(pid))):	
                return False	
            if P4AP.check_listening_on_port(int(self.grpc_port)):	
                return True	
            time.sleep(0.5)	



    def start(self, controllers):
        """
	    This method will initialize the BMV2 given the parameters
            passed to the class. Finally, it will configure the switch using P4Runtime	
	"""
	
	info("Starting P4RuntimeAP {}.\n".format(self.name))	
        
	args = [self.sw_path]	
        pid = None	
        
	# It will form the execution cmd based on the class attributes.
	for port, intf in self.intfs.items():	
            if not intf.IP():	
                args.extend(['-i', str(port) + "@" + intf.name])	
        
	if self.pcap_dump:	
            args.append("--pcap %s" % self.pcap_dump)	
        
	if self.nanomsg:	
            args.extend(['--nanolog', self.nanomsg])	
        
	args.extend(['--device-id', str(self.device_id)])	
        P4AP.device_id += 1	
        
	if self.json_path:	
            args.append(self.json_path)	
        else:	
            args.append("--no-p4")	
        
	if self.enable_debugger:	
            args.append("--debugger")	
        
	if self.log_console:	
            args.append("--log-console")	
        
	if self.thrift_port:	
            args.append('--thrift-port ' + str(self.thrift_port))	
        
	if self.grpc_port:	
            args.append("-- --grpc-server-addr 0.0.0.0:" + str(self.grpc_port))	
        

	# We put together the arguments of the cmd to execute it.
	cmd = ' '.join(args)	
        
	info(cmd + "\n")	
        with tempfile.NamedTemporaryFile() as f:	
            self.cmd(cmd + ' >'+ self.log_dir + '/' + self.log_file + ' 2>&1 & echo $! >> ' + f.name)	
            pid = int(f.read())	
        

	# We check that the BMv2 has been started correctly. 
	# If so, we proceed to configure it with P4Runtime. 
	debug("P4RuntimeAP {} PID is {}.\n".format(self.name, pid))	
        if not self.check_ap_started(pid):	
            error("P4 ap {} did not start correctly.\n".format(self.name))	
            exit(1)	
        
	info("P4RuntimeAP {} has been started.\n".format(self.name))
	self.program_ap_runtime()



    def program_ap_runtime(self):
        """ This method will use P4Runtime to program the switch using the
            content of the runtime JSON file as input.
        """

    	from mn_wifi.p4runtime_lib.simple_controller import program_switch
        grpc_port = self.grpc_port
        device_id = self.device_id
        runtime_json = self.runtime_json_path

        info('Configuring AP %s using P4Runtime with file %s' % (self.name, runtime_json))
        with open(runtime_json, 'r') as sw_conf_file:
            outfile = '%s/%s-p4runtime-requests.txt' % (self.log_dir, self.name)
            program_switch(
                addr='127.0.0.1:%d' % grpc_port,
                device_id=device_id,
                sw_conf_file=sw_conf_file,
                workdir=os.getcwd(),
                proto_dump_fpath=outfile)
