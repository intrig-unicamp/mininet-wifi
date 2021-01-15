"""
Additional Nodes for Vehicle to Grid (V2G) communication
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
from mininet.node import Node


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
        # TODO: check for the specific jar files

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
                 chargeIntf=None, **kwargs):
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

    # TODO: tls, authentication.mod, evcc controller class, contract.certificate.update.timespan to be added later on




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
            # TODO: test this
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
