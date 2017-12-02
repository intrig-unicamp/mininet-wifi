"""
Mininet Cleanup
author: Bob Lantz (rlantz@cs.stanford.edu)

Unfortunately, Mininet and OpenFlow (and the Linux kernel)
don't always clean up properly after themselves. Until they do
(or until cleanup functionality is integrated into the Python
code), this script may be used to get rid of unwanted garbage.
It may also get rid of 'false positives', but hopefully
nothing irreplaceable!
"""

from subprocess import (Popen, PIPE, check_output as co,
                        CalledProcessError)
import time
import os
import glob

from mininet.log import info


def sh(cmd):
    "Print a command and send it to the shell"
    info(cmd + '\n')
    return Popen(['/bin/sh', '-c', cmd], stdout=PIPE).communicate()[0]


def killprocs(pattern):
    "Reliably terminate processes matching a pattern (including args)"
    sh('pkill -9 -f %s' % pattern)
    # Make sure they are gone
    while True:
        try:
            pids = co(['pgrep', '-f', pattern])
        except CalledProcessError:
            pids = ''
        if pids:
            sh('pkill -9 -f %s' % pattern)
            time.sleep(.5)
        else:
            break


class Cleanup(object):
    "Wrapper for cleanup()"

    callbacks = []

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

        # Call any additional cleanup code if necessary
        for callback in cls.callbacks:
            callback()

        info("*** Cleanup complete.\n")

    @classmethod
    def addCleanupCallback(cls, callback):
        "Add cleanup callback"
        if callback not in cls.callbacks:
            cls.callbacks.append(callback)


cleanup_wifi = Cleanup.cleanup_wifi
addCleanupCallback = Cleanup.addCleanupCallback
