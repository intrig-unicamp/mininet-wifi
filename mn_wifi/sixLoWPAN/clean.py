"""

    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)


"""

from subprocess import (check_output as co)

import os
import glob

from mininet.log import info
from mininet.clean import cleanup

class Cleanup(object):
    "Wrapper for cleanup()"

    @classmethod
    def cleanup_6lowpan(cls):
        """Clean up junk which might be left over from old runs;
           do fast stuff before slow dp and link removal!"""

        try:
            info("***  Removing fakelb module and Configurations\n")
            co("lsmod | grep fakelb", shell=True)
            os.system('rmmod fakelb')
        except:
            pass

cleanup = Cleanup.cleanup_6lowpan
