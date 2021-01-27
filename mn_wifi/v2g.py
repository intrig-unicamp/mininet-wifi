"""
Additional Nodes for Vehicle to Grid (V2G) communication
Authors: Denis Donadel (denisdonadel@gmail.com) and Luca Attanasio (luca_attanasio@me.com)
"""

import atexit  # clean the mess on exit
import fileinput
# to generate the random prefix for folders
import random
import string
import sys
from os import popen

from mininet.term import makeTerm
from mininet.moduledeps import pathCheck
from mininet.node import (Node, OVSSwitch)


class Electric(Node):
    """A basic Node class with the support for V2G communication"""

    modes_available = ["AC_single_phase_core", "AC_three_phase_core", "DC_core", "DC_extended",
                       "DC_combo_core", "DC_unique"]

    auth_available = ["Contract", "ExternalPayment"]

    exi_codecs = ["exificent", "open_exi"]

    logging = {"hex": "exi.messages.showhex",
               "xml": "exi.messages.showxml",
               "signature": "signature.verification.showlog"}

    def __init__(self, name, path=None, **kwargs):
        # double check if java is available (it is needed for RiseV2G)
        pathCheck('java')
        # set the path of RiseV2G
        if path is not None:
            self.RISE_PATH = path
        else:
            self.RISE_PATH = "/usr/share/.miniV2G/RiseV2G"

        # check if it exists
        if "No such file or directory" in popen('ls {}'.format(self.RISE_PATH)).read():
            exit(
                "*** Fatal error: directory %s not found. Select the right folder which contains the needed jar files.")

        # initialize the subprocess object
        self.proc = None

        Node.__init__(self, name, **kwargs)

        # setup a random prefix for folders (to better cleanup everything in the end)
        prefix_len = 4
        self.FOLDER_PREFIX = ''.join(random.choice(string.ascii_lowercase) for i in range(prefix_len))

        # initialize the folder for the RiseV2G copy
        self.folder = ""

        # cleanup of the generated folder
        def cleaner():
            print('*** Cleaning up the mess')
            popen("rm -rd {}*".format(self.FOLDER_PREFIX))
            # this will be called more than the needed times, so the output is supressed
            popen("rm -rd RISE-V2G-Certificates > /dev/null 2>&1 ")

        atexit.register(cleaner)


    def setIntf(self, intfName):
        """Sets the intfName on the .properties file.
        :param intfName: the intfName to be setted. """

        return self.setProperty('network.interface', intfName)

    def setLoggingLevels(self, loggingLevels=None):
        """Sets the loggings levels on the .properties file. It sets to True the one provided into loggingLevels.
        :param loggingLevels: set to True the logs included, False otherwise. """
        log = True
        if isinstance(loggingLevels, str):
            loggingLevels = [loggingLevels]
        if all(m in Electric.logging.keys() for m in loggingLevels):
            for k in Electric.logging.keys():
                if k in loggingLevels:
                    log *= self.setProperty(Electric.logging[k], 'true')
                else:
                    log *= self.setProperty(Electric.logging[k], 'false')
        else:
            print("*** The possible levels are only {}.".format(Electric.logging.keys()))
            return False

        if log == 1:
            return True
        else:
            print("*** A problem occur. The possible levels are {}.".format(Electric.logging.keys()))
            return False

    def setExiCodec(self, exiCodec=None):
        """Sets the exi codec on the .properties file.
        :param exiCodec: the exi codec to be used. """
        if exiCodec in Electric.exi_codecs:
            return self.setProperty('exi.codec', exiCodec)
        else:
            print("*** The possible codec are {}.".format(Electric.exi_codecs))
            return False

    def setProperty(self, prop_name, prop_value):
        """
        Set a specified property.
        :param prop_name: the name of the property
        :param prop_value: the new value of the property
        :return True if all ok, False if exceptions occurs """

        # check and set the prefix values
        if "ev" in self.folder:
            prefix = "EVCC"
        else:
            prefix = "SECC"

        try:
            f = fileinput.input([self.folder + "/" + prefix + "Config.properties"], inplace=True)
            for line in f:
                if line.strip().startswith(prop_name):
                    line = '{} = {}\n'.format(prop_name, prop_value)
                sys.stdout.write(line)
            f.close()
            return True
        except Exception as e:
            print("*** Problem in writing properties ({}).".format(e))
            return False

    def printProperties(self):

        d = self.getProperties()
        for i in d:
            print("\t{} = {}".format(i, d[i]))

    def getProperties(self):

        # to decide if it is an ev or se it check the path;
        # if you change the default folder it is better to change this stuff
        if "ev" in self.folder:
            prefix = "EVCC"
        else:
            prefix = "SECC"

        properties = {}

        f = fileinput.input([self.folder + "/" + prefix + "Config.properties"], mode='r')
        for line in f:
            if not line.strip().replace(" ", "").startswith('#') and len(line) > 3:
                (k, v) = line.strip().replace(" ", "").split('=')
                properties.update({k: v})
        f.close()

        return properties



