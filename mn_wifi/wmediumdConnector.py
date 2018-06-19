"""

Helps starting the wmediumd service

author: Patrick Grosse (patrick.grosse@uni-muenster.de)
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

"""
import ctypes
import os
import socket
import tempfile
import subprocess
import signal
import time
import struct
import stat
import pkg_resources
from sys import version_info as py_version_info

from mininet.log import info, error, debug


class wmediumd_mode(object):

    mode = 4

    @classmethod
    def set_mode(cls, mode):
        cls.mode = mode


class snr(object):
    def __init__(self):
        wmediumd_mode.set_mode(mode=0)


class error_prob(object):
    def __init__(self):
        wmediumd_mode.set_mode(mode=1)


class spec_prob(object):
    def __init__(self):
        wmediumd_mode.set_mode(mode=2)


class interference(object):
    def __init__(self):
        wmediumd_mode.set_mode(mode=3)


class WmediumdCst:
    "wmediumd constants"

    def __init__(self):
        raise Exception("WmediumdCst cannot be initialized")
        pass

    SNR_MODE = 0
    ERRPROB_MODE = 1
    SPECPROB_MODE = 2
    INTERFERENCE_MODE = 3
    WRONG_MODE = 4

    WSERVER_SHUTDOWN_REQUEST_TYPE = 0
    WSERVER_SNR_UPDATE_REQUEST_TYPE = 1
    WSERVER_SNR_UPDATE_RESPONSE_TYPE = 2
    WSERVER_DEL_BY_MAC_REQUEST_TYPE = 3
    WSERVER_DEL_BY_MAC_RESPONSE_TYPE = 4
    WSERVER_DEL_BY_ID_REQUEST_TYPE = 5
    WSERVER_DEL_BY_ID_RESPONSE_TYPE = 6
    WSERVER_ADD_REQUEST_TYPE = 7
    WSERVER_ADD_RESPONSE_TYPE = 8
    WSERVER_ERRPROB_UPDATE_REQUEST_TYPE = 9
    WSERVER_ERRPROB_UPDATE_RESPONSE_TYPE = 10
    WSERVER_SPECPROB_UPDATE_REQUEST_TYPE = 11
    WSERVER_SPECPROB_UPDATE_RESPONSE_TYPE = 12
    WSERVER_POS_UPDATE_REQUEST_TYPE = 13
    WSERVER_POS_UPDATE_RESPONSE_TYPE = 14
    WSERVER_TXPOWER_UPDATE_REQUEST_TYPE = 15
    WSERVER_TXPOWER_UPDATE_RESPONSE_TYPE = 16
    WSERVER_GAIN_UPDATE_REQUEST_TYPE = 17
    WSERVER_GAIN_UPDATE_RESPONSE_TYPE = 18
    WSERVER_HEIGHT_UPDATE_REQUEST_TYPE = 19
    WSERVER_HEIGHT_UPDATE_RESPONSE_TYPE = 20
    WSERVER_GAUSSIAN_RANDOM_UPDATE_REQUEST_TYPE = 21
    WSERVER_GAUSSIAN_RANDOM_UPDATE_RESPONSE_TYPE = 22

    WUPDATE_SUCCESS = 0
    WUPDATE_INTF_NOTFOUND = 1
    WUPDATE_INTF_DUPLICATE = 2
    WUPDATE_WRONG_MODE = 3

    SOCKET_PATH = '/var/run/wmediumd.sock'
    LOG_PREFIX = 'wmediumd:'


class WmediumdException(Exception):
    pass


class WmediumdManager(object):
    is_connected = False
    registered_interfaces = []

    @classmethod
    def connect(cls, uds_address=WmediumdCst.SOCKET_PATH,
                mode=WmediumdCst.SNR_MODE):
        # type: (str) -> None
        """
        Connect to the wmediumd server and set Mininet-WiFi parameters
        to managed

        :param uds_address: The Unix Domain socket path
        :param mode: wmediumd mode to use if the server is not started
        :type uds_address str
        :type mode int
        """
        h = int(subprocess.check_output("lsmod | grep 'mac80211_hwsim' "
                                        "| wc -l", shell=True))
        if h == 0:
            os.system('modprobe mac80211_hwsim radios=0')
        is_started = False
        h = int(subprocess.check_output("pgrep -x \'wmediumd\' | wc -l",
                                        shell=True))
        if h > 0:
            if os.path.exists(uds_address) \
                    and stat.S_ISSOCK(os.stat(uds_address).st_mode):
                is_started = True
            else:
                raise WmediumdException("wmediumd is already started but "
                                        "without server")

        if not is_started:
            WmediumdStarter.initialize(parameters=['-l', '5'], mode=mode)
            WmediumdStarter.start_managed()
            time.sleep(1)
        WmediumdServer.connect(uds_address)
        cls.is_connected = True

        # Mininet specific
        from mn_wifi.module import module
        module.externally_managed = True

    @classmethod
    def disconnect(cls):
        # type: () -> None
        """
        Disconnect from the wmediumd server and unregister all registered
        interfaces
        """
        if not cls.is_connected:
            raise WmediumdException("WmediumdConnector is not connected to "
                                    "wmediumd")
        registered_interfaces = cls.registered_interfaces[:]
        for mac in registered_interfaces:
            cls.unregister_interface(mac)
        WmediumdServer.disconnect()
        cls.is_connected = False

    @classmethod
    def register_interface(cls, mac):
        # type: (str) -> int
        """
        Register a new interface at wmediumd
        :param mac The mac address of the interface
        :return The wmediumd station index

        :type mac: str
        :rtype int
        """
        ret = WmediumdServer.register_interface(mac)
        cls.registered_interfaces.append(mac)
        return ret

    @classmethod
    def unregister_interface(cls, mac):
        # type: (str) -> None
        """
        Unregister a station at wmediumd
        :param mac The mac address of the interface

        :type mac: str
        """
        WmediumdServer.unregister_interface(mac)
        cls.registered_interfaces.remove(mac)

    @classmethod
    def update_link_snr(cls, link):
        # type: (WmediumdSNRLink) -> None
        """
        Update the SNR of a connection at wmediumd
        :param link The link to update

        :type link: WmediumdSNRLink
        """
        WmediumdServer.update_link_snr(link)

    @classmethod
    def update_pos(cls, pos):
        # type: (WmediumdPos) -> None
        """
        Update the Pos of a node at wmediumd
        :param pos The pos to update

        :type pos: WmediumdPos
        """
        WmediumdServer.update_pos(pos)

    @classmethod
    def update_txpower(cls, txpower):
        # type: (WmediumdTXPower) -> None
        """
        Update the TXPower of a node at wmediumd
        :param txpower The txpower to update

        :type txpower: WmediumdTXPower
        """
        WmediumdServer.update_txpower(txpower)

    @classmethod
    def update_gain(cls, gain):
        # type: (WmediumdGain) -> None
        """
        Update the Gain of a node at wmediumd
        :param gain The Antenna Gain to update

        :type gain: WmediumdGain
        """
        WmediumdServer.update_gain(gain)

    @classmethod
    def update_gaussian_random(cls, gRandom):
        # type: (WmediumdGRandom) -> None
        """
        Update the gRandom of a node at wmediumd
        :param gRandom The gRandom to update

        :type gRandom: WmediumdGRandom
        """
        WmediumdServer.update_gaussian_random(gRandom)

    @classmethod
    def update_height(cls, height):
        # type: (WmediumdHeight) -> None
        """
        Update the Height of a node at wmediumd
        :param height The Antenna Height to update

        :type height: WmediumdHeight
        """
        WmediumdServer.update_height(height)

    @classmethod
    def update_link_errprob(cls, link):
        # type: (WmediumdERRPROBLink) -> None
        """
        Update the error probability of a connection at wmediumd
        :param link The link to update

        :type link: WmediumdERRPROBLink
        """
        WmediumdServer.update_link_errprob(link)

    @classmethod
    def update_link_specprob(cls, link):
        # type: (WmediumdSPECPROBLink) -> None
        """
        Update the error probability matrix of a connection at wmediumd
        :param link The link to update

        :type link: WmediumdSPECPROBLink
        """
        WmediumdServer.update_link_specprob(link)


