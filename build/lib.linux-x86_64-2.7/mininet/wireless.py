import os

from mininet.log import info


class module( object ):
    
    def __init__( self, wirelessRadios=2, enabled=True ):
        self.wirelessRadios=wirelessRadios
        self.enabled=enabled
        
        if (self.enabled==True):
            self._start_module(self.wirelessRadios)
        else:
            self._stop_module()
        
    def _start_module(self, wirelessRadios):
        info( "*** Enabling Wireless Module\n" )
        os.system( 'modprobe mac80211_hwsim radios=%s' % wirelessRadios )
        #raise Exception( 'OVSIntf cannot do ifconfig ' + cmd )
        
    def _stop_module(self):
        info( "*** Removing Wireless Module\n" )
        #os.system( 'rmmod mac80211_hwsim' )
        os.system( 'killall -9 hostapd' )

       
#    os.system("/etc/init.d/network-manager stop")
 #   os.system("sleep 3")
   # os.system("wpa_supplicant -B -c/etc/wpa_supplicant/wpa_supplicant.conf -iwlan0")
   # os.system("dhclient wlan0")