class EV(Electric):
    """An Electric Vehicle (EV) is a Node containing all the
    necessary to be able to start the communication from its
    EV Communication Controller (EVCC) with a SECC to require charging service """

    def __init__(self, name, path=None, chargingMode=None, exi=None, logging=None, sessionId=None, voltageAccuracy=None,
                 chargeIntf=None, TLS=None, **kwargs):
        self.name = str(name)
        Electric.__init__(self, self.name, path, **kwargs)

        self.folder = "{}_ev_{}".format(self.FOLDER_PREFIX, self.name)
        self.cmd("mkdir {}".format(self.folder))
        self.cmd("cp {}/EVCCConfig.properties {}/".format(self.RISE_PATH, self.folder))
        # this cp can resist to update of risev2g but you must have onyl one version
        self.cmd("cp {}/rise-v2g-evcc-*.jar {}/".format(self.RISE_PATH, self.folder))
        # cd into the right folder
        self.cmd("cd ./{}".format(self.folder))

        # set charging mode
        if chargingMode is not None:
            self.setEnergyTransferRequested(req=chargingMode)
        # setup exi codec
        if exi is not None:
            self.setExiCodec(exiCodec=exi)
        if logging is not None:
            self.setLoggingLevels(loggingLevels=logging)
        if sessionId is not None:
            self.setSessionId(id=sessionId)
        if voltageAccuracy is not None:
            self.setVoltageAccuracy(acc=voltageAccuracy)
        if TLS is not None and TLS is not "0":
            self.setTLS(TLS=TLS)
            if TLS == "1" or TLS == True or TLS == "true":
                print("*** TLS certificates not generated. If needed, generate them with setTLS().")
        # intf setup: if no interface is provided it must be provided later, otherwise default will be set
        if chargeIntf is not None:
            self.setIntf(intfName=chargeIntf)
        else:
            self.setIntf(intfName="")

    def charge(self, in_xterm=False, intf=None):
        """Starting the charging process.
        :param in_xterm: True to run the charge inside an xterm instance. Default: False.
        :param intf: the interface to which search for EVSE. Default: None, use the default interface. Has permanent effect.
        """

        print("*** Looking for EVSE and start charging...")

        # setting the interface (default: default interface)
        if intf is not None:
            self.setIntf(intf)
        elif self.getProperties()['network.interface'] == "":
            print("* No intf selected, using default interface.")
            self.setIntf(self.intf().name)

        if in_xterm:
            # run inside an xterm. You must append the return value to net.terms to terminal on exit.
            command = "cd ./{}; java -jar rise-v2g-evcc-*.jar; bash -i".format(self.folder)
            # this return a list of just one xterm, so [0] is needed
            self.proc = makeTerm(self, cmd="bash -i -c '{}'".format(command))[0]
            return self.proc
        else:
            self.proc = self.popen("cd ./{}; java -jar rise-v2g-evcc-*.jar".format(self.folder), shell=True)
            # print the stdout to the CLI at the end of the charging process
            proc_stdout = self.proc.communicate()[0].strip()
            print(proc_stdout)

    def setEnergyTransferRequested(self, req):
        """ Set the energy transfer modes
        :param req: the requested energy transfer mode
        """

        if isinstance(req, str):
            # check if the mode is available
            if req in Electric.modes_available:
                return self.setProperty('energy.transfermode.requested', req)
            else:
                print("*** Modes available: {}".format( Electric.modes_available))
                return False
        else:
            print("*** You must provide a sting.")
            return False

    def setSessionId(self, id='00'):
        """ Set the session id
        :param id: the session id
        """
        try:
            val = int(id)
            if 10 > val >= 0:
                return self.setProperty('session.id', "0"+str(val))
            elif val >= 0:
                return self.setProperty('session.id', str(val))
            else:
                print("*** You must provide a positive number.")
                return False
        except ValueError:
            print("*** You must provide a number.")
            return False

    def setVoltageAccuracy(self, acc=5):
        """ Set the voltage accuracy value
        :param acc: accuracy value. Default is 5
        """
        try:
            val = int(acc)
            if val > 0:
                return self.setProperty('voltage.accuracy', str(val))
            else:
                print("*** You must provide a positive number.")
                return False
        except ValueError:
            print("*** You must provide a number.")
            return False

    def setTLS(self, TLS=False, generate_certificates=False, SECC=None):
        """ Activate/deactivate TLS. Up to now it is possible to automatically generate keys only between entities (self and SE).
        :param TLS: True/true/1 to activate TLS, False (Default) to use TPC instead.
        :param generate_certificates: True to generate certificates, False otherwise. If True you MUST set also SE.
        :param SECC: the EVSE/SECC connected to the EV
        """
        if isinstance(TLS, str) or isinstance(TLS, int):
            if str(TLS) == 'true' or str(TLS) == '1' or int(TLS) == 1:
                res = self.setProperty('tls', 'true')
            else:
                return self.setProperty('tls', 'false')
        elif isinstance(TLS, bool):
            if TLS:
                res = self.setProperty('tls', 'true')
            else:
                return self.setProperty('tls', 'false')
        else:
            print("*** Problem on setting TLS.")
            return False
        if not res:
            print("*** Problem on setting TLS property.")
            return False

        # generate certificates if needed
        if generate_certificates:
            if SE is None or type(SECC) is not SE:
                print("*** Error: you must provide the EVSE/SECC connected to the EV.")
                return False

            # copy files needed to generate keys
            self.cmd("cd .. && cp -r {}/RISE-V2G-Certificates/ . && cd RISE-V2G-Certificates".format(self.RISE_PATH))
            # generate keys
            self.cmd("chmod +x generateCertificates.sh && ./generateCertificates.sh")
            # copy keys in the desired folders
            self.cmd("cp keystores/evccKeystore.jks ../{}/".format(self.folder))
            self.cmd("cp keystores/evccTruststore.jks ../{}/".format(self.folder))
            self.cmd("cp keystores/seccKeystore.jks ../{}/".format(SECC.folder))
            self.cmd("cp keystores/seccTruststore.jks ../{}/".format(SECC.folder))
            self.cmd("cp certs/cpsCertChain.p12 ../{}/".format(SECC.folder))
            self.cmd("cp certs/moCertChain.p12 ../{}/".format(SECC.folder))
            self.cmd("cp privateKeys/moSubCA2.pkcs8.der ../{}/".format(SECC.folder))
            # return to the right folder
            self.cmd("cd ../{}".format(self.folder))
            print("*** Certificates generated.")

    # TODO: authentication.mod, evcc controller class, contract.certificate.update.timespan to be added later on


