"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

from wifiDevices import deviceTxPower

class wifiParameters ( object ):
    """WiFi Parameters""" 
    
    def __init__( self, param=None, node=None, wlan=0, iface=None ):
        
        self.iface = iface
        if param in dir(self):
            self.__getattribute__(param)(node, wlan)
    
    #def get_rsi(self, node, wlan): 
    #    """ Get rsi info """
    #    node.rsi = (node.cmd('iwconfig %s | grep -o \'Signal.*\' | cut -f2- -d\'=\' | cut -c1-4'
    #                                        % self.iface)) 
    
    def get_frequency(self, node, wlan): 
        """ Get frequency info **in development """
        try:
            freq = node.cmd('iwconfig %s | grep -o \'Frequency.*z\' | cut -f2- -d\':\' | cut -c1-5'
                                                % self.iface)
            if freq!='':
                node.frequency[wlan] = float(freq) 
            else:
                node.frequency[wlan] = 2.412            
        except:
            node.frequency[wlan] = 2.412
    
    def get_tx_power(self, node, wlan): 
        """ Get tx_power info """
        try:
            if node.equipmentModel == None:
                node.txpower[wlan] = int(node.cmd('iwconfig %s | grep -o \'Tx-Power.*\' | cut -f2- -d\'=\' | cut -c1-3'
                                                 % self.iface))
            else:
                value = deviceTxPower(node.equipmentModel, node)
                node.txpower[wlan] = value.txPower
        except:
            pass
        
    @classmethod
    def getWiFiParameters(self, node, wlan, iface):
        self(param='get_frequency', node=node, wlan=wlan, iface=iface)
        self(param='get_tx_power', node=node, wlan=wlan, iface=iface)