class set_interference(object):

    def __init__(self, configstr, ppm, pos, txpowers,
                 fading_coefficient, noise_threshold, isnodeaps):

        self.interference(configstr, ppm, pos, txpowers,
                          fading_coefficient, noise_threshold, isnodeaps)

    def interference(self, configstr, ppm, pos, txpowers,
                     fading_coefficient, noise_threshold, isnodeaps):
        configstr += '\n\t];\n\tenable_interference = true;'
        configstr += '\n};\nmodel:\n{\n'
        configstr += '\ttype = "path_loss";\n\tpositions = ('
        first_pos = True
        for mappedpos in pos:
            posX = float(mappedpos.sta_pos[0])
            posY = float(mappedpos.sta_pos[1])
            posZ = float(mappedpos.sta_pos[2])
            if first_pos:
                first_pos = False
            else:
                configstr += ','
            configstr += '\n\t\t(%.1f, %.1f, %.1f)' % (
                posX, posY, posZ)
        configstr += '\n\t);\n\tfading_coefficient = %d;' % fading_coefficient
        configstr += '\n\tnoise_threshold = %d;' % noise_threshold
        configstr += '\n\tisnodeaps = ('
        first_isnodeap = True
        for isnodeap in isnodeaps:
            if first_isnodeap:
                configstr += '%s' % isnodeap
                first_isnodeap = False
            else:
                configstr += ', %s' % isnodeap
        configstr += ');\n\ttx_powers = ('
        first_txpower = True
        for mappedtxpower in txpowers:
            txpower = mappedtxpower.sta_txpower
            if first_txpower:
                configstr += '%s' % txpower
                first_txpower = False
            else:
                configstr += ', %s' % txpower
        if ppm.model == 'ITU':
            configstr += ');\n\tmodel_name = "itu";\n\tnFLOORS = %d;' \
                         '\n\tlF = %d;\n\tpL = %d;\n};' % \
                         (ppm.nFloors, ppm.lF, ppm.pL)
        elif ppm.model == 'logDistance':
            configstr += ');\n\tmodel_name = "log_distance";' \
                         '\n\tpath_loss_exp = %.1f;\n\txg = 0.0;\n};' \
                         % ppm.exp
        elif ppm.model == 'twoRayGround':
            configstr += ');\n\tmodel_name = "two_ray_ground";' \
                         '\n\tsL = %d;\n};' % ppm.sL
        elif ppm.model == 'logNormalShadowing':
            configstr += ');\n\tmodel_name = "log_normal_shadowing";' \
                         '\n\tpath_loss_exp = %.1f;\n\tsL = %d;\n};' \
                         % (ppm.exp, ppm.sL)
        else:
            configstr += ');\n\tmodel_name = "free_space";\n\tsL = %d;\n};' \
                         % ppm.sL
        WmediumdStarter.configstr = configstr


