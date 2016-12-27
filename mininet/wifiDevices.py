"""
Devices list to Mininet-WiFi.

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

"""

class deviceDataRate (object):
    """ Data Rate for specific equipments """

    rate = 0

    def __init__(self, sta=None, ap=None, wlan=0):
        
        """         
        :param sta: station
        :param ap: accessPoint
        :param wlan: wlan ID
        """

        if ap != None and 'equipmentModel' in ap.params.keys():
            if  ap.equipmentModel in dir(self) and sta != None:
                model = ap.equipmentModel
                self.__getattribute__(model)(sta, ap, wlan)
        elif ap != None:
            self.customDataRate_mobility(ap, 0)
        elif sta != None:
            self.customDataRate_mobility(sta, wlan)

    def customDataRate_mobility(self, node, wlan):
        """Custom Maximum Data Rate - Useful when there is mobility"""
        mode = node.params['mode'][wlan]

        if (mode == 'a'):
            self.rate = 11
        elif(mode == 'b'):
            self.rate = 3
        elif(mode == 'g'):
            self.rate = 11
        elif(mode == 'n'):
            self.rate = 600
        elif(mode == 'ac'):
            self.rate = 6777
        return self.rate

    def customDataRate_no_mobility(self, node, wlan):
        """Custom Maximum Data Rate - Useful when there is no mobility"""
        mode = node.params['mode'][wlan]

        if (mode == 'a'):
            self.rate = 20
        elif(mode == 'b'):
            self.rate = 6
        elif(mode == 'g'):
            self.rate = 20
        elif(mode == 'n'):
            self.rate = 48
        elif(mode == 'ac'):
            self.rate = 90
        return self.rate

    def DI524(self, node1, node2, wlan):
        """D-Link AirPlus G DI-524
           from http://www.dlink.com/-/media/Consumer_Products/DI/DI%20524/Manual/DI_524_Manual_EN_UK.pdf"""
        if node1.params['rssi'][wlan] != 0:
            if (node1.params['rssi'][wlan] >= -68):
                self.rate = 48
            elif (node1.params['rssi'][wlan] < -68 and node1.params['rssi'][wlan] >= -75):
                self.rate = 36
            elif (node1.params['rssi'][wlan] < -75 and node1.params['rssi'][wlan] >= -79):
                self.rate = 24
            elif (node1.params['rssi'][wlan] < -79 and node1.params['rssi'][wlan] >= -84):
                self.rate = 18
            elif (node1.params['rssi'][wlan] < -84 and node1.params['rssi'][wlan] >= -87):
                self.rate = 9
            elif (node1.params['rssi'][wlan] < -87 and node1.params['rssi'][wlan] >= -88):
                self.rate = 6
            elif (node1.params['rssi'][wlan] < -88 and node1.params['rssi'][wlan] >= -89):
                self.rate = 1
 
        return self.rate

    def TLWR740N(self, node1, node2, wlan):
        """TL-WR740N
           from http://www.tp-link.com.br/products/details/cat-9_TL-WR740N.html#specificationsf"""
        mode = node1.params['mode'][wlan]
        try:  # if Station
            if node1.params['rssi'][wlan] != 0:
                if (node1.params['rssi'][wlan] >= -68):
                    if mode == 'n':
                        self.rate = 130 - 100
                    elif mode == 'g':
                        self.rate = 54 - 43
                    elif mode == 'b':
                        self.rate = 11 - 8
                elif (node1.params['rssi'][wlan] < -68 and node1.params['rssi'][wlan] >= -85):
                    self.rate = 11
                elif (node1.params['rssi'][wlan] < -85 and node1.params['rssi'][wlan] >= -88):
                    self.rate = 6
                elif (node1.params['rssi'][wlan] < -88 and node1.params['rssi'][wlan] >= -90):
                    self.rate = 1
        except:  # if AP
            if mode == 'n':
                self.rate = 130
            elif mode == 'g':
                self.rate = 54
            elif mode == 'b':
                self.rate = 11

        return self.rate

    def WRT120N(self, node1, node2, wlan):
        """CISCO WRT120N
           from http://downloads.linksys.com/downloads/datasheet/WRT120N_V10_DS_B-WEB.pdf"""
        mode = node2.params['mode'][0]
                    
        try:  # if Station
            if node1.params['rssi'][wlan] != 0:
                if (node1.params['rssi'][wlan] >= -65):
                    if mode == 'n':
                        self.rate = 150
                    elif mode == 'g':
                        self.rate = 54
                    elif mode == 'b':
                        self.rate = 11
                elif (node1.params['rssi'][wlan] < -65 and node1.params['rssi'][wlan] >= -68):
                    if mode == 'g':
                        self.rate = 54
                    elif mode == 'b':
                        self.rate = 11
                elif (node1.params['rssi'][wlan] < -68 and node1.params['rssi'][wlan] >= -85):
                    self.rate = 11
                elif (node1.params['rssi'][wlan] < -85 and node1.params['rssi'][wlan] >= -90):
                    self.rate = 1
        except:  # if AP
            if node2.params['mode'][0] == 'n':
                self.rate = 150
            elif node2.params['mode'][0] == 'g':
                self.rate = 54
            elif node2.params['mode'][0] == 'b':
                self.rate = 11
        return self.rate

