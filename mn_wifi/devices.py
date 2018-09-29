"""
    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

"""


class GetRate (object):
    "Data Rate for specific equipments"

    rate = 0

    def __init__(self, **kwargs):
        "get data rate"
        ap = kwargs['ap'] if 'ap' in kwargs else None
        sta = kwargs['sta'] if 'sta' in kwargs else None
        wlan = kwargs['wlan'] if 'wlan' in kwargs else 0
        if ap and 'model' in ap.params:
            if ap.model in dir(self) and sta:
                model = ap.model
                self.__getattribute__(model)(**kwargs)
        else:
            node = sta
            if ap:
                node = ap
                wlan = kwargs['ap_wlan']
            self.customDataRate_mobility(node=node, wlan=wlan)

    def customDataRate_mobility(self, **kwargs):
        """Custom Maximum Data Rate - Useful when there is mobility
        
        mode: interface mode
        rate: maximum supported bandwidth (mbps)"""
        node = kwargs['node']
        wlan = kwargs['wlan']
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

    def customDataRate_no_mobility(self, **kwargs):
        """Custom Maximum Data Rate - Useful when there is no mobility
                
        mode: interface mode
        rate: maximum supported bandwidth (mbps)"""
        node = kwargs['node']
        wlan = kwargs['wlan']
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

    def DI524(self, **kwargs):
        """D-Link AirPlus G DI-524
           from http://www.dlink.com/-/media/Consumer_Products/
           DI/DI%20524/Manual/DI_524_Manual_EN_UK.pdf
           
        rssi: rssi value (dBm)
        rate: maximum supported bandwidth (mbps)"""
        node = kwargs['sta']
        wlan = kwargs['wlan']
        rate = 0

        if node.params['rssi'][wlan] != 0:
            rssi = node.params['rssi'][wlan]
            if rssi >= -68:
                rate = 48
            elif -75 <= rssi < -68:
                rate = 36
            elif -79 <= rssi < -75:
                rate = 24
            elif -84 <= rssi < -79:
                rate = 18
            elif -87 <= rssi < -84:
                rate = 9
            elif -88 <= rssi < -87:
                rate = 6
            else:
                rate = 1

        self.rate = rate
        return self.rate

    def TLWR740N(self, **kwargs):
        """TL-WR740N
           from http://www.tp-link.com.br/products/details/
           cat-9_TL-WR740N.html#specificationsf
        
        mode: interface mode
        rssi: rssi value (dBm)
        rate: maximum supported bandwidth (mbps)"""
        node = kwargs['sta']
        wlan = kwargs['wlan']
        mode = node.params['mode'][wlan]
        rate = 0

        if node:
            if node.params['rssi'][wlan] != 0:
                rssi = node.params['rssi'][wlan]
                if rssi >= -68:
                    if mode == 'n':
                        rate = 130
                    elif mode == 'g':
                        rate = 54
                    elif mode == 'b':
                        rate = 11
                elif -85 <= rssi < -68:
                    rate = 11
                elif -88 <= rssi < -85:
                    rate = 6
                else:
                    rate = 1
        else:
            if mode == 'n':
                rate = 130
            elif mode == 'g':
                rate = 54
            elif mode == 'b':
                rate = 11

        self.rate = rate
        return self.rate

    def WRT120N(self, **kwargs):
        """CISCO WRT120N
           from http://downloads.linksys.com/downloads/datasheet/
           WRT120N_V10_DS_B-WEB.pdf
        
        mode: interface mode
        rssi: rssi value (dBm)
        rate: maximum supported bandwidth (mbps)"""
        node1 = kwargs['sta']
        node2 = kwargs['ap']
        wlan = kwargs['wlan']
        mode = node2.params['mode'][0]
        rate = 0

        if node1:
            rssi = node1.params['rssi'][wlan]
            if rssi != 0:
                if rssi >= -65:
                    if mode == 'n':
                        rate = 150
                    elif mode == 'g':
                        rate = 54
                    elif mode == 'b':
                        rate = 11
                elif -68 <= rssi < -65:
                    if mode == 'g':
                        rate = 54
                    elif mode == 'b':
                        rate = 11
                elif -85 <= rssi < -68:
                    rate = 11
                else:
                    rate = 1
        else:
            if mode == 'n':
                rate = 150
            elif mode == 'g':
                rate = 54
            elif mode == 'b':
                rate = 11

        self.rate = rate
        return self.rate


class GetRange (object):
    "Range for specific equipments"

    value = 100

    def __init__(self, **kwargs):
        "get signal range"
        node = kwargs['node'] if 'node' in kwargs else None
        wlan = kwargs['wlan'] if 'wlan' in kwargs else 0

        if node and 'model' in node.params:
            model = node.params['model']
            if model in dir(self):
                self.__getattribute__(model)(node=node)
        else:
            self.customSignalRange(node=node, wlan=wlan)

    def customSignalRange(self, **kwargs):
        """Custom Signal Range
        
        mode: interface mode
        range: signal range (m)"""
        node = kwargs['node']
        wlan = kwargs['wlan']
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

    def DI524(self, **kwargs):
        """ D-Link AirPlus G DI-524
            from http://www.dlink.com/-/media/Consumer_Products/DI/
            DI%20524/Manual/DI_524_Manual_EN_UK.pdf
            indoor = 100
            outdoor = 200 
        
        range: signal range (m)"""

        self.value = 100
        return self.value

    def TLWR740N(self, **kwargs):
        """TL-WR740N
            NO REFERENCE!
        
        range: signal range (m)"""

        self.value = 50
        return self.value

    def WRT120N(self, **kwargs):
        """CISCO WRT120N
            NO REFERENCE!
        
        range: signal range (m)"""

        self.value = 50
        return self.value


class GetTxPower (object):
    "TX Power for specific equipments"

    value = 0

    def __init__(self, **kwargs):
        "get txpower"
        node = kwargs['ap'] if 'ap' in kwargs else None
        wlan = kwargs['wlan'] if 'wlan' in kwargs else 0

        if node and 'model' in node.params:
            model = node.params['model']
            if model in dir(self):
                self.__getattribute__(model)(node=node, wlan=wlan)

    def DI524(self, **kwargs):
        """D-Link AirPlus G DI-524
            from http://www.dlink.com/-/media/Consumer_Products/DI/
            DI%20524/Manual/DI_524_Manual_EN_UK.pdf
        
        txPower = transmission power (dBm)"""
        self.value = 14
        return self.value

    def TLWR740N(self, **kwargs):
        """TL-WR740N
            No REFERENCE!
        
        txPower = transmission power (dBm)"""
        self.value = 20
        return self.value

    def WRT120N(self, **kwargs):
        """CISCO WRT120N
           from http://downloads.linksys.com/downloads/datasheet/
           WRT120N_V10_DS_B-WEB.pdf"""
        ap = kwargs['ap']
        wlan = kwargs['wlan']
        mode = ap.params['mode'][wlan]

        if mode == 'b':
            self.value = 21
        elif mode == 'g':
            self.value = 18
        elif mode == 'n':
            self.value = 16
        return self.value
