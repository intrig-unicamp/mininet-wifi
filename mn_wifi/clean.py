"""

    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)


"""

from subprocess import (check_output as co)

import os
import glob

from mininet.log import info
from mininet.clean import killprocs, sh
from mn_wifi.sixLoWPAN.clean import Cleanup as sixlowpan

class Cleanup(object):
    "Wrapper for cleanup()"

    @classmethod
    def cleanup_wifi(cls):
        """Clean up junk which might be left over from old runs;
           do fast stuff before slow dp and link removal!"""

        info("***  Removing WiFi module and Configurations\n")

        try:
            co("lsmod | grep mac80211_hwsim", shell=True)
            os.system('rmmod mac80211_hwsim')
        except:
            pass

        try:
            co("lsmod | grep ifb", shell=True)
            os.system('rmmod ifb')
        except:
            pass

        killprocs('hostapd')

        if glob.glob("*.apconf"):
            os.system('rm *.apconf')
        if glob.glob("*.staconf"):
            os.system('rm *.staconf')
        if glob.glob("*wifiDirect.conf"):
            os.system('rm *wifiDirect.conf')
        if glob.glob("*.nodeParams"):
            os.system('rm *.nodeParams')

        try:
            os.system('pkill -f \'wpa_supplicant -B -Dnl80211\'')
        except:
            pass

        info("*** Killing wmediumd\n")
        sh('pkill wmediumd')

        sixlowpan.cleanup_6lowpan()

cleanup_wifi = Cleanup.cleanup_wifi
