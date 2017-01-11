"""
Helps starting the wmediumd service

author: Patrick Grosse (patrick.grosse@uni-muenster.de)
"""

import os
import pkg_resources
import socket
import tempfile
import subprocess
import signal
import time
import struct
import stat

from mininet.log import info, error
from wifiModule import module


class WmediumdConstants:
    WSERVER_SHUTDOWN_REQUEST_TYPE = 0
    WSERVER_UPDATE_REQUEST_TYPE = 1
    WSERVER_UPDATE_RESPONSE_TYPE = 2
    WSERVER_DEL_BY_MAC_REQUEST_TYPE = 3
    WSERVER_DEL_BY_MAC_RESPONSE_TYPE = 4
    WSERVER_DEL_BY_ID_REQUEST_TYPE = 5
    WSERVER_DEL_BY_ID_RESPONSE_TYPE = 6
    WSERVER_ADD_REQUEST_TYPE = 7
    WSERVER_ADD_RESPONSE_TYPE = 8

    WUPDATE_SUCCESS = 0
    WUPDATE_INTF_NOTFOUND = 1
    WUPDATE_INTF_DUPLICATE = 2

    SOCKET_PATH = '/var/run/wmediumd.sock'


class WmediumdException(Exception):
    pass


class WmediumdConnector(object):
    __LOG_PREFIX = "wmediumd:"

    is_connected = False
    registered_interfaces = []

    @classmethod
    def connect(cls, uds_address=WmediumdConstants.SOCKET_PATH):
        is_started = False
        h = int(subprocess.check_output("pgrep -x \'wmediumd\' | wc -l",
                                        shell=True))
        if h > 0:
            if os.path.exists(uds_address) and stat.S_ISSOCK(os.stat(uds_address).st_mode):
                is_started = True
            else:
                raise WmediumdException("wmediumd is already started but without server")
        if not is_started:
            WmediumdStarter.initialize(parameters=['-l', '5'])
            WmediumdStarter.start_managed()
            time.sleep(1)
        WmediumdServerConn.connect(uds_address)
        cls.is_connected = True

    @classmethod
    def disconnect(cls):
        if not cls.is_connected:
            raise WmediumdException("WmediumdConnector is not connected to wmediumd")
        for mac in cls.registered_interfaces:
            cls.unregister_interface(mac)
        cls.is_connected = False

    @classmethod
    def register_interface(cls, mac):
        # type: (str) -> int
        """

        :type mac: str
        :rtype int
        """
        info("\n%s Registering interface with mac %s" % (cls.__LOG_PREFIX, mac))
        ret, sta_id = WmediumdServerConn.send_add(mac)
        if ret != WmediumdConstants.WUPDATE_SUCCESS:
            raise WmediumdException("Received error code from wmediumd: code %d" % ret)
        return sta_id

    @classmethod
    def unregister_interface(cls, mac):
        # type: (str) -> None
        """

        :type mac: str
        """
        info("\n%s Unregistering interface with mac %s" % (cls.__LOG_PREFIX, mac))
        ret = WmediumdServerConn.send_del_by_mac(mac)
        if ret != WmediumdConstants.WUPDATE_SUCCESS:
            raise WmediumdException("Received error code from wmediumd: code %d" % ret)

    @classmethod
    def update_link(cls, link):
        # type: (WmediumdLink) -> None
        """

        :type link: WmediumdLink
        """
        ret = WmediumdServerConn.send_update(link)
        if ret != WmediumdConstants.WUPDATE_SUCCESS:
            raise WmediumdException("Received error code from wmediumd: code %d" % ret)