class deviceRange (object):
    """ Range for specific equipments """

    range = 100

    def __init__(self, ap=None, wlan=0):
        
        """         
        :param ap: accessPoint
        :param wlan: wlan ID
        """
        
        if 'equipmentModel' in ap.params.keys():        
            if ap.equipmentModel in dir(self):
                self.__getattribute__(ap.equipmentModel)(ap)
        else:
            self.customSignalRange(ap, wlan)

    def customSignalRange(self, node, wlan):
        """Custom Signal Range"""
        mode = node.params['mode'][wlan]

        if (mode == 'a'):
            self.range = 33
        elif(mode == 'b'):
            self.range = 50
        elif(mode == 'g'):
            self.range = 33
        elif(mode == 'n'):
            self.range = 70
        elif(mode == 'ac'):
            self.range = 100

        return self.range

    def DI524(self, ap):
        """ D-Link AirPlus G DI-524
            from http://www.dlink.com/-/media/Consumer_Products/DI/DI%20524/Manual/DI_524_Manual_EN_UK.pdf
            indoor = 100
            outdoor = 200 """

        self.range = 100
        return self.range

    def TLWR740N(self, ap):
        """TL-WR740N
        NO REFERENCE!"""

        self.range = 50
        return self.range

    def WRT120N(self, ap):
        """ CISCO WRT120N
        NO REFERENCE!"""

        self.range = 50
        return self.range

class deviceTxPower (object):
    """ TX Power for specific equipments """

    txPower = 0

    def __init__(self, model=None, ap=None, wlan=0):
        
        """         
        :param model: device model
        :param ap: accessPoint
        :param wlan: wlan ID
        """

        if model in dir(self):
            self.__getattribute__(model)(ap, wlan)

    def DI524(self, ap, wlan):
        """ D-Link AirPlus G DI-524
            from http://www.dlink.com/-/media/Consumer_Products/DI/DI%20524/Manual/DI_524_Manual_EN_UK.pdf"""
        self.txPower = 14
        return self.txPower

    def TLWR740N(self, ap, wlan):
        """TL-WR740N
        No REFERENCE!"""
        self.txPower = 20
        return self.txPower

    def WRT120N(self, ap, wlan):
        """CISCO WRT120N
           from http://downloads.linksys.com/downloads/datasheet/WRT120N_V10_DS_B-WEB.pdf"""
        if ap.params['mode'][wlan] == 'b':
            self.txPower = 21
        elif ap.params['mode'][wlan] == 'g':
            self.txPower = 18
        elif ap.params['mode'][wlan] == 'n':
            self.txPower = 16
        return self.txPower