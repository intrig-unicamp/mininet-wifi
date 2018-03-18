"""

    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)


"""

from subprocess import (check_output as co)

import os
import glob

from mininet.log import info
from mininet.wifi.clean import cleanup_wifi

class Cleanup(cleanup_wifi):
    "Wrapper for cleanup()"

    @classmethod
    def cleanup_6lowpan(cls):
        """Clean up junk which might be left over from old runs;
           do fast stuff before slow dp and link removal!"""

        info("***  Removing fakelb module and Configurations\n")

        try:
            co("lsmod | grep fakelb", shell=True)
            os.system('rmmod fakelb')
        except:
            pass

        if glob.glob("*.apconf"):
            os.system('rm *.apconf')
        if glob.glob("*.staconf"):
            os.system('rm *.staconf')
        if glob.glob("*wifiDirect.conf"):
            os.system('rm *wifiDirect.conf')
        if glob.glob("*.nodeParams"):
            os.system('rm *.nodeParams')

cleanup_wifi = Cleanup.cleanup_6lowpan