class WmediumdStarter(object):
    is_managed = False

    is_initialized = False
    intfrefs = None
    links = None
    executable = None
    parameters = None
    auto_add_links = None
    default_auto_snr = None

    is_connected = False
    wmd_process = None
    wmd_config_name = None
    wmd_logfile = None

    @classmethod
    def initialize(cls, intfrefs=None, links=None, executable='wmediumd', with_server=True, parameters=None,
                   auto_add_links=True, default_auto_snr=-10):
        """
        Set the data for the wmediumd daemon

        :param intfrefs: A list of all WmediumdIntfRef that should be managed in wmediumd
        :param links: A list of WmediumdLink
        :param executable: The wmediumd executable
        :param with_server: True if the wmediumd server should be started
        :param parameters: Parameters to pass to the wmediumd executable
        :param auto_add_links: If true, it will add all missing links pairs with the default_auto_snr as SNR
        :param default_auto_snr: The default SNR

        :type intfrefs: list of WmediumdIntfRef
        :type links: list of WmediumdLink
        """
        if intfrefs is None:
            intfrefs = []
        if links is None:
            links = []
        if parameters is None:
            parameters = ['-l', '4']
        if with_server:
            parameters.append('-s')
        cls.intfrefs = intfrefs
        cls.links = links
        cls.executable = executable
        cls.parameters = parameters
        cls.auto_add_links = auto_add_links
        cls.default_auto_snr = default_auto_snr
        cls.is_initialized = True

    @classmethod
    def connect_wmediumd_on_startup(cls):
        """
        This method can be called before initializing the Mininet net
        to prevent the stations from reaching each other from the beginning.

        Alternative: Call connect_wmediumd_after_startup() when the net is built
        """
        if not cls.is_initialized:
            raise WmediumdException("Use WmediumdStarter.initialize() first to set the required data")

        if cls.is_connected:
            raise WmediumdException("WmediumdStarter is already connected")

        orig_ifaceassign = module.assignIface

        @classmethod
        def intercepted_assign(other_cls, wifiNodes, physicalWlan, phyList, **params):
            orig_ifaceassign(wifiNodes, physicalWlan, phyList, **params)
            cls.connect_wmediumd_after_startup()

        module.assignIface = intercepted_assign

    @classmethod
    def connect_wmediumd_after_startup(cls):
        """
        This method can be called after initializing the Mininet net
        to prevent the stations from reaching each other.

        The stations can reach each other before this method is called and
        some scripts may use some kind of a cache (eg. iw station dump)

        Alternative: Call intercept_module_loading() before the net is built
        """
        if not cls.is_initialized:
            raise WmediumdException("Use WmediumdStarter.initialize first to set the required data")

        if cls.is_connected:
            raise WmediumdException("WmediumdStarter is already connected")

        mappedintfrefs = {}
        mappedlinks = {}

        # Map all links using the interface identifier and check for missing interfaces in the  intfrefs list
        for link in cls.links:
            link_id = link.sta1intfref.identifier() + '/' + link.sta2intfref.identifier()
            mappedlinks[link_id] = link
            found1 = False
            found2 = False
            for intfref in cls.intfrefs:
                if link.sta1intfref.get_station_name() == intfref.get_station_name():
                    if link.sta1intfref.get_station_name() == intfref.get_station_name():
                        found1 = True
                if link.sta2intfref.get_station_name() == intfref.get_station_name():
                    if link.sta2intfref.get_station_name() == intfref.get_station_name():
                        found2 = True
            if not found1:
                raise WmediumdException('%s is not part of the managed interfaces' % link.sta1intfref.identifier())
                pass
            if not found2:
                raise WmediumdException('%s is not part of the managed interfaces' % link.sta2intfref.identifier())

        # Auto add links
        if cls.auto_add_links:
            for intfref1 in cls.intfrefs:
                for intfref2 in cls.intfrefs:
                    if intfref1 != intfref2:
                        link_id = intfref1.identifier() + '/' + intfref2.identifier()
                        mappedlinks.setdefault(link_id, WmediumdLink(intfref1, intfref2, cls.default_auto_snr))

        # Create wmediumd config
        wmd_config = tempfile.NamedTemporaryFile(prefix='mn_wmd_config_', suffix='.cfg', delete=False)
        cls.wmd_config_name = wmd_config.name
        info("Name of wmediumd config: %s\n" % cls.wmd_config_name)
        configstr = 'ifaces :\n{\n\tids = ['
        intfref_id = 0
        for intfref in cls.intfrefs:
            if intfref_id != 0:
                configstr += ', '
            grepped_mac = intfref.get_intf_mac()
            configstr += '"%s"' % grepped_mac
            mappedintfrefs[intfref.identifier()] = intfref_id
            intfref_id += 1
        configstr += '];\n\tlinks = ('
        first_link = True
        for mappedlink in mappedlinks.itervalues():
            id1 = mappedlink.sta1intfref.identifier()
            id2 = mappedlink.sta2intfref.identifier()
            if first_link:
                first_link = False
            else:
                configstr += ','
            configstr += '\n\t\t(%d, %d, %d)' % (
                mappedintfrefs[id1], mappedintfrefs[id2],
                mappedlink.snr)
        configstr += '\n\t);\n};'
        wmd_config.write(configstr)
        wmd_config.close()

        # Start wmediumd using the created config
        per_data_file = pkg_resources.resource_filename('mininet', 'data/signal_table_ieee80211ax')
        cmdline = [cls.executable, "-c", cls.wmd_config_name, "-x", per_data_file]
        cmdline[1:1] = cls.parameters
        cls.wmd_logfile = tempfile.NamedTemporaryFile(prefix='mn_wmd_log_', suffix='.log', delete=not cls.is_managed)
        info("Name of wmediumd log: %s\n" % cls.wmd_logfile.name)
        if cls.is_managed:
            cmdline[0:0] = ["nohup"]
        cls.wmd_process = subprocess.Popen(cmdline, shell=False, stdout=cls.wmd_logfile,
                                           stderr=subprocess.STDOUT, preexec_fn=os.setpgrp)
        cls.is_connected = True

    @classmethod
    def start_managed(cls):
        """
        Start the connector in managed mode, which means disconnect and kill won't work,
        the process is kept running after the python process has finished
        """
        cls.is_managed = True
        if not cls.is_initialized:
            cls.initialize()
        cls.connect_wmediumd_after_startup()

    @classmethod
    def close(cls):
        """
        Kill the wmediumd process if running and delete the config
        """
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
            raise WmediumdException('WmediumdStarter is not connected to wmediumd')

    @classmethod
    def kill_wmediumd(cls):
        if cls.is_managed:
            return
        if not cls.is_connected:
            raise WmediumdException('WmediumdStarter is not connected to wmediumd')
        try:
            # SIGINT to allow closing resources
            cls.wmd_process.send_signal(signal.SIGINT)
            time.sleep(0.5)
            # SIGKILL in case it did not finish
            cls.wmd_process.send_signal(signal.SIGKILL)
        except OSError:
            pass