class SE(Electric):
    """An EV Supply Equipment (EVSE) is a Node which can
    provide charging services to an EV by communication using the
    Supply Equipment Communication Controller (SECC)
    """

    def __init__(self, name, path=None, chargingModes=None, exiCodec=None, loggingLevels=None, authOptions=None,
                 chargingFree=None, chargeIntf=None, **kwargs):
        self.name = str(name)
        Electric.__init__(self, self.name, path, **kwargs)

        self.folder = "{}_se_{}".format(self.FOLDER_PREFIX, self.name)
        self.cmd("mkdir {}".format(self.folder))
        self.cmd("cp {}/SECCConfig.properties {}/".format(self.RISE_PATH, self.folder))
        # this cp can resist to update of risev2g but you must have onyl one version
        self.cmd("cp {}/rise-v2g-secc-*.jar {}/".format(self.RISE_PATH, self.folder))
        # cd into the right folder
        self.cmd("cd ./{}".format(self.folder))

        # set charging modes
        if chargingModes is not None:
            self.setEnergyTransferModes(chargingModes)
        # setup exi codec
        if exiCodec is not None:
            self.setExiCodec(exiCodec=exiCodec)
        # setup logging
        if loggingLevels is not None:
            self.setLoggingLevels(loggingLevels=loggingLevels)
        # setup payment/auth methods
        if authOptions is not None:
            self.setAuthOption(auth=authOptions)
        # setup free charge
        if chargingFree is not None:
            self.setChargingFree(free=chargingFree)
        # intf setup: if no interface is provided it must be provided later, otherwise default will be set
        if chargeIntf is not None:
            self.setIntf(intfName=chargeIntf)
        else:
            self.setIntf(intfName="")

    def startCharge(self, in_xterm=True, intf=None):
        """
        Spawn an xterm and start the listening phase in it.
        It is not possible to launch it without xterm because otherwise sometimes it randomly crashes.
        :param intf: Interface to listen for charging requests. If None, default is used. Has permanent effect.
        :returns A popen xterm instance. To be appended to "net.terms" to assure a correct close on exit."""

        print("*** Starting waiting for EVs...")

        # setting the interface (default: default interface)
        if intf is not None:
            self.setIntf(intf)
        elif self.getProperties()['network.interface'] == "":
            print("* No intf selected, using default interface.")
            self.setIntf(self.intf().name)

        if in_xterm:
            # run inside an xterm. You must append the return value to net.terms to terminal on exit.
            command = "cd ./{}; java -jar rise-v2g-secc-*.jar; bash -i".format(self.folder)
            # this return a list of just one xterm, so [0] is needed
            self.proc = makeTerm(self, cmd="bash -i -c '{}'".format(command))[0]
        else:
            self.proc = self.popen("cd ./{}; java -jar rise-v2g-secc-*.jar".format(self.folder), shell=True)
            # print the stdout to the CLI at the start of the charging process
            proc_stdout = self.proc.communicate(timeout=15)[0].strip()
            print(proc_stdout)
        return self.proc

    def state(self):
        """ Print and return the state of a process
        :return the state of the process or False if the process does not exists."""
        if self.proc is not None:
            state = self.proc.poll()
            if state is None:
                print("* Running!")
            else:
                print("* Stopped (State: {}).".format(state))
            return state
        else:
            print("* The process does not exist. Call .startCharge() first.")
            return False

    def stopCharge(self):
        """Stops the charging process.
        :return True if stopped successfully, False if the process does not exists"""
        # TODO: maybe can be useful to print stdout
        if self.proc is not None:
            self.proc.kill()
            self.proc = None
            print("* Stopped successfully.")
            return True
        else:
            print("* The process does not exist. Call .startCharge() first.")
            return False

    def setEnergyTransferModes(self, modes):
        """ Set the energy transfer modes
        :param modes: the modes available
        """

        if isinstance(modes, list) or isinstance(modes, set):
            # check if the modes are available
            if all(m in  Electric.modes_available for m in modes):
                return self.setProperty('energy.transfermodes.supported', ', '.join(modes))
            else:
                print("*** Modes available: {}".format( Electric.modes_available))
        elif isinstance(modes, str):
            # check if the mode is available
            if modes in  Electric.modes_available:
                return self.setProperty('energy.transfermodes.supported', modes)
            else:
                print("*** Modes available: {}".format( Electric.modes_available))
                return False
        else:
            print("*** You must provide a sting, a list or a set.")
            return False

    def setChargingFree(self, free=True):
        """ Set if the nergy transfer is free or not
        :param free: charging type (True or 'true' or '1' for free charging, False 'false' or '0' otherwise)
        """
        if isinstance(free, str) or isinstance(free, int):
            if str(free) == 'true' or str(free) == '1' or int(free) == 1:
                return self.setProperty('charging.free', 'true')
            else:
                return self.setProperty('charging.free', 'false')
        elif isinstance(free, bool):
            if free:
                return self.setProperty('charging.free', 'true')
            else:
                return self.setProperty('charging.free', 'false')
        print("*** Problem on setting charging free.")
        return False

    def setAuthOption(self, auth):
        """ Set auth methods
        :param auth: auth modes to be supported
        """

        if isinstance(auth, list) or isinstance(auth, set):
            # check if the payments are available
            if all(m in Electric.auth_available for m in auth):
                return self.setProperty('authentication.modes.supported', ', '.join(auth))
            else:
                print("*** Payments available: {}".format(Electric.auth_available))
        elif isinstance(auth, str):
            # check if the mode is available
            if auth in Electric.auth_available:
                return self.setProperty('authentication.modes.supported', auth)
            else:
                print("*** Payments available: {}".format(Electric.auth_available))
                return False
        else:
            print("*** You must provide a string, a list or a set.")
            return False

        # TODO: environment.private, implementation classes to be added later on


