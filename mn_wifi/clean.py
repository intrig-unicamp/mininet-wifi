"""
    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
    author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

from subprocess import ( Popen, PIPE, check_output as co,
                         CalledProcessError )
import subprocess
from time import sleep
import psutil
import os
import glob

from mininet.log import info
from mininet.util import decode
from mn_wifi.sixLoWPAN.clean import Cleanup as CleanLowpan
from mn_wifi.wmediumdConnector import w_server


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
    def clean_simple_switch_grpc(cls):
        for p in psutil.process_iter():
            #  p.name works using psutil in version 1.2.1.
            #  In version 2.X we have to use p.name()
            if 'simple_switch_grpc' in getattr(p, 'name()', 'name'):
                info("*** Killing all running BMV2 P4 Switches\n")
                cls.killprocs('simple_switch_grpc')

    @classmethod
    def module_loaded(cls, module):
        "Checks if module is loaded"
        lsmod_proc = subprocess.Popen(['lsmod'], stdout=subprocess.PIPE)
        grep_proc = subprocess.Popen(['grep', module],
                                     stdin=lsmod_proc.stdout, stdout=subprocess.PIPE)
        grep_proc.communicate()  # Block until finished
        return grep_proc.returncode == 0

    @classmethod
    def kill_mod(cls, module):
        if cls.module_loaded(module):
            info("*** Killing %s\n" % module)
            os.system('rmmod %s' % module)

    @classmethod
    def kill_mod_proc(cls):
        cls.clean_simple_switch_grpc()

        if cls.plot:
            cls.plot.close_plot()

        cls.killprocs('sumo-gui')
        cls.killprocs('hostapd')
        sleep(0.1)
        cls.sh('pkill babel')

        w_server.disconnect()
        cls.sh('pkill wmediumd')
        sleep(0.1)

        info("\n*** Removing WiFi module and Configurations\n")
        cls.kill_mod('mac80211_hwsim')
        cls.kill_mod('ifb')

        try:
            os.system('pkill -f \'wpa_supplicant -B -Dnl80211\'')
        except:
            pass

        if glob.glob('*.apconf'):
            os.system('rm *.apconf')
        if glob.glob('*.staconf'):
            os.system('rm *.staconf')
        if glob.glob('*wifiDirect.conf'):
            os.system('rm *wifiDirect.conf')
        if glob.glob('*.nodeParams'):
            os.system('rm *.nodeParams')

        if cls.socket_port:
            info('\n*** Done\n')
            cls.sh('fuser -k %s/tcp >/dev/null 2>&1' % cls.socket_port)

        CleanLowpan.cleanup_6lowpan()

    @classmethod
    def cleanup_wifi(cls):
        """Clean up junk which might be left over from old runs;
           do fast stuff before slow dp and link removal!"""

        cls.kill_mod_proc()

        if glob.glob('*-mn-telemetry.txt'):
            os.system('rm *-mn-telemetry.txt')


cleanup_wifi = Cleanup.cleanup_wifi
