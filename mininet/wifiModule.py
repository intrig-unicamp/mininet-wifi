"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

import glob
import os
import subprocess
import time
from mininet.log import debug
from mininet.wifiMobility import mobility
from mininet.wifiAccessPoint import accessPoint
from subprocess import (check_output as co,
                         CalledProcessError)

class module(object):
    """ Start and Stop mac80211_hwsim module """

    @classmethod
    def killprocs(self, pattern):
        "Reliably terminate processes matching a pattern (including args)"
        os.system('pkill -9 -f %s' % pattern)
        # Make sure they are gone
        while True:
            try:
                pids = co([ 'pgrep', '-f', pattern ])
            except CalledProcessError:
                pids = ''
            if pids:
                os.system('pkill -9 -f %s' % pattern)
                time.sleep(.5)
            else:
                break

    @classmethod
    def loadModule(self, wifiRadios, alternativeModule=''):
        """ Start wireless Module """
        if alternativeModule == '':
            os.system('modprobe mac80211_hwsim radios=%s' % wifiRadios)
            debug('Loading %s virtual interfaces\n' % wifiRadios)
        else:
            os.system('insmod %s radios=%s' % (alternativeModule, wifiRadios))
            debug('Loading %s virtual interfaces\n' % wifiRadios)

    @classmethod
    def stop(self):
        """ Stop wireless Module """
        if glob.glob("*.conf"):
            os.system('rm *.apconf')

        try:
            subprocess.check_output("lsmod | grep mac80211_hwsim",
                                                          shell=True)
            os.system('rmmod mac80211_hwsim')
        except:
            pass
        if mobility.apList != []:
            self.killprocs('hostapd')
        if accessPoint.wpa_supplicantIsRunning:
            os.system('pkill -f \'wpa_supplicant -B -Dnl80211\'')

    @classmethod
    def start(self, wifiRadios, alternativeModule=''):
        """Starting environment"""
        self.killprocs('hostapd')
        try:
            (subprocess.check_output("lsmod | grep mac80211_hwsim",
                                                          shell=True))
            os.system('rmmod mac80211_hwsim')
        except:
            pass

        physicalWlan = self.getWlanList()  # Get Phisical Wlan(s)
        self.loadModule(wifiRadios, alternativeModule)  # Initatilize WiFi Module
        totalPhy = self.getPhy()  # Get Phy Interfaces
        return physicalWlan, totalPhy

    @classmethod
    def getWlanList(self):
        self.wlans = []
        self.wlans = (subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",
                                                      shell=True)).split('\n')
        self.wlans.pop()
        return self.wlans

    @classmethod
    def getPhy(self):
        """ Get phy """
        phy = subprocess.check_output("find /sys/kernel/debug/ieee80211 -name hwsim | cut -d/ -f 6 | sort",
                                                             shell=True).split("\n")
        phy.pop()
        phy.sort(key=len, reverse=False)
        return phy

    @classmethod
    def assingIface(self, stations, virtualWlan, physicalWlan, phyList):
        try:
            wlanList = self.getWlanIface(physicalWlan)
            for sta in stations:
                for wlan in range(0, len(sta.params['wlan'])):
                    i = virtualWlan.index(sta)
                    os.system('iw phy %s set netns %s' % (phyList[i + wlan], sta.pid))
                    sta.cmd('ip link set %s name %s up' % (wlanList[i + wlan], sta.params['wlan'][wlan]))
                    sta.params['rssi'].append(0)
                    sta.params['snr'].append(0)
                    sta.meshMac.append(0)
                    sta.ssid.append('')
        except:
            print "Please, run sudo mn -c before running your code."
            exit(1)

    @classmethod
    def getWlanIface(self, physicalWlan):
        self.newapif = []
        self.apif = subprocess.check_output("iwconfig 2>&1 | grep IEEE | awk '{print $1}'",
                                            shell=True).split('\n')
        for apif in self.apif:
            if apif not in physicalWlan and apif != "":
                self.newapif.append(apif)
        self.newapif = sorted(self.newapif)
        self.newapif.sort(key=len, reverse=False)
        return self.newapif
