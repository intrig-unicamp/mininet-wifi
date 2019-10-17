"""Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
   author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)"""


class CustomRate(object):

    rate = 0

    def __init__(self, node, wlan):
        self.customDataRate_mobility(node, wlan)

    def customDataRate_mobility(self, node, wlan):
        """Custom Maximum Data Rate - Useful when there is mobility
        mode: interface mode
        rate: maximum supported bandwidth (mbps)"""
        mode = node.params['mode'][wlan]

        if mode == 'a':
            self.rate = 11
        elif mode == 'b':
            self.rate = 3
        elif mode == 'g':
            self.rate = 11
        elif mode == 'n':
            self.rate = 600
        elif mode == 'ac':
            self.rate = 1000

        return self.rate


class DeviceRate(object):
    "Data Rate for specific equipments"

    rate = 0

    def __init__(self, node, wlan):
        model = node.params['model']
        self.__getattribute__(model)(node, wlan)

    def DI524(self, node, wlan):
        """D-Link AirPlus G DI-524
           from http://www.dlink.com/-/media/Consumer_Products/
           DI/DI%20524/Manual/DI_524_Manual_EN_UK.pdf
        rate: maximum supported bandwidth (mbps)"""
        mode = node.params['mode'][wlan]
        if mode == 'n':
            self.rate = 130
        elif mode == 'g':
            self.rate = 54
        elif mode == 'b':
            self.rate = 11

        return self.rate

    def TLWR740N(self, node, wlan):
        """TL-WR740N
           from http://www.tp-link.com.br/products/details/
           cat-9_TL-WR740N.html#specificationsf
        mode: interface mode
        rate: maximum supported bandwidth (mbps)"""
        mode = node.params['mode'][wlan]

        if mode == 'n':
            self.rate = 130
        elif mode == 'g':
            self.rate = 54
        elif mode == 'b':
            self.rate = 11

        return self.rate

    def WRT120N(self, node, wlan):
        """CISCO WRT120N
           from http://downloads.linksys.com/downloads/datasheet/
           WRT120N_V10_DS_B-WEB.pdf
        mode: interface mode
        rate: maximum supported bandwidth (mbps)"""
        mode = node.params['mode'][wlan]

        if mode == 'n':
            self.rate = 150
        elif mode == 'g':
            self.rate = 54
        elif mode == 'b':
            self.rate = 11

        return self.rate


class CustomRange(object):

    range = 0

    def __init__(self, node, wlan):
        self.customSignalRange(node, wlan)

    def customSignalRange(self, node, wlan):
        """Custom Signal Range
        mode: interface mode
        range: signal range (m)"""
        mode = node.params['mode'][wlan]

        if mode == 'a' or mode == 'g':
            self.range = 33
        elif mode == 'b':
            self.range = 50
        elif mode == 'n':
            self.range = 70
        elif mode == 'ac':
            self.range = 100
        else:
            self.range = 33

        return self.range


class DeviceRange (object):
    "Range for specific equipments"

    range = 100

    def __init__(self, node):
        self.__getattribute__(node.params['model'])()

    def DI524(self):
        """ D-Link AirPlus G DI-524
            from http://www.dlink.com/-/media/Consumer_Products/DI/
            DI%20524/Manual/DI_524_Manual_EN_UK.pdf
            indoor = 100
            outdoor = 200
        range: signal range (m)"""

        self.range = 100
        return self.range

    def TLWR740N(self):
        """TL-WR740N
            NO REFERENCE!
        range: signal range (m)"""

        self.range = 50
        return self.range

    def WRT120N(self):
        """CISCO WRT120N
            NO REFERENCE!
        range: signal range (m)"""

        self.range = 50
        return self.range


class DeviceTxPower (object):
    "TX Power for specific equipments"

    txpower = 0

    def __init__(self, node, wlan):
        "get txpower"
        model = node.params['model']
        self.__getattribute__(model)(node, wlan)

    def DI524(self, **kwargs):
        """D-Link AirPlus G DI-524
            from http://www.dlink.com/-/media/Consumer_Products/DI/
            DI%20524/Manual/DI_524_Manual_EN_UK.pdf
        txPower = transmission power (dBm)"""
        self.txpower = 14
        return self.txpower

    def TLWR740N(self, node, wlan):
        """TL-WR740N
            No REFERENCE!
        txPower = transmission power (dBm)"""
        self.txpower = 20
        return self.txpower

    def WRT120N(self, node, wlan):
        """CISCO WRT120N
           from http://downloads.linksys.com/downloads/datasheet/
           WRT120N_V10_DS_B-WEB.pdf"""
        mode = node.params['mode'][wlan]

        if mode == 'b':
            self.txpower = 21
        elif mode == 'g':
            self.txpower = 18
        elif mode == 'n':
            self.txpower = 16
        return self.txpower