# backwards compatibility
class WmediumdConn(WmediumdStarter):
    @classmethod
    def set_wmediumd_data(cls, intfrefs=None, links=None, executable='wmediumd', with_server=True, parameters=None,
                          auto_add_links=True, default_auto_snr=0):
        cls.initialize(intfrefs, links, executable, with_server, parameters,
                       auto_add_links, default_auto_snr)

    @classmethod
    def disconnect_wmediumd(cls):
        cls.close()


class WmediumdLink(object):
    def __init__(self, sta1intfref, sta2intfref, snr=10):
        """
        Describes a link between two interfaces using the SNR

        :param sta1intfref: Instance of WmediumdIntfRef
        :param sta2intfref: Instance of WmediumdIntfRef
        :param snr: Signal Noise Ratio as int

        :type sta1intfref: WmediumdIntfRef
        :type sta2intfref: WmediumdIntfRef
        :type snr: int
        """
        self.sta1intfref = sta1intfref
        self.sta2intfref = sta2intfref
        self.snr = snr


class WmediumdIntfRef(object):
    def get_station_name(self):
        """
        Get the name of the station

        :rtype: str
        """
        pass

    def get_intf_name(self):
        """
        Get the interface name

        :rtype: str
        """
        pass

    def get_intf_mac(self):
        """
        Get the MAC address of the interface

        :rtype: str
        """
        pass

    def identifier(self):
        """
        Identifier used in dicts

        :return: str
        """
        return self.get_station_name() + "." + self.get_intf_name()


class DynamicWmediumdIntfRef(WmediumdIntfRef):
    def __init__(self, sta, intf=None):
        """
        An unambiguous reference to an interface of a station

        :param sta: Mininet-Wifi station
        :param intf: Mininet interface or name of Mininet interface. If None, the default interface will be used

        :type sta: Station
        :type intf: (Intf, str, None)
        """
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

    def get_intf_mac(self):
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


class StaticWmediumdIntfRef(WmediumdIntfRef):
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
        return self.__staname

    def get_intf_name(self):
        return self.__intfname

    def get_intf_mac(self):
        return self.__intfmac