class WmediumdStarter(object):
    is_managed = False
    is_initialized = False
    is_connected = False
    wmd_process = None
    wmd_logfile = None
    wmd_config_name = None
    configstr = ''
    default_auto_errprob = 0.0
    default_auto_snr = -10

    @classmethod
    def start(cls, intfrefs=None, links=None, default_auto_snr=-10,
              default_auto_errprob=1.0, isnodeaps=None, fading_coefficient=0,
              noise_threshold=-91, pos=None, txpowers=None, ppm=None):
        """Set the data for the wmediumd daemon

        :param intfrefs: A list of all WmediumdIntfRef that should be managed
        in wmediumd
        :param links: A list of WmediumdLink
        :param parameters: Parameters to pass to the wmediumd executable
        :param default_auto_snr: The default SNR
        :param txpowers: list of txpowers
        :param pos: list of pos
        :param is nodeaps: check if the node is ap
        :param fading_coefficient: fading_coefficient
        :param ppm: propagation model"""

        kwargs = {}
        if intfrefs is None:
            intfrefs = []
        if links is None:
            links = []
        parameters = ['-l', '4', '-s']

        kwargs['intfrefs'] = intfrefs
        kwargs['links'] = links
        kwargs['pos'] = pos
        kwargs['txpowers'] = txpowers
        kwargs['isnodeaps'] = isnodeaps
        kwargs['parameters'] = parameters
        kwargs['default_auto_snr'] = default_auto_snr
        kwargs['default_auto_errprob'] = default_auto_errprob
        kwargs['fading_coefficient'] = fading_coefficient
        kwargs['noise_threshold'] = noise_threshold
        kwargs['ppm'] = ppm

        if wmediumd_mode.mode == 4:
            raise Exception("Wrong wmediumd mode given")
        cls.is_initialized = True
        cls.initialize(**kwargs)
        WmediumdServer.connect()

    @classmethod
    def initialize(cls, **kwargs):
        """
        Start wmediumd, this method should be called right after
        Mininet.configureWifiNodes()

        Notice: The stations can reach each other before this method is
        called and some scripts may use some kind of a cache (eg. iw station
        dump)
        ppm: propagation model class
        """
        if not cls.is_initialized:
            raise WmediumdException("Use WmediumdStarter.initialize first "
                                    "to set the required data")

        if cls.is_connected:
            raise WmediumdException("WmediumdStarter is already connected")

        mappedintf = {}
        mappedlinks = {}
        if wmediumd_mode.mode != WmediumdCst.INTERFERENCE_MODE:
            # Map all links using the interface id and check for missing
            # interfaces in the  intfrefs list
            for link in kwargs['links']:
                link_id = link.sta1intf.id() + '/' + link.sta2intf.id()
                mappedlinks[link_id] = link
                found1 = False
                found2 = False
                for intfref in kwargs['intfrefs']:
                    if link.sta1intf.get_station_name() == \
                            intfref.get_station_name():
                        if link.sta1intf.get_station_name() == \
                                intfref.get_station_name():
                            found1 = True
                    if link.sta2intf.get_station_name() == \
                            intfref.get_station_name():
                        if link.sta2intf.get_station_name() == \
                                intfref.get_station_name():
                            found2 = True
                if not found1:
                    raise WmediumdException('%s is not part of the managed '
                                            'interfaces'
                                            % link.sta1intf.id())
                    pass
                if not found2:
                    raise WmediumdException('%s is not part of the managed '
                                            'interfaces'
                                            % link.sta2intf.id())

        if wmediumd_mode.mode is not WmediumdCst.SPECPROB_MODE:
            if wmediumd_mode.mode is not WmediumdCst.INTERFERENCE_MODE:
                for intfref1 in kwargs['intfrefs']:
                    for intfref2 in kwargs['intfrefs']:
                        if intfref1 is not intfref2:
                            link_id = intfref1.id() + '/' + \
                                      intfref2.id()
                            if wmediumd_mode.mode == \
                                    WmediumdCst.ERRPROB_MODE:
                                mappedlinks.setdefault(
                                    link_id, WmediumdERRPROBLink(
                                        intfref1, intfref2,
                                        cls.default_auto_errprob))
                            else:
                                mappedlinks.setdefault(
                                    link_id, WmediumdSNRLink(
                                        intfref1, intfref2,
                                        cls.default_auto_snr))

            # Create wmediumd config
            wmd_config = tempfile.NamedTemporaryFile(
                prefix='mn_wmd_config_', suffix='.cfg', delete=False)
            cls.wmd_config_name = wmd_config.name
            debug("Name of wmediumd config: %s\n" % cls.wmd_config_name)
            configstr = 'ifaces:\n{\n\tids = [\n'
            intfref_id = 0
            for intfref in kwargs['intfrefs']:
                if intfref_id != 0:
                    configstr += ', \n'
                grepped_mac = intfref.get_mac()
                configstr += '\t\t"%s"' % grepped_mac
                mappedintf[intfref.id()] = intfref_id
                intfref_id += 1

            if wmediumd_mode.mode is WmediumdCst.INTERFERENCE_MODE:
                set_interference(configstr, kwargs['ppm'], kwargs['pos'],
                                 kwargs['txpowers'], kwargs['fading_coefficient'],
                                 kwargs['noise_threshold'], kwargs['isnodeaps'])
                configstr = cls.configstr
            else:
                configstr += '\n\t];\n};\nmodel:\n{\n\ttype = "'
                if wmediumd_mode.mode == WmediumdCst.ERRPROB_MODE:
                    configstr += 'prob'
                else:
                    configstr += 'snr'
                configstr += '";\n\tdefault_prob = 1.0;\n\tlinks = ('
                first_link = True
                for mappedlink in mappedlinks.values():
                    id1 = mappedlink.sta1intf.id()
                    id2 = mappedlink.sta2intf.id()
                    if first_link:
                        first_link = False
                    else:
                        configstr += ','
                    if wmediumd_mode.mode == WmediumdCst.ERRPROB_MODE:
                        configstr += '\n\t\t(%d, %d, %f)' % (
                            mappedintf[id1], mappedintf[id2],
                            mappedlink.errprob)
                    else:
                        configstr += '\n\t\t(%d, %d, %d)' % (
                            mappedintf[id1], mappedintf[id2],
                            mappedlink.snr)
                configstr += '\n\t);\n};'
            wmd_config.write(configstr.encode())
            wmd_config.close()
        # Start wmediumd using the created config
        cmdline = ['wmediumd']
        if wmediumd_mode.mode is WmediumdCst.SPECPROB_MODE:
            cmdline.append("-d")
        else:
            cmdline.append("-c")
            cmdline.append(cls.wmd_config_name)
            if wmediumd_mode.mode is WmediumdCst.SNR_MODE \
                    or wmediumd_mode.mode is WmediumdCst.INTERFERENCE_MODE:
                cmdline.append("-x")
                per_data_file = \
                    pkg_resources.resource_filename(
                        'mn_wifi', 'data/signal_table_ieee80211ax')
                cmdline.append(per_data_file)
        cmdline[1:1] = kwargs['parameters']
        cls.wmd_logfile = tempfile.NamedTemporaryFile(prefix='mn_wmd_log_',
                                                      suffix='.log',
                                                      delete=not cls.is_managed)
        debug("Name of wmediumd log: %s\n" % cls.wmd_logfile.name)
        if cls.is_managed:
            cmdline[0:0] = ["nohup"]

        cls.wmd_process = subprocess.Popen(cmdline, shell=False,
                                           stdout=cls.wmd_logfile,
                                           stderr=subprocess.STDOUT,
                                           preexec_fn=os.setpgrp)
        cls.is_connected = True

    @classmethod
    def start_managed(cls):
        """Start the connector in managed mode, which means disconnect and
        kill won't work, the process is kept running after the python
        process has finished"""
        cls.is_managed = True
        if not cls.is_initialized:
            cls.initialize()
        cls.start()

    @classmethod
    def stop(cls):
        "Kill the wmediumd process if running and delete the config"
        if cls.is_managed:
            error("\nCannot close WmediumdStarter in managed mode")
            return
        if cls.is_connected:
            cls.kill_wmediumd()
            try:
                cls.wmd_logfile.close()
            except OSError:
                pass
            try:
                os.remove(cls.wmd_config_name)
            except OSError:
                pass
            cls.is_connected = False
        else:
            raise WmediumdException('WmediumdStarter is not connected '
                                    'to wmediumd')

    @classmethod
    def kill_wmediumd(cls):
        if cls.is_managed:
            return
        if not cls.is_connected:
            raise WmediumdException('WmediumdStarter is not connected '
                                    'to wmediumd')
        try:
            # SIGINT to allow closing resources
            cls.wmd_process.send_signal(signal.SIGINT)
            time.sleep(0.5)
            # SIGKILL in case it did not finish
            cls.wmd_process.send_signal(signal.SIGKILL)
        except OSError:
            pass


