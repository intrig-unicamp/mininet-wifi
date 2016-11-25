"""
Helps starting the wmediumd service

author: Patrick Grosse (patrick.grosse@uni-muenster.de)
"""

import os
import subprocess
import tempfile
from mininet.log import info


class WmediumdConn(object):
    is_connected = False
    used_tmux = False
    wmd_process = None
    wmd_config_name = None

    @classmethod
    def connect_wmediumd(cls, intfrefs, links, executable='wmediumd', auto_add_links=True,
                         default_auto_snr=0, use_tmux=False):
        """
        Start the wmediumd process

        :param intfrefs: A list of all WmediumdIntfRef that should be managed in wmediumd
        :param links: A list of WmediumdLink
        :param executable: The wmediumd executable
        :param auto_add_links: If true, it will add all missing links pairs with the default_auto_snr as SNR
        :param default_auto_snr: The default SNR
        :param use_tmux: True if a tmux session should be used (for debugging)
        """
        if cls.is_connected:
            raise Exception('wmediumd is already initialized')
        cls.used_tmux = use_tmux

        mappedintfrefs = {}
        mappedlinks = {}

        # Map all links using the interface identifier and check for missing interfaces in the  intfrefs list
        for link in links:
            link_id = link.sta1intfref.identifier() + '/' + link.sta2intfref.identifier()
            mappedlinks[link_id] = link
            found1 = False
            found2 = False
            for intfref in intfrefs:
                if link.sta1intfref.sta == intfref.sta and link.sta1intfref.sta == intfref.sta:
                    found1 = True
                if link.sta2intfref.sta == intfref.sta and link.sta2intfref.sta == intfref.sta:
                    found2 = True
            if not found1:
                raise Exception('%s is not part of the managed interfaces' % link.sta1intfref.identifier())
                pass
            if not found2:
                raise Exception('%s is not part of the managed interfaces' % link.sta2intfref.identifier())

        # Auto add links
        if auto_add_links:
            for intfref1 in intfrefs:
                for intfref2 in intfrefs:
                    if intfref1 != intfref2:
                        link_id = intfref1.identifier() + '/' + intfref2.identifier()
                        mappedlinks.setdefault(link_id, WmediumdLink(intfref1, intfref2, default_auto_snr))

        # Create wmediumd config
        wmd_config = tempfile.NamedTemporaryFile(prefix='mn_wmd_config_', suffix='.cfg', delete=False)
        cls.wmd_config_name = wmd_config.name
        info("Name of wmediumd config: %s\n" % cls.wmd_config_name)
        configstr = 'ifaces :\n{\n\tids = ['
        intfref_id = 0
        for intfref in intfrefs:
            if intfref_id != 0:
                configstr += ', '
            grepped_mac = cls.grep_mac(intfref.sta, intfref.intf)
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
        configstr += '\n\t);\n}'
        wmd_config.write(configstr)
        wmd_config.close()

        # Start wmediumd using the created config
        if use_tmux:
            os.system('tmux new -s mnwmd -d')
            os.system('tmux send-keys \'%s -c %s\' C-m' % (executable, cls.wmd_config_name))
        else:
            cls.wmd_process = subprocess.Popen([executable, '-c', cls.wmd_config_name], stdout=subprocess.PIPE,
                                               stderr=subprocess.STDOUT)
        cls.is_connected = True

    @classmethod
    def disconnect_wmediumd(cls):
        """
        Kill the wmediumd process if running
        """
        if cls.is_connected:
            try:
                os.remove(cls.wmd_config_name)
            except Exception:
                pass
            if cls.used_tmux:
                os.system('tmux kill-session -t mnwmd')
            elif cls.wmd_process:
                cls.wmd_process.kill()
            cls.is_connected = False
        else:
            raise Exception('wmediumd is not initialized')

    @staticmethod
    def grep_mac(sta, intf):
        """
        Gets the MAC address of an interface of a station through ifconfig

        :param sta: Mininet-Wifi station
        :param intf: Name of the interface
        :return: The MAC address
        """
        output = sta.cmd(['ifconfig', intf.name, '|', 'grep', '-o', '-E',
                          '\'([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}\''])
        return output.strip()


class WmediumdLink(object):
    def __init__(self, sta1intfref, sta2intfref, snr=10):
        """
        Describes a link between two interfaces using the SNR

        :param sta1intfref: Instance of WmediumdIntfRef
        :param sta2intfref: Instance of WmediumdIntfRef
        :param snr: Signal Noise Ratio as int
        """
        self.sta1intfref = sta1intfref
        self.sta2intfref = sta2intfref
        self.snr = snr


class WmediumdIntfRef(object):
    def __init__(self, sta, intf):
        """
        An unambiguous reference to an interface of a station

        :param sta: Mininet-Wifi station
        :param intf: Mininet interface
        """
        self.sta = sta
        self.intf = intf

    def identifier(self):
        """
        Identifier used in dicts

        :return: string
        """
        return self.sta.name + "." + self.intf.name
