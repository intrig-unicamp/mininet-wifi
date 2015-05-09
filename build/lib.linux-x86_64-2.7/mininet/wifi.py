import os

from mininet.log import info
import socket
import struct
import fcntl

class module( object ):
    
    def __init__( self, wirelessRadios=4, enabled=None ):
        self.wirelessRadios=wirelessRadios
        self.enabled=enabled
        
        if (self.enabled==True):
            self._start_module(self.wirelessRadios)
        else:
            self._stop_module()
        
    def _start_module(self, wirelessRadios):
        info( "*** Enabling Wireless Module\n" )
        os.system( 'modprobe mac80211_hwsim radios=%s' % wirelessRadios )
        
    def _stop_module(self):
        #info( "*** Removing Wireless Module\n" )
        os.system( 'rmmod mac80211_hwsim' )
        os.system( 'killall -9 hostapd' )



class phyInterface ( object ):
    
    def __init__(self, wlanInterface, nextIface, phyInterfaces):
        
        self.wlanInterface = wlanInterface
        self.nextIface = nextIface 
        self.phyInterfaces = phyInterfaces 
        self.storeMacAddress=[]
        self.getMacAddress()
        
    def getMacAddress(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', 'wlan%s'[:15]) % str(self.nextIface+1))
        self.storeMacAddress.append(''.join(['%02x:' % ord(char) for char in info[18:24]])[:-1])
        return self.storeMacAddress
        #s = 'wlan%s' % self.nextIface
        #self.storeMacAddress.append(s)
   
