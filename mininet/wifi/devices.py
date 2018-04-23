"""

    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)


"""


class GetRate (object):
    "Data Rate for specific equipments"

    rate = 0

    def __init__(self, sta=None, ap=None, wlan=0):

        """sta: station
        ap: accessPoint
        wlan: wlan ID"""

        if ap and 'equipmentModel' in ap.params.keys():
            if  ap.equipmentModel in dir(self) and sta != None:
                model = ap.equipmentModel
                self.__getattribute__(model)(sta, ap, wlan)
        elif ap:
            self.customDataRate_mobility(ap, 0)
        elif sta:
            self.customDataRate_mobility(sta, wlan)

    def customDataRate_mobility(self, node, wlan):
        """Custom Maximum Data Rate - Useful when there is mobility
        
        mode: interface mode
        rate: maximum supported bandwidth (mbps)"""
        mode = node.params['mode'][wlan]
        rate = 0

        if mode == 'a':
            rate = 11
        elif mode == 'b':
            rate = 3
        elif mode == 'g':
            rate = 11
        elif mode == 'n':
            rate = 600
        elif mode == 'ac':
            rate = 1000

        self.rate = rate
        return self.rate

    def customDataRate_no_mobility(self, node, wlan):
        """Custom Maximum Data Rate - Useful when there is no mobility
                
        mode: interface mode
        rate: maximum supported bandwidth (mbps)"""
        mode = node.params['mode'][wlan]
        rate = 0

        if mode == 'a':
            rate = 20
        elif mode == 'b':
            rate = 6
        elif mode == 'g':
            rate = 20
        elif mode == 'n':
            rate = 48
        elif mode == 'ac':
            rate = 90

        self.rate = rate
        return self.rate

    @classmethod
    def apRate(cls, node, wlan):
        # If a custom rate was set.
        if 'rate' in node.params:
            cls.rate = node.params['rate']
            return cls.rate

        mode = node.params['mode'][wlan]

        if mode == 'a':
            cls.rate = 54
        elif mode == 'b':
            cls.rate = 11
        elif mode == 'g':
            cls.rate = 54
        elif mode == 'n':
            cls.rate = 600
        elif mode == 'ac':
            cls.rate = 6777
        else:
            cls.rate = 54
        return cls.rate

    def DI524(self, node1, node2, wlan):
        """D-Link AirPlus G DI-524
           from http://www.dlink.com/-/media/Consumer_Products/
           DI/DI%20524/Manual/DI_524_Manual_EN_UK.pdf
           
        rssi: rssi value (dBm)
        rate: maximum supported bandwidth (mbps)"""
        rate = 0

        if node1.params['rssi'][wlan] != 0:
            if node1.params['rssi'][wlan] >= -68:
                rate = 48
            elif -75 <= node1.params['rssi'][wlan] < -68:
                rate = 36
            elif -79 <= node1.params['rssi'][wlan] < -75:
                rate = 24
            elif -84 <= node1.params['rssi'][wlan] < -79:
                rate = 18
            elif -87 <= node1.params['rssi'][wlan] < -84:
                rate = 9
            elif -88 <= node1.params['rssi'][wlan] < -87:
                rate = 6
            elif -89 <= node1.params['rssi'][wlan] < -88:
                rate = 1

        self.rate = rate
        return self.rate

    def TLWR740N(self, node1, node2, wlan):
        """TL-WR740N
           from http://www.tp-link.com.br/products/details/
           cat-9_TL-WR740N.html#specificationsf
        
        mode: interface mode
        rssi: rssi value (dBm)
        rate: maximum supported bandwidth (mbps)"""
        mode = node1.params['mode'][wlan]
        rate = 0

        try:  # if Station
            if node1.params['rssi'][wlan] != 0:
                if node1.params['rssi'][wlan] >= -68:
                    if mode == 'n':
                        rate = 130
                    elif mode == 'g':
                        rate = 54
                    elif mode == 'b':
                        rate = 11
                elif -85 <= node1.params['rssi'][wlan] < -68:
                    rate = 11
                elif -88 <= node1.params['rssi'][wlan] < -85:
                    rate = 6
                elif -90 <= node1.params['rssi'][wlan] < -88:
                    rate = 1
        except:  # if AP
            if mode == 'n':
                rate = 130
            elif mode == 'g':
                rate = 54
            elif mode == 'b':
                rate = 11

        self.rate = rate
        return self.rate

    def WRT120N(self, node1, node2, wlan):
        """CISCO WRT120N
           from http://downloads.linksys.com/downloads/datasheet/
           WRT120N_V10_DS_B-WEB.pdf
        
        mode: interface mode
        rssi: rssi value (dBm)
        rate: maximum supported bandwidth (mbps)"""
        mode = node2.params['mode'][0]
        rate = 0

        try:  # if Station
            if node1.params['rssi'][wlan] != 0:
                if node1.params['rssi'][wlan] >= -65:
                    if mode == 'n':
                        rate = 150
                    elif mode == 'g':
                        rate = 54
                    elif mode == 'b':
                        rate = 11
                elif -68 <= node1.params['rssi'][wlan] < -65:
                    if mode == 'g':
                        rate = 54
                    elif mode == 'b':
                        rate = 11
                elif -85 <= node1.params['rssi'][wlan] < -68:
                    rate = 11
                elif -90 <= node1.params['rssi'][wlan] < -85:
                    rate = 1
        except:  # if AP
            if node2.params['mode'][0] == 'n':
                rate = 150
            elif node2.params['mode'][0] == 'g':
                rate = 54
            elif node2.params['mode'][0] == 'b':
                rate = 11

        self.rate = rate
        return self.rate