class WmediumdServerConn(object):
    __mac_struct_fmt = '6s2s'  # padding
    __mac_padding = "\0\0"

    __snr_update_request_fmt = __mac_struct_fmt + __mac_struct_fmt + 'i'
    __snr_update_response_fmt = __snr_update_request_fmt + 'i'
    __snr_update_request_struct = struct.Struct('!i' + __snr_update_request_fmt)
    __snr_update_response_struct = struct.Struct('!ii' + __snr_update_response_fmt)

    __station_del_by_mac_request_fmt = __mac_struct_fmt
    __station_del_by_mac_response_fmt = __station_del_by_mac_request_fmt + 'i'
    __station_del_by_mac_request_struct = struct.Struct('!i' + __station_del_by_mac_request_fmt)
    __station_del_by_mac_response_struct = struct.Struct('!ii' + __station_del_by_mac_response_fmt)

    __station_del_by_id_request_fmt = 'i'
    __station_del_by_id_response_fmt = __station_del_by_id_request_fmt + 'i'
    __station_del_by_id_request_struct = struct.Struct('!i' + __station_del_by_id_request_fmt)
    __station_del_by_id_response_struct = struct.Struct('!ii' + __station_del_by_id_response_fmt)

    __station_add_request_fmt = __mac_struct_fmt
    __station_add_response_fmt = __station_add_request_fmt + 'ii'
    __station_add_request_struct = struct.Struct('!i' + __station_add_request_fmt)
    __station_add_response_struct = struct.Struct('!ii' + __station_add_response_fmt)

    __base_type_struct = struct.Struct('!i')

    sock = None
    connected = False

    @classmethod
    def connect(cls, uds_address=WmediumdConstants.SOCKET_PATH):
        # type: (str) -> None
        """
        Connect to the wmediumd server
        :param uds_address: The UNIX domain socket
        """
        if cls.connected:
            raise WmediumdException("Already connected to wmediumd server")
        cls.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        print '*** Connecting to wmediumd server %s' % uds_address
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
    def send_update(cls, link):
        # type: (WmediumdLink) -> int
        """
        Send an update to the wmediumd server
        :param link: The WmediumdLink to update
        :return: A WUPDATE_* constant
        """
        cls.sock.send(cls.__create_update_request(link))
        return cls.__parse_response(WmediumdConstants.WSERVER_UPDATE_RESPONSE_TYPE,
                                    cls.__snr_update_response_struct)[-1]

    @classmethod
    def send_del_by_mac(cls, mac):
        # type: (str) -> int
        """
        Send an update to the wmediumd server
        :param mac: The mac address of the interface to be deleted
        :return: A WUPDATE_* constant
        """
        cls.sock.send(cls.__create_station_del_by_mac_request(mac))
        return cls.__parse_response(WmediumdConstants.WSERVER_DEL_BY_MAC_RESPONSE_TYPE,
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
        return cls.__parse_response(WmediumdConstants.WSERVER_DEL_BY_ID_RESPONSE_TYPE,
                                    cls.__station_del_by_id_response_struct)[-1]

    @classmethod
    def send_add(cls, mac):
        # type: (str) -> (int, int)
        """
        Send an update to the wmediumd server
        :param mac: The mac address of the new interface
        :return: A WUPDATE_* constant and on success at the second position the index
        """
        cls.sock.send(cls.__create_station_add_request(mac))
        resp = cls.__parse_response(WmediumdConstants.WSERVER_ADD_RESPONSE_TYPE,
                                    cls.__station_add_response_struct)
        return resp[-1], resp[-2]

    @classmethod
    def __create_update_request(cls, link):
        # type: (WmediumdLink) -> str
        msgtype = WmediumdConstants.WSERVER_UPDATE_REQUEST_TYPE
        mac_from = link.sta1intfref.get_intf_mac().replace(':', '').decode('hex')
        mac_to = link.sta2intfref.get_intf_mac().replace(':', '').decode('hex')
        snr = link.snr
        return cls.__snr_update_request_struct.pack(msgtype, mac_from, cls.__mac_padding, mac_to, cls.__mac_padding,
                                                    snr)

    @classmethod
    def __create_station_del_by_mac_request(cls, mac):
        # type: (str) -> str
        msgtype = WmediumdConstants.WSERVER_DEL_BY_MAC_REQUEST_TYPE
        macparsed = mac.replace(':', '').decode('hex')
        return cls.__station_del_by_mac_request_struct.pack(msgtype, macparsed, cls.__mac_padding)

    @classmethod
    def __create_station_del_by_id_request(cls, sta_id):
        # type: (int) -> str
        msgtype = WmediumdConstants.WSERVER_DEL_BY_ID_REQUEST_TYPE
        return cls.__station_del_by_id_request_struct.pack(msgtype, sta_id)

    @classmethod
    def __create_station_add_request(cls, mac):
        # type: (str) -> str
        msgtype = WmediumdConstants.WSERVER_ADD_REQUEST_TYPE
        macparsed = mac.replace(':', '').decode('hex')
        return cls.__station_add_request_struct.pack(msgtype, macparsed, cls.__mac_padding)

    @classmethod
    def __parse_response(cls, expected_type, resp_struct):
        # type: (int, struct.Struct) -> tuple
        recvd_data = cls.sock.recv(resp_struct.size)
        recvd_type = cls.__base_type_struct.unpack(recvd_data[0:4])[0]
        if recvd_type != expected_type:
            raise WmediumdException(
                "Received response of unknown type %d, expected %d" % (recvd_type, expected_type))
        return resp_struct.unpack(recvd_data)
