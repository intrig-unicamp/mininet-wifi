import os

from mininet.log import info

class module( object ):
    
    def __init__( self, wirelessRadios=4, enabled=None ):
        self.wirelessRadios=wirelessRadios
        self.enabled=enabled
        
        if (self.enabled==True):
            self._start_module(self.wirelessRadios)
        elif(self.enabled==False):
            self._stop_module()
        
    def _start_module(self, wirelessRadios):
        info( "*** Enabling Wireless Module\n" )
        os.system( 'modprobe mac80211_hwsim radios=%s' % wirelessRadios )
        
    def _stop_module(self):
        #info( "*** Removing Wireless Module\n" )
        os.system( 'rmmod mac80211_hwsim' )
        os.system( 'killall -9 hostapd' )
        
    