class WmediumdPos(object):
    def __init__(self, staintf, sta_pos):
        """
        Describes the pos of a station

        :param sta_pos: Instance of WmediumdPosRef

        :type sta_pos: WmediumdPosRef
        """
        self.staintf = staintf
        self.sta_pos = sta_pos


class WmediumdTXPower(object):
    def __init__(self, staintf, sta_txpower):
        """
        Describes the Transmission Power of a station

        :param sta_txpower: Instance of WmediumdTXPowerRef

        :type sta_txpower: WmediumdTXPowerRef
        """
        self.staintf = staintf
        self.sta_txpower = sta_txpower


class WmediumdGain(object):
    'Gain'
    def __init__(self, staintf, sta_gain):
        """
        Describes the Antenna Gain of a station

        :param sta_gain: Instance of WmediumdGainRef

        :type sta_gain: WmediumdGainRef
        """
        self.staintf = staintf
        self.sta_gain = sta_gain


class WmediumdGRandom(object):
    'Gaussing Random'
    def __init__(self, staintf, sta_gaussian_random):
        """
        Describes the Gaussian Random of a node

        :param sta_gaussian_random: Instance of WmediumdGRandomRef

        :type sta_gaussian_random: WmediumdGRandomRef
        """
        self.staintf = staintf
        self.sta_gaussian_random = sta_gaussian_random


class WmediumdHeight(object):
    'Antenna Height'
    def __init__(self, staintf, sta_height):
        """
        Describes the Antenna Height of a station

        :param sta_height: Instance of WmediumdHeightRef

        :type sta_height: WmediumdHeightRef
        """
        self.staintf = staintf
        self.sta_height = sta_height


class WmediumdSNRLink(object):
    'SNR Link'
    def __init__(self, sta1intf, sta2intf, snr=10):
        """
        Describes a link between two interfaces using the SNR

        :param sta1intf: Instance of WmediumdIntfRef
        :param sta2intf: Instance of WmediumdIntfRef
        :param snr: Signal Noise Ratio as int

        :type sta1intf: WmediumdIntfRef
        :type sta2intf: WmediumdIntfRef
        :type snr: int
        """
        self.sta1intf = sta1intf
        self.sta2intf = sta2intf
        self.snr = snr


class WmediumdERRPROBLink(object):
    'ERRPROBLink'
    def __init__(self, sta1intf, sta2intf, errprob=0.2):
        """
        Describes a link between two interfaces using the error probability

        :param sta1intf: Instance of WmediumdIntfRef
        :param sta2intf: Instance of WmediumdIntfRef
        :param errprob: The error probability in the range [0.0;1.0]

        :type sta1intf: WmediumdIntfRef
        :type sta2intf: WmediumdIntfRef
        :type errprob: float
        """
        self.sta1intf = sta1intf
        self.sta2intf = sta2intf
        self.errprob = errprob


class WmediumdSPECPROBLink(object):
    'SPECPROB Link'
    def __init__(self, sta1intf, sta2intf, errprobs):
        """
        Describes a link between two interfaces using a matrix of error
        probabilities

        :param sta1intf: Instance of WmediumdIntfRef
        :param sta2intf: Instance of WmediumdIntfRef
        :param errprobs: The error probabilities in the range [0.0;1.0],
        the two dimensional array has as first
            dimension the packet size index and as the second the data
            rate index: errprobs[size_idx][rate_idx]

        :type sta1intf: WmediumdIntfRef
        :type sta2intf: WmediumdIntfRef
        :type errprobs: float[][]
        """
        self.sta1intf = sta1intf
        self.sta2intf = sta2intf
        self.errprobs = errprobs


class WmediumdIntfRef:
    'Intf Ref'
    def __init__(self, staname, intfname, intfmac):
        """
        An unambiguous reference to an interface of a station

        :param staname: Station name
        :param intfname: Interface name
        :param intfmac: Interface MAC address

        :type staname: str
        :type intfname: str
        :type intfmac: str
        """
        self.__staname = staname
        self.__intfname = intfname
        self.__intfmac = intfmac

    def get_station_name(self):
        """
        Get the name of the station

        :rtype: str
        """
        return self.__staname

    def get_intf_name(self):
        """
        Get the interface name

        :rtype: str
        """
        return self.__intfname

    def get_mac(self):
        """
        Get the MAC address of the interface

        :rtype: str
        """
        return self.__intfmac

    def id(self):
        """
        Id used in dicts

        :rtype: str
        """
        return self.get_station_name() + "." + self.get_intf_name()


class DynamicWmediumdIntfRef(WmediumdIntfRef):
    'Intf Ref'
    def __init__(self, sta, intf=None):
        """
        An unambiguous reference to an interface of a station

        :param sta: Mininet-Wifi station
        :param intf: Mininet interface or name of Mininet interface.
        If None, the default interface will be used

        :type sta: Station
        :type intf: Union [Intf, str, None]
        """
        WmediumdIntfRef.__init__(self, "", "", "")
        self.__sta = sta
        self.__intf = intf

    def get_station_name(self):
        return self.__sta.name

    def get_intf_name(self):
        if not self.__intf:
            return self.__sta.params['wlan'][0]
        elif isinstance(self.__intf, str):
            return self.__intf
        elif isinstance(self.__intf, int):
            return self.__sta.params['wlan'][self.__intf]

    def get_mac(self):
        intf_name = self.get_intf_name()
        index = 0
        found = False
        for wlan_intf in self.__sta.params['wlan']:
            if wlan_intf == intf_name:
                found = True
                break
            index += 1
        if found:
            return self.__sta.params['mac'][index]


