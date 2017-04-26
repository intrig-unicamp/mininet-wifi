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
    def addMesh(self, node, nextIP, ipBaseNum, prefixLen, cls=None, **params):
        """
        Configure wireless mesh
        
        node: name of the node
        cls: custom association class/constructor
        params: parameters for node
        """
        if node.type == 'station':
            wlan = node.ifaceToAssociate
        else:
            wlan = 0
        node.func[wlan] = 'mesh'

        options = { 'ip': ipAdd(nextIP,
                                  ipBaseNum=ipBaseNum,
                                  prefixLen=prefixLen) + 
                                  '/%s' % prefixLen}
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
        node.ifaceToAssociate += 1
        
    @classmethod
    def addHoc(self, node, nextIP, ipBaseNum, prefixLen, cls=None, **params):
        """
        Configure AdHoc
        
        node: name of the node
        cls: custom association class/constructor
        params: parameters for station
           
        """
        wlan = node.ifaceToAssociate
        node.func[wlan] = 'adhoc'
        
        options = { 'ip': ipAdd(nextIP,
                                  ipBaseNum=ipBaseNum,
                                  prefixLen=prefixLen) + 
                                  '/%s' % prefixLen}
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
                else:
                    posX = node.params['position'][0]
                    posY = node.params['position'][1]
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
            if len(node.params['ssid']) > 1 and wlan==0:
                break
                
    @classmethod    
    def configureAP(self, ap, wlanID=0):
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
                    ap.wpa_key_mgmt = 'WPA-EAP'
                    ap.rsn_pairwise = 'TKIP CCMP'
                    ap.wpa_passphrase = ap.params['passwd'][0]
                elif ap.params['encrypt'][wlan] == 'wpa2':
                    ap.auth_algs = 1
                    ap.wpa = 2
                    ap.wpa_key_mgmt = 'WPA-PSK'
                    ap.rsn_pairwise = 'CCMP'
                    ap.wpa_passphrase = ap.params['passwd'][0]
                elif ap.params['encrypt'][wlan] == 'wep':
                    ap.auth_algs = 2
                    ap.wep_key0 = ap.params['passwd'][0]
                
            cls = AccessPoint
            cls(ap, wlan=wlan)
            
            if ap.func[0] != 'ap':
                ap.params['frequency'][wlan] = setChannelParams.frequency(ap, 0)
                wlanID = 0
            setChannelParams.recordParams(None, ap)

            if len(ap.params['ssid']) > 1 and wlan==0:
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
                self.configureAP(ap)
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
        if self.nroads!=0:
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
    def configureWifiNodes(self, stations, accessPoints, cars, switches, wifiRadios, \
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
        module.start(nodes, wifiRadios, self.alternativeModule, **params)               
        self.configureWirelessLink(stations, accessPoints, cars, switches) 
        self.configureAPs(accessPoints)
        isWiFi = True 
        
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
        
        return stations, accessPoints, isWiFi

    @classmethod
    def getAPsInRange(self, sta, accessPoints):
        """ 
        
        :param sta: station
        :param accessPoints: list of access points
        """
        for ap in accessPoints:
            if 'ssid' in ap.params and len(ap.params['ssid']) > 1:
                break
            if 'position' in ap.params:
                dist = setChannelParams.getDistance(sta, ap)
                if dist < ap.params['range']:
                    for wlan in range(0, len(sta.params['wlan'])):
                        cls = Association
                        cls.configureWirelessLink(sta, ap, wlan, self.useWmediumd)
                        if self.rec_rssi:
                            os.system('hwsim_mgmt -k %s %s >/dev/null 2>&1' % (sta.phyID[wlan], abs(int(sta.params['rssi'][wlan]))))
            
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
                if 'position' in node.params and 'link' not in node.params:
                    self.getAPsInRange(node, accessPoints)
                for wlan in range(0, len(node.params['wlan'])):
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
    def getDistance(self, src, dst, nodes):
        """ 
        Gets the distance between two nodes
        
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
                            self.printDistance(src, dst)
        except:
            print ("node %s or/and node %s does not exist or there is no position defined" % (dst, src))
            
    @classmethod
    def printDistance(self, src, dst):
        """ 
        Prints the distance between two points
        
        :params src: source node
        :params dst: destination node
        """
        dist = setChannelParams.getDistance(src, dst)
        info ("The distance between %s and %s is %.2f meters\n" % (src, dst, float(dist)))
                    
    @classmethod
    def closeMininetWiFi(self ):
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
        