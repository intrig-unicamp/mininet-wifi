"""
    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
    author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

from subprocess import ( Popen, PIPE, check_output as co,
                         CalledProcessError )
import time
import os
import glob

from mininet.log import info
from mininet.util import decode
from mn_wifi.sixLoWPAN.clean import Cleanup as sixlowpan
from mn_wifi.plot import plot2d, plot3d


class Cleanup(object):
    "Wrapper for cleanup()"
    socket_port = 0

    @classmethod
    def sh(cls, cmd):
        "Print a command and send it to the shell"
        result = Popen(['/bin/sh', '-c', cmd], stdout=PIPE).communicate()[0]
        return decode(result)

    @classmethod
    def killprocs(cls, pattern):
        "Reliably terminate processes matching a pattern (including args)"
        cls.sh('pkill -9 -f %s' % pattern)
        # Make sure they are gone
        while True:
            try:
                pids = co(['pgrep', '-f', pattern])
            except CalledProcessError:
                pids = ''
            if pids:
                cls.sh('pkill -9 -f %s' % pattern)
                time.sleep(.5)
            else:
                break

    @classmethod
    def kill_mod_proc(cls):

        try:
            plot2d.closePlot()
        except:
            pass
        try:
            plot3d.closePlot()
        except:
            pass

        info("\n*** Removing WiFi module and Configurations\n")
        try:
            co("lsmod | grep mac80211_hwsim", shell=True)
            os.system('rmmod mac80211_hwsim >/dev/null 2>&1')
        except:
            pass

        try:
            co("lsmod | grep ifb", shell=True)
            os.system('rmmod ifb')
        except:
            pass

        try:
            os.system('pkill -f \'wpa_supplicant -B -Dnl80211\'')
        except:
            pass

        if cls.socket_port:
            info('\n*** Done\n')
            cls.sh('fuser -k %s/tcp >/dev/null 2>&1' % cls.socket_port)

        cls.killprocs('sumo-gui &> /dev/null')
        cls.killprocs('hostapd &> /dev/null')
        cls.killprocs('babeld &> /dev/null')
        cls.killprocs('wmediumd &> /dev/null')


    @classmethod
    def cleanup_wifi(cls):
        """Clean up junk which might be left over from old runs;
           do fast stuff before slow dp and link removal!"""

        cls.kill_mod_proc()

        if glob.glob('*-mn-telemetry.txt'):
            os.system('rm *-mn-telemetry.txt')
        if glob.glob('*.apconf'):
            os.system('rm *.apconf')
        if glob.glob('*.staconf'):
            os.system('rm *.staconf')
        if glob.glob('*wifiDirect.conf'):
            os.system('rm *wifiDirect.conf')
        if glob.glob('*.nodeParams'):
            os.system('rm *.nodeParams')

        sixlowpan.cleanup_6lowpan()


cleanup_wifi = Cleanup.cleanup_wifi
