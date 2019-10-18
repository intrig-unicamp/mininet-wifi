"""
    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
    author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

from subprocess import ( Popen, PIPE, check_output as co,
                         CalledProcessError )
from time import sleep
import os
import glob

from mininet.log import info
from mininet.util import decode
from mn_wifi.sixLoWPAN.clean import Cleanup as sixlowpan


class Cleanup(object):
    "Wrapper for cleanup()"
    socket_port = 0
    plot = None

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
                sleep(.5)
            else:
                break

    @classmethod
    def kill_mod_proc(cls):

        if cls.plot:
            cls.plot.closePlot()

        cls.killprocs('sumo-gui')
        cls.killprocs('hostapd')
        sleep(0.1)
        cls.sh('pkill babel')
        cls.sh('pkill wmediumd')
        sleep(0.1)

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

        if glob.glob('*.apconf'):
            os.system('rm *.apconf')
        if glob.glob('*.staconf'):
            os.system('rm *.staconf')
        if glob.glob('*wifiDirect.conf'):
            os.system('rm *wifiDirect.conf')
        if glob.glob('*.nodeParams'):
            os.system('rm *.nodeParams')

    @classmethod
    def cleanup_wifi(cls):
        """Clean up junk which might be left over from old runs;
           do fast stuff before slow dp and link removal!"""

        cls.kill_mod_proc()

        if glob.glob('*-mn-telemetry.txt'):
            os.system('rm *-mn-telemetry.txt')

        sixlowpan.cleanup_6lowpan()


cleanup_wifi = Cleanup.cleanup_wifi