class MitMOVSSwitch(OVSSwitch):
    "Open vSwitch switch acting as Man-in-the-middle. Depends on ovs-vsctl."

    def __init__(self, name, failMode='secure', datapath='kernel',
                 inband=False, protocols=None,
                 reconnectms=1000, stp=False, batch=False, **params):
        """name: name for switch
           failMode: controller loss behavior (secure|standalone)
           datapath: userspace or kernel mode (kernel|user)
           inband: use in-band control (False)
           protocols: use specific OpenFlow version(s) (e.g. OpenFlow13)
                      Unspecified (or old OVS version) uses OVS default
           reconnectms: max reconnect timeout in ms (0/None for default)
           stp: enable STP (False, requires failMode=standalone)
           batch: enable batch startup (False)"""
        OVSSwitch.__init__(self, name, **params)

    def add_mim_flows(self, server, client, mim, use_ipv6=True):
        """Sets the flows for a MiM switch.
        :param server: the server (e.g. se1)
        :param client: the client (e.g. ev1)
        :param mim: the MiM node (e.g. mim)
        :param use_ipv6: choose ipv6 or ipv4 (default: true)"""

        print("*** Adding flows to mim...")

        if use_ipv6:
            # TODO: should be addded to mininet node itself in the future
            get_ipv6 = "ifconfig %s | grep inet6 | grep -o -P '(?<=inet6 ).*(?= prefixlen)'"
            serverIPV6 = server.cmd(get_ipv6 % (server.intf().name)).rstrip()
            clientIPV6 = client.cmd(get_ipv6 % (client.intf().name)).rstrip()
            mimIPV6 = mim.cmd(get_ipv6 % (mim.intf().name)).rstrip()

            # write ips to a common file in home folder
            # pre-condition to run mim: allows the mim to know the ips of ev1 and se1
            f = open(".common_ips.txt", "w+")
            f.write(serverIPV6 + "\n" + clientIPV6)
            f.close()

        ### IPV4
        if not use_ipv6:
            # CLI EXAMPLE
            # sh ovs-ofctl add-flow s1 dl_src=00:00:00:00:00:01,dl_dst=00:00:00:00:00:03,actions=mod_nw_dst:10.0.0.3,output:3
            # sh ovs-ofctl add-flow s1 dl_src=00:00:00:00:00:03,dl_dst=00:00:00:00:00:01,actions=mod_nw_src:10.0.0.2,output:1
            # sh ovs-ofctl add-flow s1 dl_type=0x806,nw_proto=1,actions=flood

            #  PART 1 fake communication with server : IPV4 ONLY
            # mac 10.0.0.2 is already linked to (3) by mim arpspoof
            # malicious node changes the ip
            # server -> mim, mim
            self.cmd("ovs-ofctl", "add-flow", "s1",
                     "dl_src=%s,dl_dst=%s,actions=mod_nw_dst:%s,output:3" % (server.MAC(), mim.MAC(), mim.IP()))
            # mim -> client, server
            self.cmd("ovs-ofctl", "add-flow", "s1",
                     "dl_src=%s,dl_dst=%s,actions=mod_nw_src:%s,output:1" % (mim.MAC(), server.MAC(), client.IP()))

            # PART 2 fake communication with client : IPV4 ONLY
            # server -> mim, mim
            self.cmd("ovs-ofctl", "add-flow", "s1",
                     "dl_src=%s,dl_dst=%s,actions=mod_nw_dst:%s,output:3" % (client.MAC(), mim.MAC(), mim.IP()))
            # mim -> client, server
            self.cmd("ovs-ofctl", "add-flow", "s1",
                     "dl_src=%s,dl_dst=%s,actions=mod_nw_src:%s,output:2" % (mim.MAC(), client.MAC(), server.IP()))

            # flood the arp relys to all nodes who requested them
            self.cmd("ovs-ofctl", "add-flow", "s1", "dl_type=0x806,nw_proto=1,actions=flood")
        ### IPV6
        else:
            # ICMPv6 messages are sent in the network to understand the mac addresses of the others

            # INCLUDE UDP MESSAGES

            # PART 1 fake communication with server : IPV6 ONLY (0x86dd)
            # direction: server -> any
            # action: change ip to mim and send flow to mim
            self.cmd("ovs-ofctl", "add-flow", "s1",
                     "dl_type=0x86dd,ipv6_src=%s,in_port=1,actions=set_field:%s-\>ipv6_dst,output:3" % (
                     serverIPV6, mimIPV6))
            # messages generated by parasite6 need redirection to server
            # direction: client (generated by mim) -> server
            # action: send flow to server
            self.cmd("ovs-ofctl", "add-flow", "s1",
                     "dl_type=0x86dd,ipv6_dst=%s,in_port=3,actions=set_field:%s-\>ipv6_src,output:1" % (
                     serverIPV6, clientIPV6))

            # PART 2 fake communication with client : IPV6 ONLY (0x86dd)
            # direction: client -> any
            # action: change ip to mim and send flow to mim

            self.cmd("ovs-ofctl", "add-flow", "s1", "--strict",
                     "priority=1,dl_type=0x86dd,ipv6_src=%s,in_port=2,actions=set_field:%s-\>ipv6_dst,output:3" % (
                     clientIPV6, mimIPV6))
            # udp = nw_proto 17
            self.cmd("ovs-ofctl", "add-flow", "s1", "--strict",
                     "priority=3,dl_type=0x86dd,ipv6_src=%s,in_port=2,nw_proto=17,tp_dst=%d,actions=mod_tp_dst:%d,output:3" % (
                     clientIPV6, 15118, 15119))
            # # messages generated by parasite6 need redirection to server
            # # direction: server (generated by mim) -> client
            # # action: send flow to client
            self.cmd("ovs-ofctl", "add-flow", "s1",
                     "dl_type=0x86dd,ipv6_dst=%s,in_port=3,actions=set_field:%s-\>ipv6_src,output:2" % (
                     clientIPV6, serverIPV6))

        # print(self.cmd("ovs-ofctl", "dump-flows", "s1"))


