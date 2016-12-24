"""
Helps starting the wmediumd service

author: Patrick Grosse (patrick.grosse@uni-muenster.de)
"""

import os
import socket
import tempfile
import subprocess

import signal

import time

import struct

from link import Intf
from mininet.log import info
from wifiModule import module


class WmediumdConn(object):
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
    def set_wmediumd_data(cls, intfrefs, links, executable='wmediumd', with_server=False, parameters=None,
                          auto_add_links=True,
                          default_auto_snr=0):
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

        Alternative: Call connect_wmediumd() when the net is built
        """
        if not cls.is_initialized:
            raise Exception("Use set_wmediumd_data first to set the required data")

        if cls.is_connected:
            raise Exception('wmediumd is already initialized')

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
            raise Exception("Use set_wmediumd_data first to set the required data")

        if cls.is_connected:
            raise Exception('wmediumd is already initialized')

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
                raise Exception('%s is not part of the managed interfaces' % link.sta1intfref.identifier())
                pass
            if not found2:
                raise Exception('%s is not part of the managed interfaces' % link.sta2intfref.identifier())

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
        cls.wmd_logfile = tempfile.NamedTemporaryFile(prefix='mn_wmd_log_', suffix='.log')
        info("Name of wmediumd log: %s\n" % cls.wmd_logfile.name)
        cmdline = [cls.executable, "-c", cls.wmd_config_name]
        cmdline[1:1] = cls.parameters
        cls.wmd_process = subprocess.Popen(cmdline, shell=False, stdout=cls.wmd_logfile,
                                           stderr=subprocess.STDOUT, preexec_fn=prevent_sig_passtrough)
        cls.is_connected = True

    @classmethod
    def disconnect_wmediumd(cls):
        """
        Kill the wmediumd process if running and delete the config
        """
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
            raise Exception('wmediumd is not started')

    @classmethod
    def kill_wmediumd(cls):
        try:
            # SIGINT to allow closing resources
            cls.wmd_process.send_signal(signal.SIGINT)
            time.sleep(0.5)
            # SIGKILL in case it did not finish
            cls.wmd_process.send_signal(signal.SIGKILL)
        except OSError:
            pass


class WmediumdLink(object):
    def __init__(self, sta1intfref, sta2intfref, snr=10):
        """
        Describes a link between two interfaces using the SNR

        :param sta1intfref: Instance of WmediumdIntfRef
        :param sta2intfref: Instance of WmediumdIntfRef
        :param snr: Signal Noise Ratio as int

        :type sta1intfref: WmediumdIntfRef
        :type sta2intfref: WmediumdIntfRef
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
        if isinstance(self.__intf, Intf):
            return self.__intf.name
        elif not self.__intf:
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


def prevent_sig_passtrough():
    """
    preexec_fn for Popen to prevent signals being passed through
    """
    os.setpgrp()


class WmediumdServerConn(object):
    __mac_struct_fmt = '6s'
    __snr_update_request_struct = struct.Struct('!' + __mac_struct_fmt + __mac_struct_fmt + 'B')
    __snr_update_response_struct = struct.Struct(__snr_update_request_struct.format + 'B')
    sock = None
    connected = False

    @classmethod
    def connect(cls, uds_address='/tmp/wserver_soc'):
        # type: (str) -> None
        """
        Connect to the wmediumd server
        :param uds_address: The UNIX domain socket
        """
        if cls.connected:
            raise Exception("Already connected to wmediumd server")
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
            raise Exception("Not yet connected to wmediumd server")
        cls.sock.close()
        cls.connected = False

    @classmethod
    def send_update(cls, link):
        # type: (WmediumdLink) -> int
        """
        Send an update to the wmediumd server
        :param link: The WmediumdLink to update
        :return: 0 for success, 1 if the interface was not found, 2 the SNR is invalid
        """
        cls.sock.send(cls.__create_update_request(link))
        return cls.sock.recv(cls.__snr_update_response_struct.size)

    @classmethod
    def __create_update_request(cls, link):
        # type: (WmediumdLink) -> str
        mac_from = link.sta1intfref.get_intf_mac().replace(':', '').decode('hex')
        mac_to = link.sta2intfref.get_intf_mac().replace(':', '').decode('hex')
        snr = link.snr
        return cls.__snr_update_request_struct.pack(mac_from, mac_to, snr)

    @classmethod
    def __parse_update_response(cls, recv_bytes):
        # type: (str) -> int
        tup = cls.__snr_update_response_struct.unpack(recv_bytes)
        return tup[-1]
