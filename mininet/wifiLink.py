"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""
import numpy as np
import os
import re

from mininet.wifiDevices import deviceDataRate
from mininet.log import debug, info
from wifiPropagationModels import propagationModel
from mininet.wmediumdConnector import WmediumdServerConn, WmediumdSNRLink
from mininet.link import TCLinkWirelessStation
from scipy.spatial.distance import pdist

class link (object):

    dist = 0
    noise = 0
    equationLoss = '(dist * 2) / 1000'
    equationDelay = '(dist / 10) + 1'
    equationLatency = '2 + dist'
    equationBw = 'custombw * (1.1 ** -dist)'
    ifb = False

    def __init__(self, sta=None, ap=None, wlan=0, dist=0):
        """"
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        :param dist: distance between source and destination
        """

        delay_ = self.setDelay(dist)
        latency_ = self.setLatency(dist)
        loss_ = self.setLoss(dist)
        bw_ = self.setBW(sta=sta, ap=ap, dist=dist, wlan=wlan)
        self.tc(sta, wlan, bw_, loss_, latency_, delay_)

    @classmethod
    def getDistance(self, src, dst):
        """ Get the distance between two nodes 
        
        :param src: source node
        :param dst: destination node
        """
        pos_src = src.params['position']
        pos_dst = dst.params['position']
        points = np.array([(pos_src[0], pos_src[1], pos_src[2]), (pos_dst[0], pos_dst[1], pos_dst[2])])
        return float(pdist(points))

    @classmethod
    def setDelay(self, dist):
        """"Based on RandomPropagationDelayModel
        
        :param dist: distance between source and destination
        """
        return eval(self.equationDelay)

    @classmethod
    def setLatency(self, dist):
        """
        :param dist: distance between source and destination
        """
        return eval(self.equationLatency)

    @classmethod
    def setLoss(self, dist):
        """
        :param dist: distance between source and destination
        """
        return eval(self.equationLoss)

    @classmethod
    def setBW(self, sta=None, ap=None, wlan=0, dist=0):
        """set BW

        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        :param dist: distance between source and destination
        """
        value = deviceDataRate(sta, ap, wlan)
        custombw = value.rate
        rate = eval(self.equationBw)

        if rate <= 0.0:
            rate = 0.1
        return rate

    @classmethod
    def setRSSI(self, node1=None, node2=None, wlan=0, dist=0):
        """set RSSI
        
        :param node1: station
        :param node2: access point
        :param wlan: wlan ID
        :param dist: distance
        """
        value = propagationModel(node1, node2, dist, wlan)
        return float(value.rssi)  # random.uniform(value.rssi-1, value.rssi+1)

    @classmethod
    def tc(self, sta, wlan, bw, loss, latency, delay):
        """Applies TC
        
        :param sta: station
        :param wlan: wlan ID
        :param bw: bandwidth (mbps)
        :param loss: loss (%)
        :param latency: latency (ms)
        :param delay: delay (ms)
        """

        if self.ifb:
            sta.pexec("tc qdisc replace dev ifb%s \
                root handle 2: netem rate %.2fmbit \
                loss %.1f%% \
                latency %.2fms \
                delay %.2fms " % (sta.ifb[wlan], bw, loss, latency, delay))
        if 'encrypt' in sta.params:
            """tbf is applied to encrypt, cause we have noticed troubles with wpa_supplicant and netem"""
            tc = 'tc qdisc replace dev %s root handle 1: tbf '\
                 'rate %.2fmbit '\
                 'burst 15000b '\
                 'lat %.2fms' % (sta.params['wlan'][wlan], bw, latency)
            sta.pexec(tc)
        else:
            tc = "tc qdisc replace dev %s " \
                 "root handle 2: netem " \
                 "rate %.2fmbit " \
                 "loss %.1f%% " \
                 "latency %.2fms " \
                 "delay %.2fms" % (sta.params['wlan'][wlan], bw, loss, latency, delay)
            sta.pexec(tc)
                # corrupt 0.1%%" % (sta.params['wlan'][wlan], bw, loss, latency, delay))

    @classmethod
    def frequency(self, node, wlan):
        """Gets frequency based on channel number
        
        :param node: node
        :param wlan: wlan ID
        """
        freq = 0
        if node.params['channel'][wlan] == 1:
            freq = 2.412
        elif node.params['channel'][wlan] == 2:
            freq = 2.417
        elif node.params['channel'][wlan] == 3:
            freq = 2.422
        elif node.params['channel'][wlan] == 4:
            freq = 2.427
        elif node.params['channel'][wlan] == 5:
            freq = 2.432
        elif node.params['channel'][wlan] == 6:
            freq = 2.437
        elif node.params['channel'][wlan] == 7:
            freq = 2.442
        elif node.params['channel'][wlan] == 8:
            freq = 2.447
        elif node.params['channel'][wlan] == 9:
            freq = 2.452
        elif node.params['channel'][wlan] == 10:
            freq = 2.457
        elif node.params['channel'][wlan] == 11:
            freq = 2.462
        elif node.params['channel'][wlan] == 36:
            freq = 5.18
        elif node.params['channel'][wlan] == 40:
            freq = 5.2
        elif node.params['channel'][wlan] == 44:
            freq = 5.22
        elif node.params['channel'][wlan] == 48:
            freq = 5.24
        elif node.params['channel'][wlan] == 52:
            freq = 5.26
        elif node.params['channel'][wlan] == 56:
            freq = 5.28
        elif node.params['channel'][wlan] == 60:
            freq = 5.30
        elif node.params['channel'][wlan] == 64:
            freq = 5.32
        elif node.params['channel'][wlan] == 100:
            freq = 5.50
        elif node.params['channel'][wlan] == 104:
            freq = 5.52
        elif node.params['channel'][wlan] == 108:
            freq = 5.54
        elif node.params['channel'][wlan] == 112:
            freq = 5.56
        elif node.params['channel'][wlan] == 116:
            freq = 5.58
        elif node.params['channel'][wlan] == 120:
            freq = 5.60
        elif node.params['channel'][wlan] == 124:
            freq = 5.62
        elif node.params['channel'][wlan] == 128:
            freq = 5.64
        elif node.params['channel'][wlan] == 132:
            freq = 5.66
        elif node.params['channel'][wlan] == 136:
            freq = 5.68
        elif node.params['channel'][wlan] == 140:
            freq = 5.70
        elif node.params['channel'][wlan] == 149:
            freq = 5.745
        elif node.params['channel'][wlan] == 153:
            freq = 5.765
        elif node.params['channel'][wlan] == 157:
            freq = 5.785
        elif node.params['channel'][wlan] == 161:
            freq = 5.805
        elif node.params['channel'][wlan] == 165:
            freq = 5.825
        return freq

