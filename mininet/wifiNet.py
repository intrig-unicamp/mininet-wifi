"""
Provides support to wifi

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

import os
import random
import sys
from time import sleep

from mininet.node import AccessPoint
from mininet.log import info
from mininet.wmediumdConnector import DynamicWmediumdIntfRef, WmediumdSNRLink, WmediumdStarter, \
                    WmediumdTXPower, WmediumdPosition, WmediumdConstants, WmediumdServerConn
from mininet.wifiChannel import setChannelParams
from mininet.wifiDevices import deviceRange, deviceDataRate
from mininet.wifiAdHocConnectivity import pairingAdhocNodes
from mininet.wifiMeshRouting import listNodes, meshRouting
from mininet.wifiMobility import mobility
from mininet.wifiPlot import plot2d, plot3d
from mininet.wifiModule import module
from mininet.wifiPropagationModels import propagationModel
from mininet.link import TCLinkWirelessAP, TCLinkWirelessStation, Association
from mininet.util import macColonHex, ipAdd
from mininet.vanet import vanet

sys.path.append(str(os.getcwd()) + '/mininet/')
from sumo.runner import sumo

class mininetWiFi(object):

    associationControlMethod = ''
    alternativeModule = ''
    rec_rssi = False
    useWmediumd = False
    is3d = False
    isMobility = False
    DRAW = False
    alreadyPlotted = False
    enable_interference = False
    isVanet = False
    ifb = False
    isWiFi = False
    wifiRadios = 0
    seed_ = 10
    init_time = 0
    MAX_X = 0
    MAX_Y = 0
    MAX_Z = 0
    nroads = 0
    srcConn = []
    dstConn = []
    wlinks = []
    plotNodes = []

    @classmethod
    def addParameters(self, node, autoSetMacs, params, mode='managed'):
        """adds parameters to wireless nodes

        node: node
        autoSetMacs: set MAC addrs automatically like IP addresses
        params: parameters
        defaults: Default IP and MAC addresses
        mode: if interface is running in managed or master mode
        """
        node.params['frequency'] = []
        node.params['wlan'] = []
        node.params['mac'] = []
        node.phyID = []

        passwd = ("%s" % params.pop('passwd', {}))
        if(passwd != "{}"):
            passwd = passwd.split(',')
            node.params['passwd'] = []

        encrypt = ("%s" % params.pop('encrypt', {}))
        if(encrypt != "{}"):
            encrypt = encrypt.split(',')
            node.params['encrypt'] = []

        if (mode == 'managed'):
            node.passwd = ''
            node.params['apsInRange'] = []
            node.params['associatedTo'] = []
            node.params['rssi'] = []
            node.params['snr'] = []
            node.ifaceToAssociate = 0
            node.max_x = 0
            node.max_y = 0
            node.min_x = 0
            node.min_y = 0
            node.max_v = 0
            node.min_v = 0
            node.constantVelocity = 1.0
            node.constantDistance = 1.0

            # max_speed
            if 'max_speed' in params:
                node.max_speed = int(params['max_speed'])
            else:
                node.max_speed = 10

            # min_speed
            if 'min_speed' in params:
                node.min_speed = int(params['min_speed'])
            else:
                node.min_speed = 1

        # speed
        if 'speed' in params:
            node.speed = int(params['speed'])

        # max_x
        if 'max_x' in params:
            node.max_x = int(params['max_x'])

        # max_y
        if 'max_y' in params:
            node.max_y = int(params['max_y'])

        # min_x
        if 'min_x' in params:
            node.min_x = int(params['min_x'])

        # min_y
        if 'min_y' in params:
            node.min_y = int(params['min_y'])

        # min_v
        if 'min_v' in params:
            node.min_v = int(params['min_v'])

        # max_v
        if 'max_v' in params:
            node.max_v = int(params['max_v'])

        # constantVelocity
        if 'constantVelocity' in params:
            node.constantVelocity = int(params['constantVelocity'])

        # constantDistance
        if 'constantDistance' in params:
            node.constantDistance = int(params['constantDistance'])

        # position
        if 'position' in node.params:
            position = node.params['position']
            position = position.split(',')
            node.params['position'] = [float(position[0]), float(position[1]), float(position[2])]
        else:
            position = ("%s" % params.pop('position', {}))
            if(position != "{}"):
                position = position.split(',')
                node.params['position'] = position

        wlans = self.countWiFiIfaces(params)

        for n in range(wlans):
            self.addParamsToNode(node)
            if mode == 'managed':
                self.appendAssociatedTo(node)
                if 'ssid' not in node.params:
                    self.appendEncrypt(node, n, passwd, encrypt)

            if mode == 'master':
                if 'phywlan' in node.params:
                    n = 1
                node.params['wlan'].append(node.name + '-wlan' + str(n + 1))
                if 'link' in params and params['link'] == 'mesh':
                    self.appendRSSI(node)
                    self.appendSNR(node)
                    self.appendAssociatedTo(node)
            else:
                node.params['wlan'].append(node.name + '-wlan' + str(n))
                self.appendRSSI(node)
                self.appendSNR(node)
            node.params.pop("wlans", None)

        if (mode == 'managed'):
            self.addMacParamToNode(node, wlans, autoSetMacs, params)
            self.addIpParamToNode(node, wlans, autoSetMacs, params)

        self.addAntennaHeightParamToNode(node, wlans, params)
        self.addAntennaGainParamToNode(node, wlans, params)
        self.addTxPowerParamToNode(node, wlans, params)
        self.addChannelParamToNode(node, wlans, params)
        self.addModeParamToNode(node, wlans, params)

        # Equipment Model
        equipmentModel = ("%s" % params.pop('equipmentModel', {}))
        if(equipmentModel != "{}"):
            node.equipmentModel = equipmentModel

        # Range
        if 'range' in params:
            node.params['range'] = int(params['range'])
        else:
            if mode == 'master' or node.func[0] == 'ap':
                value = deviceRange(node)
                node.params['range'] = value.range
            else:
                value = deviceRange(node)
                node.params['range'] = value.range - 15

        if mode == 'master' or 'ssid' in node.params:
            node.params['associatedStations'] = []
            node.params['stationsInRange'] = {}
            node.wds = False

            if 'config' in node.params:
                config = node.params['config']
                if(config != []):
                    config = node.params['config'].split(',')
                    for conf in config:
                        if 'wpa=' in conf or 'wpe=' in conf:
                            node.params['encrypt'] = []
                        if 'wpa=' in conf:
                            node.params['encrypt'].append('wpa')
                        if 'wpe=' in conf:
                            node.params['encrypt'].append('wpe')

            if mode == 'master':
                node.params['mac'] = []
                node.params['mac'].append('')
                if 'mac' in params:
                    node.params['mac'][0] = params[ 'mac' ]

                ssid = ("%s" % params.pop('ssid', {}))
                ssid = ssid.split(',')
                node.params['ssid'] = []
                if(ssid[0] != "{}"):
                    if len(ssid) == 1:
                        node.params['ssid'].append(ssid[0])
                        if(encrypt != "{}"):
                            node.params['encrypt'].append(encrypt[0])
                        if(passwd != "{}"):
                            node.params['passwd'].append(passwd[0])
                    else:
                        for n in range(len(ssid)):
                            node.params['ssid'].append(ssid[n])
                            self.appendEncrypt(node, n, passwd, encrypt)
                else:
                    node.params['ssid'].append(params[ 'ssid' ])

    @classmethod
    def addParamsToNode(self, node):
        node.params['frequency'].append(2.412)
        node.func.append('none')
        node.phyID.append(0)

    @classmethod
    def appendAssociatedTo(self, node):
        node.params['associatedTo'].append('')

    @classmethod
    def appendSNR(self, node):
        node.params['snr'].append(40)

    @classmethod
    def appendEncrypt(self, node, n, passwd, encrypt):
        if(passwd != "{}"):
            if len(passwd) == 1:
                node.params['passwd'].append(passwd[0])
            else:
                node.params['passwd'].append(passwd[n])
        if(encrypt != "{}"):
            if len(encrypt) == 1:
                node.params['encrypt'].append(encrypt[0])
            else:
                node.params['encrypt'].append(encrypt[n])

    @classmethod
    def appendRSSI(self, node):
        node.params['rssi'].append(-60)

    @classmethod
    def addIpParamToNode(self, node, wlans=0, autoSetMacs=False, params=None, isVirtualIface=False):
        if isVirtualIface:
            node.params['ip'].append(node.params['ip'][0])
        else:
            ip = ("%s" % params.pop('ip', {}))
            if(ip != "{}"):
                ip = ip.split(',')
                node.params['ip'] = []
                for n in range(len(node.params['wlan'])):
                    node.params['ip'].append('0/0')
                    if len(ip) > n:
                        node.params['ip'][n] = ip[n]
            elif autoSetMacs:
                node.params['ip'] = []
                for n in range(wlans):
                    node.params['ip'].append('0/0')
                    node.params['ip'][n] = params[ 'ip' ]
            else:
                try:
                    for n in range(wlans):
                        node.params['ip'].append('0/0')
                except:
                    node.params['ip'] = []
                    node.params['ip'].append(params[ 'ip' ])
                    for n in range(1, wlans):
                        node.params['ip'].append('0/0')

    @classmethod
    def addMacParamToNode(self, node, wlans=0, autoSetMacs=False, params=None, isVirtualIface=False, macID=0):
        if isVirtualIface:
            new_mac = list(node.params['mac'][0])
            new_mac[7] = str(macID)
            node.params['mac'].append("".join(new_mac))
        else:
            mac = ("%s" % params.pop('mac', {}))
            if(mac != "{}"):
                mac = mac.split(',')
                node.params['mac'] = []
                for n in range(len(node.params['wlan'])):
                    node.params['mac'].append('')
                    if len(mac) > n:
                        node.params['mac'][n] = mac[n]
            elif autoSetMacs:
                node.params['mac'] = []
                for n in range(wlans):
                    node.params['mac'].append('')
                    node.params['mac'][n] = params[ 'mac' ]
            else:
                node.params['mac'] = []
                for n in range(wlans):
                    node.params['mac'].append('')

    @classmethod
    def addAntennaHeightParamToNode(self, node, wlans=0, params=None, isVirtualIface=False):
        if isVirtualIface:
            node.params['antennaHeight'].append(node.params['antennaHeight'][0])
        else:
            if 'antennaHeight' in params:
                node.params['antennaHeight'] = []
                for n in range(wlans):
                    node.params['antennaHeight'].append(int(params['antennaHeight']))
            else:
                node.params['antennaHeight'] = []
                for n in range(wlans):
                    node.params['antennaHeight'].append(1)

    @classmethod
    def addAntennaGainParamToNode(self, node, wlans=0, params=None, isVirtualIface=False):
        if isVirtualIface:
            node.params['antennaGain'].append(node.params['antennaGain'][0])
        else:
            if 'antennaGain' in params:
                node.params['antennaGain'] = []
                for n in range(wlans):
                    node.params['antennaGain'].append(int(params['antennaGain']))
            else:
                node.params['antennaGain'] = []
                for n in range(wlans):
                    node.params['antennaGain'].append(5)

    @classmethod
    def addModeParamToNode(self, node, wlans=0, params=None, isVirtualIface=False):
        if isVirtualIface:
            node.params['mode'].append(node.params['mode'][0])
        else:
            if 'mode' in params:
                node.params['mode'] = []
                for n in range(wlans):
                    node.params['mode'].append(params['mode'])
            else:
                node.params['mode'] = []
                for n in range(wlans):
                    node.params['mode'].append(params['mode'])

    @classmethod
    def addChannelParamToNode(self, node, wlans=0, params=None, isVirtualIface=False):
        if isVirtualIface:
            node.params['channel'].append(node.params['channel'][0])
        else:
            if 'channel' in params:
                node.params['channel'] = []
                for n in range(wlans):
                    node.params['channel'].append(int(params['channel']))
            else:
                node.params['channel'] = []
                for n in range(wlans):
                    node.params['channel'].append(1)

    @classmethod
    def addTxPowerParamToNode(self, node, wlans=0, params=None, isVirtualIface=False):
        if isVirtualIface:
            node.params['txpower'].append(node.params['txpower'][0])
        else:
            if 'txpower' in params:
                node.params['txpower'] = []
                for n in range(wlans):
                    node.params['txpower'].append(int(params['txpower']))
            else:
                node.params['txpower'] = []
                for n in range(wlans):
                    node.params['txpower'].append(14)

    @classmethod
    def countWiFiIfaces(self, params):
        wlans = ("%s" % params.pop('wlans', {}))
        if(wlans != "{}"):
            wlans = int(wlans)
            self.wifiRadios += int(wlans)
        else:
            wlans = 1
            self.wifiRadios += 1
        return wlans

    @classmethod
    def createVirtualIfaces(self, nodes):
        for node in nodes:
            if 'nvif' in node.params:
                nvif = node.params['nvif']
                wlan = 0
                for vif_ in range(0, nvif):
                    vif = node.params['wlan'][wlan] + str(vif_ + 1)
                    node.params['wlan'].append(vif)
                    self.addParamsToNode(node)
                    self.addTxPowerParamToNode(node, isVirtualIface=True)
                    self.addChannelParamToNode(node, isVirtualIface=True)
                    self.addMacParamToNode(node, isVirtualIface=True, macID=(vif_ + 1))
                    self.appendRSSI(node)
                    self.appendSNR(node)
                    self.appendAssociatedTo(node)
                    self.addAntennaGainParamToNode(node, isVirtualIface=True)
                    self.addAntennaHeightParamToNode(node, isVirtualIface=True)
                    node.cmd('iw dev %s interface add %s type station' % (node.params['wlan'][wlan], vif))
                    cls = TCLinkWirelessStation
                    cls(node, intfName1=vif)
                    self.configureMacAddr(node)

    @classmethod
    def addMesh(self, node, cls=None, **params):
        """
        Configure wireless mesh
        
        node: name of the node
        cls: custom association class/constructor
        params: parameters for node
        """
        if 'intf' in params:
            for intf_ in node.params['wlan']:
                if params['intf'] == intf_:
                    wlan = node.params['wlan'].index(intf_)
        else:
            if node.type == 'station':
                wlan = node.ifaceToAssociate
            else:
                wlan = 0

        node.func[wlan] = 'mesh'

        options = { 'ip': ipAdd(params['nextIP'],
                                  ipBaseNum=params['ipBaseNum'],
                                  prefixLen=params['prefixLen']) +
                                  '/%s' % params['prefixLen']}
        options.update(params)

        node.params['ssid'] = []
        if hasattr(node, 'meshMac'):
            for n in range(len(node.params['wlan'])):
                node.meshMac.append('')
                node.params['ssid'].append('')
        else:
            node.meshMac = []
            for n in range(len(node.params['wlan'])):
                node.meshMac.append('')
                node.params['ssid'].append('')

        ip = ("%s" % params.pop('ip', {}))
        if ip == "{}":
            ip = options['ip']

        ssid = ("%s" % params['ssid'])
        if(ssid != "{}"):
            node.params['ssid'][wlan] = ssid
        else:
            node.params['ssid'][wlan] = 'meshNetwork'

        deviceRange(node)

        value = deviceDataRate(sta=node, wlan=wlan)
        self.bw = value.rate

        options['node'] = node
        options.update(params)

        # Set default MAC - this should probably be in Link
        options.setdefault('addr1', self.randMac())

        cls = Association
        cls.configureMesh(node, wlan)
        if 'intf' not in params:
            node.ifaceToAssociate += 1

    @classmethod
    def addHoc(self, node, cls=None, **params):
        """
        Configure AdHoc
        
        node: name of the node
        cls: custom association class/constructor
        params: parameters for station
           
        """
        if 'intf' in params:
            for intf_ in node.params['wlan']:
                if params['intf'] == intf_:
                    wlan = node.params['wlan'].index(intf_)
        else:
            wlan = node.ifaceToAssociate

        node.func[wlan] = 'adhoc'

        options = { 'ip': ipAdd(params['nextIP'],
                                  ipBaseNum=params['ipBaseNum'],
                                  prefixLen=params['prefixLen']) +
                                  '/%s' % params['prefixLen']}
        options.update(params)

        node.params['cell'] = []
        node.params['ssid'] = []
        for w in range(0, len(node.params['wlan'])):
            node.params['cell'].append('')
            node.params['ssid'].append('')

        ip = ("%s" % params.pop('ip', {}))
        if ip == "{}":
            ip = options['ip']

        ssid = ("%s" % params.pop('ssid', {}))
        if(ssid != "{}"):
            node.params['ssid'][wlan] = ssid
            node.params['associatedTo'][wlan] = ssid
        else:
            node.params['ssid'][wlan] = 'adhocNetwork'
            node.params['associatedTo'][wlan] = 'adhocNetwork'

        cell = ("%s" % params.pop('cell', {}))
        if(cell != "{}"):
            node.params['cell'][wlan] = cell
        else:
            node.params['cell'][wlan] = 'FE:4C:6A:B5:A9:7E'

        deviceRange(node)

        value = deviceDataRate(sta=node, wlan=wlan)
        self.bw = value.rate

        options['sta'] = node
        options.update(params)
        # Set default MAC - this should probably be in Link
        options.setdefault('addr1', self.randMac())

        cls = Association
        cls.configureAdhoc(node)
        if 'intf' not in params:
            node.ifaceToAssociate += 1

    @staticmethod
    def randMac():
        "Return a random, non-multicast MAC address"
        return macColonHex(random.randint(1, 2 ** 48 - 1) & 0xfeffffffffff |
                            0x020000000000)

    @staticmethod
    def wmediumdConnect():
        WmediumdServerConn.connect()

    @classmethod
    def configureWmediumd(self, stations, accessPoints):
        """ 
        Updates values for frequency and channel
        
        :param stations: list of stations
        :param accessPoints: list of access points
        """
        intfrefs = []
        links = []
        positions = []
        txpowers = []
        nodes = stations + accessPoints

        for node in nodes:
            node.wmediumdIface = DynamicWmediumdIntfRef(node)
            intfrefs.append(node.wmediumdIface)

        if self.enable_interference:
            mode = WmediumdConstants.WMEDIUMD_MODE_INTERFERENCE
            for node in nodes:
                if 'position' not in node.params:
                    posX = 0
                    posY = 0
                    node.lastpos = [0, 0, 0]
                else:
                    posX = node.params['position'][0]
                    posY = node.params['position'][1]
                    node.lastpos = node.params['position']
                positions.append(WmediumdPosition(node.wmediumdIface, [posX, posY]))
                txpowers.append(WmediumdTXPower(node.wmediumdIface, float(node.params['txpower'][0])))
        else:
            mode = WmediumdConstants.WMEDIUMD_MODE_SNR
            for node in self.wlinks:
                links.append(WmediumdSNRLink(node[0].wmediumdIface, node[1].wmediumdIface, node[0].params['snr'][0]))
                links.append(WmediumdSNRLink(node[1].wmediumdIface, node[0].wmediumdIface, node[0].params['snr'][0]))

        WmediumdStarter.initialize(intfrefs, links, mode=mode, positions=positions, enable_interference=self.enable_interference, \
                                   auto_add_links=False, txpowers=txpowers, with_server=True)
        WmediumdStarter.start()

    @classmethod
    def updateParams(self, sta, ap, wlan):
        """ 
        Updates values for frequency and channel
        
        :param sta: station
        :param ap: access point
        :param wlan: wlan ID
        """

        sta.params['frequency'][wlan] = setChannelParams.frequency(ap, 0)
        sta.params['channel'][wlan] = ap.params['channel'][0]

    @classmethod
    def checkAPAdhoc(self, stations, accessPoints):
        """
        configure APAdhoc
        
        :param stations: list of stations
        :param accessPoints: list of access points
        """
        isApAdhoc = []
        for sta in stations:
            if sta.func[0] == 'ap':
                accessPoints.append(sta)
                isApAdhoc.append(sta)

        for ap in isApAdhoc:
            stations.remove(ap)
            ap.params.pop('rssi', None)
            ap.params.pop('snr', None)
            ap.params.pop('apsInRange', None)
            ap.params.pop('associatedTo', None)

            for wlan in (1, len(ap.params['wlan'])):
                ap.params['mac'].append('')

        return stations, accessPoints

    @classmethod
    def restartNetworkManager(self):
        """Restart network manager if the mac address of the AP is not included at 
        /etc/NetworkManager/NetworkManager.conf"""
        nm_is_running = os.system('service network-manager status 2>&1 | grep -ic running >/dev/null 2>&1')
        if AccessPoint.writeMacAddress and nm_is_running != 256:
            info('Mac Address(es) of AP(s) is(are) being added into /etc/NetworkManager/NetworkManager.conf\n')
            info('Restarting network-manager...\n')
            os.system('service network-manager restart')
        AccessPoint.writeMacAddress = False

    @classmethod
    def verifyNetworkManager(self, node, wlanID=0):
        """
        First verify if the mac address of the ap is included at NetworkManager.conf
        
        :param node: node
        :param wlanID: wlan ID
        """
        if 'phywlan' in node.params:
            wlanID = 1
        for wlan in range(len(node.params['wlan']) + wlanID):
            if wlanID == 1:
                wlan = 0
            if 'inNamespace' not in node.params or wlanID == 1:
                if node.type != 'station':
                    options = dict()
                    if 'phywlan' not in node.params:
                        intf = module.wlan_list[0]
                        module.wlan_list.pop(0)
                        node.renameIface(intf, node.params['wlan'][wlan])
                    else:
                        iface = node.params['phywlan']
                        options.setdefault('intfName1', iface)
                    cls = TCLinkWirelessAP
                    cls(node, **options)
            AccessPoint.setIPMAC(node, wlan)
            if 'phywlan' in node.params:
                node.params.pop("phywlan", None)
            if len(node.params['ssid']) > 1 and wlan == 0:
                break

    @classmethod
    def configureAP(self, ap, wlanID=0, aplist=None):
        """Configure AP
        
        :param ap: ap node
        :param wlanID: wlan ID
        """
        if 'phywlan' in ap.params:
            wlanID = 1
        for wlan in range(len(ap.params['wlan']) + wlanID):
            if wlanID == 1:
                wlan = 0
            if 'encrypt' in ap.params and 'config' not in ap.params:
                if ap.params['encrypt'][wlan] == 'wpa':
                    ap.auth_algs = 1
                    ap.wpa = 1
                    if 'ieee80211r' in ap.params:
                        ap.wpa_key_mgmt = 'FT-EAP'
                    else:
                        ap.wpa_key_mgmt = 'WPA-EAP'
                    ap.rsn_pairwise = 'TKIP CCMP'
                    ap.wpa_passphrase = ap.params['passwd'][0]
                elif ap.params['encrypt'][wlan] == 'wpa2':
                    ap.auth_algs = 1
                    ap.wpa = 2
                    if 'ieee80211r' in ap.params:
                        ap.wpa_key_mgmt = 'FT-PSK'
                    else:
                        ap.wpa_key_mgmt = 'WPA-PSK'
                    ap.rsn_pairwise = 'CCMP'
                    ap.wpa_passphrase = ap.params['passwd'][0]
                elif ap.params['encrypt'][wlan] == 'wep':
                    ap.auth_algs = 2
                    ap.wep_key0 = ap.params['passwd'][0]

            cls = AccessPoint
            cls(ap, wlan=wlan, aplist=aplist)

            if ap.func[0] != 'ap':
                ap.params['frequency'][wlan] = setChannelParams.frequency(ap, 0)
                wlanID = 0
            setChannelParams.recordParams(None, ap)

            if len(ap.params['ssid']) > 1 and wlan == 0:
                break

    @classmethod
    def configureAPs(self, accessPoints):
        """Configure All APs
        
        :param accessPoints: list of access points
        """
        for node in accessPoints:
            if len(node.params['ssid']) > 1:
                for i in range(1, len(node.params['ssid'])):
                    node.params['wlan'].append('%s-%s' % (node.params['wlan'][0], i))
                    node.params['mode'].append(node.params['mode'][0])
                    node.params['frequency'].append(node.params['frequency'][0])
                    node.params['mac'].append('')
            else:
                for i in range(1, len(node.params['wlan'])):
                    node.params['mac'].append('')
            self.verifyNetworkManager(node)
        self.restartNetworkManager()

        for ap in accessPoints:
            if 'link' not in ap.params:
                self.configureAP(ap, aplist=accessPoints)
                ap.phyID = module.phyID
                module.phyID += 1

    @classmethod
    def configureWirelessLink(self, stations, accessPoints, cars, switches):
        """
        Configure Wireless Link
        
        :param stations: list of stations
        :param accessPoints: list of access points
        :param cars: list of cars
        :param switches: list of switches
        """
        nodes = stations + cars
        stations, accessPoints = self.checkAPAdhoc(stations, accessPoints)
        for node in nodes:
            for wlan in range(0, len(node.params['wlan'])):
                if node not in switches:
                    cls = TCLinkWirelessStation
                    cls(node, intfName1=node.params['wlan'][wlan])
                if 'car' in node.name and node.type == 'station':
                        node.cmd('iw dev %s-wlan%s interface add %s-mp%s type mp' % (node, wlan, node, wlan))
                        node.cmd('ifconfig %s-mp%s up' % (node, wlan))
                        node.cmd('iw dev %s-mp%s mesh join %s' % (node, wlan, 'ssid'))
                        node.func[wlan] = 'mesh'
                elif node.type == 'station' and node in switches:
                    node.type = 'WirelessMeshAP'
                    self.configureMacAddr(node)
                else:
                    if 'ssid' not in node.params:
                        if node.params['txpower'][wlan] != 20:
                            node.setTxPower(node.params['wlan'][wlan], node.params['txpower'][wlan])
            if node not in switches:
                self.configureMacAddr(node)
        return stations, accessPoints

    @classmethod
    def plotGraph(self, max_x=0, max_y=0, max_z=0):
        """ 
        Plots Graph 
        
        :params max_x: maximum X
        :params max_y: maximum Y
        :params max_z: maximum Z
        """
        self.DRAW = True
        self.MAX_X = max_x
        self.MAX_Y = max_y
        if max_z != 0:
            self.MAX_Z = max_z
            self.is3d = True
            mobility.continuePlot = 'plot3d.graphPause()'

    @classmethod
    def checkDimension(self, nodes):
        try:
            if self.is3d:
                plot3d.instantiateGraph(self.MAX_X, self.MAX_Y, self.MAX_Z)
                plot3d.graphInstantiateNodes(nodes)
            else:
                plot2d.instantiateGraph(self.MAX_X, self.MAX_Y)
                plot2d.plotGraph(nodes, self.srcConn, self.dstConn)
                plot2d.graphPause()
        except:
            info('Warning: This OS does not support GUI. Running without GUI.\n')
            self.DRAW = False

    @classmethod
    def startGraph(self, stations, accessPoints, is3d=False):
        self.alreadyPlotted = True
        if not self.isMobility and self.DRAW:
            for sta in stations:
                if sta.func[0] == 'ap':
                    accessPoints.append(sta)
                    stations.remove(sta)

            if mobility.accessPoints == []:
                mobility.accessPoints = accessPoints
            if mobility.stations == []:
                mobility.stations = stations

            nodes = []
            nodes = self.plotNodes

            for ap in accessPoints:
                if 'position' in ap.params:
                    nodes.append(ap)

            for sta in stations:
                if 'position' in sta.params:
                    nodes.append(sta)

            self.checkDimension(nodes)

    @classmethod
    def startMobility(self, stations, accessPoints, **kwargs):
        "Starts Mobility"
        mobilityModel = ''
        self.isMobility = True

        if 'model' in kwargs:
            mobilityModel = kwargs['model']

        if 'AC' in kwargs:
            self.associationControlMethod = kwargs['AC']

        if mobilityModel != '' or self.isVanet:
            staMov = []
            for sta in stations:
                if 'position' not in sta.params:
                    staMov.append(sta)
                    sta.params['position'] = 0, 0, 0

            if not self.isVanet:
                params = self.setMobilityParams(stations, accessPoints, staMov, **kwargs)
                mobility.start(**params)
            else:
                params = self.setMobilityParams(stations, accessPoints, staMov, **kwargs)
                vanet(**params)

        info("Mobility started at %s second(s)\n" % kwargs['time'])

    @classmethod
    def stopMobility(self, stations, accessPoints, **kwargs):
        "Stops Mobility"
        kwargs['is3d'] = self.is3d
        params = self.setMobilityParams(stations, accessPoints, **kwargs)
        mobility.stop(**params)

    @classmethod
    def setMobilityParams(self, stations, accessPoints, staMov=[], **kwargs):
        "Set Mobility Parameters"
        mobilityparam = dict()

        if 'model' in kwargs:
            mobilityparam.setdefault('model', kwargs['model'])
        if 'time' in kwargs:
            mobilityparam.setdefault('final_time', kwargs['time'])
        if self.nroads != 0:
            mobilityparam.setdefault('nroads', self.nroads)

        if 'model' in kwargs or self.isVanet:
            if 'max_x' in kwargs:
                self.MAX_X = kwargs['max_x']
            if 'max_y' in kwargs:
                self.MAX_Y = kwargs['max_y']
            if 'min_v' in kwargs:
                mobilityparam.setdefault('min_v', kwargs['min_v'])
            if 'max_v' in kwargs:
                mobilityparam.setdefault('max_v', kwargs['max_v'])
            if 'time' in kwargs:
                self.init_time = kwargs['time']

        mobilityparam.setdefault('seed', self.seed_)
        mobilityparam.setdefault('DRAW', self.DRAW)
        mobilityparam.setdefault('plotNodes', self.plotNodes)
        mobilityparam.setdefault('stations', stations)
        mobilityparam.setdefault('aps', accessPoints)
        mobilityparam.setdefault('dstConn', self.dstConn)
        mobilityparam.setdefault('srcConn', self.srcConn)
        mobilityparam.setdefault('MAX_X', self.MAX_X)
        mobilityparam.setdefault('MAX_Y', self.MAX_Y)
        mobilityparam.setdefault('MAX_Z', self.MAX_Z)
        mobilityparam.setdefault('AC', self.associationControlMethod)
        mobilityparam.setdefault('rec_rssi', self.rec_rssi)
        mobilityparam.setdefault('init_time', self.init_time)
        mobilityparam.setdefault('staMov', staMov)
        return mobilityparam

    @classmethod
    def useExternalProgram(self, **params):
        """
        Opens an external program
        
        :params program: any program (useful for SUMO)
        :params **params config_file: file configuration
        """
        self.isVanet = True
        for car in params['cars']:
            car.params['position'] = 0, 0, 0
        if params['program'] == 'sumo' or params['program'] == 'sumo-gui':
            sumo(**params)

    @classmethod
    def configureMacAddr(self, node):
        """
        Configure Mac Address
        
        :param node: node
        """
        for wlan in range(0, len(node.params['wlan'])):
            iface = node.params['wlan'][wlan]
            if node.params['mac'][wlan] == '':
                node.params['mac'][wlan] = node.getMAC(iface)
            else:
                mac = node.params['mac'][wlan]
                node.setMAC(mac, iface)
            if node.type == 'WirelessMeshAP':
                node.convertIfaceToMesh(node, wlan)
                cls = TCLinkWirelessAP
                cls(node, intfName1=node.params['wlan'][wlan])

    @classmethod
    def configureWifiNodes(self, stations, accessPoints, cars, switches, \
                           nextIP, ipBaseNum, prefixLen, useWmediumd):
        """
        Configure WiFi Nodes
        
        :param stations: list of stations
        :param accessPoints: list of access points
        :param cars: list of cars
        :param switches: list of switches
        :param wifiRadios: number of wireless radios
        :param wmediumd: loads wmediumd 
        """
        self.useWmediumd = useWmediumd

        params = {}
        if self.ifb:
            setChannelParams.ifb = True
            params['ifb'] = self.ifb
        params['useWmediumd'] = useWmediumd
        nodes = stations + accessPoints + cars
        module.start(nodes, self.wifiRadios, self.alternativeModule, **params)
        self.configureWirelessLink(stations, accessPoints, cars, switches)
        self.createVirtualIfaces(stations)
        self.configureAPs(accessPoints)
        self.isWiFi = True

        # useful if there no link between sta and any other device
        for car in cars:
            self.addMesh(car.params['carsta'], nextIP, ipBaseNum, prefixLen, ssid='mesh-ssid')
            stations.remove(car.params['carsta'])
            stations.append(car)
            car.params['wlan'].append(0)
            car.params['rssi'].append(0)
            car.params['snr'].append(0)
            car.params['channel'].append(0)
            car.params['txpower'].append(0)
            car.params['antennaGain'].append(0)
            car.params['antennaHeight'].append(0)
            car.params['associatedTo'].append('')
            car.params['frequency'].append(0)

        return stations, accessPoints

    @classmethod
    def autoAssociation(self, stations, accessPoints):
        """
        This is useful to make the users' life easier
        
        :param stations: list of stations
        :param accessPoints: list of access points
        """
        ap = []
        for node in accessPoints:
            if 'link' in node.params:
                ap.append(node)

        nodes = stations + ap

        if not self.isVanet:
            for node in nodes:
                pairingAdhocNodes.ssid_ID += 1
                for wlan in range(0, len(node.params['wlan'])):
                    if 'position' in node.params and 'link' not in node.params:
                        mobility.accessPoints = accessPoints
                        mobility.handoverCheck(node, wlan)
                    if 'position' in node.params and node.func[wlan] == 'adhoc' and node.params['associatedTo'][wlan] == '':
                        value = pairingAdhocNodes(node, wlan, nodes)
                        dist = value.dist
                        if dist >= 0.01:
                            setChannelParams(sta=node, wlan=wlan, dist=dist)
                    elif 'position' in node.params and node.func[wlan] == 'mesh':
                        if node.type == 'vehicle':
                            node = node.params['carsta']
                            wlan = 0
                        dist = listNodes.pairingNodes(node, wlan, nodes)
                        if dist >= 0.01:
                            setChannelParams(sta=node, wlan=wlan, dist=dist)
                if meshRouting.routing == 'custom':
                    meshRouting(nodes)

    @classmethod
    def propagationModel(self, stations, accessPoints, model, exp=2, sL=1, lF=0, pL=0, nFloors=0, gRandom=0):
        """
        Attributes for Propagation Model

        :params model: propagation model
        :params exp: exponent
        :params sL: system Loss
        :params lF: floor penetration loss factor
        :params pL: power Loss Coefficient
        :params nFloors: number of floors
        :params gRandom: gaussian random variable
        """
        propagationModel.model = model
        propagationModel.exp = exp
        setChannelParams.sl = sL
        setChannelParams.lF = lF
        setChannelParams.nFloors = nFloors
        setChannelParams.gRandom = gRandom
        setChannelParams.pL = pL

        for sta in stations:
            if 'position' in sta.params and sta not in mobility.stations:
                mobility.stations.append(sta)
        for ap in accessPoints:
            if 'position' in ap.params and ap not in mobility.accessPoints:
                mobility.accessPoints.append(ap)

    @classmethod
    def meshRouting(self, routing):
        """
        Defines the mesh routing

        :params routing: the mesh routing (default: custom)
        """
        if routing != '':
            meshRouting.routing = routing

    @classmethod
    def getDistance(self, src, dst):
        dist = setChannelParams.getDistance(src, dst)
        return dist

    @classmethod
    def printDistance(self, src, dst, nodes):
        """ 
        Prints the distance between two points
        
        :params src: source node
        :params dst: destination node
        :params nodes: list of nodes
        """
        try:
            for host1 in nodes:
                if src == str(host1):
                    src = host1
                    for host2 in nodes:
                        if dst == str(host2):
                            dst = host2
                            dist = self.getDistance(src, dst)
                            info ("The distance between %s and %s is %.2f meters\n" % (src, dst, float(dist)))
        except:
            print ("node %s or/and node %s does not exist or there is no position defined" % (dst, src))

    @classmethod
    def configureMobility(self, *args, **kwargs):
        "Configure mobility parameters"
        mobility.configure(*args, **kwargs)

    @classmethod
    def setDataRate(self, sta=None, ap=None, wlan=0):
        value = deviceDataRate(sta, ap, wlan)
        return value

    @classmethod
    def associationControl(self, ac):
        """Defines an association control
        :params ac: the association control
        """
        mobility.associationControlMethod = ac

    @classmethod
    def setChannelEquation(self, **params):
        """ 
        Set Channel Equation. The user may change the equation defined in wifiChannel.py by any other.
        
        :params bw: bandwidth (mbps)
        :params delay: delay (ms)
        :params latency: latency (ms)
        :params loss: loss (%)
        """
        if 'bw' in params:
            setChannelParams.equationBw = params['bw']
        if 'delay' in params:
            setChannelParams.equationDelay = params['delay']
        if 'latency' in params:
            setChannelParams.equationLatency = params['latency']
        if 'loss' in params:
            setChannelParams.equationLoss = params['loss']

    @classmethod
    def closeMininetWiFi(self):
        "Close Mininet-WiFi"
        mobility.continuePlot = 'exit()'
        mobility.continueParams = 'exit()'
        sleep(2)
        if self.is3d:
            plot3d.closePlot()
        else:
            plot2d.closePlot()
        module.stop()  # Stopping WiFi Module

        if self.useWmediumd:
            WmediumdServerConn.disconnect()
            WmediumdStarter.stop()
