import os
import subprocess
import socket
import struct
import fcntl
import fileinput

from mininet.wifiDevices import deviceDataRate

class accessPoint(object):

    wpa_supplicantIsRunning = False

    def __init__(self, ap, country_code=None, auth_algs=None, wpa=None, intf=None, wlan=None,
              wpa_key_mgmt=None, rsn_pairwise=None, wpa_passphrase=None, encrypt=None,
              wep_key0=None, **params):

        ap.params['mac'] = []
        for i in range(ap.n_ssids + 1):
            ap.params['mac'].append('')

        if 'phywlan' not in ap.params:
            self.renameIface(ap, intf, ap.params['wlan'][wlan])
            ap.params['mac'][wlan] = self.getMacAddress(ap, wlan)

        self.start (ap, country_code, auth_algs, wpa, wlan,
              wpa_key_mgmt, rsn_pairwise, wpa_passphrase, encrypt,
              wep_key0, **params)

    def renameIface(self, ap, intf, newname):
        "Rename interface"
        ap.pexec('ifconfig %s down' % intf)
        ap.pexec('ip link set %s name %s' % (intf, newname))
        ap.pexec('ifconfig %s up' % newname)

    def start(self, ap, country_code=None, auth_algs=None, wpa=None, wlan=None,
              wpa_key_mgmt=None, rsn_pairwise=None, wpa_passphrase=None, encrypt=None,
              wep_key0=None, **params):
        """ Starting Access Point """

        self.cmd = ("echo \'")
        if 'phywlan' not in ap.params:
            self.cmd = self.cmd + ("interface=%s" % ap.params['wlan'][wlan])  # the interface used by the AP
        else:
            wlan = ap.params.get('phywlan')
            self.cmd = self.cmd + ("interface=%s" % wlan)  # the interface used by the AP
        self.cmd = self.cmd + ("\ndriver=nl80211")
        self.cmd = self.cmd + ("\nssid=%s" % ap.ssid[0])  # the name of the AP

        if ap.params['mode'][0] == 'n' or ap.params['mode'][0] == 'ac'or ap.params['mode'][0] == 'a':
            self.cmd = self.cmd + ("\nhw_mode=g")
        else:
            self.cmd = self.cmd + ("\nhw_mode=%s" % ap.params['mode'][0])
        self.cmd = self.cmd + ("\nchannel=%s" % ap.params['channel'][0])  # the channel to use
        if 'phywlan' not in ap.params:
            self.cmd = self.cmd + ("\nwme_enabled=1")
            self.cmd = self.cmd + ("\nwmm_enabled=1")
        if encrypt == 'wpa':
            self.cmd = self.cmd + ("\nauth_algs=%s" % auth_algs)
            self.cmd = self.cmd + ("\nwpa=%s" % wpa)
            self.cmd = self.cmd + ("\nwpa_key_mgmt=%s" % wpa_key_mgmt)
            self.cmd = self.cmd + ("\nwpa_passphrase=%s" % wpa_passphrase)
        elif encrypt == 'wpa2':
            self.cmd = self.cmd + ("\nauth_algs=%s" % auth_algs)
            self.cmd = self.cmd + ("\nwpa=%s" % wpa)
            self.cmd = self.cmd + ("\nwpa_key_mgmt=%s" % wpa_key_mgmt)
            self.cmd = self.cmd + ("\nrsn_pairwise=%s" % rsn_pairwise)
            self.cmd = self.cmd + ("\nwpa_passphrase=%s" % wpa_passphrase)
        elif encrypt == 'wep':
            self.cmd = self.cmd + ("\nauth_algs=%s" % auth_algs)
            self.cmd = self.cmd + ("\nwep_default_key=%s" % 0)
            self.cmd = self.cmd + ("\nwep_key0=%s" % wep_key0)
            
        self.cmd = self.cmd + '\nctrl_interface=/var/run/hostapd'
        self.cmd = self.cmd + '\nctrl_interface_group=0'

        if 'config' in ap.params.keys():
            config = ap.params['config']
            if(config != []):
                config = ap.params['config'].split(',')
                ap.params.pop("config", None)
                for conf in config:
                    self.cmd = self.cmd + "\n" + conf

        if(ap.n_ssids) > 0:
            for i in range(1, ap.n_ssids + 1):
                self.cmd = self.cmd + ('\n')
                self.cmd = self.cmd + ("\nbss=%s-wlan%s-%s" % (ap, wlan, i))
                self.cmd = self.cmd + ("\nssid=%s-%s" % (ap.ssid[0], i))
                ap.params['mac'][i] = ap.params['mac'][wlan][:-1] + str(i)
                self.checkNetworkManager(ap.params['mac'][i])
                ap.params['wlan'].append('%s-wlan%s-%s' % (ap, wlan, i))

        self.APfile(self.cmd, ap, wlan)

    def getMacAddress(self, ap, wlan):
        """ get Mac Address of any Interface """
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        info = fcntl.ioctl(s.fileno(), 0x8927, struct.pack('256s', '%s'[:15]) % ap.params['wlan'][wlan])
        mac = (''.join(['%02x:' % ord(char) for char in info[18:24]])[:-1])
        self.checkNetworkManager(mac)
        return mac

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
    def apBridge(self, ap, wlan):
        """ AP Bridge """
        if 'phywlan' not in ap.params:
            os.system("ovs-vsctl add-port %s %s" % (ap, ap.params['wlan'][wlan]))
        else:
            wlan = ap.params.get('phywlan')
            os.system("ovs-vsctl add-port %s %s" % (ap, wlan))

    def setBw(self, ap, iface):
        """ Set bw to AP """
        value = deviceDataRate(ap, None, None)
        bw = value.rate

        ap.cmd("tc qdisc replace dev %s \
            root handle 2: tbf rate %sMbit burst 15000 latency 2ms" % (iface, bw))
        # Reordering packets
        ap.cmd('tc qdisc add dev %s parent 2:1 handle 10: pfifo limit 1000' % (iface))

    def APfile(self, cmd, ap, wlan):
        """ run an Access Point and create the config file  """
        if 'phywlan' not in ap.params:
            iface = ap.params['wlan'][wlan]
        else:
            iface = ap.params.get('phywlan')
            os.system('ifconfig %s down' % iface)
            os.system('ifconfig %s up' % iface)
        content = cmd + ("\' > %s.apconf" % iface)
        os.system(content)
        cmd = ("hostapd -B %s.apconf &" % iface)
        try:
            subprocess.check_output(cmd, shell=True)
        except:
            print ('error with hostapd. Please, run sudo mn -c in order to fix it or check if hostapd is\
                                             working properly in your machine.')
            exit(1)
        self.setBw(ap, iface)
