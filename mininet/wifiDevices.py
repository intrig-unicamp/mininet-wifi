"""
Devices list to Mininet-WiFi.

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)

"""

class deviceDataRate ( object ):
    """ Data Rate for specific equipments """
    
    rate = 0
    
    def __init__( self, ap=None, sta=None, wlan=None ):
       
        if ap.equipmentModel in dir(self):
            model = ap.equipmentModel
            self.__getattribute__(model)(ap, sta, wlan)
    
    def DI524(self, ap, sta, wlan):
        """D-Link AirPlus G DI-524
           from http://www.dlink.com/-/media/Consumer_Products/DI/DI%20524/Manual/DI_524_Manual_EN_UK.pdf"""  
        
        if sta.rssi[wlan] != 0:
            if (sta.rssi[wlan] >= -68):
                self.rate = 48
            elif (sta.rssi[wlan] < -68 and sta.rssi[wlan] >= -75):
                self.rate = 36
            elif (sta.rssi[wlan] < -75 and sta.rssi[wlan] >= -79):
                self.rate = 24
            elif (sta.rssi[wlan] < -79 and sta.rssi[wlan] >= -84):
                self.rate = 18
            elif (sta.rssi[wlan] < -84 and sta.rssi[wlan] >= -87):
                self.rate = 9
            elif (sta.rssi[wlan] < -87 and sta.rssi[wlan] >= -88):
                self.rate = 6
            elif (sta.rssi[wlan] < -88 and sta.rssi[wlan] >= -89):
                self.rate = 1
        return self.rate
    
    def TLWR740N(self, ap, sta, wlan):
        """TL-WR740N
           from http://www.tp-link.com.br/products/details/cat-9_TL-WR740N.html#specificationsf"""
        try: # if Station
            if sta.rssi[wlan] != 0:
                if (sta.rssi[wlan] >= -68):
                    if ap.mode == 'n':
                        self.rate = 130
                    elif ap.mode == 'g':
                        self.rate = 54
                    elif ap.mode == 'b':
                        self.rate = 11
                elif (sta.rssi[wlan] < -68 and sta.rssi[wlan] >= -85):
                    self.rate = 11
                elif (sta.rssi[wlan] < -85 and sta.rssi[wlan] >= -88):
                    self.rate = 6
                elif (sta.rssi[wlan] < -88 and sta.rssi[wlan] >= -90):
                    self.rate = 1
        except: # if AP
            if ap.mode == 'n':
                self.rate = 130
            elif ap.mode == 'g':
                self.rate = 54
            elif ap.mode == 'b':
                self.rate = 11
        return self.rate
    
    def WRT120N(self, ap, sta, wlan):
        """CISCO WRT120N
           from http://downloads.linksys.com/downloads/datasheet/WRT120N_V10_DS_B-WEB.pdf"""
        try: # if Station
            if sta.rssi[wlan] != 0:
                if (sta.rssi[wlan] >= -65):
                    if ap.mode == 'n':
                        self.rate = 150
                    elif ap.mode == 'g':
                        self.rate = 54
                    elif ap.mode == 'b':
                        self.rate = 11
                elif (sta.rssi[wlan] < -65 and sta.rssi[wlan] >= -68):
                    if ap.mode == 'g':
                        self.rate = 54
                    elif ap.mode == 'b':
                        self.rate = 11
                elif (sta.rssi[wlan] < -68 and sta.rssi[wlan] >= -85):
                    self.rate = 11
                elif (sta.rssi[wlan] < -85 and sta.rssi[wlan] >= -90):
                    self.rate = 1
        except: # if AP
            if ap.mode == 'n':
                self.rate = 150
            elif ap.mode == 'g':
                self.rate = 54
            elif ap.mode == 'b':
                self.rate = 11
        return self.rate
       
class deviceRange ( object ):
    """ Range for specific equipments """
    
    range = 100
    
    def __init__( self, ap=None ):
        self.equipmentModel = ap.equipmentModel
        self.ap = ap        
                
        if self.equipmentModel in dir(self):
            self.__getattribute__(self.equipmentModel)(self.ap)
    
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
    
class deviceTxPower ( object ):
    """ TX Power for specific equipments """
    
    txPower = 0
    
    def __init__( self, model=None, ap=None ):
        self.model = model
        self.ap = ap
        
        if self.model in dir(self):
            self.__getattribute__(self.model)(self.ap)
    
    def DI524(self, ap):
        """ D-Link AirPlus G DI-524
            from http://www.dlink.com/-/media/Consumer_Products/DI/DI%20524/Manual/DI_524_Manual_EN_UK.pdf"""
        self.txPower = 14
        return self.txPower
    
    def TLWR740N(self, ap):
        """TL-WR740N
        No REFERENCE!"""
        self.txPower = 20
        return self.txPower    
    
    def WRT120N(self, ap):
        """CISCO WRT120N
           from http://downloads.linksys.com/downloads/datasheet/WRT120N_V10_DS_B-WEB.pdf"""
        if ap.mode == 'b':
            self.txPower = 21
        elif ap.mode == 'g':
            self.txPower = 18
        elif ap.mode == 'n':
            self.txPower = 16
        return self.txPower

            