class Association(object):

    printCon = True
    bgscan = ''

    @classmethod
    def configureAdhoc(self, node, wlan, enable_wmediumd):
        "Configure Wireless Ad Hoc"
        iface = node.params['wlan'][wlan]
        node.func[wlan] = 'adhoc'
        node.setIP(node.params['ip'][wlan], intf='%s' % iface)
        node.cmd('iw dev %s set type ibss' % iface)
        if 'position' not in node.params or enable_wmediumd:
            node.params['associatedTo'][wlan] = node.params['ssid'][wlan]
            debug("associating %s to %s...\n" % (iface, node.params['ssid'][wlan]))
            node.pexec('iwconfig %s channel %s essid %s ap 02:CA:FF:EE:BA:01 mode ad-hoc'\
                       % (iface, node.params['channel'][wlan], node.params['associatedTo'][wlan]))
            #sta.pexec('iwconfig %s ap %s' % (iface, sta.params['cell'][wlan]))

    @classmethod
    def configureMesh(self, node, wlan):
        "Configure Wireless Mesh Interface"
        node.func[wlan] = 'mesh'
        self.meshAssociation(node, wlan)
        if node.params['wlan'][wlan] not in str(node.intf):
            cls = TCLinkWirelessStation
            cls(node, intfName1=node.params['wlan'][wlan])

    @classmethod
    def meshAssociation(self, node, wlan):
        "Performs Mesh Association"
        debug("associating %s to %s...\n" % (node.params['wlan'][wlan], node.params['ssid'][wlan]))
        node.pexec('iw dev %s mesh join %s' % (node.params['wlan'][wlan], node.params['ssid'][wlan]))

    @classmethod
    def setSNRWmediumd(self, sta, ap, snr):
        "Send SNR to wmediumd"
        WmediumdServerConn.send_snr_update(WmediumdSNRLink(sta.wmIface[0], ap.wmIface[0], snr))
        WmediumdServerConn.send_snr_update(WmediumdSNRLink(ap.wmIface[0], sta.wmIface[0], snr))

    @classmethod
    def configureWirelessLink(self, sta, ap, wlan, useWmediumd=False, enable_interference=False):
        """ 
        Updates RSSI and Others...
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """

        dist = link.getDistance(sta, ap)
        if dist <= ap.params['range']:
            for wlan in range(0, len(sta.params['wlan'])):
                if not enable_interference:
                    if sta.params['rssi'][wlan] == 0:
                        self.updateParams(sta, ap, wlan)
                if sta.params['associatedTo'][wlan] == '' and ap not in sta.params['associatedTo']:
                    sta.params['associatedTo'][wlan] = ap
                    cls = Association
                    cls.associate_infra(sta, ap, wlan)
                    if not useWmediumd:
                        if dist >= 0.01:
                            link(sta, ap, wlan, dist)
                    if sta not in ap.params['associatedStations']:
                        ap.params['associatedStations'].append(sta)
                if not enable_interference:
                    rssi_ = link.setRSSI(sta, ap, wlan, dist)
                    sta.params['rssi'][wlan] = rssi_
            if ap not in sta.params['apsInRange']:
                sta.params['apsInRange'].append(ap)
                if not enable_interference:
                    ap.params['stationsInRange'][sta] = rssi_

    @classmethod
    def updateParams(self, sta, ap, wlan):
        """
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """

        sta.params['frequency'][wlan] = link.frequency(ap, 0)
        sta.params['channel'][wlan] = ap.params['channel'][0]
        sta.params['mode'][wlan] = ap.params['mode'][0]

    @classmethod
    def associate(self, sta, ap, useWmediumd, enable_interference=False):
        """ Associate to Access Point """
        wlan = sta.ifaceToAssociate
        if 'position' in sta.params:
            self.configureWirelessLink(sta, ap, wlan, useWmediumd, enable_interference)
        else:
            self.associate_infra(sta, ap, wlan)
            sta.params['associatedTo'][wlan] = ap
            ap.params['associatedStations'].append(sta)
        sta.ifaceToAssociate += 1

    @classmethod
    def associate_noEncrypt(self, sta, ap, wlan):
        """ 
        Association when there is no encrypt
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """
        debug('iwconfig %s essid %s ap %s\n' % (sta.params['wlan'][wlan], ap.params['ssid'][0], \
                                                    ap.params['mac'][0]))
        sta.pexec('iwconfig %s essid %s ap %s' % (sta.params['wlan'][wlan], ap.params['ssid'][0], \
                                                    ap.params['mac'][0]))

    @classmethod
    def associate_infra(self, sta, ap, wlan):
        """ 
        Association when infra
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """
        if 'encrypt' not in ap.params:
            self.associate_noEncrypt(sta, ap, wlan)
        else:
            if ap.params['encrypt'][0] == 'wpa' or ap.params['encrypt'][0] == 'wpa2':
                self.associate_wpa(sta, ap, wlan)
            elif ap.params['encrypt'][0] == 'wep':
                self.associate_wep(sta, ap, wlan)
        if self.printCon:
            iface = sta.params['wlan'][wlan]
            info("Associating %s to %s\n" % (iface, ap))
        sta.params['frequency'][wlan] = link.frequency(ap, 0)

    @classmethod
    def wpaFile(self, sta, ap, wlan):
        """ 
        creates a wpa config file
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """
        if 'config' not in ap.params or 'config' not in sta.params:
            if 'authmode' not in ap.params:
                if 'passwd' not in sta.params:
                    passwd = ap.params['passwd'][0]
                else:
                    passwd = sta.params['passwd'][wlan]

        cmd = 'ctrl_interface=/var/run/wpa_supplicant\nnetwork={\n'

        if 'config' in sta.params:
            config = sta.params['config']
            if(config != []):
                config = sta.params['config'].split(',')
                sta.params.pop("config", None)
                for conf in config:
                    cmd = cmd + "   " + conf + "\n"
        else:
            cmd = cmd + '   ssid=\"%s\"\n' % ap.params['ssid'][0]
            if 'authmode' not in ap.params:
                cmd = cmd + '   psk=\"%s\"\n' % passwd
                cmd = cmd + '   proto=%s\n' % ap.params['encrypt'][0].upper()
                cmd = cmd + '   pairwise=%s\n' % ap.rsn_pairwise
            cmd = cmd + '   key_mgmt=%s\n' % ap.wpa_key_mgmt
            if self.bgscan != '':
                cmd = cmd + '   %s\n' % self.bgscan
            if 'authmode' in ap.params and ap.params['authmode'] == '8021x':
                cmd = cmd + '   eap=PEAP\n'
                cmd = cmd + '   identity=\"%s\"\n' % sta.params['radius_identity']
                cmd = cmd + '   password=\"%s\"\n' % sta.params['radius_passwd']
                cmd = cmd + '   phase2=\"autheap=MSCHAPV2\"\n'
        cmd = cmd + '}'

        fileName = '%s_%s.staconf' % (sta.name, wlan)
        os.system('echo \'%s\' > %s' % (cmd, fileName))

    @classmethod
    def associate_wpa(self, sta, ap, wlan):
        """ 
        Association when WPA
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """
        pidfile = "mn%d_%s_%s_wpa.pid" % (os.getpid(), sta.name, wlan)
        self.wpaFile(sta, ap, wlan)
        debug("wpa_supplicant -B -Dnl80211 -P %s -i %s -c %s_%s.staconf\n"
                % (pidfile, sta.params['wlan'][wlan], sta.name, wlan))
        sta.pexec("wpa_supplicant -B -Dnl80211 -P %s -i %s -c %s_%s.staconf"
                % (pidfile, sta.params['wlan'][wlan], sta.name, wlan))

    @classmethod
    def associate_wep(self, sta, ap, wlan):
        """ 
        Association when WEP
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """
        if 'passwd' not in sta.params:
            passwd = ap.params['passwd'][0]
        else:
            passwd = sta.params['passwd'][wlan]
        sta.pexec('iw dev %s connect %s key d:0:%s' \
                % (sta.params['wlan'][wlan], ap.params['ssid'][0], passwd))