class MitMNode(Electric):
    """Electric node class for man in the middle."""

    def __init__(self, name, path=None, **kwargs):
        self.name = str(name)
        Electric.__init__(self, self.name, '/usr/share/.miniV2G/V2Gdecoder', **kwargs)

        self.folder = "{}_mim_{}".format(self.FOLDER_PREFIX, self.name)
        self.cmd("mkdir {}".format(self.folder))
        self.cmd("cp -r {}/schemas {}/".format(self.RISE_PATH, self.folder))
        self.cmd("cp {}/V2Gdecoder.jar {}/".format(self.RISE_PATH, self.folder))
        # cd into the right folder
        self.cmd("cd ./{}".format(self.folder))

    def start_server(self, dos_attack=False):
        in_xterm = True

        print("*** Starting the MiM server...")
        if in_xterm:
            # run inside an xterm. You must append the return value to net.terms to terminal on exit.
            if not dos_attack:
                command = "python v2g_mim_server.py; bash -i".format(self.folder)
            else:
                command = "python v2g_mim_server.py -d; bash -i".format(self.folder)
            # this return a list of just one xterm, so [0] is needed
            self.proc = makeTerm(self, cmd="bash -i -c '{}'".format(command))[0]
            return self.proc

    def start_decoder(self, in_xterm=True):
        """Starting the decoder.
        :param in_xterm: True to run the charge inside an xterm instance. Default: False."""

        print("*** Starting the decoder...")

        if in_xterm:
            # run inside an xterm. You must append the return value to net.terms to terminal on exit.
            command = "cd ./{}; java -jar V2Gdecoder.jar -w; bash -i".format(self.folder)
            # this return a list of just one xterm, so [0] is needed
            self.proc = makeTerm(self, cmd="bash -i -c '{}'".format(command))[0]
            return self.proc
        else:
            self.cmd("cd ./{}; java -jar V2Gdecoder.jar -w".format(self.folder), "2>/dev/null 1>/dev/null &")
            # self.proc = self.popen("cd ./{}; java -jar V2Gdecoder.jar -w".format(self.folder), shell=True)
            # # print the stdout to the CLI at the end of the charging process
            # proc_stdout = self.proc.communicate()[0].strip()
            # print(proc_stdout)

    def start_spoof(self, server=None, client=None, use_ipv6=True):

        # IPV4
        # cli example
        # arpspoof -i h3-eth0 -c own -t 10.0.0.1 10.0.0.2 2>/dev/null 1>/dev/null & # send arp reply
        if not use_ipv6:
            if server != None and client != None:
                # fake MAC in arp table of server
                self.cmd("arpspoof", "-i %s" % self.intf().name, "-t %s" % server.IP(), "%s" % client.IP(),
                         " 2>/dev/null 1>/dev/null &")
                # fake MAC in arp table of client
                self.cmd("arpspoof", "-i %s" % self.intf().name, "-t %s" % client.IP(), "%s" % server.IP(),
                         " 2>/dev/null 1>/dev/null &")
                print("*** Starting arpspoof (have you installed it?)...")
        # IPV6
        else:
            self.cmd('echo 1 > /proc/sys/net/ipv6/conf/all/forwarding')
            self.cmd('ip6tables -I OUTPUT -p icmpv6 --icmpv6-type redirect -j DROP')
            self.cmd('parasite6 %s' % self.intf().name, "2>/dev/null 1>/dev/null &")
            print("*** Starting parasite6 (have you installed it?)...")