class GetRange (object):
    "Range for specific equipments"

    value = 100

    def __init__(self, ap=None, wlan=0):

        """ ap: accessPoint
        wlan: wlan ID """

        if 'equipmentModel' in ap.params:
            if ap.params['equipmentModel'] in dir(self):
                self.__getattribute__(ap.params['equipmentModel'])(ap)
        else:
            self.customSignalRange(ap, wlan)

    def customSignalRange(self, node, wlan):
        """Custom Signal Range
        
        mode: interface mode
        range: signal range (m)"""
        mode = node.params['mode'][wlan]

        if mode == 'a':
            range_ = 33
        elif mode == 'b':
            range_ = 50
        elif mode == 'g':
            range_ = 33
        elif mode == 'n':
            range_ = 70
        elif mode == 'ac':
            range_ = 100
        else:
            range_ = 33

        self.value = range_
        return self.value

    def DI524(self, ap):
        """ D-Link AirPlus G DI-524
            from http://www.dlink.com/-/media/Consumer_Products/DI/
            DI%20524/Manual/DI_524_Manual_EN_UK.pdf
            indoor = 100
            outdoor = 200 
        
        range: signal range (m)"""

        self.value = 100
        return self.value

    def TLWR740N(self, ap):
        """TL-WR740N
            NO REFERENCE!
        
        range: signal range (m)"""

        self.value = 50
        return self.value

    def WRT120N(self, ap):
        """CISCO WRT120N
            NO REFERENCE!
        
        range: signal range (m)"""

        self.value = 50
        return self.value


class GetTxPower (object):
    "TX Power for specific equipments"

    value = 0

    def __init__(self, ap=None, wlan=0):

        """:param model: device model
        :param ap: accessPoint
        :param wlan: wlan ID"""

        if 'equipmentModel' in ap.params:
            if ap.params['equipmentModel'] in dir(self):
                self.__getattribute__(ap.params['equipmentModel'])(ap, wlan)

    def DI524(self, ap, wlan):
        """D-Link AirPlus G DI-524
            from http://www.dlink.com/-/media/Consumer_Products/DI/
            DI%20524/Manual/DI_524_Manual_EN_UK.pdf
        
        txPower = transmission power (dBm)"""
        self.value = 14
        return self.value

    def TLWR740N(self, ap, wlan):
        """TL-WR740N
            No REFERENCE!
        
        txPower = transmission power (dBm)"""
        self.value = 20
        return self.value

    def WRT120N(self, ap, wlan):
        """CISCO WRT120N
           from http://downloads.linksys.com/downloads/datasheet/
           WRT120N_V10_DS_B-WEB.pdf
           
        txPower = transmission power (dBm)"""

        if ap.params['mode'][wlan] == 'b':
            self.value = 21
        elif ap.params['mode'][wlan] == 'g':
            self.value = 18
        elif ap.params['mode'][wlan] == 'n':
            self.value = 16
        return self.value
