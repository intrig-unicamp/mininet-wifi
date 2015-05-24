import os

from mininet.log import info



class module( object ):
    
    def __init__( self, wirelessRadios=2, enabled=None ):
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
        #info( "*** Removing Wireless Module\n" )
        #os.system( 'rmmod mac80211_hwsim' )
        os.system( 'killall -9 hostapd' )
        
        
class wlink( object ):
    
    
    
    def __init__( self, callfun=None, intf=0, src=None):
        
        self.src=src
        self.intf=intf
        self.callfun=callfun


        if (callfun=="start"):
            self.start()
    #def stop(self):
    #    if (src[:3] == "sta"):
    #        Mininet.host.cmdPrint(src, "ifconfig")
             #for n in range(len(Mininet.wirelessdeviceControl)):
            #if (str(Mininet.wirelessdeviceControl[n]) in Mininet.stationName ):
                #cls = Mininet.host
                #h = cls( Mininet.wirelessdeviceControl[n] )
                #Mininet.host.cmdPrint(h, "ifconfig")
                #self.host.cmdPrint(h, "ifconfig %s-wlan0 down" % (str(self.wirelessdeviceControl[n])))
                #self.host.cmdPrint(h, "ip link set dev %s-wlan0 name wlan%s" % (str(self.wirelessdeviceControl[n]), str(n+1)))
                #os.system("ip link set dev %s-wlan0 name wlan%s" % (self.wirelessdeviceControl[n], n))
    def start(self):
        self.cmd("h1 ifconfig") 
        #os.system("iw phy phy%s set netns %s" % (self.nextIface, h.pid))
        #self.host.cmd(h,"ip link set dev wlan%s name %s-wlan0" % (self.nextIface, h))
        #self.host.cmd(h,"ifconfig %s-wlan0 up" % h)
        #self.host.cmd(h,"ifconfig") 
        
    
