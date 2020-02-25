"""
   Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
   @author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

from mn_wifi.mobility import Mobility
from threading import Thread as thread
from time import sleep


class Energy(object):

    def __init__(self, nodes):
        Mobility.thread_ = thread(target=self.start, args=(nodes,))
        Mobility.thread_.daemon = True
        Mobility.thread_._keep_alive = True
        Mobility.thread_.start()

    def start(self, nodes):
        while Mobility.thread_._keep_alive:
            sleep(1)  # set sleep time to 1 second
            for node in nodes:
                self.level(node)

    def level(self, node):
        # this is only an empirical approach
        # we may need to consider tx/rx packets,
        # txpower; etc
        level = node.params['battery']
        level *= 0.1
        node.params['battery'] = level
        if level == 0:
            self.shutdown_intfs(node)

    def shutdown_intfs(self, node):
        # passing through all the interfaces
        # but we need to consider interfaces separately
        for wlan in node.params['wlans']:
            self.ipLink(wlan)

    def ipLink(self, intf):
        "Configure ourselves using ip link"
        self.cmd('ip link set %s down' % intf)