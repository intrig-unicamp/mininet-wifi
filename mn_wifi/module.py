# author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)


import os
import re
import subprocess
import logging
from sys import version_info as py_version_info
from mininet.log import debug, info, error


class module(object):
    "wireless module"

    prefix = ""
    externally_managed = False
    devices_created_dynamically = False

    def __init__(self, nodes, n_radios, alt_module, **params):
        self.start(nodes, n_radios, alt_module, **params)

    def start(self, nodes, n_radios, alt_module, **params):
        """Starts environment
        :param nodes: list of wireless nodes
        :param n_radios: number of wifi radios
        :param alt_module: dir of a mac80211_hwsim alternative module
        :param **params: ifb -  Intermediate Functional Block device"""
        try:
            h = subprocess.check_output("ps -aux | grep -ic \'hostapd\'",
                                        shell=True)
            if h >= 2:
                os.system('pkill -f \'hostapd\'')
        except:
            pass

        physicalWlans = self.get_physical_wlan()  # Gets Physical Wlan(s)
        self.load_module(n_radios, nodes, alt_module, **params)  # Initatilize WiFi Module
        phys = self.get_phy()  # Get Phy Interfaces
        self.assign_iface(nodes, physicalWlans, phys, **params)  # iface assign

    def load_module(self, n_radios, nodes, alt_module, **params):
        """Load WiFi Module
        :param n_radios: number of wifi radios
        :param alt_module: dir of a mac80211_hwsim alternative module"""
        debug('Loading %s virtual wifi interfaces\n' % n_radios)
        if not self.externally_managed:
            if alt_module:
                output_ = os.system('insmod %s radios=0 >/dev/null 2>&1'
                                    % alt_module)
            else:
                output_ = os.system('modprobe mac80211_hwsim radios=0 '
                                    '>/dev/null 2>&1')

            """output_ is different of zero in Kernel 3.13.x. radios=0 doesn't
             work in such kernel version"""
            if output_ == 0:
                self.__create_hwsim_mgmt_devices(n_radios, nodes, **params)
            else:
                # Useful for kernel <= 3.13.x
                if n_radios == 0:
                    n_radios = 1
                if alt_module:
                    os.system('insmod %s radios=%s' % (alt_module,
                                                       n_radios))
                else:
                    os.system('modprobe mac80211_hwsim radios=%s' % n_radios)
        else:
            self.devices_created_dynamically = True
            self.__create_hwsim_mgmt_devices(n_radios, nodes, **params)

    def __create_hwsim_mgmt_devices(self, n_radios, nodes, **params):
        # generate prefix
        num = 0
        hwsim_ids = []
        numokay = False
        self.prefix = ""
        cmd = "find /sys/kernel/debug/ieee80211 " \
              "-name hwsim | cut -d/ -f 6 | sort"

        if py_version_info < (3, 0):
            phys = subprocess.check_output\
                (cmd, shell=True).split("\n")
        else:
            phys = subprocess.check_output\
                (cmd, shell=True).decode('utf-8').split("\n")

        while not numokay:
            self.prefix = "mn%02ds" % num
            numokay = True
            for phy in phys:
                if phy.startswith(self.prefix):
                    num += 1
                    numokay = False
                    break

        if 'docker' in params:
            self.docker_config(n_radios=n_radios, nodes=nodes, num=num, **params)
        else:
            try:
                for i in range(0, n_radios):
                    p = subprocess.Popen(["hwsim_mgmt", "-c", "-n", self.prefix +
                                          ("%02d" % i)], stdin=subprocess.PIPE,
                                         stdout=subprocess.PIPE,
                                         stderr=subprocess.PIPE, bufsize=-1)
                    output, err_out = p.communicate()
                    if p.returncode == 0:
                        if py_version_info < (3, 0):
                            m = re.search("ID (\d+)", output)
                        else:
                            m = re.search("ID (\d+)", output.decode())
                        debug("Created mac80211_hwsim device with ID %s\n" % m.group(1))
                        hwsim_ids.append(m.group(1))
                    else:
                        error("\nError on creating mac80211_hwsim device with name %s"
                              % (self.prefix + ("%02d" % i)))
                        error("\nOutput: %s" % output)
                        error("\nError: %s" % err_out)
            except:
                info("Warning! If you already had Mininet-WiFi installed "
                     "please run util/install.sh -W and then sudo make install.\n")

    def get_physical_wlan(self):
        'Gets the list of physical wlans that already exist'
        wlans = []
        if py_version_info < (3, 0):
            wlans = (subprocess.check_output("iw dev 2>&1 | grep Interface "
                                             "| awk '{print $2}'",
                                             shell=True)).split("\n")
        else:
            wlans = (subprocess.check_output("iw dev 2>&1 | grep Interface "
                                             "| awk '{print $2}'",
                                             shell=True)).decode('utf-8').split("\n")
        wlans.pop()
        return wlans

    def get_phy(self):
        'Gets all phys after starting the wireless module'
        if py_version_info < (3, 0):
            phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name "
                                          "hwsim | cut -d/ -f 6 | sort",
                                          shell=True).split("\n")
        else:
            phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name "
                                          "hwsim | cut -d/ -f 6 | sort",
                                          shell=True).decode('utf-8').split("\n")
        phy.pop()
        phy.sort(key=len, reverse=False)
        return phy

    @classmethod
    def load_ifb(cls, wlans):
        """ Loads IFB
        :param wlans: Number of wireless interfaces
        """
        debug('\nLoading IFB: modprobe ifb numifbs=%s' % wlans)
        os.system('modprobe ifb numifbs=%s' % wlans)

    def docker_config(self, n_radios=0, nodes=None, dir='~/',
                      ip='172.17.0.1', num=0, **params):

        file = self.prefix + 'docker_mn-wifi.sh'
        os.system('rm %s' % file)
        os.system("echo '#!/bin/bash' >> %s" % file)
        os.system("echo 'pid=$(sudo -S docker inspect -f '{{.State.Pid}}' "
                  "%s)' >> %s" % (params['container'], file))
        os.system("echo 'sudo -S mkdir -p /var/run/netns' >> %s" % file)
        os.system("echo 'sudo -S ln -s /proc/$pid/ns/net/ /var/run/netns/$pid'"
                  " >> %s" % file)

        radios = []
        nodes_ = ''
        phys_ = ''
        for node in nodes:
            nodes_ = nodes_ + node.name + ' '
            radios.append(nodes.index(node))

        for radio in range(0, n_radios):
            os.system("echo 'sudo -S hwsim_mgmt -c -n %s%s' >> %s"
                      % (self.prefix, "%02d" % radio, file))
            if radio in radios:
                radio_id = self.prefix + "%02d" % radio
                phys_ = phys_ + radio_id + ' '
        os.system("echo 'nodes=(%s)' >> %s" % (nodes_, file))
        os.system("echo 'phys=(%s)' >> %s" % (phys_, file))
        os.system("echo 'j=0' >> %s" % file)
        os.system("echo 'for i in ${phys[@]}' >> %s" % file)
        os.system("echo 'do' >> %s" % file)
        os.system("echo '    pid=$(ps -aux | grep \"mininet:${nodes[$j]}$\" "
                  "| grep -v 'hostapd' | awk \"{print \$2}\" "
                  "| awk \"NR>=%s&&NR<=%s\")' >> %s" % (num+1, num+1, file))
        os.system("echo '    sudo iw phy $i set netns $pid' >> %s" % file)
        os.system("echo '    j=$((j+1))' >> %s" % file)
        os.system("echo 'done' >> %s" % file)
        os.system("scp %s %s@%s:%s" % (file, params['ssh_user'], ip, dir))
        os.system("ssh %s@%s \'chmod +x %s%s; %s%s\'"
                  % (params['ssh_user'], ip, dir, file, dir, file))

    def rename(self, node, wintf, newname):
        debug('\n')
        node.pexec('ip link set %s down' % wintf)
        debug('\n')
        node.pexec('ip link set %s name %s' % (wintf, newname))
        debug('\n')
        node.pexec('ip link set %s up' % newname)

    def assign_iface(self, nodes, physicalWlans, phys, **params):
        """Assign virtual interfaces for all nodes
        :param nodes: list of wireless nodes
        :param physicalWlans: list of Physical Wlans
        :param phys: list of phys
        :param **params: ifb -  Intermediate Functional Block device"""
        from mn_wifi.node import AP

        log_filename = '/tmp/mn-wifi-mac80211_hwsim.log'
        self.logging_to_file("%s" % log_filename)

        try:
            if 'docker' in params:
                wlan_list = []
                for phy in range(0, len(phys)):
                    wlan_list.append('wlan%s' % phy)
            else:
                wlan_list = self.get_wlan_iface(physicalWlans)

            debug("\n*** Configuring interfaces with appropriated network"
                  "-namespaces...\n")
            for node in nodes:
                for wlan in range(0, len(node.params['wlan'])):
                    if isinstance(node, AP) and not node.inNamespace:
                        self.rename(node, wlan_list[0], node.params['wlan'][wlan])
                    else:
                        if 'docker' not in params:
                            if py_version_info < (3, 0):
                                rfkill = subprocess.check_output(
                                    'rfkill list | grep %s | awk \'{print $1}\''
                                    '| tr -d ":"' % phys[0], shell=True).split('\n')
                            else:
                                rfkill = subprocess.check_output(
                                    'rfkill list | grep %s | awk \'{print $1}\''
                                    '| tr -d ":"' % phys[0],
                                    shell=True).decode('utf-8').split('\n')
                            debug('rfkill unblock %s\n' % rfkill[0])
                            os.system('rfkill unblock %s' % rfkill[0])
                            os.system('iw phy %s set netns %s' % (phys[0], node.pid))
                        node.cmd('ip link set %s down' % wlan_list[0])
                        node.cmd('ip link set %s name %s'
                                 % (wlan_list[0], node.params['wlan'][wlan]))
                    wlan_list.pop(0)
                    phys.pop(0)
        except:
            logging.exception("Warning:")
            info("Warning! Error when loading mac80211_hwsim. "
                 "Please run sudo 'mn -c' before running your code.\n")
            info("Further information available at %s.\n" % log_filename)
            exit(1)

    def logging_to_file(self, filename):
        logging.basicConfig(filename=filename,
                            filemode='a',
                            level=logging.DEBUG,
                            format='%(asctime)s - %(levelname)s - %(message)s',
                           )

    def get_wlan_iface(self, physicalWlan):
        """Build a new wlan list removing the physical wlan
        :param physicalWlans: list of Physical Wlans"""
        wlan_list = []

        if py_version_info < (3, 0):
            iface_list = subprocess.check_output("iw dev 2>&1 | grep Interface | "
                                                 "awk '{print $2}'",
                                                 shell=True).split('\n')

        else:
            iface_list = subprocess.check_output("iw dev 2>&1 | grep Interface | "
                                                 "awk '{print $2}'",
                                                 shell=True).decode('utf-8').split('\n')
        for iface in iface_list:
            if iface and iface not in physicalWlan:
                wlan_list.append(iface)
        wlan_list = sorted(wlan_list)
        wlan_list.sort(key=len, reverse=False)
        return wlan_list