class WmediumdServer(object):
    'Server Conn'
    __mac_struct_fmt = '6s'

    __base_struct_fmt = 'B'
    __base_struct = struct.Struct('!' + __base_struct_fmt)

    __snr_update_request_fmt = \
        __base_struct_fmt + __mac_struct_fmt + __mac_struct_fmt + 'i'
    __snr_update_response_fmt = \
        __base_struct_fmt + __snr_update_request_fmt + 'B'
    __snr_update_request_struct = \
        struct.Struct('!' + __snr_update_request_fmt)
    __snr_update_response_struct = \
        struct.Struct('!' + __snr_update_response_fmt)

    __pos_update_request_fmt = \
        __base_struct_fmt + __mac_struct_fmt + 'f' + 'f' + 'f'
    __pos_update_response_fmt = \
        __base_struct_fmt + __pos_update_request_fmt + 'B'
    __pos_update_request_struct = \
        struct.Struct('!' + __pos_update_request_fmt)
    __pos_update_response_struct = \
        struct.Struct('!' + __pos_update_response_fmt)

    __txpower_update_request_fmt = \
        __base_struct_fmt + __mac_struct_fmt + 'i'
    __txpower_update_response_fmt = \
        __base_struct_fmt + __txpower_update_request_fmt + 'B'
    __txpower_update_request_struct = \
        struct.Struct('!' + __txpower_update_request_fmt)
    __txpower_update_response_struct = \
        struct.Struct('!' + __txpower_update_response_fmt)

    __gain_update_request_fmt = \
        __base_struct_fmt + __mac_struct_fmt + 'i'
    __gain_update_response_fmt = \
        __base_struct_fmt + __gain_update_request_fmt + 'B'
    __gain_update_request_struct = \
        struct.Struct('!' + __gain_update_request_fmt)
    __gain_update_response_struct = \
        struct.Struct('!' + __gain_update_response_fmt)

    __gaussian_random_update_request_fmt = \
        __base_struct_fmt + __mac_struct_fmt + 'f'
    __gaussian_random_update_response_fmt = \
        __base_struct_fmt + __gaussian_random_update_request_fmt + 'B'
    __gaussian_random_update_request_struct = \
        struct.Struct('!' + __gaussian_random_update_request_fmt)
    __gaussian_random_update_response_struct = \
        struct.Struct('!' + __gaussian_random_update_response_fmt)

    __height_update_request_fmt = \
        __base_struct_fmt + __mac_struct_fmt + 'i'
    __height_update_response_fmt = \
        __base_struct_fmt + __height_update_request_fmt + 'B'
    __height_update_request_struct = \
        struct.Struct('!' + __height_update_request_fmt)
    __height_update_response_struct = \
        struct.Struct('!' + __height_update_response_fmt)

    __errprob_update_request_fmt = \
        __base_struct_fmt + __mac_struct_fmt + __mac_struct_fmt + 'i'
    __errprob_update_response_fmt = \
        __base_struct_fmt + __errprob_update_request_fmt + 'B'
    __errprob_update_request_struct = \
        struct.Struct('!' + __errprob_update_request_fmt)
    __errprob_update_response_struct = \
        struct.Struct('!' + __errprob_update_response_fmt)

    __specprob_update_request_fmt = \
        __base_struct_fmt + __mac_struct_fmt + __mac_struct_fmt + '144i'
    __specprob_update_response_fmt = \
        __base_struct_fmt + __mac_struct_fmt + __mac_struct_fmt + 'B'
    __specprob_update_request_struct = \
        struct.Struct('!' + __specprob_update_request_fmt)
    __specprob_update_response_struct = \
        struct.Struct('!' + __specprob_update_response_fmt)

    __station_del_by_mac_request_fmt = \
        __base_struct_fmt + __mac_struct_fmt
    __station_del_by_mac_response_fmt = \
        __base_struct_fmt + __station_del_by_mac_request_fmt + 'B'
    __station_del_by_mac_request_struct = \
        struct.Struct('!' + __station_del_by_mac_request_fmt)
    __station_del_by_mac_response_struct = \
        struct.Struct('!' + __station_del_by_mac_response_fmt)

    __station_del_by_id_request_fmt = __base_struct_fmt + 'i'
    __station_del_by_id_response_fmt = \
        __base_struct_fmt + __station_del_by_id_request_fmt + 'B'
    __station_del_by_id_request_struct = \
        struct.Struct('!' + __station_del_by_id_request_fmt)
    __station_del_by_id_response_struct = \
        struct.Struct('!' + __station_del_by_id_response_fmt)

    __station_add_request_fmt = __base_struct_fmt + __mac_struct_fmt
    __station_add_response_fmt = \
        __base_struct_fmt + __station_add_request_fmt + 'iB'
    __station_add_request_struct = \
        struct.Struct('!' + __station_add_request_fmt)
    __station_add_response_struct = \
        struct.Struct('!' + __station_add_response_fmt)

    sock = None
    connected = False

    @classmethod
    def connect(cls, uds_address=WmediumdCst.SOCKET_PATH):
        # type: (str) -> None
        """
        Connect to the wmediumd server
        :param uds_address: The UNIX domain socket
        """
        if cls.connected:
            raise WmediumdException("Already connected to wmediumd server")
        cls.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        info('*** Connecting to wmediumd server %s\n' % uds_address)
        time.sleep(1)
        cls.sock.connect(uds_address)
        cls.connected = True

    @classmethod
    def disconnect(cls):
        # type: () -> None
        """
        Disconnect from the wmediumd server
        """
        if not cls.connected:
            raise WmediumdException("Not yet connected to wmediumd server")
        cls.sock.close()
        cls.connected = False

    @classmethod
    def register_interface(cls, mac):
        # type: (str) -> int
        """
        Register a new interface at wmediumd
        :param mac The mac address of the interface
        :return The wmediumd station index

        :type mac: str
        :rtype int
        """
        info("\n%s Registering interface with mac %s"
             % (WmediumdCst.LOG_PREFIX, mac))
        ret, sta_id = WmediumdServer.send_add(mac)
        if ret != WmediumdCst.WUPDATE_SUCCESS:
            raise WmediumdException("Received error code from wmediumd: "
                                    "code %d" % ret)
        return sta_id

    @classmethod
    def unregister_interface(cls, mac):
        # type: (str) -> None
        """
        Unregister a station at wmediumd
        :param mac The mac address of the interface

        :type mac: str
        """
        info("\n%s Unregistering interface with mac %s"
             % (WmediumdCst.LOG_PREFIX, mac))
        ret = WmediumdServer.send_del_by_mac(mac)
        if ret != WmediumdCst.WUPDATE_SUCCESS:
            raise WmediumdException("Received error code from wmediumd: "
                                    "code %d" % ret)

    @classmethod
    def update_link_snr(cls, link):
        # type: (WmediumdSNRLink) -> None
        """
        Update the SNR of a connection at wmediumd
        :param link The link to update

        :type link: WmediumdLink
        """
        ret = WmediumdServer.send_snr_update(link)
        if ret != WmediumdCst.WUPDATE_SUCCESS:
            raise WmediumdException("Received error code from wmediumd: "
                                    "code %d" % ret)

    @classmethod
    def update_pos(cls, pos):
        # type: (WmediumdPos) -> None
        """
        Update the Pos of a connection at wmediumd
        :param pos The pos to update

        :type pos: WmediumdPos
        """
        ret = WmediumdServer.send_pos_update(pos)
        if ret != WmediumdCst.WUPDATE_SUCCESS:
            raise WmediumdException("Received error code from wmediumd: "
                                    "code %d" % ret)

    @classmethod
    def update_txpower(cls, txpower):
        # type: (WmediumdTXPower) -> None
        """
        Update the TXPower of a connection at wmediumd
        :param txpower The txpower to update

        :type txpower: WmediumdTXPower
        """
        ret = WmediumdServer.send_txpower_update(txpower)
        if ret != WmediumdCst.WUPDATE_SUCCESS:
            raise WmediumdException("Received error code from wmediumd: "
                                    "code %d" % ret)

    @classmethod
    def update_gain(cls, gain):
        # type: (WmediumdGain) -> None
        """
        Update the Antenna Gain of a connection at wmediumd
        :param gain The gain to update

        :type gain: WmediumdGain
        """
        ret = WmediumdServer.send_gain_update(gain)
        if ret != WmediumdCst.WUPDATE_SUCCESS:
            raise WmediumdException("Received error code from wmediumd: "
                                    "code %d" % ret)

    @classmethod
    def update_gaussian_random(cls, gRandom):
        # type: (WmediumdGRandom) -> None
        """
        Update the Gaussian Random of a connection at wmediumd
        :param gRandom The gRandom to update

        :type gRandom: WmediumdGRandom
        """
        ret = WmediumdServer.send_gaussian_random_update(gRandom)
        if ret != WmediumdCst.WUPDATE_SUCCESS:
            raise WmediumdException("Received error code from wmediumd: "
                                    "code %d" % ret)

    @classmethod
    def update_height(cls, height):
        # type: (WmediumdHeight) -> None
        """
        Update the Antenna Height of a connection at wmediumd
        :param height The height to update

        :type height: WmediumdHeight
        """
        ret = WmediumdServer.send_height_update(height)
        if ret != WmediumdCst.WUPDATE_SUCCESS:
            raise WmediumdException("Received error code from wmediumd: "
                                    "code %d" % ret)

    @classmethod
    def update_link_errprob(cls, link):
        # type: (WmediumdERRPROBLink) -> None
        """
        Update the ERRPROB of a connection at wmediumd
        :param link The link to update

        :type link: WmediumdLink
        """
        ret = WmediumdServer.send_errprob_update(link)
        if ret != WmediumdCst.WUPDATE_SUCCESS:
            raise WmediumdException("Received error code from wmediumd: "
                                    "code %d" % ret)

    @classmethod
    def update_link_specprob(cls, link):
        # type: (WmediumdSPECPROBLink) -> None
        """
        Update the SPECPROB of a connection at wmediumd
        :param link The link to update

        :type link: WmediumdLink
        """
        ret = WmediumdServer.send_specprob_update(link)
        if ret != WmediumdCst.WUPDATE_SUCCESS:
            raise WmediumdException("Received error code from wmediumd: "
                                    "code %d" % ret)

    @classmethod
    def send_snr_update(cls, link):
        # type: (WmediumdSNRLink) -> int
        """
        Send an update to the wmediumd server
        :param link: The WmediumdSNRLink to update
        :return: A WUPDATE_* constant
        """
        debug("%s Updating SNR from interface %s to interface %s to "
              "value %d\n" % (WmediumdCst.LOG_PREFIX,
                              link.sta1intf.get_mac(),
                              link.sta2intf.get_mac(), link.snr))
        cls.sock.send(cls.__create_snr_update_request(link))
        return cls.__parse_response(
            WmediumdCst.WSERVER_SNR_UPDATE_RESPONSE_TYPE,
            cls.__snr_update_response_struct)[-1]

    @classmethod
    def send_pos_update(cls, pos):
        # type: (WmediumdPos) -> int
        """
        Send an update to the wmediumd server
        :param pos: The WmediumdPos to update
        :return: A WUPDATE_* constant
        """
        posX = pos.sta_pos[0]
        posY = pos.sta_pos[1]
        posZ = pos.sta_pos[2]
        debug("%s Updating Pos of %s to x=%s, y=%s, z=%s\n" % (
            WmediumdCst.LOG_PREFIX, pos.staintf.get_mac(),
            posX, posY, posZ))
        cls.sock.send(cls.__create_pos_update_request(pos))
        return cls.__parse_response(
            WmediumdCst.WSERVER_POS_UPDATE_RESPONSE_TYPE,
            cls.__pos_update_response_struct)[-1]

    @classmethod
    def send_txpower_update(cls, txpower):
        # type: (WmediumdTXPower) -> int
        """
        Send an update to the wmediumd server
        :param txpower: The WmediumdTXPower to update
        :return: A WUPDATE_* constant
        """
        txpower_ = txpower.sta_txpower
        debug("%s Updating TxPower of %s to %d\n" % (
            WmediumdCst.LOG_PREFIX, txpower.staintf.get_mac(),
            txpower_))
        cls.sock.send(cls.__create_txpower_update_request(txpower))
        return cls.__parse_response(
            WmediumdCst.WSERVER_TXPOWER_UPDATE_RESPONSE_TYPE,
            cls.__txpower_update_response_struct)[-1]

    @classmethod
    def send_gain_update(cls, gain):
        # type: (WmediumdGain) -> int
        """
        Send an update to the wmediumd server
        :param gain: The WmediumdGain to update
        :return: A WUPDATE_* constant
        """
        gain_ = gain.sta_gain
        debug("%s Updating Antenna Gain of %s to %d\n" % (
            WmediumdCst.LOG_PREFIX, gain.staintf.get_mac(),
            gain_))
        cls.sock.send(cls.__create_gain_update_request(gain))
        return cls.__parse_response(
            WmediumdCst.WSERVER_GAIN_UPDATE_RESPONSE_TYPE,
            cls.__gain_update_response_struct)[-1]

    @classmethod
    def send_gaussian_random_update(cls, gRandom):
        # type: (WmediumdGRandom) -> int
        """
        Send an update to the wmediumd server
        :param gRandom: The WmediumdGRandom to update
        :return: A WUPDATE_* constant
        """
        gRandom_ = gRandom.sta_gaussian_random
        debug("%s Updating Gaussian Random of %s to %s\n" % (
            WmediumdCst.LOG_PREFIX, gRandom.staintf.get_mac(),
            gRandom_))
        cls.sock.send(cls.__create_gaussian_random_update_request(gRandom))
        return cls.__parse_response(
            WmediumdCst.WSERVER_GAUSSIAN_RANDOM_UPDATE_RESPONSE_TYPE,
            cls.__gaussian_random_update_response_struct)[-1]

    @classmethod
    def send_height_update(cls, height):
        # type: (WmediumdHeight) -> int
        """
        Send an update to the wmediumd server
        :param height: The WmediumdHeight to update
        :return: A WUPDATE_* constant
        """
        height_ = height.sta_height
        debug("%s Updating Antenna Height of %s to %d\n" % (
            WmediumdCst.LOG_PREFIX, height.staintf.get_mac(),
            height_))
        cls.sock.send(cls.__create_height_update_request(height))
        return cls.__parse_response(
            WmediumdCst.WSERVER_HEIGHT_UPDATE_RESPONSE_TYPE,
            cls.__height_update_response_struct)[-1]

    @classmethod
    def send_errprob_update(cls, link):
        # type: (WmediumdERRPROBLink) -> int
        """
        Send an update to the wmediumd server
        :param link: The WmediumdERRPROBLink to update
        :return: A WUPDATE_* constant
        """
        debug("\n%s Updating ERRPROB from interface %s to interface %s "
              "to value %f" % (
                  WmediumdCst.LOG_PREFIX, link.sta1intf.get_mac(),
                  link.sta2intf.get_mac(),
                  link.errprob))
        cls.sock.send(cls.__create_errprob_update_request(link))
        return cls.__parse_response(
            WmediumdCst.WSERVER_ERRPROB_UPDATE_RESPONSE_TYPE,
            cls.__errprob_update_response_struct)[-1]

    @classmethod
    def send_specprob_update(cls, link):
        # type: (WmediumdSPECPROBLink) -> int
        """
        Send an update to the wmediumd server
        :param link: The WmediumdSPECPROBLink to update
        :return: A WUPDATE_* constant
        """
        debug("\n%s Updating SPECPROB from interface %s to interface %s" % (
            WmediumdCst.LOG_PREFIX, link.sta1intf.get_mac(),
            link.sta2intf.get_mac()))
        cls.sock.send(cls.__create_specprob_update_request(link))
        return cls.__parse_response(
            WmediumdCst.WSERVER_SPECPROB_UPDATE_RESPONSE_TYPE,
            cls.__specprob_update_response_struct)[-1]

    @classmethod
    def send_del_by_mac(cls, mac):
        # type: (str) -> int
        """
        Send an update to the wmediumd server
        :param mac: The mac address of the interface to be deleted
        :return: A WUPDATE_* constant
        """
        cls.sock.send(cls.__create_station_del_by_mac_request(mac))
        return cls.__parse_response(
            WmediumdCst.WSERVER_DEL_BY_MAC_RESPONSE_TYPE,
            cls.__station_del_by_mac_response_struct)[-1]

    @classmethod
    def send_del_by_id(cls, sta_id):
        # type: (int) -> int
        """
        Send an update to the wmediumd server
        :param sta_id: The wmediumd index of the station
        :return: A WUPDATE_* constant
        """
        cls.sock.send(cls.__create_station_del_by_id_request(sta_id))
        return cls.__parse_response(
            WmediumdCst.WSERVER_DEL_BY_ID_RESPONSE_TYPE,
            cls.__station_del_by_id_response_struct)[-1]

    @classmethod
    def send_add(cls, mac):
        # type: (str) -> (int, int)
        """
        Send an update to the wmediumd server
        :param mac: The mac address of the new interface
        :return: A WUPDATE_* constant and on success at the second pos
        the index
        """
        cls.sock.send(cls.__create_station_add_request(mac))
        resp = cls.__parse_response(
            WmediumdCst.WSERVER_ADD_RESPONSE_TYPE,
            cls.__station_add_response_struct)
        return resp[-1], resp[-2]

    @classmethod
    def __create_snr_update_request(cls, link):
        "snr update request"
        # type: (WmediumdSNRLink) -> str
        msgtype = WmediumdCst.WSERVER_SNR_UPDATE_REQUEST_TYPE
        if py_version_info < (3, 0):
            mac_from = link.sta1intf.get_mac().replace(':', '').decode('hex')
            mac_to = link.sta2intf.get_mac().replace(':', '').decode('hex')
        else:
            mac_from = bytes.fromhex(link.sta1intf.get_mac().replace(':', ''))
            mac_to = bytes.fromhex(link.sta2intf.get_mac().replace(':', ''))
        snr = int(link.snr)
        return cls.__snr_update_request_struct.pack(msgtype, mac_from,
                                                    mac_to, snr)

    @classmethod
    def __create_pos_update_request(cls, pos):
        "pos update request"
        # type: (WmediumdPos) -> str
        msgtype = WmediumdCst.WSERVER_POS_UPDATE_REQUEST_TYPE
        if py_version_info < (3, 0):
            mac = pos.staintf.get_mac().replace(':', '').decode('hex')
        else:
            mac = bytes.fromhex(pos.staintf.get_mac().replace(':', ''))
        posX = pos.sta_pos[0]
        posY = pos.sta_pos[1]
        posZ = pos.sta_pos[2]
        return cls.__pos_update_request_struct.pack(msgtype, mac,
                                                    posX, posY, posZ)

    @classmethod
    def __create_txpower_update_request(cls, txpower):
        "tx power update request"
        # type: (WmediumdTXPower) -> str
        msgtype = WmediumdCst.WSERVER_TXPOWER_UPDATE_REQUEST_TYPE
        if py_version_info < (3, 0):
            mac = txpower.staintf.get_mac().replace(':', '').decode('hex')
        else:
            mac = bytes.fromhex((txpower.staintf.get_mac().replace(':', '')))
        txpower_ = txpower.sta_txpower
        return cls.__txpower_update_request_struct.pack(msgtype, mac, txpower_)

    @classmethod
    def __create_gain_update_request(cls, gain):
        "antenna gain update request"
        # type: (WmediumdGain) -> str
        msgtype = WmediumdCst.WSERVER_GAIN_UPDATE_REQUEST_TYPE
        if py_version_info < (3, 0):
            mac = gain.staintf.get_mac().replace(':', '').decode('hex')
        else:
            mac = bytes.fromhex((gain.staintf.get_mac().replace(':', '')))
        gain_ = gain.sta_gain
        return cls.__gain_update_request_struct.pack(msgtype, mac, gain_)

    @classmethod
    def __create_gaussian_random_update_request(cls, gRandom):
        "gaussian random update request"
        # type: (WmediumdGRandom) -> str
        msgtype = WmediumdCst.WSERVER_GAUSSIAN_RANDOM_UPDATE_REQUEST_TYPE
        if py_version_info < (3, 0):
            mac = gRandom.staintf.get_mac().replace(':', '').decode('hex')
        else:
            mac = bytes.fromhex((gRandom.staintf.get_mac().replace(':', '')))
        gRandom_ = gRandom.sta_gaussian_random
        return cls.__gaussian_random_update_request_struct.pack(msgtype, mac,
                                                                gRandom_)

    @classmethod
    def __create_height_update_request(cls, height):
        "height update request"
        # type: (WmediumdHeight) -> str
        msgtype = WmediumdCst.WSERVER_HEIGHT_UPDATE_REQUEST_TYPE
        if py_version_info < (3, 0):
            mac = height.staintf.get_mac().replace(':', '').decode('hex')
        else:
            mac = bytes.fromhex((height.staintf.get_mac().replace(':', '')))
        height_ = height.sta_height
        return cls.__height_update_request_struct.pack(msgtype, mac, height_)

    @classmethod
    def __create_errprob_update_request(cls, link):
        "error prob update request"
        # type: (WmediumdERRPROBLink) -> str
        msgtype = WmediumdCst.WSERVER_ERRPROB_UPDATE_REQUEST_TYPE
        if py_version_info < (3, 0):
            mac_from = link.sta1intf.get_mac().replace(':', '').decode('hex')
            mac_to = link.sta2intf.get_mac().replace(':', '').decode('hex')
        else:
            mac_from = bytes.fromhex((link.sta1intf.get_mac().replace(':', '')))
            mac_to = bytes.fromhex((link.sta2intf.get_mac().replace(':', '')))
        errprob = cls.__conv_float_to_fixed_point(link.errprob)
        return cls.__errprob_update_request_struct.pack(msgtype, mac_from, mac_to,
                                                        errprob)

    @classmethod
    def __create_specprob_update_request(cls, link):
        "specprob update request"
        # type: (WmediumdSPECPROBLink) -> str
        msgtype = WmediumdCst.WSERVER_SPECPROB_UPDATE_REQUEST_TYPE
        if py_version_info < (3, 0):
            mac_from = link.sta1int.get_mac().replace(':', '').decode('hex')
            mac_to = link.sta2intf.get_mac().replace(':', '').decode('hex')
        else:
            mac_from = bytes.fromhex((link.sta1intf.get_mac().replace(':', '')))
            mac_to = bytes.fromhex((link.sta2intf.get_mac().replace(':', '')))
        fixed_points = [None] * 144
        for size_idx in range(0, 12):
            for rate_idx in range(0, 12):
                fixed_points[size_idx * 12 + rate_idx] = \
                    cls.__conv_float_to_fixed_point(
                        link.errprobs[size_idx][rate_idx])
        return cls.__specprob_update_request_struct.pack(msgtype, mac_from,
                                                         mac_to, *fixed_points)

    @classmethod
    def __create_station_del_by_mac_request(cls, mac):
        "del station by mac"
        # type: (str) -> str
        msgtype = WmediumdCst.WSERVER_DEL_BY_MAC_REQUEST_TYPE
        macparsed = mac.replace(':', '').decode('hex')
        return cls.__station_del_by_mac_request_struct.pack(msgtype, macparsed)

    @classmethod
    def __create_station_del_by_id_request(cls, sta_id):
        "del station by id"
        # type: (int) -> str
        msgtype = WmediumdCst.WSERVER_DEL_BY_ID_REQUEST_TYPE
        return cls.__station_del_by_id_request_struct.pack(msgtype, sta_id)

    @classmethod
    def __create_station_add_request(cls, mac):
        "add station"
        # type: (str) -> str
        msgtype = WmediumdCst.WSERVER_ADD_REQUEST_TYPE
        macparsed = mac.replace(':', '').decode('hex')
        return cls.__station_add_request_struct.pack(msgtype, macparsed)

    @classmethod
    def __parse_response(cls, expected_type, resp_struct):
        "parse response"
        # type: (int, struct.Struct) -> tuple
        recvd_data = cls.sock.recv(resp_struct.size)
        # recvd_type = cls.__base_struct.unpack(recvd_data[0])[0]
        # if recvd_type != expected_type:
        #    raise WmediumdException(
        #        "Received response of unknown type %d, expected %d" % (recvd_type,
        # expected_type))
        return resp_struct.unpack(recvd_data)

    @classmethod
    def __conv_float_to_fixed_point(cls, d):
        shift_amount = 31
        one_shifted = ctypes.c_int32(1 << 31).value
        beforecomma = int(d)
        aftercomma = int(((d - beforecomma) * one_shifted))
        return ctypes.c_int32(beforecomma << shift_amount).value + aftercomma
