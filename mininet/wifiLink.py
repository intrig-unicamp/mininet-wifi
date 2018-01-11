"""
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""
import os

from mininet.wifiDevices import deviceDataRate
from mininet.log import debug, info
from mininet.wifiPropagationModels import propagationModel
from mininet.wmediumdConnector import WmediumdServerConn, WmediumdSNRLink

class wirelessLink (object):

    dist = 0
    noise = 0
    equationLoss = '(dist * 2) / 1000'
    equationDelay = '(dist / 10) + 1'
    equationLatency = '(dist / 10)/2'
    equationBw = ' * (1.01 ** -dist)'
    ifb = False

    def __init__(self, sta=None, ap=None, wlan=0, ap_wlan=0, dist=0):
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
    def setDelay(cls, dist):
        """"Based on RandomPropagationDelayModel
        
        :param dist: distance between source and destination
        """
        return eval(cls.equationDelay)

    @classmethod
    def setLatency(cls, dist):
        """
        :param dist: distance between source and destination
        """
        return eval(cls.equationLatency)

    @classmethod
    def setLoss(cls, dist):
        """
        :param dist: distance between source and destination
        """
        return eval(cls.equationLoss)

    @classmethod
    def setBW(cls, sta=None, ap=None, wlan=0, dist=0):
        """set BW

        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        :param dist: distance between source and destination
        """
        value = deviceDataRate(sta, ap, wlan)
        custombw = value.rate
        rate = eval(str(custombw) + cls.equationBw)

        if rate <= 0.0:
            rate = 0.1
        return rate

    @classmethod
    def setRSSI(cls, node1=None, node2=None, wlan=0, dist=0):
        """set RSSI
        
        :param node1: station
        :param node2: access point
        :param wlan: wlan ID
        :param dist: distance
        """
        value = propagationModel(node1, node2, dist, wlan)
        return float(value.rssi)  # random.uniform(value.rssi-1, value.rssi+1)

    @classmethod
    def delete(cls, node):
        "Delete interfaces"
        from mininet.node import Car

        for wlan in node.params['wlan']:
            if isinstance(node, Car) and node.params['wlan'].index(wlan) == 1:
                node = node.params['carsta']
                wlan = node.params['wlan'][0]
            if isinstance(wlan, basestring):
                node.cmd('iw dev ' + wlan + ' del')
                node.delIntf(wlan)
                node.intf = None

    @classmethod
    def tc(cls, sta, wlan, bw, loss, latency, delay):
        """Applies TC
        
        :param sta: station
        :param wlan: wlan ID
        :param bw: bandwidth (mbps)
        :param loss: loss (%)
        :param latency: latency (ms)
        :param delay: delay (ms)"""
        #delay is not being used

        if cls.ifb:
            sta.pexec("tc qdisc replace dev ifb%s \
                root handle 2: netem rate %.2fmbit \
                loss %.1f%% \
                latency %.2fms " % (sta.ifb[wlan], bw, loss, latency))
        if 'encrypt' in sta.params['associatedTo'][wlan].params:
            """tbf is applied to encrypt, cause we have noticed troubles 
            with wpa_supplicant and netem"""
            tc = 'tc qdisc replace dev %s root handle 1: tbf '\
                 'rate %.2fmbit '\
                 'burst 15000b '\
                 'lat %.2fms' % (sta.params['wlan'][wlan], bw, latency)
            sta.pexec(tc)
        else:
            tc = "tc qdisc replace dev %s " \
                 "root handle 2: netem " \
                 "rate %.4fmbit " \
                 "loss %.1f%% " \
                 "latency %.2fms " % (sta.params['wlan'][wlan], bw, loss,
                                   latency)
            sta.pexec(tc)
                # corrupt 0.1%%" % (sta.params['wlan'][wlan], bw, loss,
            # latency, delay))

class Association(object):

    printCon = True
    bgscan = ''

    @classmethod
    def setSNRWmediumd(cls, sta, ap, snr):
        "Send SNR to wmediumd"
        WmediumdServerConn.send_snr_update(WmediumdSNRLink(
            sta.wmIface[0], ap.wmIface[0], snr))
        WmediumdServerConn.send_snr_update(WmediumdSNRLink(
            ap.wmIface[0], sta.wmIface[0], snr))

    @classmethod
    def configureWirelessLink(cls, sta, ap, enable_wmediumd=False, wmediumd_mode=False,
                              ap_wlan=0):
        """ 
        Updates RSSI and Others...
        
        :param sta: station
        :param ap: access point
        """

        dist = sta.get_distance_to(ap)
        if dist <= ap.params['range'][0]:
            for wlan in range(0, len(sta.params['wlan'])):
                if wmediumd_mode is not 'interference':
                    if sta.params['rssi'][wlan] == 0:
                        cls.updateParams(sta, ap, wlan, ap_wlan)
                if sta.params['associatedTo'][wlan] == '' \
                        and ap not in sta.params['associatedTo']:
                    Association.associate_infra(sta, ap, wlan, ap_wlan)
                    if not enable_wmediumd:
                        if dist >= 0.01:
                            wirelessLink(sta, ap, wlan, ap_wlan, dist)
                    if sta not in ap.params['associatedStations']:
                        ap.params['associatedStations'].append(sta)
                if wmediumd_mode is not 'interference':
                    rssi_ = wirelessLink.setRSSI(sta, ap, wlan, dist)
                    sta.params['rssi'][wlan] = rssi_
            if ap not in sta.params['apsInRange']:
                sta.params['apsInRange'].append(ap)
                if wmediumd_mode is not 'interference':
                    ap.params['stationsInRange'][sta] = rssi_

    @classmethod
    def updateParams(cls, sta, ap, wlan):
        """
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """

        sta.params['frequency'][wlan] = ap.getFrequency(0)
        sta.params['channel'][wlan] = ap.params['channel'][0]
        sta.params['mode'][wlan] = ap.params['mode'][0]

    @classmethod
    def associate(cls, sta, ap, enable_wmediumd, wmediumd_mode,
                  sta_wlan=0, ap_wlan=0):
        "Associate to Access Point"
        if sta_wlan == 0:
            sta_wlan = sta.ifaceToAssociate
        if 'position' in sta.params:
            cls.configureWirelessLink(sta, ap, enable_wmediumd,
                                      wmediumd_mode, ap_wlan)
        else:
            cls.associate_infra(sta, ap, sta_wlan, ap_wlan)
            sta.params['associatedTo'][sta_wlan] = ap
            ap.params['associatedStations'].append(sta)
        sta.ifaceToAssociate += 1

    @classmethod
    def associate_noEncrypt(cls, sta, ap, sta_wlan, ap_wlan):
        """ 
        Association when there is no encrypt
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """
        #debug('iw dev %s connect %s %s\n'
        #      % (sta.params['wlan'][wlan], ap.params['ssid'][0], ap.params['mac'][0]))
        #sta.pexec('iw dev %s connect %s %s'
        #          % (sta.params['wlan'][wlan], ap.params['ssid'][0], ap.params['mac'][0]))
        #iwconfig is still necessary, since iw doesn't include essid like iwconfig does.
        debug('iwconfig %s essid %s ap %s\n' % (
            sta.params['wlan'][sta_wlan], ap.params['ssid'][ap_wlan],
            ap.params['mac'][ap_wlan]))
        sta.pexec('iwconfig %s essid %s ap %s' % (
            sta.params['wlan'][sta_wlan], ap.params['ssid'][ap_wlan],
            ap.params['mac'][ap_wlan]))

    @classmethod
    def associate_infra(cls, sta, ap, wlan, ap_wlan):
        """Association when infra
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID"""
        if 'ieee80211r' in ap.params and ap.params['ieee80211r'] == 'yes':
            if sta.params['associatedTo'][wlan] == '':
                cls.associate_wpa(sta, ap, wlan, ap_wlan)
            else:
                cls.handover_ieee80211r(sta, ap, wlan, ap_wlan)
        elif 'encrypt' not in ap.params:
            cls.associate_noEncrypt(sta, ap, wlan, ap_wlan)
        else:
            if sta.params['associatedTo'][wlan] == '':
                if ap.params['encrypt'][0] == 'wpa' or ap.params['encrypt'][0] == 'wpa2':
                    cls.associate_wpa(sta, ap, wlan, ap_wlan)
                elif ap.params['encrypt'][0] == 'wep':
                    cls.associate_wep(sta, ap, wlan, ap_wlan)
        if cls.printCon:
            iface = sta.params['wlan'][wlan]
            info("Associating %s to %s\n" % (iface, ap))
        sta.params['frequency'][wlan] = ap.getFrequency(0)
        sta.params['associatedTo'][wlan] = ap

    @classmethod
    def wpaFile(cls, sta, ap, wlan, ap_wlan):
        """ 
        creates a wpa config file
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """
        if 'config' not in ap.params or 'config' not in sta.params:
            if 'authmode' not in ap.params:
                if 'passwd' not in sta.params:
                    passwd = ap.params['passwd'][ap_wlan]
                else:
                    passwd = sta.params['passwd'][wlan]

        cmd = 'ctrl_interface=/var/run/wpa_supplicant\nnetwork={\n'

        if 'config' in sta.params:
            config = sta.params['config']
            if config is not []:
                config = sta.params['config'].split(',')
                sta.params.pop("config", None)
                for conf in config:
                    cmd = cmd + "   " + conf + "\n"
        else:
            cmd = cmd + '   ssid=\"%s\"\n' % ap.params['ssid'][ap_wlan]
            if 'authmode' not in ap.params:
                cmd = cmd + '   psk=\"%s\"\n' % passwd
                cmd = cmd + '   proto=%s\n' % ap.params['encrypt'][ap_wlan].upper()
                cmd = cmd + '   pairwise=%s\n' % ap.rsn_pairwise
            cmd = cmd + '   key_mgmt=%s\n' % ap.wpa_key_mgmt
            if cls.bgscan != '':
                cmd = cmd + '   %s\n' % cls.bgscan
            if 'authmode' in ap.params and ap.params['authmode'] == '8021x':
                cmd = cmd + '   eap=PEAP\n'
                cmd = cmd + '   identity=\"%s\"\n' % sta.params['radius_identity']
                cmd = cmd + '   password=\"%s\"\n' % sta.params['radius_passwd']
                cmd = cmd + '   phase2=\"autheap=MSCHAPV2\"\n'
        cmd = cmd + '}'

        fileName = '%s_%s.staconf' % (sta.name, wlan)
        os.system('echo \'%s\' > %s' % (cmd, fileName))

    @classmethod
    def associate_wpa(cls, sta, ap, wlan, ap_wlan):
        """ 
        Association when WPA
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """
        pidfile = "mn%d_%s_%s_wpa.pid" % (os.getpid(), sta.name, wlan)
        cls.wpaFile(sta, ap, wlan, ap_wlan)
        debug("wpa_supplicant -B -Dnl80211 -P %s -i %s -c %s_%s.staconf\n"
              % (pidfile, sta.params['wlan'][wlan], sta.name, wlan))
        sta.pexec("wpa_supplicant -B -Dnl80211 -P %s -i %s -c %s_%s.staconf"
                  % (pidfile, sta.params['wlan'][wlan], sta.name, wlan))

    @classmethod
    def handover_ieee80211r(cls, sta, ap, wlan, ap_wlan):
        debug('wpa_cli -i %s roam %s\n' % (sta.params['wlan'][wlan],
                                           ap.params['mac'][ap_wlan]))
        sta.pexec('wpa_cli -i %s roam %s' % (sta.params['wlan'][wlan],
                                             ap.params['mac'][ap_wlan]))

    @classmethod
    def associate_wep(cls, sta, ap, wlan, ap_wlan):
        """ 
        Association when WEP
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """
        if 'passwd' not in sta.params:
            passwd = ap.params['passwd'][ap_wlan]
        else:
            passwd = sta.params['passwd'][wlan]
        sta.pexec('iw dev %s connect %s key d:0:%s' \
                % (sta.params['wlan'][wlan], ap.params['ssid'][ap_wlan], passwd))
